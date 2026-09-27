#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidPursuer.h"

namespace PursuerIntroFunctional
{
	constexpr TCHAR TestId[] = TEXT("PursuerIntro_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Pursuer Intro Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR ReactorPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MissionPath[] = TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro.DA_Mission_PursuerIntro");
	constexpr TCHAR WeaponRosterPath[] = TEXT("/Game/Data/Missions/DA_Mission_WeaponRoster.DA_Mission_WeaponRoster");
	constexpr TCHAR MissionId[] = TEXT("Mission_PursuerIntro");
	constexpr TCHAR ObjectiveId[] = TEXT("Obj_EncounterPursuer");
	constexpr TCHAR EventId[] = TEXT("Event_PursuerEncountered");
	constexpr TCHAR PursuerLabel[] = TEXT("BP_Pursuer");
	constexpr TCHAR TriggerLabel[] = TEXT("Reactor_PursuerTrigger");
	constexpr TCHAR ExpectedLine[] = TEXT("That was in a person. It's still walking.");
	const FVector PursuerLocation(-1800.f, 0.f, -4710.f);
	const FVector TriggerLocation(-1100.f, 0.f, -4710.f);
	const FVector TerminalLocation(-1950.f, -1650.f, -4700.f);
	const FVector CheckpointLocation(25.f, 0.f, -4740.f);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path)) return Package->IsDirty();
		return false;
	}

	class FPursuerIntroFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }
		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.f;
			bAnyAssertFailed = false;
			bRequestedStream = false;
			Owner.SetStage(TEXT("Preflight"));
		}
		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
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
			case EStage::WaitReady: TickWaitReady(Owner, *Record, DeltaTime); break;
			case EStage::Proof: TickProof(Owner, *Record); break;
			case EStage::WaitDespawn: TickWaitDespawn(Owner, *Record, DeltaTime); break;
			case EStage::EndPie:
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitStopped;
				break;
			case EStage::WaitStopped:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress() || WaitSeconds > 20.f)
				{
					const bool bDirty = PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(CryoPackage) || PackageIsDirty(ComputePackage) || PackageIsDirty(NeuroPackage) || PackageIsDirty(ReactorPackage);
					if (bDirty && Record->FailureReason.IsEmpty())
					{
						Record->FailureReason = TEXT("A locked package was dirty after the pursuer intro test.");
						bAnyAssertFailed = true;
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, WaitDespawn, EndPie, WaitStopped };

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			(void)Owner;
		}

		bool AssertTrue(FOrganoidPlaytestRecord& Record, const FString& Id, bool bPassed, const FString& Expected, const FString& Actual, const FString& ActorId)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, false);
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

		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUD ? HUD->GetBoundHUDWidget() : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			UProjectOrganoidObjectiveDataAsset* WeaponRoster = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, WeaponRosterPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Mission && Mission->Tasks.Num() == 1 ? &Mission->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Mission && Mission->MissionId == FName(MissionId), MissionId, Mission ? Mission->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Mission && Mission->MissionTitle.ToString() == TEXT("Pursuer"), TEXT("Pursuer"), Mission ? Mission->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Mission && Mission->MissionDescription.ToString().Contains(TEXT("behind the glass")), TEXT("behind the glass"), Mission ? Mission->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_null"), Mission && Mission->NextMissionAsset.IsNull(), TEXT("null"), Mission && Mission->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main && Task->Objective.ObjectiveId == FName(ObjectiveId) && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, ObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.auto_target"), Task && Task->bAutoActivate && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(EventId) && Task->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete, EventId, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("handoff.weapon_roster"), WeaponRoster && WeaponRoster->NextMissionAsset.ToSoftObjectPath().ToString() == MissionPath, MissionPath, WeaponRoster ? WeaponRoster->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Pursuers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, PursuerLabel) : TArray<AActor*>();
			TArray<AActor*> Triggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TriggerLabel) : TArray<AActor*>();
			AProjectOrganoidPursuer* Pursuer = Pursuers.Num() == 1 ? Cast<AProjectOrganoidPursuer>(Pursuers[0]) : nullptr;
			AProjectOrganoidPursuerTrigger* Trigger = Triggers.Num() == 1 ? Cast<AProjectOrganoidPursuerTrigger>(Triggers[0]) : nullptr;
			TArray<AActor*> Terminals = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Terminal_ControlSpine")) : TArray<AActor*>();
			TArray<AActor*> Checkpoints = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Checkpoint_BasinRim")) : TArray<AActor*>();
			AssertTrue(Record, TEXT("actor.count"), Pursuers.Num() == 1 && Pursuer != nullptr, TEXT("1"), FString::FromInt(Pursuers.Num()), PursuerLabel);
			AssertTrue(Record, TEXT("actor.location"), Pursuer && Pursuer->GetActorLocation().Equals(PursuerLocation, 1.f), TEXT("-1800,0,-4710"), Pursuer ? Pursuer->GetActorLocation().ToString() : TEXT("missing"), PursuerLabel);
			AssertTrue(Record, TEXT("actor.package"), Pursuer && Pursuer->GetOutermost() && Pursuer->GetOutermost()->GetName().Contains(TEXT("SL_Epitope_Reactor")), ReactorPackage, Pursuer && Pursuer->GetOutermost() ? Pursuer->GetOutermost()->GetName() : TEXT("missing"), PursuerLabel);
			AssertTrue(Record, TEXT("actor.no_chase"), Pursuer && !Pursuer->IsA(APawn::StaticClass()) && !Pursuer->DropsWeapon() && FMath::IsNearlyEqual(Pursuer->GetSlowPlayRate(), 0.45f), TEXT("slow"), Pursuer ? FString::SanitizeFloat(Pursuer->GetSlowPlayRate()) : TEXT("missing"), PursuerLabel);
			AssertTrue(Record, TEXT("trigger.count"), Triggers.Num() == 1 && Trigger != nullptr, TEXT("1"), FString::FromInt(Triggers.Num()), TriggerLabel);
			AssertTrue(Record, TEXT("trigger.location"), Trigger && Trigger->GetActorLocation().Equals(TriggerLocation, 1.f), TEXT("-1100,0,-4710"), Trigger ? Trigger->GetActorLocation().ToString() : TEXT("missing"), TriggerLabel);
			AssertTrue(Record, TEXT("guard.terminal"), Terminals.Num() == 1 && Terminals[0]->GetActorLocation().Equals(TerminalLocation, 1.f), TEXT("-1950,-1650,-4700"), Terminals.Num() == 1 ? Terminals[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Terminal_ControlSpine"));
			AssertTrue(Record, TEXT("guard.checkpoint"), Checkpoints.Num() == 1 && Checkpoints[0]->GetActorLocation().Equals(CheckpointLocation, 1.f), TEXT("25,0,-4740"), Checkpoints.Num() == 1 ? Checkpoints[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Checkpoint_BasinRim"));
			if (bAnyAssertFailed || !Mission || !Pursuer || !Trigger)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Pursuer intro contract missing.") : Record.FailureReason);
				return;
			}
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			(void)Record;
			if (!Owner.RequestStartPie(MapPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			WaitSeconds = 0.f;
			bRequestedStream = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedStream)
			{
				const FName ReactorName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel5_Reactor);
				if (!ReactorName.IsNone())
				{
					Levels->AddStreamRequest(ReactorName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedStream = true;
				}
			}
			if (Character && bRequestedStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, PursuerLabel).Num() == 1 && FindHud(World, Character))
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with BP_Pursuer."));
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, PursuerLabel) : TArray<AActor*>();
			TArray<AActor*> Triggers = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TriggerLabel) : TArray<AActor*>();
			AProjectOrganoidPursuer* Pursuer = Found.Num() == 1 ? Cast<AProjectOrganoidPursuer>(Found[0]) : nullptr;
			AProjectOrganoidPursuerTrigger* Trigger = Triggers.Num() == 1 ? Cast<AProjectOrganoidPursuerTrigger>(Triggers[0]) : nullptr;
			if (!World || !Character || !Widget || !Objectives || !Power || !Pursuer || !Trigger)
			{
				FailAndStop(Owner, Record, TEXT("Pursuer intro actors or subsystems missing."));
				return;
			}
			const EProjectOrganoidPowerState ReactorBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor);
			AssertTrue(Record, TEXT("hidden.before"), Pursuer->IsHidden(), TEXT("hidden"), Pursuer->IsHidden() ? TEXT("hidden") : TEXT("visible"), PursuerLabel);
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			const bool bLoaded = Mission && Objectives->LoadMission(Mission, false);
			AssertTrue(Record, TEXT("mission.active"), bLoaded && Objectives->GetActiveMissionId() == FName(MissionId), MissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.f);
			const float Damage = Pursuer->TakeDamage(40.f, FDamageEvent(), nullptr, Character);
			AssertTrue(Record, TEXT("invulnerable"), Damage == 0.f && !Pursuer->CanBeDamaged(), TEXT("0"), FString::SanitizeFloat(Damage), PursuerLabel);
			Character->SetActorLocation(TriggerLocation, false, nullptr, ETeleportType::TeleportPhysics);
			Trigger->NotifyPlayerOverlap(Character);
			FProjectOrganoidObjective Objective;
			const bool bObjective = Objectives->GetObjective(FName(ObjectiveId), Objective);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			AssertTrue(Record, TEXT("shown"), Pursuer->IsRevealed() && !Pursuer->IsHidden(), TEXT("shown"), Pursuer->IsRevealed() ? TEXT("shown") : TEXT("hidden"), PursuerLabel);
			AssertTrue(Record, TEXT("bang"), Pursuer->HasPlayedBang(), TEXT("true"), Pursuer->HasPlayedBang() ? TEXT("true") : TEXT("false"), PursuerLabel);
			AssertTrue(Record, TEXT("nathan.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("nathan.duration"), Widget->GetTransientNotificationSecondsRemaining() > 6.f && Widget->GetTransientNotificationSecondsRemaining() <= 7.f, TEXT("7"), FString::SanitizeFloat(Widget->GetTransientNotificationSecondsRemaining()), TEXT("HUD"));
			AssertTrue(Record, TEXT("objective.complete"), bObjective && Objective.State == EProjectOrganoidObjectiveState::Completed && Objective.CurrentProgress == 1, TEXT("1"), bObjective ? FString::FromInt(Objective.CurrentProgress) : TEXT("missing"), ObjectiveId);
			AssertTrue(Record, TEXT("mission.stays"), Objectives->GetActiveMissionId() == FName(MissionId) && Objectives->IsMissionComplete(FName(MissionId)), MissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("power.unchanged"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == ReactorBefore, TEXT("unchanged"), TEXT("checked"), TEXT("Power"));
			AssertTrue(Record, TEXT("no_drop"), !Pursuer->DropsWeapon(), TEXT("false"), TEXT("false"), PursuerLabel);
			PursuerActor = Pursuer;
			WaitSeconds = 0.f;
			Stage = bAnyAssertFailed ? EStage::EndPie : EStage::WaitDespawn;
		}

		void TickWaitDespawn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 13.f)
			{
				return;
			}
			AssertTrue(Record, TEXT("despawn"), !PursuerActor.IsValid(), TEXT("destroyed"), PursuerActor.IsValid() ? TEXT("alive") : TEXT("destroyed"), PursuerLabel);
			Stage = EStage::EndPie;
			(void)Owner;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedStream = false;
		TWeakObjectPtr<AProjectOrganoidPursuer> PursuerActor;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FPursuerIntroFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterPursuerIntroFunctional;
}
