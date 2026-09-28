#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidTransformedScientist.h"
#include "ProjectOrganoidWeaponTypes.h"

namespace FirstCombatFunctional
{
	constexpr TCHAR TestId[] = TEXT("FirstCombat_Functional");
	constexpr TCHAR DisplayName[] = TEXT("First Combat Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR ReactorPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MissionPath[] = TEXT("/Game/Data/Missions/DA_Mission_FirstCombat.DA_Mission_FirstCombat");
	constexpr TCHAR PursuerPath[] = TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro.DA_Mission_PursuerIntro");
	constexpr TCHAR MissionId[] = TEXT("Mission_FirstCombat");
	constexpr TCHAR ObjectiveId[] = TEXT("Obj_DefeatTransformed");
	constexpr TCHAR EventId[] = TEXT("Event_FirstCombatDefeated");
	constexpr TCHAR ScientistLabel[] = TEXT("BP_TransformedScientist");
	constexpr TCHAR TriggerLabel[] = TEXT("Reactor_FirstCombatTrigger");
	constexpr TCHAR ExpectedLine[] = TEXT("It's still wearing the lab coat. Christ.");
	const FVector ScientistLocation(-800.f, 800.f, -4710.f);
	const FVector TriggerLocation(-500.f, 0.f, -4710.f);
	const FVector PursuerLocation(-1800.f, 0.f, -4710.f);
	const FVector PursuerTriggerLocation(-1100.f, 0.f, -4710.f);
	const FVector TerminalLocation(-1950.f, -1650.f, -4700.f);
	const FVector CheckpointLocation(25.f, 0.f, -4740.f);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path)) return Package->IsDirty();
		return false;
	}

	class FFirstCombatFunctional : public IOrganoidPlaytestCase
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
						Record->FailureReason = TEXT("A locked package was dirty after the first combat test.");
						bAnyAssertFailed = true;
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, EndPie, WaitStopped };

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
			UProjectOrganoidObjectiveDataAsset* PursuerMission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, PursuerPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Mission && Mission->Tasks.Num() == 1 ? &Mission->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Mission && Mission->MissionId == FName(MissionId), MissionId, Mission ? Mission->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Mission && Mission->MissionTitle.ToString() == TEXT("First Combat"), TEXT("First Combat"), Mission ? Mission->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Mission && Mission->MissionDescription.ToString().Contains(TEXT("lab coat")) && Mission->MissionDescription.ToString().Contains(TEXT("Lytic")), TEXT("lab coat"), Mission ? Mission->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_node_zero"), Mission && Mission->NextMissionAsset.ToSoftObjectPath().ToString() == TEXT("/Game/Data/Missions/DA_Mission_NodeZero.DA_Mission_NodeZero"), TEXT("/Game/Data/Missions/DA_Mission_NodeZero.DA_Mission_NodeZero"), Mission ? Mission->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main && Task->Objective.ObjectiveId == FName(ObjectiveId) && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, ObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.auto_target"), Task && Task->bAutoActivate && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(EventId) && Task->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete, EventId, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("handoff.pursuer"), PursuerMission && PursuerMission->NextMissionAsset.ToSoftObjectPath().ToString() == MissionPath, MissionPath, PursuerMission ? PursuerMission->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Scientists = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, ScientistLabel) : TArray<AActor*>();
			TArray<AActor*> Triggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TriggerLabel) : TArray<AActor*>();
			AProjectOrganoidTransformedScientist* Scientist = Scientists.Num() == 1 ? Cast<AProjectOrganoidTransformedScientist>(Scientists[0]) : nullptr;
			AProjectOrganoidFirstCombatTrigger* Trigger = Triggers.Num() == 1 ? Cast<AProjectOrganoidFirstCombatTrigger>(Triggers[0]) : nullptr;
			TArray<AActor*> Terminals = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Terminal_ControlSpine")) : TArray<AActor*>();
			TArray<AActor*> Checkpoints = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Checkpoint_BasinRim")) : TArray<AActor*>();
			TArray<AActor*> Pursuers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("BP_Pursuer")) : TArray<AActor*>();
			TArray<AActor*> PursuerTriggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Reactor_PursuerTrigger")) : TArray<AActor*>();
			AssertTrue(Record, TEXT("actor.count"), Scientists.Num() == 1 && Scientist != nullptr, TEXT("1"), FString::FromInt(Scientists.Num()), ScientistLabel);
			AssertTrue(Record, TEXT("actor.location"), Scientist && Scientist->GetActorLocation().Equals(ScientistLocation, 1.f), TEXT("-800,800,-4710"), Scientist ? Scientist->GetActorLocation().ToString() : TEXT("missing"), ScientistLabel);
			AssertTrue(Record, TEXT("actor.package"), Scientist && Scientist->GetOutermost() && Scientist->GetOutermost()->GetName().Contains(TEXT("SL_Epitope_Reactor")), ReactorPackage, Scientist && Scientist->GetOutermost() ? Scientist->GetOutermost()->GetName() : TEXT("missing"), ScientistLabel);
			AssertTrue(Record, TEXT("actor.combat"), Scientist && !Scientist->IsA(APawn::StaticClass()) && !Scientist->UsesPursuerAI() && FMath::IsNearlyEqual(Scientist->GetMaxHealth(), 100.f) && FMath::IsNearlyEqual(Scientist->GetSlowPlayRate(), 0.45f), TEXT("100"), Scientist ? FString::SanitizeFloat(Scientist->GetMaxHealth()) : TEXT("missing"), ScientistLabel);
			AssertTrue(Record, TEXT("trigger.count"), Triggers.Num() == 1 && Trigger != nullptr, TEXT("1"), FString::FromInt(Triggers.Num()), TriggerLabel);
			AssertTrue(Record, TEXT("trigger.location"), Trigger && Trigger->GetActorLocation().Equals(TriggerLocation, 1.f), TEXT("-500,0,-4710"), Trigger ? Trigger->GetActorLocation().ToString() : TEXT("missing"), TriggerLabel);
			AssertTrue(Record, TEXT("guard.terminal"), Terminals.Num() == 1 && Terminals[0]->GetActorLocation().Equals(TerminalLocation, 1.f), TEXT("-1950,-1650,-4700"), Terminals.Num() == 1 ? Terminals[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Terminal_ControlSpine"));
			AssertTrue(Record, TEXT("guard.checkpoint"), Checkpoints.Num() == 1 && Checkpoints[0]->GetActorLocation().Equals(CheckpointLocation, 1.f), TEXT("25,0,-4740"), Checkpoints.Num() == 1 ? Checkpoints[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Checkpoint_BasinRim"));
			AssertTrue(Record, TEXT("guard.pursuer"), Pursuers.Num() == 1 && Pursuers[0]->GetActorLocation().Equals(PursuerLocation, 1.f), TEXT("-1800,0,-4710"), Pursuers.Num() == 1 ? Pursuers[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("BP_Pursuer"));
			AssertTrue(Record, TEXT("guard.pursuer_trigger"), PursuerTriggers.Num() == 1 && PursuerTriggers[0]->GetActorLocation().Equals(PursuerTriggerLocation, 1.f), TEXT("-1100,0,-4710"), PursuerTriggers.Num() == 1 ? PursuerTriggers[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Reactor_PursuerTrigger"));
			if (bAnyAssertFailed || !Mission || !Scientist || !Trigger)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("First combat contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, ScientistLabel).Num() == 1 && FindHud(World, Character))
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with BP_TransformedScientist."));
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, ScientistLabel) : TArray<AActor*>();
			TArray<AActor*> Triggers = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TriggerLabel) : TArray<AActor*>();
			AProjectOrganoidTransformedScientist* Scientist = Found.Num() == 1 ? Cast<AProjectOrganoidTransformedScientist>(Found[0]) : nullptr;
			AProjectOrganoidFirstCombatTrigger* Trigger = Triggers.Num() == 1 ? Cast<AProjectOrganoidFirstCombatTrigger>(Triggers[0]) : nullptr;
			if (!World || !Character || !Widget || !Objectives || !Power || !Inventory || !Scientist || !Trigger)
			{
				FailAndStop(Owner, Record, TEXT("First combat actors or subsystems missing."));
				return;
			}
			const EProjectOrganoidPowerState ReactorBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor);
			const int32 AmmoBefore = Inventory->CountAmmoOfType(EProjectOrganoidAmmoType::Pistol);
			AssertTrue(Record, TEXT("hidden.before"), Scientist->IsHidden() && FMath::IsNearlyEqual(Scientist->GetHealth(), 100.f), TEXT("hidden"), Scientist->IsHidden() ? TEXT("hidden") : TEXT("visible"), ScientistLabel);
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			const bool bLoaded = Mission && Objectives->LoadMission(Mission, false);
			AssertTrue(Record, TEXT("mission.active"), bLoaded && Objectives->GetActiveMissionId() == FName(MissionId), MissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.f);
			Scientist->ApplyWeaponHit(28.f, Character);
			AssertTrue(Record, TEXT("sealed.before"), FMath::IsNearlyEqual(Scientist->GetHealth(), 100.f), TEXT("100"), FString::SanitizeFloat(Scientist->GetHealth()), ScientistLabel);
			Character->SetActorLocation(TriggerLocation, false, nullptr, ETeleportType::TeleportPhysics);
			Trigger->NotifyPlayerOverlap(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			AssertTrue(Record, TEXT("shown"), Scientist->IsRevealed() && !Scientist->IsHidden(), TEXT("shown"), Scientist->IsRevealed() ? TEXT("shown") : TEXT("hidden"), ScientistLabel);
			AssertTrue(Record, TEXT("nathan.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("nathan.duration"), Widget->GetTransientNotificationSecondsRemaining() > 6.f && Widget->GetTransientNotificationSecondsRemaining() <= 7.f, TEXT("7"), FString::SanitizeFloat(Widget->GetTransientNotificationSecondsRemaining()), TEXT("HUD"));
			Scientist->ApplyWeaponHit(28.f, Character);
			AssertTrue(Record, TEXT("lytic.damage"), FMath::IsNearlyEqual(Scientist->GetHealth(), 72.f), TEXT("72"), FString::SanitizeFloat(Scientist->GetHealth()), ScientistLabel);
			Scientist->ApplyWeaponHit(16.f, Character);
			AssertTrue(Record, TEXT("roster.damage"), FMath::IsNearlyEqual(Scientist->GetHealth(), 56.f), TEXT("56"), FString::SanitizeFloat(Scientist->GetHealth()), ScientistLabel);
			Scientist->ApplyWeaponHit(56.f, Character);
			FProjectOrganoidObjective Objective;
			const bool bObjective = Objectives->GetObjective(FName(ObjectiveId), Objective);
			AssertTrue(Record, TEXT("death"), Scientist->IsDead() && FMath::IsNearlyEqual(Scientist->GetHealth(), 0.f), TEXT("dead"), Scientist->IsDead() ? TEXT("dead") : TEXT("alive"), ScientistLabel);
			AssertTrue(Record, TEXT("drop.lytic"), Scientist->GetDroppedLyticCharges() == 1 && Scientist->WasLyticChargeGranted() && Inventory->CountAmmoOfType(EProjectOrganoidAmmoType::Pistol) == AmmoBefore + 1, TEXT("1"), FString::FromInt(Scientist->GetDroppedLyticCharges()), ScientistLabel);
			AssertTrue(Record, TEXT("objective.complete"), bObjective && Objective.State == EProjectOrganoidObjectiveState::Completed && Objective.CurrentProgress == 1, TEXT("1"), bObjective ? FString::FromInt(Objective.CurrentProgress) : TEXT("missing"), ObjectiveId);
			AssertTrue(Record, TEXT("mission.advances"), Objectives->GetActiveMissionId() == FName(TEXT("Mission_NodeZero")), TEXT("Mission_NodeZero"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("power.unchanged"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == ReactorBefore, TEXT("unchanged"), TEXT("checked"), TEXT("Power"));
			AssertTrue(Record, TEXT("no_pursuer_ai"), !Scientist->UsesPursuerAI() && !Scientist->IsA(APawn::StaticClass()), TEXT("false"), TEXT("false"), ScientistLabel);
			Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedStream = false;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FFirstCombatFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterFirstCombatFunctional;
}
