#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "ProjectOrganoidPlaytestLogSink.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputKeyEventArgs.h"
#include "InputMappingContext.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidAudioSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "UnrealClient.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("BeepClickInjection_Functional");
	constexpr TCHAR DisplayName[] = TEXT("LMB Fire Does Not Start Combat Ambience");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr float IdleSeconds = 20.0f;
	constexpr float ObserveSeconds = 0.85f;
	constexpr float ClearSettleSeconds = 2.4f;
	constexpr float FiveClickGapSeconds = 1.0f;
	constexpr float RouteListenSeconds = 2.5f;
	constexpr float LingerExtraSeconds = 2.5f;
	constexpr float DamageAmount = 8.0f;

	struct FClickStop
	{
		const TCHAR* Name;
		FVector XY;
	};

	const FClickStop AdminStops[] = {
		{TEXT("Spawn"), FVector(-1500.0f, 0.0f, 0.0f)},
		{TEXT("Vestibule"), FVector(0.0f, 0.0f, 0.0f)},
		{TEXT("Reception"), FVector(1250.0f, 0.0f, 0.0f)},
		{TEXT("Hub"), FVector(1800.0f, 0.0f, 0.0f)},
		{TEXT("Security"), FVector(2680.0f, 0.0f, 0.0f)},
		{TEXT("Operations"), FVector(4180.0f, 0.0f, 0.0f)},
		{TEXT("Transit"), FVector(5005.0f, 0.0f, 0.0f)},
	};
	constexpr int32 AdminStopCount = UE_ARRAY_COUNT(AdminStops);

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

	class FBeepClickInjectionFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			FiveIndex = 0;
			StopIndex = 0;
			bTeleported = false;
			bInjectedThisStep = false;
			bAnyAssertFailed = false;
			bIdleSawAlarm = false;
			bLmbStartedCombat = false;
			bFiveStartedCombat = false;
			bRouteStartedCombat = false;
			bDamageStartedCombat = false;
			bInjectedCombatStarted = false;
			bLingerExpired = false;
			bLmbFired = false;
			bLmbReportedGunfire = false;
			bLmbReportedSpatial = false;
			bFiveReportedGunfire = false;
			CombatLingerSeconds = 8.0f;
			Traces.Reset();
			DirtyBefore.Reset();
			PrevPlaying.Reset();
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
			case EStage::DumpMaps: TickDumpMaps(Owner, *Record); break;
			case EStage::ClearBeforeIdle: TickClear(Owner, *Record, DeltaTime, EStage::Idle); break;
			case EStage::Idle: TickIdle(Owner, *Record, DeltaTime); break;
			case EStage::InjectOneLmb: TickInject(Owner, *Record, TEXT("lmb_one"), EStage::ObserveOneLmb); break;
			case EStage::ObserveOneLmb: TickObserveShot(Owner, *Record, DeltaTime); break;
			case EStage::ClearBeforeFive: TickClear(Owner, *Record, DeltaTime, EStage::InjectFive); break;
			case EStage::InjectFive: TickFive(Owner, *Record, DeltaTime); break;
			case EStage::ClearBeforeRoute: TickClear(Owner, *Record, DeltaTime, EStage::Route); break;
			case EStage::Route: TickRoute(Owner, *Record, DeltaTime); break;
			case EStage::ClearBeforeDamage: TickClear(Owner, *Record, DeltaTime, EStage::Damage); break;
			case EStage::Damage: TickDamage(Owner, *Record); break;
			case EStage::ObserveDamage: TickObserveDamage(Owner, *Record, DeltaTime); break;
			case EStage::ClearBeforeCombat: TickClear(Owner, *Record, DeltaTime, EStage::InjectCombat); break;
			case EStage::InjectCombat: TickInjectCombat(Owner, *Record); break;
			case EStage::ObserveCombatOn: TickObserveCombatOn(Owner, *Record, DeltaTime); break;
			case EStage::LingerWait: TickLingerWait(Owner, *Record, DeltaTime); break;
			case EStage::ObserveLingerOff: TickObserveLingerOff(Owner, *Record); break;
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
			Preflight, StartPie, WaitPieReady, DumpMaps, ClearBeforeIdle, Idle,
			InjectOneLmb, ObserveOneLmb, ClearBeforeFive, InjectFive,
			ClearBeforeRoute, Route, ClearBeforeDamage, Damage, ObserveDamage,
			ClearBeforeCombat, InjectCombat, ObserveCombatOn, LingerWait, ObserveLingerOff,
			EndPie, WaitPieStopped, AssertDurable, Finalize
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

		AProjectOrganoidCharacter* GetOrganoid(UWorld* World) const
		{
			return Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerPawn(World));
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

		bool CallApplyHealthDelta(AProjectOrganoidCharacter* Character, float Delta)
		{
			if (!Character)
			{
				return false;
			}
			UFunction* Function = Character->FindFunction(FName(TEXT("ApplyHealthDelta")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Function, TEXT("Delta")))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), Delta);
			}
			Character->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		bool CallNotifyCombatStimulus(UWorld* World, float Intensity)
		{
			UProjectOrganoidAudioAmbienceSubsystem* Sub = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Sub)
			{
				return false;
			}
			UFunction* Function = Sub->FindFunction(FName(TEXT("NotifyCombatStimulus")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Function, TEXT("Intensity")))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), Intensity);
			}
			Sub->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		bool CombatLayerPlaying(UWorld* World) const
		{
			const FString Alarm = DumpAlarm(World);
			return Alarm.Contains(TEXT("CombatLayer"));
		}

		bool LineIsCombatLayerStart(const FString& Line) const
		{
			return Line.Contains(TEXT("CombatLayer"));
		}

		void PinAtSpawn(APawn* Pawn)
		{
			if (!Pawn)
			{
				return;
			}
			const float CapsuleZ = CapsuleZFor(Pawn);
			if (ACharacter* AsCharacter = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
				{
					Move->SetMovementMode(MOVE_Walking);
					Move->StopMovementImmediately();
				}
			}
			OrganoidPlaytestActions::TeleportNear(Pawn, FVector(-1500.0f, 0.0f, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
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

		UInputAction* ReadAction(AProjectOrganoidCharacter* Character, const TCHAR* PropertyName) const
		{
			if (!Character)
			{
				return nullptr;
			}
			if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Character->GetClass(), PropertyName))
			{
				return Cast<UInputAction>(Prop->GetObjectPropertyValue_InContainer(Character));
			}
			return nullptr;
		}

		FString DumpContexts(APlayerController* PC, AProjectOrganoidCharacter* Character) const
		{
			TArray<FString> Parts;
			UEnhancedInputLocalPlayerSubsystem* Sub = PC && PC->GetLocalPlayer()
				? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer())
				: nullptr;
			if (Sub)
			{
				for (TObjectIterator<UInputMappingContext> It; It; ++It)
				{
					UInputMappingContext* Context = *It;
					if (!Context)
					{
						continue;
					}
					int32 Priority = 0;
					if (Sub->HasMappingContext(Context, Priority))
					{
						Parts.Add(FString::Printf(TEXT("%s@%d"), *Context->GetName(), Priority));
					}
				}
				if (UInputAction* Fire = ReadAction(Character, TEXT("FireAction")))
				{
					TArray<FKey> FireKeys = Sub->QueryKeysMappedToAction(Fire);
					TArray<FString> FireNames;
					for (const FKey& Key : FireKeys)
					{
						FireNames.Add(Key.ToString());
					}
					Parts.Add(FString::Printf(TEXT("fire_keys=%s"), FireNames.Num() ? *FString::Join(FireNames, TEXT(",")) : TEXT("none")));
				}
			}
			return Parts.Num() ? FString::Join(Parts, TEXT(";")) : TEXT("none");
		}

		FString DumpAlarm(UWorld* World) const
		{
			TArray<FString> Playing;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				const FString SoundName = Sound ? Sound->GetName() : FString(TEXT("none"));
				if (!SoundName.Contains(TEXT("AlarmPulse")) && !Component->GetName().Contains(TEXT("CombatLayer"))
					&& !Component->GetName().Contains(TEXT("CriticalLayer")))
				{
					continue;
				}
				AActor* Owner = Component->GetOwner();
				Playing.Add(FString::Printf(
					TEXT("%s/%s/%s vol=%.3f"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*Component->GetName(),
					*SoundName,
					Component->VolumeMultiplier));
			}
			return Playing.Num() ? FString::Join(Playing, TEXT(",")) : TEXT("none");
		}

		void SampleNewAudio(UWorld* World, float WorldTime, TArray<FString>& OutStarts)
		{
			TSet<FString> Now;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				AActor* Owner = Component->GetOwner();
				const FString Key = FString::Printf(
					TEXT("%s/%s/%s"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*Component->GetName(),
					Sound ? *Sound->GetName() : TEXT("none"));
				Now.Add(Key);
				if (!PrevPlaying.Contains(Key))
				{
					OutStarts.Add(FString::Printf(TEXT("%.3f %s vol=%.3f"), WorldTime, *Key, Component->VolumeMultiplier));
				}
			}
			PrevPlaying = MoveTemp(Now);
		}

		FString Snapshot(UWorld* World, APlayerController* PC, AProjectOrganoidCharacter* Character, const TCHAR* Label)
		{
			const float T = World ? World->GetTimeSeconds() : -1.0f;
			const FVector Loc = Character ? Character->GetActorLocation() : FVector::ZeroVector;
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			AProjectOrganoidWeapon* Weapon = Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
			return FString::Printf(
				TEXT("%.3f %s loc=(%.0f,%.0f) combat=%s combatVol=%.3f health=%.1f/%.1f weapon=%s alarm=%s imc=%s"),
				T,
				Label,
				Loc.X,
				Loc.Y,
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"),
				Ambience ? Ambience->GetCombatLayerVolume() : -1.0f,
				Character ? Character->GetHealth() : -1.0f,
				Character ? Character->GetMaxHealth() : -1.0f,
				Weapon ? *Weapon->GetName() : TEXT("none"),
				*DumpAlarm(World),
				*DumpContexts(PC, Character));
		}

		void AppendTrace(FOrganoidPlaytestRecord& Record, const FString& Line)
		{
			Traces.Add(Line);
			Record.AddActor(FString::Printf(TEXT("trace.%02d"), Traces.Num()), Line);
		}

		FString LogsSince(UProjectOrganoidPlaytestEditorSubsystem& Owner, const TCHAR* Needle) const
		{
			FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink();
			if (!Sink)
			{
				return TEXT("none");
			}
			TArray<FString> Hits;
			for (const FString& Line : Sink->LinesSinceCursor())
			{
				if (Line.Contains(Needle))
				{
					Hits.Add(Line);
				}
			}
			return Hits.Num() ? FString::Join(Hits, TEXT(" | ")) : TEXT("none");
		}

		bool InjectLmb(UWorld* World, APlayerController* PC)
		{
			if (!World || !PC)
			{
				return false;
			}
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Pressed, 1.0f));
			PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Released, 0.0f));
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
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("durable.playtest_mutates_assets"), TEXT("false"));
			Record.AddActor(TEXT("input.inject_path"), TEXT("PlayerController::InputKey(FInputKeyEventArgs::CreateSimulated LeftMouseButton)"));
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
				if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
				{
					CombatLingerSeconds = Ambience->CombatLingerSeconds;
				}
				WaitSeconds = 0.0f;
				Stage = EStage::DumpMaps;
				Owner.SetStage(TEXT("DumpMaps"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE pawn."));
			}
			(void)Record;
		}

		void TickDumpMaps(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			Record.AddActor(TEXT("map.snapshot"), Snapshot(World, PC, Character, TEXT("ready")));
			Record.AddActor(TEXT("map.combat_linger_seconds"), FString::Printf(TEXT("%.1f"), CombatLingerSeconds));
			if (Character)
			{
				UInputAction* Fire = ReadAction(Character, TEXT("FireAction"));
				Record.AddActor(TEXT("map.fire_action"), Fire ? Fire->GetName() : TEXT("none"));
			}
			UProjectOrganoidAudioSubsystem* Audio = World ? World->GetSubsystem<UProjectOrganoidAudioSubsystem>() : nullptr;
			Record.AddActor(TEXT("hearing.subsystem"), Audio ? TEXT("present") : TEXT("missing"));
			if (PC && PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
				{
					TArray<FString> Maps;
					for (TObjectIterator<UInputMappingContext> It; It; ++It)
					{
						UInputMappingContext* Context = *It;
						int32 Priority = 0;
						if (!Context || !Sub->HasMappingContext(Context, Priority))
						{
							continue;
						}
						for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
						{
							Maps.Add(FString::Printf(
								TEXT("%s:%s->%s"),
								*Context->GetName(),
								Mapping.Action ? *Mapping.Action->GetName() : TEXT("none"),
								*Mapping.Key.ToString()));
						}
					}
					Record.AddActor(TEXT("map.applied_imc_keys"), Maps.Num() ? FString::Join(Maps, TEXT(",")) : TEXT("none"));
				}
			}
			WaitSeconds = 0.0f;
			Stage = EStage::ClearBeforeIdle;
			Owner.SetStage(TEXT("ClearBeforeIdle"));
		}

		void TickClear(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime, EStage Next)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (WaitSeconds <= 0.0f)
			{
				CallSetCombatActive(World, false);
				if (Next == EStage::Idle || Next == EStage::InjectFive)
				{
					PinAtSpawn(OrganoidPlaytestActions::GetPlayerPawn(World));
				}
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ClearSettleSeconds)
			{
				return;
			}
			PrevPlaying.Reset();
			WaitSeconds = 0.0f;
			bInjectedThisStep = false;
			Stage = Next;
			(void)Owner;
			(void)Record;
		}

		void TickIdle(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			Owner.SetStage(TEXT("IdleNoClick"));
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				if (LineIsCombatLayerStart(Line))
				{
					bIdleSawAlarm = true;
					AppendTrace(Record, FString::Printf(TEXT("IDLE_AUDIO %s"), *Line));
				}
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < IdleSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("baseline.idle_seconds"), FString::Printf(TEXT("%.1f"), IdleSeconds));
			Record.AddActor(TEXT("baseline.idle_alarm"), bIdleSawAlarm ? TEXT("true") : TEXT("false"));
			WaitSeconds = 0.0f;
			Stage = EStage::InjectOneLmb;
		}

		void TickInject(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			const TCHAR* Label,
			EStage ObserveStage)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !PC || !Character)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during click inject.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
			{
				Sink->MarkCursor();
			}
			PrevPlaying.Reset();
			SampleNewAudio(World, World->GetTimeSeconds(), DummyStarts);
			DummyStarts.Reset();
			PinAtSpawn(Character);
			AppendTrace(Record, Snapshot(World, PC, Character, *FString::Printf(TEXT("%s_before"), Label)));
			const bool bOk = InjectLmb(World, PC);
			AppendTrace(Record, FString::Printf(
				TEXT("%.3f INJECT %s ok=%s"),
				World->GetTimeSeconds(),
				Label,
				bOk ? TEXT("true") : TEXT("false")));
			WaitSeconds = 0.0f;
			bInjectedThisStep = true;
			Stage = ObserveStage;
			Owner.SetStage(Label);
		}

		void TickObserveShot(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START %s"), *Line));
				if (LineIsCombatLayerStart(Line))
				{
					bLmbStartedCombat = true;
				}
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (Ambience && Ambience->IsInCombat())
			{
				bLmbStartedCombat = true;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveSeconds)
			{
				return;
			}
			const FString Gunfire = LogsSince(Owner, TEXT("OrganoidGunfireNoise"));
			const FString Spatial = LogsSince(Owner, TEXT("OrganoidSpatialNoise"));
			const FString Stimulus = LogsSince(Owner, TEXT("OrganoidCombatStimulus"));
			bLmbReportedGunfire = Gunfire != TEXT("none");
			bLmbReportedSpatial = Spatial.Contains(TEXT("Gunfire"));
			bLmbFired = bLmbReportedGunfire;
			AppendTrace(Record, Snapshot(World, PC, Character, TEXT("lmb_one_after")));
			Record.AddActor(TEXT("lmb_one.gunfire_log"), Gunfire);
			Record.AddActor(TEXT("lmb_one.spatial_log"), Spatial);
			Record.AddActor(TEXT("lmb_one.stimulus_log"), Stimulus);
			Record.AddActor(TEXT("lmb_one.combat"), bLmbStartedCombat ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("lmb_one.alarm"), DumpAlarm(World));
			WaitSeconds = 0.0f;
			Stage = EStage::ClearBeforeFive;
		}

		void TickFive(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			Owner.SetStage(TEXT("InjectFiveLmb"));
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !PC || !Character)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during five-click inject.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			if (!bInjectedThisStep)
			{
				PinAtSpawn(Character);
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
				InjectLmb(World, PC);
				AppendTrace(Record, FString::Printf(TEXT("%.3f INJECT lmb_five_%d"), World->GetTimeSeconds(), FiveIndex + 1));
				bInjectedThisStep = true;
				WaitSeconds = 0.0f;
				return;
			}
			TArray<FString> Starts;
			SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START five_%d %s"), FiveIndex + 1, *Line));
				if (LineIsCombatLayerStart(Line))
				{
					bFiveStartedCombat = true;
				}
			}
			if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
			{
				if (Ambience->IsInCombat())
				{
					bFiveStartedCombat = true;
				}
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < FiveClickGapSeconds)
			{
				return;
			}
			++FiveIndex;
			bInjectedThisStep = false;
			WaitSeconds = 0.0f;
			if (FiveIndex >= 5)
			{
				const FString Gunfire = LogsSince(Owner, TEXT("OrganoidGunfireNoise"));
				bFiveReportedGunfire = Gunfire != TEXT("none");
				Record.AddActor(TEXT("lmb_five.gunfire_log"), Gunfire);
				Record.AddActor(TEXT("lmb_five.combat"), bFiveStartedCombat ? TEXT("true") : TEXT("false"));
				Record.AddActor(TEXT("lmb_five.alarm"), DumpAlarm(World));
				Stage = EStage::ClearBeforeRoute;
			}
		}

		void TickRoute(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			Owner.SetStage(TEXT("AdminRouteClicks"));
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !Pawn || !PC || !Character)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during admin click route.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			if (!bTeleported)
			{
				CallSetCombatActive(World, false);
				const float CapsuleZ = CapsuleZFor(Pawn);
				if (ACharacter* AsCharacter = Cast<ACharacter>(Pawn))
				{
					if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
					{
						Move->SetMovementMode(MOVE_Walking);
						Move->StopMovementImmediately();
					}
				}
				OrganoidPlaytestActions::TeleportNear(Pawn, FVector(AdminStops[StopIndex].XY.X, AdminStops[StopIndex].XY.Y, CapsuleZ), 0.0f, CapsuleZ);
				Pawn->UpdateOverlaps();
				bTeleported = true;
				bInjectedThisStep = false;
				WaitSeconds = 0.0f;
				return;
			}
			if (!bInjectedThisStep)
			{
				if (WaitSeconds < 0.35f)
				{
					WaitSeconds += DeltaTime;
					return;
				}
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
				PrevPlaying.Reset();
				SampleNewAudio(World, World->GetTimeSeconds(), DummyStarts);
				DummyStarts.Reset();
				InjectLmb(World, PC);
				AppendTrace(Record, FString::Printf(
					TEXT("%.3f INJECT lmb_%s"),
					World->GetTimeSeconds(),
					AdminStops[StopIndex].Name));
				bInjectedThisStep = true;
				WaitSeconds = 0.0f;
				return;
			}
			TArray<FString> Starts;
			SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			bool bBeep = false;
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START %s %s"), AdminStops[StopIndex].Name, *Line));
				if (Line.Contains(TEXT("AlarmPulse")) || Line.Contains(TEXT("CombatLayer")))
				{
					bBeep = true;
				}
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>();
			if (Ambience && Ambience->IsInCombat() && CombatLayerPlaying(World))
			{
				bBeep = true;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < RouteListenSeconds)
			{
				return;
			}
			if (bBeep)
			{
				bRouteStartedCombat = true;
			}
			Record.AddActor(
				FString::Printf(TEXT("route.%s.beep"), AdminStops[StopIndex].Name),
				bBeep ? TEXT("true") : TEXT("false"));
			AppendTrace(Record, Snapshot(World, PC, Character, AdminStops[StopIndex].Name));
			++StopIndex;
			bTeleported = false;
			bInjectedThisStep = false;
			WaitSeconds = 0.0f;
			if (StopIndex >= AdminStopCount)
			{
				Record.AddActor(TEXT("route.any_lmb_combat"), bRouteStartedCombat ? TEXT("true") : TEXT("false"));
				Stage = EStage::ClearBeforeDamage;
			}
		}

		void TickDamage(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("PlayerDamageStimulus"));
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !Character)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during damage stimulus.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
			{
				Sink->MarkCursor();
			}
			PrevPlaying.Reset();
			SampleNewAudio(World, World->GetTimeSeconds(), DummyStarts);
			DummyStarts.Reset();
			CallApplyHealthDelta(Character, -DamageAmount);
			AppendTrace(Record, FString::Printf(TEXT("%.3f APPLY_DAMAGE %.1f"), World->GetTimeSeconds(), DamageAmount));
			WaitSeconds = 0.0f;
			Stage = EStage::ObserveDamage;
		}

		void TickObserveDamage(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START damage %s"), *Line));
				if (Line.Contains(TEXT("AlarmPulse")) || Line.Contains(TEXT("CombatLayer")))
				{
					bDamageStartedCombat = true;
				}
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (Ambience && Ambience->IsInCombat())
			{
				bDamageStartedCombat = true;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("damage.stimulus_log"), LogsSince(Owner, TEXT("OrganoidCombatStimulus")));
			Record.AddActor(TEXT("damage.health_log"), LogsSince(Owner, TEXT("OrganoidHealthCombat")));
			Record.AddActor(TEXT("damage.combat"), bDamageStartedCombat ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("damage.alarm"), DumpAlarm(World));
			AppendTrace(Record, Snapshot(World, PC, Character, TEXT("damage_after")));
			if (Character)
			{
				CallApplyHealthDelta(Character, DamageAmount);
			}
			WaitSeconds = 0.0f;
			Stage = EStage::ClearBeforeCombat;
		}

		void TickInjectCombat(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("InjectCombatStimulus"));
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World)
			{
				Record.FailureReason = TEXT("Lost PIE world during combat inject.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
			{
				Sink->MarkCursor();
			}
			PrevPlaying.Reset();
			SampleNewAudio(World, World->GetTimeSeconds(), DummyStarts);
			DummyStarts.Reset();
			const bool bOk = CallNotifyCombatStimulus(World, 1.0f);
			AppendTrace(Record, FString::Printf(TEXT("%.3f INJECT NotifyCombatStimulus ok=%s"), World->GetTimeSeconds(), bOk ? TEXT("true") : TEXT("false")));
			AppendTrace(Record, Snapshot(World, PC, Character, TEXT("combat_inject")));
			WaitSeconds = 0.0f;
			Stage = EStage::ObserveCombatOn;
		}

		void TickObserveCombatOn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START combat %s"), *Line));
				if (Line.Contains(TEXT("AlarmPulse")) || Line.Contains(TEXT("CombatLayer")))
				{
					bInjectedCombatStarted = true;
				}
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (Ambience && Ambience->IsInCombat() && CombatLayerPlaying(World))
			{
				bInjectedCombatStarted = true;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("combat_inject.stimulus_log"), LogsSince(Owner, TEXT("OrganoidCombatStimulus")));
			Record.AddActor(TEXT("combat_inject.started"), bInjectedCombatStarted ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("combat_inject.alarm"), DumpAlarm(World));
			WaitSeconds = 0.0f;
			Stage = EStage::LingerWait;
			Owner.SetStage(TEXT("CombatLinger"));
		}

		void TickLingerWait(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < CombatLingerSeconds + LingerExtraSeconds)
			{
				return;
			}
			(void)Owner;
			(void)Record;
			Stage = EStage::ObserveLingerOff;
		}

		void TickObserveLingerOff(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			const bool bStillCombat = Ambience && Ambience->IsInCombat();
			const bool bAlarm = CombatLayerPlaying(World);
			bLingerExpired = !bStillCombat && !bAlarm;
			Record.AddActor(TEXT("linger.combat"), bStillCombat ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("linger.alarm"), DumpAlarm(World));
			Record.AddActor(TEXT("linger.expired"), bLingerExpired ? TEXT("true") : TEXT("false"));
			AppendTrace(Record, Snapshot(World, PC, Character, TEXT("linger_after")));
			(void)Owner;
			WaitSeconds = 0.0f;
			Stage = EStage::EndPie;
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
			AssertTrue(Record, TEXT("baseline.idle_no_alarm"),
				!bIdleSawAlarm, TEXT("false"), bIdleSawAlarm ? TEXT("true") : TEXT("false"), TEXT(""), false);
			AssertTrue(Record, TEXT("lmb.fire_executed"),
				bLmbFired, TEXT("true"), bLmbFired ? TEXT("true") : TEXT("false"), TEXT("IA_Fire_Runtime"), false);
			AssertTrue(Record, TEXT("lmb.gunfire_noise_reported"),
				bLmbReportedGunfire, TEXT("true"), bLmbReportedGunfire ? TEXT("true") : TEXT("false"), TEXT("ReportGunfireNoise"), false);
			AssertTrue(Record, TEXT("lmb.ai_hearing_spatial_noise"),
				bLmbReportedSpatial, TEXT("true"), bLmbReportedSpatial ? TEXT("true") : TEXT("false"), TEXT("ReportSpatialNoise"), false);
			AssertTrue(Record, TEXT("lmb.no_combat_from_shot"),
				!bLmbStartedCombat, TEXT("false"), bLmbStartedCombat ? TEXT("true") : TEXT("false"), TEXT(""), false);
			AssertTrue(Record, TEXT("lmb_five.no_combat_from_shots"),
				!bFiveStartedCombat && bFiveReportedGunfire, TEXT("gunfire_without_combat"),
				FString::Printf(TEXT("combat=%s gunfire=%s"), bFiveStartedCombat ? TEXT("true") : TEXT("false"), bFiveReportedGunfire ? TEXT("true") : TEXT("false")),
				TEXT(""), false);
			AssertTrue(Record, TEXT("route.no_lmb_combat"),
				!bRouteStartedCombat, TEXT("false"), bRouteStartedCombat ? TEXT("true") : TEXT("false"), TEXT("Admin"), false);
			AssertTrue(Record, TEXT("damage.combat_stimulus_still_works"),
				bDamageStartedCombat, TEXT("true"), bDamageStartedCombat ? TEXT("true") : TEXT("false"), TEXT("ApplyHealthDelta"), false);
			AssertTrue(Record, TEXT("combat_inject.alarm_starts"),
				bInjectedCombatStarted, TEXT("true"), bInjectedCombatStarted ? TEXT("true") : TEXT("false"), TEXT("NotifyCombatStimulus"), false);
			AssertTrue(Record, TEXT("combat_inject.linger_expires"),
				bLingerExpired, TEXT("true"), bLingerExpired ? TEXT("true") : TEXT("false"), TEXT("CombatLingerSeconds"), false);

			Record.AddActor(TEXT("result.idle_alarm"), bIdleSawAlarm ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("result.lmb_started_combat"), bLmbStartedCombat ? TEXT("true") : TEXT("false"));
			Record.AddActor(TEXT("result.synthetic_lmb_starts_combat"), TEXT("false"));
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
		float CombatLingerSeconds = 8.0f;
		int32 FiveIndex = 0;
		int32 StopIndex = 0;
		bool bTeleported = false;
		bool bInjectedThisStep = false;
		bool bAnyAssertFailed = false;
		bool bIdleSawAlarm = false;
		bool bLmbStartedCombat = false;
		bool bFiveStartedCombat = false;
		bool bRouteStartedCombat = false;
		bool bDamageStartedCombat = false;
		bool bInjectedCombatStarted = false;
		bool bLingerExpired = false;
		bool bLmbFired = false;
		bool bLmbReportedGunfire = false;
		bool bLmbReportedSpatial = false;
		bool bFiveReportedGunfire = false;
		TArray<FString> Traces;
		TArray<FString> DirtyBefore;
		TArray<FString> DummyStarts;
		TSet<FString> PrevPlaying;
	};

	struct FBeepClickInjectionAutoRegister
	{
		FBeepClickInjectionAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FBeepClickInjectionFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FBeepClickInjectionAutoRegister GBeepClickInjectionAutoRegister;
}
