#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "ProjectOrganoidPlaytestLogSink.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("RoomEntryBeep_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Decon Hazard Damage Does Not Start Combat Ambience");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HazardLabel[] = TEXT("Hazard_DeconUVC");
	constexpr TCHAR VestibuleTriggerLabel[] = TEXT("Admin_RoomTrigger_Vestibule");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr float OutsideWaitSeconds = 2.0f;
	constexpr float ObserveASeconds = 3.0f;
	constexpr float StayBSeconds = 5.0f;
	constexpr float ObserveCSeconds = 2.0f;
	constexpr float ObserveDSeconds = 0.85f;
	constexpr float ClearSettleSeconds = 2.4f;
	constexpr float HostileDamage = 8.0f;
	constexpr float CombatVolumeQuiet = 0.12f;

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

	class FRoomEntryBeepFunctional : public IOrganoidPlaytestCase
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
			bIdentified = false;
			HealthAtEntry = -1.0f;
			HealthAfterA = -1.0f;
			HealthAfterB = -1.0f;
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
			case EStage::Identify: TickIdentify(Owner, *Record); break;
			case EStage::ClearCombat: TickClear(Owner, *Record, DeltaTime, EStage::WaitOutsideA); break;
			case EStage::WaitOutsideA: TickWaitOutside(Owner, *Record, DeltaTime); break;
			case EStage::EnterA: TickEnter(Owner, *Record, TEXT("A")); break;
			case EStage::ObserveA: TickObserveA(Owner, *Record, DeltaTime); break;
			case EStage::StayB: TickStayB(Owner, *Record, DeltaTime); break;
			case EStage::ExitC: TickCross(Owner, *Record, false, TEXT("C_OUT"), EStage::ObserveCOut); break;
			case EStage::ObserveCOut: TickObserveC(Owner, *Record, DeltaTime, false); break;
			case EStage::EnterC: TickCross(Owner, *Record, true, TEXT("C_IN"), EStage::ObserveCIn); break;
			case EStage::ObserveCIn: TickObserveC(Owner, *Record, DeltaTime, true); break;
			case EStage::ExitBeforeD: TickCross(Owner, *Record, false, TEXT("D_OUT"), EStage::RestoreBeforeD); break;
			case EStage::RestoreBeforeD: TickRestoreBeforeD(Owner, *Record, DeltaTime); break;
			case EStage::HostileD: TickHostileD(Owner, *Record); break;
			case EStage::ObserveD: TickObserveD(Owner, *Record, DeltaTime); break;
			case EStage::ClearAfterD: TickClearAfterD(Owner, *Record, DeltaTime); break;
			case EStage::EnterE: TickEnter(Owner, *Record, TEXT("E")); break;
			case EStage::DrainE: TickDrainE(Owner, *Record, DeltaTime); break;
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
			Preflight, StartPie, WaitPieReady, Identify, ClearCombat, WaitOutsideA,
			EnterA, ObserveA, StayB, ExitC, ObserveCOut, EnterC, ObserveCIn,
			ExitBeforeD, RestoreBeforeD, HostileD, ObserveD, ClearAfterD,
			EnterE, DrainE, EndPie, WaitPieStopped, AssertDurable, Finalize
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
			Record.FailureReason = Reason;
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			(void)Owner;
		}

		AProjectOrganoidCharacter* GetOrganoid(UWorld* World) const
		{
			return Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerPawn(World));
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

		bool CallApplyHealthDelta(AProjectOrganoidCharacter* Character, float Delta, EProjectOrganoidHealthDeltaSource Source)
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
			if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, TEXT("Source")))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
					static_cast<int64>(Source));
			}
			Character->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		void PlacePawn(APawn* Pawn, const FVector& XY)
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
			OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
		}

		FString DumpPlaying(UWorld* World) const
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
				AActor* Owner = Component->GetOwner();
				Playing.Add(FString::Printf(
					TEXT("%s/%s/%s vol=%.3f"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*Component->GetName(),
					Sound ? *Sound->GetName() : TEXT("none"),
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
					OutStarts.Add(FString::Printf(
						TEXT("%.3f %s vol=%.3f"),
						WorldTime,
						*Key,
						Component->VolumeMultiplier));
				}
			}
			PrevPlaying = MoveTemp(Now);
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
				const FString CompName = Component->GetName();
				if (!SoundName.Contains(TEXT("AlarmPulse")) && !SoundName.Contains(TEXT("HazardHiss"))
					&& !CompName.Contains(TEXT("CombatLayer")) && !CompName.Contains(TEXT("HazardAudio"))
					&& !CompName.Contains(TEXT("CriticalLayer")))
				{
					continue;
				}
				AActor* Owner = Component->GetOwner();
				Playing.Add(FString::Printf(
					TEXT("%s/%s/%s vol=%.3f"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*CompName,
					*SoundName,
					Component->VolumeMultiplier));
			}
			return Playing.Num() ? FString::Join(Playing, TEXT(",")) : TEXT("none");
		}

		bool CombatLayerPlaying(UWorld* World) const
		{
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				if (Component->GetName().Contains(TEXT("CombatLayer")))
				{
					return true;
				}
			}
			return false;
		}

		bool HissPlaying(UWorld* World) const
		{
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				const FString SoundName = Sound ? Sound->GetName() : FString();
				if (Component->GetName().Contains(TEXT("HazardAudio")) || SoundName.Contains(TEXT("HazardHiss")))
				{
					return true;
				}
			}
			return false;
		}

		bool AlarmPulsePlaying(UWorld* World) const
		{
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				if (Sound && Sound->GetName().Contains(TEXT("AlarmPulse")) && Component->VolumeMultiplier > 0.02f)
				{
					return true;
				}
			}
			return false;
		}

		bool CriticalLayerSilent(UWorld* World) const
		{
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Ambience || Ambience->GetCriticalLayerVolume() > 0.05f)
			{
				return false;
			}
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				if (!Component->GetName().Contains(TEXT("CriticalLayer")))
				{
					continue;
				}
				if (Component->VolumeMultiplier > 0.02f)
				{
					return false;
				}
			}
			return true;
		}

		FString Snapshot(UWorld* World, const TCHAR* Label) const
		{
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			const FVector Loc = Character ? Character->GetActorLocation() : FVector::ZeroVector;
			return FString::Printf(
				TEXT("%.3f %s loc=(%.0f,%.0f) health=%.1f combat=%s hazard=%s combatVol=%.3f critVol=%.3f state=%s alarm=%s playing=%s"),
				World ? World->GetTimeSeconds() : -1.0f,
				Label,
				Loc.X,
				Loc.Y,
				Character ? Character->GetHealth() : -1.0f,
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"),
				(Ambience && Ambience->IsInHazard()) ? TEXT("true") : TEXT("false"),
				Ambience ? Ambience->GetCombatLayerVolume() : -1.0f,
				Ambience ? Ambience->GetCriticalLayerVolume() : -1.0f,
				Ambience ? *UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"),
				*DumpAlarm(World),
				*DumpPlaying(World));
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

		bool CombatQuiet(UWorld* World) const
		{
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Ambience)
			{
				return false;
			}
			return !Ambience->IsInCombat() && Ambience->GetCombatLayerVolume() <= CombatVolumeQuiet && !CombatLayerPlaying(World);
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
				WaitSeconds = 0.0f;
				Stage = EStage::Identify;
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE pawn."));
			}
			(void)Record;
		}

		void TickIdentify(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Hazard = OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel);
			AActor* Vestibule = OrganoidPlaytestActions::FindUniqueByLabel(World, VestibuleTriggerLabel);
			AActor* Door = OrganoidPlaytestActions::FindUniqueByLabel(World, DoorLabel);
			if (!Hazard || !Vestibule || !Door)
			{
				FailAndStop(Owner, Record, TEXT("Could not find live Hazard_DeconUVC, Vestibule trigger, or Access Door."));
				return;
			}

			FVector HazardOrigin;
			FVector HazardExtent;
			Hazard->GetActorBounds(true, HazardOrigin, HazardExtent);
			InsideXY = FVector(HazardOrigin.X, HazardOrigin.Y, 0.0f);
			const FVector HazardMin = HazardOrigin - HazardExtent;
			OutsideXY = FVector(HazardOrigin.X, HazardMin.Y - 200.0f, 0.0f);
			Record.AddActor(TEXT("room.hazard"), HazardLabel);
			Record.AddActor(TEXT("room.inside_xy"), FString::Printf(TEXT("(%.0f,%.0f)"), InsideXY.X, InsideXY.Y));
			Record.AddActor(TEXT("room.outside_xy"), FString::Printf(TEXT("(%.0f,%.0f)"), OutsideXY.X, OutsideXY.Y));
			bIdentified = true;
			WaitSeconds = 0.0f;
			Stage = EStage::ClearCombat;
			(void)Owner;
		}

		void TickClear(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime, EStage Next)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (WaitSeconds <= 0.0f)
			{
				CallSetCombatActive(World, false);
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ClearSettleSeconds)
			{
				return;
			}
			PrevPlaying.Reset();
			WaitSeconds = 0.0f;
			Stage = Next;
			(void)Owner;
			(void)Record;
		}

		void TickWaitOutside(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (WaitSeconds <= 0.0f && Pawn)
			{
				PlacePawn(Pawn, OutsideXY);
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OutsideWaitSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("A.outside"), Snapshot(World, TEXT("outside")));
			AssertTrue(Record, TEXT("A.outside_hiss_silent"), !HissPlaying(World), TEXT("false"),
				HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			AssertTrue(Record, TEXT("A.outside_critical_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("A.outside_no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			if (!CombatQuiet(World) || bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason.IsEmpty()
					? TEXT("Combat already active before Decon entry.")
					: Record.FailureReason);
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::EnterA;
			Owner.SetStage(TEXT("A_Enter"));
		}

		void TickEnter(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const TCHAR* Label)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during Decon entry."));
				return;
			}
			HealthAtEntry = Character ? Character->GetHealth() : -1.0f;
			PlacePawn(Pawn, InsideXY);
			AppendTrace(Record, FString::Printf(TEXT("%.3f ENTER %s loc=(%.0f,%.0f) health=%.1f"),
				World->GetTimeSeconds(), Label, Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y, HealthAtEntry));
			if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
			{
				Sink->MarkCursor();
			}
			PrevPlaying.Reset();
			TArray<FString> Dummy;
			SampleNewAudio(World, World->GetTimeSeconds(), Dummy);
			WaitSeconds = 0.0f;
			if (FCString::Strcmp(Label, TEXT("E")) == 0)
			{
				Stage = EStage::DrainE;
				Owner.SetStage(TEXT("E_Drain"));
			}
			else
			{
				Stage = EStage::ObserveA;
				Owner.SetStage(TEXT("A_Observe"));
			}
		}

		void TickObserveA(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START A %s"), *Line));
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveASeconds)
			{
				return;
			}

			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			HealthAfterA = Character ? Character->GetHealth() : -1.0f;
			Record.AddActor(TEXT("A.after"), Snapshot(World, TEXT("inside")));
			Record.AddActor(TEXT("A.health_hazard"), LogsSince(Owner, TEXT("OrganoidHealthHazard")));
			Record.AddActor(TEXT("A.health_combat"), LogsSince(Owner, TEXT("OrganoidHealthCombat")));
			Record.AddActor(TEXT("A.stimulus"), LogsSince(Owner, TEXT("OrganoidCombatStimulus")));
			Record.AddActor(TEXT("A.alarm"), DumpAlarm(World));

			const bool bHiss = HissPlaying(World);
			const bool bHazard = Ambience && Ambience->IsInHazard();
			const bool bHazardState = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::Hazard;
			const bool bHealthDropped = HealthAfterA >= 0.0f && HealthAtEntry >= 0.0f && HealthAfterA < HealthAtEntry - 1.0f;
			const bool bQuietCombat = CombatQuiet(World);
			const bool bNoCombatLog = LogsSince(Owner, TEXT("OrganoidHealthCombat")).Equals(TEXT("none"));
			const bool bNoStimulus = LogsSince(Owner, TEXT("OrganoidCombatStimulus")).Equals(TEXT("none"));

			AssertTrue(Record, TEXT("A.hiss_plays"), bHiss, TEXT("true"), bHiss ? TEXT("true") : TEXT("false"), HazardLabel, false);
			AssertTrue(Record, TEXT("A.health_decreased"), bHealthDropped,
				TEXT("health dropped"), FString::Printf(TEXT("%.1f -> %.1f"), HealthAtEntry, HealthAfterA), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("A.hazard_active"), bHazard, TEXT("true"), bHazard ? TEXT("true") : TEXT("false"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("A.hazard_state"), bHazardState, TEXT("Hazard"),
				Ambience ? UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("A.no_combat"), bQuietCombat, TEXT("false"),
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			AssertTrue(Record, TEXT("A.no_combat_alarmpulse"), bQuietCombat && bNoStimulus, TEXT("none"), DumpAlarm(World), TEXT("combat"), false);
			AssertTrue(Record, TEXT("A.no_health_combat_log"), bNoCombatLog, TEXT("none"), LogsSince(Owner, TEXT("OrganoidHealthCombat")), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("A.critical_layer_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("A.no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			const float Dps = (HealthAtEntry >= 0.0f && HealthAfterA >= 0.0f && ObserveASeconds > 0.0f)
				? (HealthAtEntry - HealthAfterA) / ObserveASeconds
				: -1.0f;
			AssertTrue(Record, TEXT("A.uvc_dps"), FMath::Abs(Dps - 6.0f) <= 1.5f, TEXT("6"),
				FString::Printf(TEXT("%.2f"), Dps), TEXT("vitals"), false);

			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::StayB;
			Owner.SetStage(TEXT("B_Stay"));
		}

		void TickStayB(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START B %s"), *Line));
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < StayBSeconds)
			{
				return;
			}

			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			HealthAfterB = Character ? Character->GetHealth() : -1.0f;
			Record.AddActor(TEXT("B.after"), Snapshot(World, TEXT("stay")));
			Record.AddActor(TEXT("B.alarm"), DumpAlarm(World));
			const bool bStillDropping = HealthAfterB >= 0.0f && HealthAfterA >= 0.0f && HealthAfterB < HealthAfterA - 1.0f;
			const bool bQuietCombat = CombatQuiet(World);
			AssertTrue(Record, TEXT("B.health_still_dropping"), bStillDropping,
				TEXT("health dropped further"), FString::Printf(TEXT("%.1f -> %.1f"), HealthAfterA, HealthAfterB), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("B.no_combat_alarmpulse"), bQuietCombat, TEXT("false"),
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			AssertTrue(Record, TEXT("B.critical_layer_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("B.no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			const float MaxHealth = Character ? Character->GetMaxHealth() : 100.0f;
			AssertTrue(Record, TEXT("B.above_30"), HealthAfterB > MaxHealth * 0.30f, TEXT(">30%"),
				FString::Printf(TEXT("%.1f"), HealthAfterB), TEXT("vitals"), false);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::ExitC;
			Owner.SetStage(TEXT("C_Exit"));
		}

		void TickCross(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, bool bInside, const TCHAR* Label, EStage Next)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during occupancy cross."));
				return;
			}
			PlacePawn(Pawn, bInside ? InsideXY : OutsideXY);
			AppendTrace(Record, FString::Printf(TEXT("%.3f CROSS %s loc=(%.0f,%.0f)"),
				World->GetTimeSeconds(), Label, Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y));
			WaitSeconds = 0.0f;
			Stage = Next;
			Owner.SetStage(Label);
			(void)Record;
		}

		void TickObserveC(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime, bool bInside)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START C %s"), *Line));
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveCSeconds)
			{
				return;
			}

			const bool bHiss = HissPlaying(World);
			const bool bQuietCombat = CombatQuiet(World);
			Record.AddActor(bInside ? TEXT("C.in") : TEXT("C.out"), Snapshot(World, bInside ? TEXT("reenter") : TEXT("exit")));
			if (bInside)
			{
				AssertTrue(Record, TEXT("C.hiss_on_reenter"), bHiss, TEXT("true"), bHiss ? TEXT("true") : TEXT("false"), HazardLabel, false);
				AssertTrue(Record, TEXT("C.no_combat_alarmpulse"), bQuietCombat, TEXT("false"),
					bQuietCombat ? TEXT("false") : TEXT("true"), TEXT("combat"), false);
				AssertTrue(Record, TEXT("C.reenter_no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
					AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
				AssertTrue(Record, TEXT("C.reenter_critical_silent"), CriticalLayerSilent(World), TEXT("true"),
					CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
				if (bAnyAssertFailed)
				{
					Stage = EStage::EndPie;
					return;
				}
				WaitSeconds = 0.0f;
				Stage = EStage::ExitBeforeD;
			}
			else
			{
				AssertTrue(Record, TEXT("C.hiss_stops_on_exit"), !bHiss, TEXT("false"), bHiss ? TEXT("true") : TEXT("false"), HazardLabel, false);
				AssertTrue(Record, TEXT("C.exit_no_combat_alarmpulse"), bQuietCombat, TEXT("false"),
					bQuietCombat ? TEXT("false") : TEXT("true"), TEXT("combat"), false);
				AssertTrue(Record, TEXT("C.exit_no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
					AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
				if (bAnyAssertFailed)
				{
					Stage = EStage::EndPie;
					return;
				}
				WaitSeconds = 0.0f;
				Stage = EStage::EnterC;
			}
			(void)Owner;
		}

		void TickRestoreBeforeD(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (WaitSeconds <= 0.0f && Character)
			{
				const float Missing = Character->GetMaxHealth() - Character->GetHealth();
				if (Missing > KINDA_SMALL_NUMBER)
				{
					CallApplyHealthDelta(Character, Missing, EProjectOrganoidHealthDeltaSource::Generic);
				}
				CallSetCombatActive(World, false);
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ClearSettleSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("D.before"), Snapshot(World, TEXT("restored")));
			WaitSeconds = 0.0f;
			Stage = EStage::HostileD;
			Owner.SetStage(TEXT("D_Hostile"));
		}

		void TickHostileD(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE pawn during hostile damage."));
				return;
			}
			if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
			{
				Sink->MarkCursor();
			}
			PrevPlaying.Reset();
			TArray<FString> Dummy;
			SampleNewAudio(World, World->GetTimeSeconds(), Dummy);
			CallApplyHealthDelta(Character, -HostileDamage, EProjectOrganoidHealthDeltaSource::Generic);
			AppendTrace(Record, FString::Printf(TEXT("%.3f HOSTILE_DAMAGE %.1f"), World->GetTimeSeconds(), HostileDamage));
			WaitSeconds = 0.0f;
			Stage = EStage::ObserveD;
			Owner.SetStage(TEXT("D_Observe"));
			(void)Record;
		}

		void TickObserveD(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			bool bSawCombatPulse = false;
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START D %s"), *Line));
				if (Line.Contains(TEXT("CombatLayer")) || Line.Contains(TEXT("AlarmPulse")))
				{
					bSawCombatPulse = true;
				}
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ObserveDSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("D.after"), Snapshot(World, TEXT("hostile")));
			Record.AddActor(TEXT("D.stimulus"), LogsSince(Owner, TEXT("OrganoidCombatStimulus")));
			Record.AddActor(TEXT("D.health_combat"), LogsSince(Owner, TEXT("OrganoidHealthCombat")));
			Record.AddActor(TEXT("D.alarm"), DumpAlarm(World));
			const bool bCombat = Ambience && Ambience->IsInCombat();
			const bool bPulse = bSawCombatPulse || CombatLayerPlaying(World) || (Ambience && Ambience->GetCombatLayerVolume() > CombatVolumeQuiet);
			AssertTrue(Record, TEXT("D.hostile_raises_combat"), bCombat, TEXT("true"), bCombat ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			AssertTrue(Record, TEXT("D.hostile_alarmpulse"), bPulse, TEXT("true"), bPulse ? TEXT("true") : TEXT("false"), TEXT("OrganoidCombatLayer"), false);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::ClearAfterD;
			Owner.SetStage(TEXT("D_Clear"));
		}

		void TickClearAfterD(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (WaitSeconds <= 0.0f && Character)
			{
				const float Missing = Character->GetMaxHealth() - Character->GetHealth();
				if (Missing > KINDA_SMALL_NUMBER)
				{
					CallApplyHealthDelta(Character, Missing, EProjectOrganoidHealthDeltaSource::Generic);
				}
				CallSetCombatActive(World, false);
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ClearSettleSeconds)
			{
				return;
			}
			(void)Record;
			WaitSeconds = 0.0f;
			Stage = EStage::EnterE;
			Owner.SetStage(TEXT("E_Enter"));
		}

		void TickDrainE(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			TArray<FString> Starts;
			if (World)
			{
				SampleNewAudio(World, World->GetTimeSeconds(), Starts);
			}
			for (const FString& Line : Starts)
			{
				AppendTrace(Record, FString::Printf(TEXT("AUDIO_START E %s"), *Line));
			}
			WaitSeconds += DeltaTime;
			const float Health = Character ? Character->GetHealth() : -1.0f;
			const float MaxHealth = Character ? Character->GetMaxHealth() : 100.0f;
			const bool bCriticalHp = Health >= 0.0f && Health <= MaxHealth * 0.30f + 0.05f;
			if (!bCriticalHp && WaitSeconds < 20.0f)
			{
				if (Ambience && Ambience->IsInCombat())
				{
					FailAndStop(Owner, Record, TEXT("Combat rose from UVC drain before CriticalHealth."));
				}
				return;
			}

			Record.AddActor(TEXT("E.after"), Snapshot(World, TEXT("critical")));
			Record.AddActor(TEXT("E.health"), FString::Printf(TEXT("%.1f / %.1f"), Health, MaxHealth));
			Record.AddActor(TEXT("E.stimulus"), LogsSince(Owner, TEXT("OrganoidCombatStimulus")));
			Record.AddActor(TEXT("E.alarm"), DumpAlarm(World));
			const bool bCriticalState = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth;
			const bool bNoCombat = Ambience && !Ambience->IsInCombat();
			const bool bHiss = HissPlaying(World);
			AssertTrue(Record, TEXT("E.reached_critical_hp"), bCriticalHp,
				TEXT("<=30%"), FString::Printf(TEXT("%.1f"), Health), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("E.critical_state"), bCriticalState, TEXT("CriticalHealth"),
				Ambience ? UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("E.critical_layer_audible"), Ambience && Ambience->GetCriticalLayerVolume() >= 0.05f,
				TEXT(">=0.05"), FString::Printf(TEXT("%.3f"), Ambience ? Ambience->GetCriticalLayerVolume() : -1.0f),
				TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("E.hazard_hiss_still_playing"), bHiss, TEXT("true"), bHiss ? TEXT("true") : TEXT("false"), HazardLabel, false);
			AssertTrue(Record, TEXT("E.no_combat_from_hazard"), bNoCombat, TEXT("false"),
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			WaitSeconds = 0.0f;
			Stage = EStage::EndPie;
			(void)Owner;
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
			AssertTrue(Record, TEXT("room.identified"),
				bIdentified, TEXT("true"), bIdentified ? TEXT("true") : TEXT("false"), HazardLabel, false);
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
		float HealthAtEntry = -1.0f;
		float HealthAfterA = -1.0f;
		float HealthAfterB = -1.0f;
		bool bAnyAssertFailed = false;
		bool bIdentified = false;
		FVector InsideXY = FVector::ZeroVector;
		FVector OutsideXY = FVector::ZeroVector;
		TArray<FString> Traces;
		TArray<FString> DirtyBefore;
		TSet<FString> PrevPlaying;
	};

	struct FRoomEntryBeepAutoRegister
	{
		FRoomEntryBeepAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FRoomEntryBeepFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FRoomEntryBeepAutoRegister GRoomEntryBeepAutoRegister;
}
