#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "ISubmixBufferListener.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHazardVolume.h"
#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidHostBase.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundSubmix.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("BeepMixCapture_Functional");
	constexpr TCHAR TestIdCombatMute[] = TEXT("BeepMixCapture_CombatMute_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Recurring Beep Mixed-Output Capture Diagnostic");
	constexpr TCHAR DisplayNameCombatMute[] = TEXT("Recurring Beep Mix Capture Combat-Layer Mute A/B");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr float StopListenSeconds = 4.0f;
	constexpr float MuteSeconds = 16.0f;
	constexpr float RestoreListenSeconds = 8.0f;

	struct FListenStop
	{
		const TCHAR* Name;
		FVector XY;
	};

	const FListenStop RouteStops[] = {
		{TEXT("Spawn"), FVector(-1500.0f, 0.0f, 0.0f)},
		{TEXT("Vestibule"), FVector(0.0f, 0.0f, 0.0f)},
		{TEXT("Reception"), FVector(1250.0f, 0.0f, 0.0f)},
		{TEXT("Hub"), FVector(1800.0f, 0.0f, 0.0f)},
		{TEXT("Security"), FVector(2680.0f, 0.0f, 0.0f)},
		{TEXT("Records"), FVector(2680.0f, -400.0f, 0.0f)},
		{TEXT("Executive"), FVector(3200.0f, -1850.0f, 0.0f)},
		{TEXT("Operations"), FVector(4180.0f, 0.0f, 0.0f)},
		{TEXT("Transit"), FVector(5005.0f, 0.0f, 0.0f)},
		{TEXT("Service"), FVector(5005.0f, -800.0f, 0.0f)},
	};
	constexpr int32 RouteCount = UE_ARRAY_COUNT(RouteStops);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	void CollectDirtyPackageNames(TArray<FString>& Out)
	{
		Out.Reset();
		TArray<UPackage*> WorldDirty;
		TArray<UPackage*> ContentDirty;
		FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
		FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
		for (UPackage* Package : WorldDirty)
		{
			if (Package)
			{
				Out.AddUnique(Package->GetName());
			}
		}
		for (UPackage* Package : ContentDirty)
		{
			if (Package)
			{
				Out.AddUnique(Package->GetName());
			}
		}
		Out.Sort();
	}

	class FOrganoidSubmixCapture final : public ISubmixBufferListener
	{
	public:
		FString ListenerName;
		mutable FCriticalSection Mutex;
		TArray<float> Samples;
		int32 Channels = 2;
		int32 SampleRate = 48000;
		int32 MuteMarkerSamples = -1;
		int32 RestoreMarkerSamples = -1;
		int32 Callbacks = 0;
		FString DeviceLabel;

		explicit FOrganoidSubmixCapture(const FString& InName)
			: ListenerName(InName)
		{
		}

		virtual const FString& GetListenerName() const override
		{
			return ListenerName;
		}

		virtual void OnNewSubmixBuffer(
			const USoundSubmix* /*OwningSubmix*/,
			float* AudioData,
			int32 NumSamples,
			int32 NumChannels,
			const int32 InSampleRate,
			double /*AudioClock*/) override
		{
			if (!AudioData || NumSamples <= 0)
			{
				return;
			}
			FScopeLock Lock(&Mutex);
			Channels = FMath::Max(1, NumChannels);
			SampleRate = FMath::Max(8000, InSampleRate);
			++Callbacks;
			const int32 Start = Samples.Num();
			Samples.AddUninitialized(NumSamples);
			FMemory::Memcpy(Samples.GetData() + Start, AudioData, sizeof(float) * NumSamples);
		}

		void MarkMute()
		{
			FScopeLock Lock(&Mutex);
			MuteMarkerSamples = Samples.Num();
		}

		void MarkRestore()
		{
			FScopeLock Lock(&Mutex);
			RestoreMarkerSamples = Samples.Num();
		}

		float DurationSeconds() const
		{
			FScopeLock Lock(&Mutex);
			if (SampleRate <= 0 || Channels <= 0)
			{
				return 0.0f;
			}
			return static_cast<float>(Samples.Num()) / static_cast<float>(SampleRate * Channels);
		}
	};

	bool WritePcmWav(const FString& Path, const TArray<float>& FloatSamples, int32 Channels, int32 SampleRate)
	{
		if (FloatSamples.Num() == 0 || Channels <= 0 || SampleRate <= 0)
		{
			return false;
		}
		const int32 DataBytes = FloatSamples.Num() * static_cast<int32>(sizeof(int16));
		TArray<uint8> File;
		File.Reserve(44 + DataBytes);
		auto AppendU32 = [&File](uint32 Value)
		{
			File.Add(static_cast<uint8>(Value & 0xff));
			File.Add(static_cast<uint8>((Value >> 8) & 0xff));
			File.Add(static_cast<uint8>((Value >> 16) & 0xff));
			File.Add(static_cast<uint8>((Value >> 24) & 0xff));
		};
		auto AppendU16 = [&File](uint16 Value)
		{
			File.Add(static_cast<uint8>(Value & 0xff));
			File.Add(static_cast<uint8>((Value >> 8) & 0xff));
		};
		File.Append(reinterpret_cast<const uint8*>("RIFF"), 4);
		AppendU32(static_cast<uint32>(36 + DataBytes));
		File.Append(reinterpret_cast<const uint8*>("WAVE"), 4);
		File.Append(reinterpret_cast<const uint8*>("fmt "), 4);
		AppendU32(16);
		AppendU16(1);
		AppendU16(static_cast<uint16>(Channels));
		AppendU32(static_cast<uint32>(SampleRate));
		AppendU32(static_cast<uint32>(SampleRate * Channels * 2));
		AppendU16(static_cast<uint16>(Channels * 2));
		AppendU16(16);
		File.Append(reinterpret_cast<const uint8*>("data"), 4);
		AppendU32(static_cast<uint32>(DataBytes));
		for (float Sample : FloatSamples)
		{
			const int16 Pcm = static_cast<int16>(FMath::Clamp(Sample, -1.0f, 1.0f) * 32767.0f);
			AppendU16(static_cast<uint16>(Pcm));
		}
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		return FFileHelper::SaveArrayToFile(File, *Path);
	}

	class FBeepMixCaptureFunctional : public IOrganoidPlaytestCase
	{
	public:
		explicit FBeepMixCaptureFunctional(bool bInSuppressCombat)
			: bSuppressCombat(bInSuppressCombat)
		{
		}

		virtual FString GetTestId() const override
		{
			return bSuppressCombat ? FString(TestIdCombatMute) : FString(TestId);
		}
		virtual FString GetDisplayName() const override
		{
			return bSuppressCombat ? FString(DisplayNameCombatMute) : FString(DisplayName);
		}
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			RouteIndex = 0;
			bTeleported = false;
			bAnyAssertFailed = false;
			bCaptureStarted = false;
			bMuted = false;
			SavedPrimaryVolume = 1.0f;
			PrevPlayingKeys.Reset();
			StartEvents.Reset();
			DirtyBefore.Reset();
			PieCapture.Reset();
			EditorCapture.Reset();
			bLoggedCombatRise = false;
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			RestorePrimaryVolume();
			StopCapture();
			Owner.RequestEndPieIfStarted();
			Owner.SetStage(TEXT("Abort"));
		}

		virtual void Tick(UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) override
		{
			FOrganoidPlaytestRecord* Record = Owner.GetActiveRecord();
			if (!Record)
			{
				return;
			}
			switch (Stage)
			{
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie: TickStartPie(Owner, *Record); break;
			case EStage::WaitPieReady: TickWaitPieReady(Owner, *Record, DeltaTime); break;
			case EStage::Route: TickRoute(Owner, *Record, DeltaTime); break;
			case EStage::Mute: TickMute(Owner, *Record, DeltaTime); break;
			case EStage::Restore: TickRestore(Owner, *Record, DeltaTime); break;
			case EStage::StopCapture: TickStopCapture(Owner, *Record); break;
			case EStage::EndPie:
				RestorePrimaryVolume();
				StopCapture();
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitPieStopped;
				break;
			case EStage::WaitPieStopped: TickWaitPieStopped(Owner, *Record, DeltaTime); break;
			case EStage::AssertDurable: TickAssertDurable(Owner, *Record); break;
			case EStage::Finalize: Finalize(Owner, *Record); break;
			}
		}

	private:
		enum class EStage : uint8
		{
			Preflight,
			StartPie,
			WaitPieReady,
			Route,
			Mute,
			Restore,
			StopCapture,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		bool AssertTrue(
			FOrganoidPlaytestRecord& Record,
			const FString& Id,
			bool bPassed,
			const FString& Expected,
			const FString& Actual,
			const FString& ActorId,
			bool bDurable)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, bDurable);
			if (!bPassed)
			{
				bAnyAssertFailed = true;
				if (Record.FailureReason.IsEmpty())
				{
					Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
				}
			}
			return bPassed;
		}

		FAudioDevice* DeviceFromWorld(UWorld* World) const
		{
			if (!World)
			{
				return nullptr;
			}
			FAudioDeviceHandle Handle = World->GetAudioDevice();
			return Handle.IsValid() ? Handle.GetAudioDevice() : nullptr;
		}

		FAudioDevice* EditorDevice() const
		{
			if (!GEngine)
			{
				return nullptr;
			}
			FAudioDeviceHandle Handle = GEngine->GetMainAudioDevice();
			return Handle.IsValid() ? Handle.GetAudioDevice() : nullptr;
		}

		bool AttachCapture(TSharedPtr<FOrganoidSubmixCapture, ESPMode::ThreadSafe>& Capture, FAudioDevice* Device, const FString& Name)
		{
			if (!Device)
			{
				return false;
			}
			Capture = MakeShared<FOrganoidSubmixCapture, ESPMode::ThreadSafe>(Name);
			Capture->DeviceLabel = FString::Printf(TEXT("%p"), Device);
			Device->RegisterSubmixBufferListener(Capture.ToSharedRef(), Device->GetMainSubmixObject());
			return true;
		}

		void DetachCapture(TSharedPtr<FOrganoidSubmixCapture, ESPMode::ThreadSafe>& Capture, FAudioDevice* Device)
		{
			if (Capture.IsValid() && Device)
			{
				Device->UnregisterSubmixBufferListener(Capture.ToSharedRef(), Device->GetMainSubmixObject());
			}
		}

		void StartCapture(UWorld* PieWorld)
		{
			if (bCaptureStarted)
			{
				return;
			}
			FAudioDevice* PieDevice = DeviceFromWorld(PieWorld);
			FAudioDevice* EdDevice = EditorDevice();
			bPieAttached = AttachCapture(PieCapture, PieDevice, TEXT("pie_master"));
			bEditorAttached = false;
			if (EdDevice && EdDevice != PieDevice)
			{
				bEditorAttached = AttachCapture(EditorCapture, EdDevice, TEXT("editor_master"));
			}
			bCaptureStarted = true;
		}

		void StopCapture()
		{
			if (!bCaptureStarted)
			{
				return;
			}
			UWorld* PieWorld = OrganoidPlaytestActions::GetPieWorld();
			DetachCapture(PieCapture, DeviceFromWorld(PieWorld));
			if (bEditorAttached)
			{
				DetachCapture(EditorCapture, EditorDevice());
			}
			bCaptureStarted = false;
		}

		void RestorePrimaryVolume()
		{
			if (!bMuted)
			{
				return;
			}
			if (UWorld* World = OrganoidPlaytestActions::GetPieWorld())
			{
				if (FAudioDevice* Device = DeviceFromWorld(World))
				{
					Device->SetTransientPrimaryVolume(SavedPrimaryVolume);
				}
			}
			bMuted = false;
		}

		bool CallSetCombatActive(UWorld* World, bool bActive)
		{
			UProjectOrganoidAudioAmbienceSubsystem* Sub = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Sub)
			{
				return false;
			}
			UFunction* Function = Sub->FindFunction(FName(TEXT("SetCombatActive")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Function, TEXT("bActive")))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), bActive);
			}
			Sub->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		float CapsuleZFor(APawn* Pawn) const
		{
			float CapsuleZ = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleZ = Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
				}
			}
			return CapsuleZ;
		}

		bool TeleportPawnTo(APawn* Pawn, const FVector& XY)
		{
			if (!Pawn)
			{
				return false;
			}
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->GravityScale = 1.0f;
					Move->SetMovementMode(MOVE_Walking);
					Move->StopMovementImmediately();
				}
			}
			const float CapsuleZ = CapsuleZFor(Pawn);
			const bool bMoved = OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
			return bMoved;
		}

		void SampleStarts(UWorld* World, float WorldTime)
		{
			TSet<FString> Now;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				if (bSuppressCombat)
				{
					const FString CompName = Component->GetName();
					if (CompName.Contains(TEXT("OrganoidCombatLayer")) || CompName.Contains(TEXT("OrganoidCriticalLayer")))
					{
						Component->Stop();
						Component->SetVolumeMultiplier(0.0f);
						continue;
					}
				}
				USoundBase* Sound = Component->GetSound();
				AActor* Owner = Component->GetOwner();
				const FString Key = FString::Printf(
					TEXT("%s/%s/%s"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*Component->GetName(),
					Sound ? *Sound->GetName() : TEXT("none"));
				Now.Add(Key);
				if (!PrevPlayingKeys.Contains(Key))
				{
					StartEvents.Add(FString::Printf(TEXT("%.2f %s vol=%.3f"), WorldTime, *Key, Component->VolumeMultiplier));
				}
			}
			PrevPlayingKeys = MoveTemp(Now);

			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>();
			if (Ambience && Ambience->IsInCombat() && !bLoggedCombatRise)
			{
				bLoggedCombatRise = true;
				APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
				AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Pawn);
				TArray<FString> Nearby;
				const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Actor = *It;
					if (!Actor)
					{
						continue;
					}
					const bool bInteresting =
						Actor->IsA(AProjectOrganoidHostBase::StaticClass())
						|| Actor->IsA(AProjectOrganoidHazardZone::StaticClass())
						|| Actor->IsA(AProjectOrganoidHazardVolume::StaticClass())
						|| Actor->GetName().Contains(TEXT("Weapon"));
					if (!bInteresting)
					{
						continue;
					}
					const float Dist = FVector::Dist(Origin, Actor->GetActorLocation());
					if (Dist <= 2500.0f)
					{
						Nearby.Add(FString::Printf(TEXT("%s@%.0f"), *OrganoidPlaytestActions::ActorLabel(Actor), Dist));
					}
				}
				StartEvents.Add(FString::Printf(
					TEXT("%.2f COMBAT_RISE health=%.1f/%.1f loc=(%.0f,%.0f) nearby=%s"),
					WorldTime,
					Character ? Character->GetHealth() : -1.0f,
					Character ? Character->GetMaxHealth() : -1.0f,
					Origin.X,
					Origin.Y,
					Nearby.Num() > 0 ? *FString::Join(Nearby, TEXT(",")) : TEXT("none")));
			}
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Preflight"));
			if (!GEditor)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Editor is not available."));
				return;
			}
			if (GEditor->IsPlaySessionInProgress())
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("capture.combat_suppressed"), bSuppressCombat ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("capture.method"), TEXT("ISubmixBufferListener on FAudioDevice::GetMainSubmixObject"));
			Record.AddActor(TEXT("editor_sounds_enabled"), GEditor->CanPlayEditorSound() ? TEXT("true") : TEXT("false"));
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("StartPie"));
			if (!Owner.RequestStartPie(MapPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			(void)Record;
			WaitSeconds = 0.0f;
			Stage = EStage::WaitPieReady;
		}

		void TickWaitPieReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (World && Pawn)
			{
				CallSetCombatActive(World, false);
				StartCapture(World);
				WaitSeconds = 0.0f;
				bTeleported = false;
				RouteIndex = 0;
				Stage = EStage::Route;
				Owner.SetStage(TEXT("Route"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE pawn."));
			}
			(void)Record;
		}

		void TickRoute(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during mix capture route.");
				bAnyAssertFailed = true;
				Stage = EStage::StopCapture;
				return;
			}
			if (!bTeleported)
			{
				if (!TeleportPawnTo(Pawn, RouteStops[RouteIndex].XY))
				{
					Record.FailureReason = FString::Printf(TEXT("Teleport to %s failed."), RouteStops[RouteIndex].Name);
					bAnyAssertFailed = true;
					Stage = EStage::StopCapture;
					return;
				}
				bTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			SampleStarts(World, World->GetTimeSeconds());
			if (WaitSeconds < StopListenSeconds)
			{
				return;
			}
			++RouteIndex;
			bTeleported = false;
			WaitSeconds = 0.0f;
			if (RouteIndex >= RouteCount)
			{
				Stage = EStage::Mute;
				Owner.SetStage(TEXT("Mute"));
			}
			(void)Record;
		}

		void TickMute(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during mute capture.");
				bAnyAssertFailed = true;
				Stage = EStage::StopCapture;
				return;
			}
			if (!bMuted)
			{
				if (FAudioDevice* Device = DeviceFromWorld(World))
				{
					SavedPrimaryVolume = 1.0f;
					if (PieCapture.IsValid())
					{
						PieCapture->MarkMute();
					}
					if (EditorCapture.IsValid())
					{
						EditorCapture->MarkMute();
					}
					Device->SetTransientPrimaryVolume(0.0f);
				}
				bMuted = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			SampleStarts(World, World->GetTimeSeconds());
			if (WaitSeconds < MuteSeconds)
			{
				return;
			}
			Stage = EStage::Restore;
			Owner.SetStage(TEXT("Restore"));
			(void)Record;
		}

		void TickRestore(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (bMuted)
			{
				if (PieCapture.IsValid())
				{
					PieCapture->MarkRestore();
				}
				if (EditorCapture.IsValid())
				{
					EditorCapture->MarkRestore();
				}
				RestorePrimaryVolume();
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			if (World)
			{
				SampleStarts(World, World->GetTimeSeconds());
			}
			if (WaitSeconds < RestoreListenSeconds)
			{
				return;
			}
			(void)Owner;
			(void)Record;
			Stage = EStage::StopCapture;
			Owner.SetStage(TEXT("StopCapture"));
		}

		void TickStopCapture(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("BeepCapture"));
			IFileManager::Get().MakeDirectory(*Dir, true);
			auto Dump = [&](const TSharedPtr<FOrganoidSubmixCapture, ESPMode::ThreadSafe>& Capture, const TCHAR* Prefix)
			{
				if (!Capture.IsValid())
				{
					Record.AddActor(FString::Printf(TEXT("%s.attached"), Prefix), TEXT("false"));
					return;
				}
				TArray<float> Copy;
				int32 Channels;
				int32 SampleRate;
				int32 MuteMarker;
				int32 RestoreMarker;
				int32 Callbacks;
				{
					FScopeLock Lock(&Capture->Mutex);
					Copy = Capture->Samples;
					Channels = Capture->Channels;
					SampleRate = Capture->SampleRate;
					MuteMarker = Capture->MuteMarkerSamples;
					RestoreMarker = Capture->RestoreMarkerSamples;
					Callbacks = Capture->Callbacks;
				}
				const FString WavPath = FPaths::Combine(Dir, FString::Printf(TEXT("%s%s.wav"), Prefix, bSuppressCombat ? TEXT("_combatmute") : TEXT("")));
				const bool bWrote = WritePcmWav(WavPath, Copy, Channels, SampleRate);
				const float Duration = (SampleRate > 0 && Channels > 0)
					? static_cast<float>(Copy.Num()) / static_cast<float>(SampleRate * Channels)
					: 0.0f;
				Record.AddActor(FString::Printf(TEXT("%s.attached"), Prefix), TEXT("true"));
				Record.AddActor(FString::Printf(TEXT("%s.device"), Prefix), Capture->DeviceLabel);
				Record.AddActor(FString::Printf(TEXT("%s.wav"), Prefix), WavPath);
				Record.AddActor(FString::Printf(TEXT("%s.wrote"), Prefix), bWrote ? TEXT("true") : TEXT("false"));
				Record.AddActor(FString::Printf(TEXT("%s.duration"), Prefix), FString::Printf(TEXT("%.3f"), Duration));
				Record.AddActor(FString::Printf(TEXT("%s.sample_rate"), Prefix), FString::FromInt(SampleRate));
				Record.AddActor(FString::Printf(TEXT("%s.channels"), Prefix), FString::FromInt(Channels));
				Record.AddActor(FString::Printf(TEXT("%s.callbacks"), Prefix), FString::FromInt(Callbacks));
				Record.AddActor(FString::Printf(TEXT("%s.mute_sample"), Prefix), FString::FromInt(MuteMarker));
				Record.AddActor(FString::Printf(TEXT("%s.restore_sample"), Prefix), FString::FromInt(RestoreMarker));
				if (bWrote)
				{
					WroteOk = true;
					LastWavPath = WavPath;
					LastMuteSample = MuteMarker;
					LastRestoreSample = RestoreMarker;
					LastChannels = Channels;
					LastRate = SampleRate;
				}
			};

			Dump(PieCapture, TEXT("pie"));
			Dump(EditorCapture, TEXT("editor"));
			Record.AddActor(TEXT("events.audio_starts"), StartEvents.Num() > 0 ? FString::Join(StartEvents, TEXT(" || ")) : TEXT("none"));
			Record.AddActor(TEXT("events.audio_start_count"), FString::FromInt(StartEvents.Num()));
			Record.AddActor(TEXT("capture.same_device"),
				(PieCapture.IsValid() && EditorCapture.IsValid() && PieCapture->DeviceLabel == EditorCapture->DeviceLabel)
					? TEXT("true")
					: TEXT("false"));

			StopCapture();
			RestorePrimaryVolume();

			AssertTrue(Record, TEXT("capture.pie_wav_written"),
				WroteOk, TEXT("true"), WroteOk ? TEXT("true") : TEXT("false"), TEXT("pie_master"), false);
			AssertTrue(Record, TEXT("capture.duration_ge_30s"),
				PieCaptureDurationFromRecord(Record) >= 30.0f,
				TEXT(">=30"), Record.Actors.Contains(TEXT("pie.duration")) ? Record.Actors.FindRef(TEXT("pie.duration")) : TEXT("0"),
				TEXT("pie_master"), false);

			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		float PieCaptureDurationFromRecord(const FOrganoidPlaytestRecord& Record) const
		{
			if (const FString* Value = Record.Actors.Find(TEXT("pie.duration")))
			{
				return FCString::Atof(**Value);
			}
			return 0.0f;
		}

		void TickWaitPieStopped(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (GEditor && GEditor->IsPlaySessionInProgress())
			{
				if (WaitSeconds > 20.0f)
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE to stop."));
				}
				return;
			}
			(void)Record;
			Stage = EStage::AssertDurable;
			Owner.SetStage(TEXT("AssertDurable"));
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"),
				DirtyAfter.Num() == DirtyBefore.Num(),
				FString::FromInt(DirtyBefore.Num()), FString::FromInt(DirtyAfter.Num()), TEXT("packages"), true);
			AssertTrue(Record, TEXT("durable.playtest_mutates_assets"),
				true, TEXT("false"), TEXT("false"), TEXT(""), true);
			(void)Owner;
			Stage = EStage::Finalize;
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			Owner.CompleteActive(
				bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass,
				Record.FailureReason);
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		int32 RouteIndex = 0;
		bool bSuppressCombat = false;
		bool bTeleported = false;
		bool bAnyAssertFailed = false;
		bool bCaptureStarted = false;
		bool bPieAttached = false;
		bool bEditorAttached = false;
		bool bMuted = false;
		bool WroteOk = false;
		float SavedPrimaryVolume = 1.0f;
		FString LastWavPath;
		int32 LastMuteSample = -1;
		int32 LastRestoreSample = -1;
		int32 LastChannels = 2;
		int32 LastRate = 48000;
		TSharedPtr<FOrganoidSubmixCapture, ESPMode::ThreadSafe> PieCapture;
		TSharedPtr<FOrganoidSubmixCapture, ESPMode::ThreadSafe> EditorCapture;
		TSet<FString> PrevPlayingKeys;
		TArray<FString> StartEvents;
		TArray<FString> DirtyBefore;
		bool bLoggedCombatRise = false;
	};

	struct FBeepMixCaptureAutoRegister
	{
		FBeepMixCaptureAutoRegister()
		{
			{
				FOrganoidPlaytestCatalogEntry Entry;
				Entry.TestId = TestId;
				Entry.DisplayName = DisplayName;
				Entry.MapPackage = MapPackage;
				Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
				{
					return MakeShared<FBeepMixCaptureFunctional>(false);
				};
				FOrganoidPlaytestRegistry::Register(Entry);
			}
			{
				FOrganoidPlaytestCatalogEntry Entry;
				Entry.TestId = TestIdCombatMute;
				Entry.DisplayName = DisplayNameCombatMute;
				Entry.MapPackage = MapPackage;
				Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
				{
					return MakeShared<FBeepMixCaptureFunctional>(true);
				};
				FOrganoidPlaytestRegistry::Register(Entry);
			}
		}
	};

	static FBeepMixCaptureAutoRegister GBeepMixCaptureAutoRegister;
}
