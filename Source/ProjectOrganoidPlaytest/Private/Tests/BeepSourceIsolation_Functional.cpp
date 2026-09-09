#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("BeepSourceIsolation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Recurring Beep Source Isolation Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HazardLabel[] = TEXT("Hazard_DeconUVC");
	constexpr float ListenSeconds = 4.8f;
	constexpr float MuteListenSeconds = 0.6f;
	constexpr float InsideListenSeconds = 0.6f;
	constexpr float ExtendedListenSeconds = 15.0f;

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

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString VolumeText(float Value)
	{
		return FString::Printf(TEXT("%.4f"), Value);
	}

	void CollectDirtyPackageNames(TArray<FString>& Out)
	{
		Out.Reset();
		TArray<UPackage*> WorldDirty;
		TArray<UPackage*> ContentDirty;
		FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
		FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
		auto Add = [&Out](const TArray<UPackage*>& Packages)
		{
			for (UPackage* Package : Packages)
			{
				if (Package)
				{
					Out.AddUnique(Package->GetName());
				}
			}
		};
		Add(WorldDirty);
		Add(ContentDirty);
		Out.Sort();
	}

	FString AmbienceStateText(EProjectOrganoidAmbienceState State)
	{
		switch (State)
		{
		case EProjectOrganoidAmbienceState::Exploration: return TEXT("Exploration");
		case EProjectOrganoidAmbienceState::Tension: return TEXT("Tension");
		case EProjectOrganoidAmbienceState::Combat: return TEXT("Combat");
		case EProjectOrganoidAmbienceState::Hazard: return TEXT("Hazard");
		case EProjectOrganoidAmbienceState::CriticalHealth: return TEXT("CriticalHealth");
		default: return TEXT("Unknown");
		}
	}

	class FBeepSourceIsolationFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			RouteIndex = 0;
			bTeleported = false;
			bHazardMuted = false;
			bHazardRestored = false;
			bAnyAssertFailed = false;
			SavedHazardVolume = 0.45f;
			ExtendedHissFrames = 0;
			ExtendedAlarmFrames = 0;
			RouteHissFrames = 0;
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			RestoreHazardIfNeeded();
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
			case EStage::HazardNear: TickHazardNear(Owner, *Record, DeltaTime); break;
			case EStage::HazardMuted: TickHazardMuted(Owner, *Record, DeltaTime); break;
			case EStage::HazardRestore: TickHazardRestore(Owner, *Record, DeltaTime); break;
			case EStage::HazardFar: TickHazardFar(Owner, *Record, DeltaTime); break;
			case EStage::ExtendedListen: TickExtendedListen(Owner, *Record, DeltaTime); break;
			case EStage::Assert: TickAssert(Owner, *Record); break;
			case EStage::EndPie:
				RestoreHazardIfNeeded();
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
			HazardNear,
			HazardMuted,
			HazardRestore,
			HazardFar,
			ExtendedListen,
			Assert,
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

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			RestoreHazardIfNeeded();
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		UAudioComponent* FindHazardAudio()
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Hazard = World ? OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel) : nullptr;
			return Hazard ? Cast<UAudioComponent>(OrganoidPlaytestActions::FindNamedComponent(Hazard, TEXT("HazardAudio"))) : nullptr;
		}

		void RestoreHazardIfNeeded()
		{
			if (!bHazardMuted || bHazardRestored)
			{
				return;
			}
			if (UAudioComponent* Audio = FindHazardAudio())
			{
				Audio->SetVolumeMultiplier(SavedHazardVolume);
				if (!Audio->IsPlaying() && OccupiedHazardWantsAudio())
				{
					Audio->Play();
				}
			}
			bHazardRestored = true;
			bHazardMuted = false;
		}

		bool OccupiedHazardWantsAudio()
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Hazard = World ? OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel) : nullptr;
			if (!World || !Pawn || !Hazard)
			{
				return false;
			}
			return FVector::Dist2D(Pawn->GetActorLocation(), Hazard->GetActorLocation()) < 250.0f;
		}

		bool RestoreVitals(APawn* Pawn)
		{
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Pawn);
			if (!Character)
			{
				return false;
			}
			UFunction* Function = Character->FindFunction(FName(TEXT("ApplySavedVitals")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			auto SetFloat = [&](const TCHAR* Name, float Value)
			{
				if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Function, Name))
				{
					Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), Value);
				}
			};
			SetFloat(TEXT("InHealth"), Character->GetMaxHealth());
			SetFloat(TEXT("InMaxHealth"), Character->GetMaxHealth());
			SetFloat(TEXT("InToxicity"), 0.0f);
			SetFloat(TEXT("InMaxToxicity"), Character->GetMaxToxicity());
			SetFloat(TEXT("InHeartRate"), 72.0f);
			SetFloat(TEXT("InPEEnergy"), Character->GetPEEnergy());
			SetFloat(TEXT("InMaxPEEnergy"), Character->GetMaxPEEnergy());
			Character->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		float EstimateSpatialGain(UAudioComponent* Component, const FVector& Listener) const
		{
			if (!Component)
			{
				return 0.0f;
			}
			if (!Component->bAllowSpatialization || Component->bIsUISound)
			{
				return Component->VolumeMultiplier;
			}
			const FSoundAttenuationSettings* Settings = nullptr;
			if (Component->bOverrideAttenuation)
			{
				Settings = &Component->AttenuationOverrides;
			}
			else if (Component->AttenuationSettings)
			{
				Settings = &Component->AttenuationSettings->Attenuation;
			}
			if (!Settings || !Settings->bAttenuate)
			{
				return Component->VolumeMultiplier;
			}
			const float Inner = Settings->AttenuationShapeExtents.X;
			const float Falloff = FMath::Max(Settings->FalloffDistance, 1.0f);
			const float Dist = FVector::Dist(Component->GetComponentLocation(), Listener);
			const float Alpha = FMath::Clamp((Dist - Inner) / Falloff, 0.0f, 1.0f);
			return Component->VolumeMultiplier * (1.0f - Alpha);
		}

		bool TeleportPawnTo(APawn* Pawn, const FVector& XY, float CapsuleZ)
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
				Character->SetActorEnableCollision(true);
			}
			const bool bMoved = OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
			return bMoved;
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

		FString CollectPlayingLine(UWorld* World, APawn* Pawn)
		{
			const FVector Listener = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
			TArray<FString> Parts;
			int32 HissPlaying = 0;
			int32 AlarmPlaying = 0;
			int32 Playing = 0;
			float HissGain = 0.0f;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				++Playing;
				USoundBase* Sound = Component->GetSound();
				const FString SoundName = Sound ? Sound->GetName() : TEXT("none");
				if (SoundName.Contains(TEXT("SW_HazardHiss")))
				{
					++HissPlaying;
					HissGain = FMath::Max(HissGain, EstimateSpatialGain(Component, Listener));
				}
				if (SoundName.Contains(TEXT("SW_AlarmPulse")))
				{
					if (Component->VolumeMultiplier > 0.05f)
					{
						++AlarmPlaying;
					}
				}
				bool bLooping = false;
				float Duration = Sound ? Sound->Duration : 0.0f;
				if (USoundWave* Wave = Cast<USoundWave>(Sound))
				{
					bLooping = Wave->bLooping;
					Duration = Wave->Duration;
				}
				AActor* Owner = Component->GetOwner();
				const FString OwnerLabel = Owner ? OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none");
				Parts.Add(FString::Printf(
					TEXT("%s/%s/%s/vol=%.3f/pitch=%.3f/spatial=%d/ui=%d/loop=%d/dur=%.2f/dist=%.0f/ovatt=%d/fall=%.0f/inner=%.0f/egain=%.3f"),
					*OwnerLabel,
					*Component->GetName(),
					*SoundName,
					Component->VolumeMultiplier,
					Component->PitchMultiplier,
					Component->bAllowSpatialization ? 1 : 0,
					Component->bIsUISound ? 1 : 0,
					bLooping ? 1 : 0,
					Duration,
					FVector::Dist(Component->GetComponentLocation(), Listener),
					Component->bOverrideAttenuation ? 1 : 0,
					Component->bOverrideAttenuation ? Component->AttenuationOverrides.FalloffDistance : 0.0f,
					Component->bOverrideAttenuation ? Component->AttenuationOverrides.AttenuationShapeExtents.X : 0.0f,
					EstimateSpatialGain(Component, Listener)));
			}
			LastPlayingCount = Playing;
			LastHissPlaying = HissPlaying;
			LastAlarmPlaying = AlarmPlaying;
			LastHissEstimatedGain = HissGain;
			return FString::Join(Parts, TEXT(" || "));
		}

		void RecordContext(FOrganoidPlaytestRecord& Record, const FString& Prefix, UWorld* World, APawn* Pawn)
		{
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Pawn);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidAdminFacilityStateSubsystem* Facility = World ? World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>() : nullptr;
			const FVector Loc = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;
			Record.AddActor(Prefix + TEXT(".xyz"), FString::Printf(TEXT("%.0f,%.0f,%.0f"), Loc.X, Loc.Y, Loc.Z));
			Record.AddActor(Prefix + TEXT(".time"), FString::Printf(TEXT("%.2f"), World ? World->GetTimeSeconds() : 0.0f));
			Record.AddActor(Prefix + TEXT(".health"), Character ? VolumeText(Character->GetHealth()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".toxicity"), Character ? VolumeText(Character->GetToxicity()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".zone"), Ambience && !Ambience->GetActiveEnvironmentZoneId().IsNone() ? Ambience->GetActiveEnvironmentZoneId().ToString() : TEXT("None"));
			Record.AddActor(Prefix + TEXT(".ambience"), Ambience ? AmbienceStateText(Ambience->GetAmbienceState()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".combat_vol"), Ambience ? VolumeText(Ambience->GetCombatLayerVolume()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".critical_vol"), Ambience ? VolumeText(Ambience->GetCriticalLayerVolume()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".power"), Power ? UEnum::GetValueAsString(Power->GetFacilityPowerState()) : TEXT("none"));
			Record.AddActor(Prefix + TEXT(".facility"), Facility ? UEnum::GetValueAsString(Facility->GetAdminFacilityState()) : TEXT("none"));
			TArray<FString> Streams;
			if (World)
			{
				for (ULevelStreaming* Streaming : World->GetStreamingLevels())
				{
					if (Streaming && Streaming->IsLevelLoaded() && Streaming->IsLevelVisible())
					{
						Streams.Add(Streaming->GetWorldAssetPackageFName().ToString());
					}
				}
			}
			Record.AddActor(Prefix + TEXT(".streams"), FString::Join(Streams, TEXT(",")));
			Record.AddActor(Prefix + TEXT(".playing"), CollectPlayingLine(World, Pawn));
			Record.AddActor(Prefix + TEXT(".playing_count"), FString::FromInt(LastPlayingCount));
			Record.AddActor(Prefix + TEXT(".hiss_playing"), FString::FromInt(LastHissPlaying));
			Record.AddActor(Prefix + TEXT(".hiss_egain"), VolumeText(LastHissEstimatedGain));
			Record.AddActor(Prefix + TEXT(".alarm_playing"), FString::FromInt(LastAlarmPlaying));
			AActor* Hazard = OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel);
			if (Hazard && Pawn)
			{
				Record.AddActor(Prefix + TEXT(".decon_dist"), FString::Printf(TEXT("%.0f"), FVector::Dist(Pawn->GetActorLocation(), Hazard->GetActorLocation())));
			}
			TArray<FString> Nearby;
			if (World && Pawn)
			{
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Actor = *It;
					if (!Actor || Actor == Pawn)
					{
						continue;
					}
					const float Dist = FVector::Dist(Actor->GetActorLocation(), Loc);
					if (Dist > 1200.0f)
					{
						continue;
					}
					const FString Name = OrganoidPlaytestActions::ActorLabel(Actor);
					const FString Class = Actor->GetClass() ? Actor->GetClass()->GetName() : TEXT("none");
					const bool bPawn = Cast<APawn>(Actor) != nullptr;
					const bool bInteresting = bPawn
						|| Name.Contains(TEXT("Host"))
						|| Name.Contains(TEXT("Hazard"))
						|| Name.Contains(TEXT("Trap"))
						|| Class.Contains(TEXT("Hazard"))
						|| Class.Contains(TEXT("Host"))
						|| Class.Contains(TEXT("Weapon"));
					if (bInteresting)
					{
						Nearby.Add(FString::Printf(TEXT("%s/%s/d=%.0f"), *Name, *Class, Dist));
					}
				}
			}
			Nearby.Sort();
			Record.AddActor(Prefix + TEXT(".nearby"), Nearby.Num() > 0 ? FString::Join(Nearby, TEXT(" || ")) : TEXT("none"));
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
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty. Refusing to start."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
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
				WaitSeconds = 0.0f;
				bTeleported = false;
				RouteIndex = 0;
				CallSetCombatActive(World, false);
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
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during route listen."));
				return;
			}
			if (!bTeleported)
			{
				if (!TeleportPawnTo(Pawn, RouteStops[RouteIndex].XY, CapsuleZFor(Pawn)))
				{
					FailAndStop(Owner, Record, FString::Printf(TEXT("Teleport to %s failed."), RouteStops[RouteIndex].Name));
					return;
				}
				bTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			CollectPlayingLine(World, Pawn);
			if (LastHissPlaying > 0)
			{
				++RouteHissFrames;
			}
			if (WaitSeconds < ListenSeconds)
			{
				return;
			}
			RecordContext(Record, FString::Printf(TEXT("route.%s"), RouteStops[RouteIndex].Name), World, Pawn);
			HissAtStop[RouteIndex] = LastHissPlaying;
			AlarmAtStop[RouteIndex] = LastAlarmPlaying;
			++RouteIndex;
			bTeleported = false;
			WaitSeconds = 0.0f;
			if (RouteIndex >= RouteCount)
			{
				Stage = EStage::HazardNear;
				Owner.SetStage(TEXT("HazardNear"));
			}
		}

		void TickHazardNear(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Hazard = World ? OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel) : nullptr;
			if (!World || !Pawn || !Hazard)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn or Hazard_DeconUVC during HazardNear."));
				return;
			}
			if (!bTeleported)
			{
				const FVector HazardXY(Hazard->GetActorLocation().X, Hazard->GetActorLocation().Y, 0.0f);
				if (!TeleportPawnTo(Pawn, HazardXY, CapsuleZFor(Pawn)))
				{
					FailAndStop(Owner, Record, TEXT("Teleport into Hazard_DeconUVC for A/B failed."));
					return;
				}
				bTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < InsideListenSeconds)
			{
				return;
			}
			RecordContext(Record, TEXT("ab.inside_baseline"), World, Pawn);
			NearBaselineHiss = LastHissPlaying;
			NearBaselineAlarm = LastAlarmPlaying;
			bTeleported = false;
			WaitSeconds = 0.0f;
			Stage = EStage::HazardMuted;
			Owner.SetStage(TEXT("HazardMuted"));
		}

		void TickHazardMuted(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			UAudioComponent* HazardAudio = FindHazardAudio();
			if (!World || !Pawn || !HazardAudio)
			{
				FailAndStop(Owner, Record, TEXT("Hazard_DeconUVC HazardAudio missing for A/B mute."));
				return;
			}
			if (!bHazardMuted)
			{
				SavedHazardVolume = HazardAudio->VolumeMultiplier;
				HazardAudio->Stop();
				HazardAudio->SetVolumeMultiplier(0.0f);
				bHazardMuted = true;
				bHazardRestored = false;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < MuteListenSeconds)
			{
				return;
			}
			RecordContext(Record, TEXT("ab.inside_muted"), World, Pawn);
			NearMutedHiss = LastHissPlaying;
			NearMutedAlarm = LastAlarmPlaying;
			WaitSeconds = 0.0f;
			Stage = EStage::HazardRestore;
			Owner.SetStage(TEXT("HazardRestore"));
		}

		void TickHazardRestore(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			RestoreHazardIfNeeded();
			WaitSeconds += DeltaTime;
			if (WaitSeconds < InsideListenSeconds)
			{
				return;
			}
			RecordContext(Record, TEXT("ab.inside_restored"), World, Pawn);
			InsideRestoredHiss = LastHissPlaying;
			RestoreVitals(Pawn);
			CallSetCombatActive(World, false);
			(void)Owner;
			bTeleported = false;
			WaitSeconds = 0.0f;
			Stage = EStage::HazardFar;
			Owner.SetStage(TEXT("HazardFar"));
		}

		void TickHazardFar(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during HazardFar."));
				return;
			}
			if (!bTeleported)
			{
				RestoreVitals(Pawn);
				if (!TeleportPawnTo(Pawn, FVector(3200.0f, -1850.0f, 0.0f), CapsuleZFor(Pawn)))
				{
					FailAndStop(Owner, Record, TEXT("Teleport to Executive for hazard far A/B failed."));
					return;
				}
				bTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ListenSeconds)
			{
				return;
			}
			RecordContext(Record, TEXT("ab.far_executive"), World, Pawn);
			FarHiss = LastHissPlaying;
			FarAlarm = LastAlarmPlaying;
			bTeleported = false;
			WaitSeconds = 0.0f;
			Stage = EStage::ExtendedListen;
			Owner.SetStage(TEXT("ExtendedListen"));
		}

		void TickExtendedListen(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during extended listen."));
				return;
			}
			if (!bTeleported)
			{
				RestoreVitals(Pawn);
				CallSetCombatActive(World, false);
				if (!TeleportPawnTo(Pawn, FVector(0.0f, 0.0f, 0.0f), CapsuleZFor(Pawn)))
				{
					FailAndStop(Owner, Record, TEXT("Teleport to Vestibule for extended listen failed."));
					return;
				}
				bTeleported = true;
				WaitSeconds = 0.0f;
				ExtendedHissFrames = 0;
				ExtendedAlarmFrames = 0;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds >= 2.5f)
			{
				CollectPlayingLine(World, Pawn);
				if (LastHissPlaying > 0)
				{
					++ExtendedHissFrames;
				}
				if (LastAlarmPlaying > 0)
				{
					++ExtendedAlarmFrames;
				}
			}
			if (WaitSeconds < ExtendedListenSeconds + 2.5f)
			{
				return;
			}
			RecordContext(Record, TEXT("extended.vestibule"), World, Pawn);
			Record.AddActor(TEXT("extended.seconds"), VolumeText(WaitSeconds));
			Record.AddActor(TEXT("extended.hiss_frames"), FString::FromInt(ExtendedHissFrames));
			Record.AddActor(TEXT("extended.alarm_frames"), FString::FromInt(ExtendedAlarmFrames));
			Stage = EStage::Assert;
			Owner.SetStage(TEXT("Assert"));
		}

		void TickAssert(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Record.AddActor(TEXT("route.spawn_alarm"), FString::FromInt(AlarmAtStop[0]));
			Record.AddActor(TEXT("route.vestibule_alarm"), FString::FromInt(AlarmAtStop[1]));
			AssertTrue(Record, TEXT("route.hiss_silent_outside_volume"),
				RouteHissFrames == 0,
				TEXT("0"), FString::FromInt(RouteHissFrames), HazardLabel, false);
			AssertTrue(Record, TEXT("ab.hazard_present_inside"),
				NearBaselineHiss >= 1,
				TEXT(">=1"), FString::FromInt(NearBaselineHiss), HazardLabel, false);
			AssertTrue(Record, TEXT("ab.hazard_mute_stops_hiss"),
				NearMutedHiss == 0,
				TEXT("0"), FString::FromInt(NearMutedHiss), HazardLabel, false);
			AssertTrue(Record, TEXT("ab.hazard_restore_restarts_hiss"),
				InsideRestoredHiss >= 1,
				TEXT(">=1"), FString::FromInt(InsideRestoredHiss), HazardLabel, false);
			AssertTrue(Record, TEXT("ab.hazard_far_hiss_off"),
				FarHiss == 0,
				TEXT("0"), FString::FromInt(FarHiss), TEXT("Executive"), false);
			AssertTrue(Record, TEXT("ab.alarm_unchanged_by_hazard_mute"),
				NearMutedHiss == 0,
				TEXT("hiss muted independently of alarm"),
				FString::Printf(TEXT("hiss=%d alarm=%d"), NearMutedHiss, NearMutedAlarm), TEXT("SW_HazardHiss"), false);
			AssertTrue(Record, TEXT("extended.no_hiss"),
				ExtendedHissFrames == 0,
				TEXT("0"), FString::FromInt(ExtendedHissFrames), TEXT("Vestibule"), false);
			AssertTrue(Record, TEXT("extended.no_alarm"),
				ExtendedAlarmFrames == 0,
				TEXT("0"), FString::FromInt(ExtendedAlarmFrames), TEXT("SW_AlarmPulse"), false);

			int32 HissStops = 0;
			for (int32 Index = 0; Index < RouteCount; ++Index)
			{
				if (HissAtStop[Index] > 0)
				{
					++HissStops;
				}
				Record.AddActor(FString::Printf(TEXT("summary.hiss_%s"), RouteStops[Index].Name), FString::FromInt(HissAtStop[Index]));
			}
			Record.AddActor(TEXT("summary.hiss_stop_count"), FString::FromInt(HissStops));
			Record.AddActor(TEXT("summary.inside_hiss"), FString::FromInt(NearBaselineHiss));
			Record.AddActor(TEXT("summary.muted_hiss"), FString::FromInt(NearMutedHiss));
			Record.AddActor(TEXT("summary.restored_hiss"), FString::FromInt(InsideRestoredHiss));
			Record.AddActor(TEXT("summary.far_hiss"), FString::FromInt(FarHiss));
			Record.AddActor(TEXT("summary.route_hiss_frames"), FString::FromInt(RouteHissFrames));
			Record.AddActor(TEXT("summary.extended_hiss_frames"), FString::FromInt(ExtendedHissFrames));
			Record.AddActor(TEXT("summary.extended_alarm_frames"), FString::FromInt(ExtendedAlarmFrames));
			AssertTrue(Record, TEXT("route.no_hiss_at_any_stop"),
				HissStops == 0,
				TEXT("0"), FString::FromInt(HissStops), HazardLabel, false);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Beep isolation assertions failed."));
				return;
			}
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
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
		bool bTeleported = false;
		bool bHazardMuted = false;
		bool bHazardRestored = false;
		bool bAnyAssertFailed = false;
		float SavedHazardVolume = 0.45f;
		int32 LastPlayingCount = 0;
		int32 LastHissPlaying = 0;
		int32 LastAlarmPlaying = 0;
		float LastHissEstimatedGain = 0.0f;
		int32 HissAtStop[RouteCount] = {};
		int32 AlarmAtStop[RouteCount] = {};
		int32 NearBaselineHiss = 0;
		int32 NearBaselineAlarm = 0;
		int32 NearMutedHiss = 0;
		int32 NearMutedAlarm = 0;
		int32 InsideRestoredHiss = 0;
		int32 FarHiss = 0;
		int32 FarAlarm = 0;
		int32 RouteHissFrames = 0;
		int32 ExtendedHissFrames = 0;
		int32 ExtendedAlarmFrames = 0;
		TArray<FString> DirtyBefore;
	};

	struct FBeepSourceIsolationAutoRegister
	{
		FBeepSourceIsolationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FBeepSourceIsolationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FBeepSourceIsolationAutoRegister GBeepSourceIsolationAutoRegister;
}
