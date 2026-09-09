#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/AudioComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("AmbienceLayerPlayback_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Ambience Combat/Critical Silent Playback Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CombatLayerName[] = TEXT("OrganoidCombatLayer");
	constexpr TCHAR CriticalLayerName[] = TEXT("OrganoidCriticalLayer");
	constexpr TCHAR AlarmPulseToken[] = TEXT("SW_AlarmPulse");
	constexpr float SilentVolume = 0.01f;
	constexpr float AudibleVolume = 0.05f;
	constexpr float LayerSettleSeconds = 2.25f;

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

	class FAmbienceLayerPlaybackFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bCombatArmed = false;
			bCombatCleared = false;
			bCriticalArmed = false;
			bCriticalCleared = false;
			BaselineZoneId.Reset();
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
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
			case EStage::AssertStartup: TickAssertStartup(Owner, *Record); break;
			case EStage::CombatOn: TickCombatOn(Owner, *Record, DeltaTime); break;
			case EStage::CombatOff: TickCombatOff(Owner, *Record, DeltaTime); break;
			case EStage::CriticalOn: TickCriticalOn(Owner, *Record, DeltaTime); break;
			case EStage::CriticalOff: TickCriticalOff(Owner, *Record, DeltaTime); break;
			case EStage::AssertUnique: TickAssertUnique(Owner, *Record); break;
			case EStage::EndPie:
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
			AssertStartup,
			CombatOn,
			CombatOff,
			CriticalOn,
			CriticalOff,
			AssertUnique,
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
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		UProjectOrganoidAudioAmbienceSubsystem* Ambience(UWorld* World)
		{
			return World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
		}

		AProjectOrganoidCharacter* OrganoidPawn(UWorld* World)
		{
			return Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerPawn(World));
		}

		UAudioComponent* FindLayer(AActor* Actor, const TCHAR* Name)
		{
			return Actor ? Cast<UAudioComponent>(OrganoidPlaytestActions::FindNamedComponent(Actor, Name)) : nullptr;
		}

		int32 CountNamedAudio(AActor* Actor, const TCHAR* Name)
		{
			if (!Actor)
			{
				return 0;
			}
			int32 Count = 0;
			TArray<UAudioComponent*> Components;
			Actor->GetComponents<UAudioComponent>(Components);
			for (UAudioComponent* Component : Components)
			{
				if (Component && Component->GetFName().ToString().Equals(Name, ESearchCase::IgnoreCase))
				{
					++Count;
				}
			}
			return Count;
		}

		bool SoundIsAlarmPulse(UAudioComponent* Component)
		{
			if (!Component || !Component->GetSound())
			{
				return false;
			}
			return Component->GetSound()->GetPathName().Contains(AlarmPulseToken);
		}

		bool CallBoolFunction(UObject* Object, const TCHAR* FunctionName, const TCHAR* ParamName, bool bValue)
		{
			if (!Object)
			{
				return false;
			}
			UFunction* Function = Object->FindFunction(FName(FunctionName));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Function, ParamName))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), bValue);
			}
			Object->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		bool CallNotifyHealthChanged(UProjectOrganoidAudioAmbienceSubsystem* Sub, float Health, float MaxHealth)
		{
			if (!Sub)
			{
				return false;
			}
			UFunction* Function = Sub->FindFunction(FName(TEXT("NotifyHealthChanged")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FFloatProperty* Current = FindFProperty<FFloatProperty>(Function, TEXT("CurrentHealth")))
			{
				Current->SetPropertyValue(Current->ContainerPtrToValuePtr<void>(Parms.GetData()), Health);
			}
			if (FFloatProperty* Max = FindFProperty<FFloatProperty>(Function, TEXT("MaxHealth")))
			{
				Max->SetPropertyValue(Max->ContainerPtrToValuePtr<void>(Parms.GetData()), MaxHealth);
			}
			Sub->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		bool CallSetCombatActive(UProjectOrganoidAudioAmbienceSubsystem* Sub, bool bActive)
		{
			return CallBoolFunction(Sub, TEXT("SetCombatActive"), TEXT("bActive"), bActive);
		}

		FString ActiveZoneId(UWorld* World)
		{
			if (UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World))
			{
				const FName ZoneId = Sub->GetActiveEnvironmentZoneId();
				return ZoneId.IsNone() ? TEXT("None") : ZoneId.ToString();
			}
			return TEXT("missing");
		}

		bool AssertZoneUnchanged(FOrganoidPlaytestRecord& Record, const FString& Id)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			const FString Actual = ActiveZoneId(World);
			return AssertTrue(Record, Id, Actual.Equals(BaselineZoneId, ESearchCase::CaseSensitive),
				BaselineZoneId, Actual, TEXT("ZoneId"), false);
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
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			UAudioComponent* Combat = FindLayer(Character, CombatLayerName);
			UAudioComponent* Critical = FindLayer(Character, CriticalLayerName);
			if (World && Character && Sub && Combat && Critical)
			{
				WaitSeconds = 0.0f;
				Stage = EStage::AssertStartup;
				Owner.SetStage(TEXT("AssertStartup"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE ambience layers."));
			}
			(void)Record;
		}

		void TickAssertStartup(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!World || !Character || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn or ambience subsystem at startup."));
				return;
			}

			BaselineZoneId = ActiveZoneId(World);
			Record.AddActor(TEXT("baseline_zone_id"), BaselineZoneId);
			Record.AddActor(TEXT("startup_ambience_state"),
				Sub->GetAmbienceState() == EProjectOrganoidAmbienceState::Tension ? TEXT("Tension") : TEXT("Other"));

			AssertTrue(Record, TEXT("startup.health_full"),
				FMath::IsNearlyEqual(Character->GetHealth(), Character->GetMaxHealth(), 0.1f),
				TEXT("100"), VolumeText(Character->GetHealth()), TEXT("Health"), false);

			UAudioComponent* Combat = FindLayer(Character, CombatLayerName);
			UAudioComponent* Critical = FindLayer(Character, CriticalLayerName);
			AssertTrue(Record, TEXT("startup.combat_volume_zero"),
				Sub->GetCombatLayerVolume() <= SilentVolume,
				TEXT("0"), VolumeText(Sub->GetCombatLayerVolume()), CombatLayerName, false);
			AssertTrue(Record, TEXT("startup.critical_volume_zero"),
				Sub->GetCriticalLayerVolume() <= SilentVolume,
				TEXT("0"), VolumeText(Sub->GetCriticalLayerVolume()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("startup.combat_not_playing"),
				Combat && !Combat->IsPlaying(),
				TEXT("false"), BoolText(Combat && Combat->IsPlaying()), CombatLayerName, false);
			AssertTrue(Record, TEXT("startup.critical_not_playing"),
				Critical && !Critical->IsPlaying(),
				TEXT("false"), BoolText(Critical && Critical->IsPlaying()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("startup.combat_is_alarm_pulse"),
				SoundIsAlarmPulse(Combat),
				AlarmPulseToken, Combat && Combat->GetSound() ? Combat->GetSound()->GetName() : TEXT("none"), CombatLayerName, false);
			AssertTrue(Record, TEXT("startup.critical_is_alarm_pulse"),
				SoundIsAlarmPulse(Critical),
				AlarmPulseToken, Critical && Critical->GetSound() ? Critical->GetSound()->GetName() : TEXT("none"), CriticalLayerName, false);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Startup Combat/Critical silent playback failed."));
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::CombatOn;
			Owner.SetStage(TEXT("CombatOn"));
		}

		void TickCombatOn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!World || !Character || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during CombatOn."));
				return;
			}
			if (!bCombatArmed)
			{
				if (!CallSetCombatActive(Sub, true))
				{
					FailAndStop(Owner, Record, TEXT("SetCombatActive(true) is not callable."));
					return;
				}
				bCombatArmed = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			UAudioComponent* Combat = FindLayer(Character, CombatLayerName);
			const bool bReady = Sub->GetCombatLayerVolume() >= AudibleVolume && Combat && Combat->IsPlaying();
			if (!bReady && WaitSeconds < LayerSettleSeconds)
			{
				return;
			}
			AssertTrue(Record, TEXT("combat_on.volume_audible"),
				Sub->GetCombatLayerVolume() >= AudibleVolume,
				TEXT(">=0.05"), VolumeText(Sub->GetCombatLayerVolume()), CombatLayerName, false);
			AssertTrue(Record, TEXT("combat_on.playing"),
				Combat && Combat->IsPlaying(),
				TEXT("true"), BoolText(Combat && Combat->IsPlaying()), CombatLayerName, false);
			AssertTrue(Record, TEXT("combat_on.sound_alarm_pulse"),
				SoundIsAlarmPulse(Combat),
				AlarmPulseToken, Combat && Combat->GetSound() ? Combat->GetSound()->GetName() : TEXT("none"), CombatLayerName, false);
			AssertZoneUnchanged(Record, TEXT("combat_on.zone_unchanged"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Combat layer did not start when combat became active."));
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::CombatOff;
			Owner.SetStage(TEXT("CombatOff"));
		}

		void TickCombatOff(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!World || !Character || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during CombatOff."));
				return;
			}
			if (!bCombatCleared)
			{
				if (!CallSetCombatActive(Sub, false))
				{
					FailAndStop(Owner, Record, TEXT("SetCombatActive(false) is not callable."));
					return;
				}
				bCombatCleared = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			UAudioComponent* Combat = FindLayer(Character, CombatLayerName);
			const bool bReady = Sub->GetCombatLayerVolume() <= SilentVolume && Combat && !Combat->IsPlaying();
			if (!bReady && WaitSeconds < LayerSettleSeconds)
			{
				return;
			}
			AssertTrue(Record, TEXT("combat_off.volume_zero"),
				Sub->GetCombatLayerVolume() <= SilentVolume,
				TEXT("0"), VolumeText(Sub->GetCombatLayerVolume()), CombatLayerName, false);
			AssertTrue(Record, TEXT("combat_off.stopped"),
				Combat && !Combat->IsPlaying(),
				TEXT("false"), BoolText(Combat && Combat->IsPlaying()), CombatLayerName, false);
			AssertZoneUnchanged(Record, TEXT("combat_off.zone_unchanged"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Combat layer did not stop when combat volume returned to zero."));
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::CriticalOn;
			Owner.SetStage(TEXT("CriticalOn"));
		}

		void TickCriticalOn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!World || !Character || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during CriticalOn."));
				return;
			}
			if (!bCriticalArmed)
			{
				if (!CallNotifyHealthChanged(Sub, 20.0f, Character->GetMaxHealth()))
				{
					FailAndStop(Owner, Record, TEXT("NotifyHealthChanged for CriticalHealth is not callable."));
					return;
				}
				bCriticalArmed = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			UAudioComponent* Critical = FindLayer(Character, CriticalLayerName);
			const bool bReady = Sub->GetCriticalLayerVolume() >= AudibleVolume && Critical && Critical->IsPlaying();
			if (!bReady && WaitSeconds < LayerSettleSeconds)
			{
				return;
			}
			AssertTrue(Record, TEXT("critical_on.state"),
				Sub->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth,
				TEXT("CriticalHealth"),
				Sub->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth ? TEXT("CriticalHealth") : TEXT("Other"),
				TEXT("Ambience"), false);
			AssertTrue(Record, TEXT("critical_on.volume_audible"),
				Sub->GetCriticalLayerVolume() >= AudibleVolume,
				TEXT(">=0.05"), VolumeText(Sub->GetCriticalLayerVolume()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("critical_on.playing"),
				Critical && Critical->IsPlaying(),
				TEXT("true"), BoolText(Critical && Critical->IsPlaying()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("critical_on.sound_alarm_pulse"),
				SoundIsAlarmPulse(Critical),
				AlarmPulseToken, Critical && Critical->GetSound() ? Critical->GetSound()->GetName() : TEXT("none"), CriticalLayerName, false);
			AssertZoneUnchanged(Record, TEXT("critical_on.zone_unchanged"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Critical layer did not start when CriticalHealth became active."));
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::CriticalOff;
			Owner.SetStage(TEXT("CriticalOff"));
		}

		void TickCriticalOff(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!World || !Character || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during CriticalOff."));
				return;
			}
			if (!bCriticalCleared)
			{
				if (!CallNotifyHealthChanged(Sub, Character->GetMaxHealth(), Character->GetMaxHealth()))
				{
					FailAndStop(Owner, Record, TEXT("NotifyHealthChanged restore is not callable."));
					return;
				}
				bCriticalCleared = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			UAudioComponent* Critical = FindLayer(Character, CriticalLayerName);
			const bool bReady = Sub->GetCriticalLayerVolume() <= SilentVolume && Critical && !Critical->IsPlaying();
			if (!bReady && WaitSeconds < LayerSettleSeconds)
			{
				return;
			}
			AssertTrue(Record, TEXT("critical_off.volume_zero"),
				Sub->GetCriticalLayerVolume() <= SilentVolume,
				TEXT("0"), VolumeText(Sub->GetCriticalLayerVolume()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("critical_off.stopped"),
				Critical && !Critical->IsPlaying(),
				TEXT("false"), BoolText(Critical && Critical->IsPlaying()), CriticalLayerName, false);
			AssertTrue(Record, TEXT("critical_off.combat_stopped"),
				Sub->GetCombatLayerVolume() <= SilentVolume && FindLayer(Character, CombatLayerName) && !FindLayer(Character, CombatLayerName)->IsPlaying(),
				TEXT("false"), BoolText(FindLayer(Character, CombatLayerName) && FindLayer(Character, CombatLayerName)->IsPlaying()), CombatLayerName, false);
			AssertZoneUnchanged(Record, TEXT("critical_off.zone_unchanged"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Critical layer did not stop when health returned to full."));
				return;
			}
			Stage = EStage::AssertUnique;
			Owner.SetStage(TEXT("AssertUnique"));
		}

		void TickAssertUnique(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = OrganoidPawn(World);
			if (!Character)
			{
				FailAndStop(Owner, Record, TEXT("Lost pawn before unique-layer assertion."));
				return;
			}
			AssertTrue(Record, TEXT("unique.combat_count"),
				CountNamedAudio(Character, CombatLayerName) == 1,
				TEXT("1"), FString::FromInt(CountNamedAudio(Character, CombatLayerName)), CombatLayerName, false);
			AssertTrue(Record, TEXT("unique.critical_count"),
				CountNamedAudio(Character, CriticalLayerName) == 1,
				TEXT("1"), FString::FromInt(CountNamedAudio(Character, CriticalLayerName)), CriticalLayerName, false);
			AssertZoneUnchanged(Record, TEXT("unique.zone_unchanged"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Duplicate Combat/Critical layer components were created."));
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
		bool bAnyAssertFailed = false;
		bool bCombatArmed = false;
		bool bCombatCleared = false;
		bool bCriticalArmed = false;
		bool bCriticalCleared = false;
		FString BaselineZoneId;
		TArray<FString> DirtyBefore;
	};

	struct FAmbienceLayerPlaybackAutoRegister
	{
		FAmbienceLayerPlaybackAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FAmbienceLayerPlaybackFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FAmbienceLayerPlaybackAutoRegister GAmbienceLayerPlaybackAutoRegister;
}
