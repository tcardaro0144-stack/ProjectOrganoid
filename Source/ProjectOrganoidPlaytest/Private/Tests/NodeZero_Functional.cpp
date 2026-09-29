#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSterlingEscapeCinematic.h"

namespace NodeZeroFunctional
{
	constexpr TCHAR TestId[] = TEXT("NodeZero_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Node Zero Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR ReactorPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MissionPath[] = TEXT("/Game/Data/Missions/DA_Mission_NodeZero.DA_Mission_NodeZero");
	constexpr TCHAR FirstCombatPath[] = TEXT("/Game/Data/Missions/DA_Mission_FirstCombat.DA_Mission_FirstCombat");
	constexpr TCHAR ConclusionPath[] = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion.DA_Mission_TheConclusion");
	constexpr TCHAR ResearchPath[] = TEXT("/Game/Data/Missions/DA_Mission_ResearchStation.DA_Mission_ResearchStation");
	constexpr TCHAR MissionId[] = TEXT("Mission_NodeZero");
	constexpr TCHAR ConclusionMissionId[] = TEXT("Mission_TheConclusion");
	constexpr TCHAR ReachId[] = TEXT("Obj_ReachNodeZero");
	constexpr TCHAR FateId[] = TEXT("Obj_ChooseFate");
	constexpr TCHAR ReachEvent[] = TEXT("Event_NodeZeroReached");
	constexpr TCHAR FateEvent[] = TEXT("Event_FateChosen");
	constexpr TCHAR CoreLabel[] = TEXT("BP_NodeZeroCore");
	constexpr TCHAR TerminalLabel[] = TEXT("Terminal_SterlingFinal");
	constexpr TCHAR EscapeCinematicLabel[] = TEXT("BP_SterlingEscapeCinematic");
	constexpr TCHAR EscapeTriggerLabel[] = TEXT("BP_SterlingEscapeTrigger");
	constexpr TCHAR ExpectedLine[] = TEXT("I built it to heal. It learned to keep.");
	constexpr TCHAR ExpectedExtractLine[] = TEXT("You take it, you become the carrier.");
	const FVector CoreLocation(-200.f, 0.f, -4710.f);
	const FVector TerminalLocation(-200.f, 400.f, -4710.f);
	const FVector EscapeTriggerLocation(-200.f, 600.f, -4710.f);
	const FVector EscapeCinematicLocation(-200.f, 500.f, -4710.f);
	const FVector PursuerLocation(-1800.f, 0.f, -4710.f);
	const FVector PursuerTriggerLocation(-1100.f, 0.f, -4710.f);
	const FVector ScientistLocation(-800.f, 800.f, -4710.f);
	const FVector CombatTriggerLocation(-500.f, 0.f, -4710.f);
	const FVector SpineLocation(-1950.f, -1650.f, -4700.f);
	const FVector CheckpointLocation(25.f, 0.f, -4740.f);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path)) return Package->IsDirty();
		return false;
	}

	class FNodeZeroFunctional : public IOrganoidPlaytestCase
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
						Record->FailureReason = TEXT("A locked package was dirty after the Node Zero test.");
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
			UProjectOrganoidObjectiveDataAsset* FirstCombat = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, FirstCombatPath);
			UProjectOrganoidObjectiveDataAsset* Conclusion = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConclusionPath);
			const FProjectOrganoidMissionTaskDefinition* Reach = Mission && Mission->Tasks.Num() == 2 ? &Mission->Tasks[0] : nullptr;
			const FProjectOrganoidMissionTaskDefinition* Fate = Mission && Mission->Tasks.Num() == 2 ? &Mission->Tasks[1] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Mission && Mission->MissionId == FName(MissionId), MissionId, Mission ? Mission->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Mission && Mission->MissionTitle.ToString() == TEXT("Node Zero"), TEXT("Node Zero"), Mission ? Mission->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Mission && Mission->MissionDescription.ToString() == TEXT("Sterling's confession points to the core. The Organoid's heart is still beating. Shut it down or take it with you."), TEXT("confession"), Mission ? Mission->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_conclusion"), Mission && Mission->NextMissionAsset.ToSoftObjectPath().ToString() == ConclusionPath, ConclusionPath, Mission ? Mission->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.main"), Reach && Reach->Objective.Type == EProjectOrganoidObjectiveType::Main && Reach->Objective.ObjectiveId == FName(ReachId) && Reach->bAutoActivate && Reach->Objective.TargetProgress == 1 && Reach->Objective.PrerequisiteObjectiveIds.Num() == 0, ReachId, Reach ? Reach->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.side"), Fate && Fate->Objective.Type == EProjectOrganoidObjectiveType::Side && Fate->Objective.ObjectiveId == FName(FateId) && Fate->bAutoActivate && Fate->Objective.TargetProgress == 1 && Fate->Objective.PrerequisiteObjectiveIds.Num() == 0, FateId, Fate ? Fate->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.reach_event"), Reach && Reach->EventTriggers.Num() == 1 && Reach->EventTriggers[0].EventId == FName(ReachEvent) && Reach->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete, ReachEvent, Reach && Reach->EventTriggers.Num() == 1 ? Reach->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.fate_event"), Fate && Fate->EventTriggers.Num() == 1 && Fate->EventTriggers[0].EventId == FName(FateEvent) && Fate->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete, FateEvent, Fate && Fate->EventTriggers.Num() == 1 ? Fate->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("handoff.first_combat"), FirstCombat && FirstCombat->NextMissionAsset.ToSoftObjectPath().ToString() == MissionPath, MissionPath, FirstCombat ? FirstCombat->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("handoff.conclusion_stays"), Conclusion && Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() == ResearchPath, ResearchPath, Conclusion ? Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Cores = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, CoreLabel) : TArray<AActor*>();
			TArray<AActor*> Terminals = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TerminalLabel) : TArray<AActor*>();
			AProjectOrganoidNodeZeroCore* Core = Cores.Num() == 1 ? Cast<AProjectOrganoidNodeZeroCore>(Cores[0]) : nullptr;
			TArray<AActor*> Spine = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Terminal_ControlSpine")) : TArray<AActor*>();
			TArray<AActor*> Checkpoints = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Checkpoint_BasinRim")) : TArray<AActor*>();
			TArray<AActor*> Pursuers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("BP_Pursuer")) : TArray<AActor*>();
			TArray<AActor*> PursuerTriggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Reactor_PursuerTrigger")) : TArray<AActor*>();
			TArray<AActor*> Scientists = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("BP_TransformedScientist")) : TArray<AActor*>();
			TArray<AActor*> CombatTriggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TEXT("Reactor_FirstCombatTrigger")) : TArray<AActor*>();
			TArray<AActor*> EscapeCinematics = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, EscapeCinematicLabel) : TArray<AActor*>();
			TArray<AActor*> EscapeTriggers = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, EscapeTriggerLabel) : TArray<AActor*>();
			AssertTrue(Record, TEXT("actor.count"), Cores.Num() == 1 && Core != nullptr, TEXT("1"), FString::FromInt(Cores.Num()), CoreLabel);
			AssertTrue(Record, TEXT("actor.location"), Core && Core->GetActorLocation().Equals(CoreLocation, 1.f), TEXT("-200,0,-4710"), Core ? Core->GetActorLocation().ToString() : TEXT("missing"), CoreLabel);
			AssertTrue(Record, TEXT("actor.package"), Core && Core->GetOutermost() && Core->GetOutermost()->GetName().Contains(TEXT("SL_Epitope_Reactor")), ReactorPackage, Core && Core->GetOutermost() ? Core->GetOutermost()->GetName() : TEXT("missing"), CoreLabel);
			AssertTrue(Record, TEXT("actor.interactable"), Core && Core->bIsInteractable && Core->GetShutdownStage() == 0 && !Core->UsesPursuerAI() && !Core->IsA(APawn::StaticClass()), TEXT("interactable"), Core ? TEXT("present") : TEXT("missing"), CoreLabel);
			AssertTrue(Record, TEXT("terminal.count"), Terminals.Num() == 1 && Terminals[0] && Terminals[0]->GetClass() && Terminals[0]->GetClass()->GetName().Equals(TEXT("ProjectOrganoidTerminal")), TEXT("1"), FString::FromInt(Terminals.Num()), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.location"), Terminals.Num() == 1 && Terminals[0]->GetActorLocation().Equals(TerminalLocation, 1.f) && Terminals[0]->GetOutermost() && Terminals[0]->GetOutermost()->GetName().Contains(TEXT("SL_Epitope_Reactor")), TEXT("-200,400,-4710"), Terminals.Num() == 1 ? Terminals[0]->GetActorLocation().ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("escape.cinematic"), EscapeCinematics.Num() == 1 && Cast<AProjectOrganoidSterlingEscapeCinematic>(EscapeCinematics[0]) != nullptr && EscapeCinematics[0]->GetActorLocation().Equals(EscapeCinematicLocation, 1.f), TEXT("1"), FString::FromInt(EscapeCinematics.Num()), EscapeCinematicLabel);
			AssertTrue(Record, TEXT("escape.trigger"), EscapeTriggers.Num() == 1 && Cast<AProjectOrganoidSterlingEscapeTrigger>(EscapeTriggers[0]) != nullptr && EscapeTriggers[0]->GetActorLocation().Equals(EscapeTriggerLocation, 1.f), TEXT("1"), FString::FromInt(EscapeTriggers.Num()), EscapeTriggerLabel);
			AssertTrue(Record, TEXT("guard.spine"), Spine.Num() == 1 && Spine[0]->GetActorLocation().Equals(SpineLocation, 1.f), TEXT("-1950,-1650,-4700"), Spine.Num() == 1 ? Spine[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Terminal_ControlSpine"));
			AssertTrue(Record, TEXT("guard.checkpoint"), Checkpoints.Num() == 1 && Checkpoints[0]->GetActorLocation().Equals(CheckpointLocation, 1.f), TEXT("25,0,-4740"), Checkpoints.Num() == 1 ? Checkpoints[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Checkpoint_BasinRim"));
			AssertTrue(Record, TEXT("guard.pursuer"), Pursuers.Num() == 1 && Pursuers[0]->GetActorLocation().Equals(PursuerLocation, 1.f), TEXT("-1800,0,-4710"), Pursuers.Num() == 1 ? Pursuers[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("BP_Pursuer"));
			AssertTrue(Record, TEXT("guard.pursuer_trigger"), PursuerTriggers.Num() == 1 && PursuerTriggers[0]->GetActorLocation().Equals(PursuerTriggerLocation, 1.f), TEXT("-1100,0,-4710"), PursuerTriggers.Num() == 1 ? PursuerTriggers[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Reactor_PursuerTrigger"));
			AssertTrue(Record, TEXT("guard.scientist"), Scientists.Num() == 1 && Scientists[0]->GetActorLocation().Equals(ScientistLocation, 1.f), TEXT("-800,800,-4710"), Scientists.Num() == 1 ? Scientists[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("BP_TransformedScientist"));
			AssertTrue(Record, TEXT("guard.combat_trigger"), CombatTriggers.Num() == 1 && CombatTriggers[0]->GetActorLocation().Equals(CombatTriggerLocation, 1.f), TEXT("-500,0,-4710"), CombatTriggers.Num() == 1 ? CombatTriggers[0]->GetActorLocation().ToString() : TEXT("missing"), TEXT("Reactor_FirstCombatTrigger"));
			if (bAnyAssertFailed || !Mission || !Core || Terminals.Num() != 1 || EscapeCinematics.Num() != 1 || EscapeTriggers.Num() != 1)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Node Zero contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, CoreLabel).Num() == 1 && FindHud(World, Character))
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with BP_NodeZeroCore."));
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adaptations = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, CoreLabel) : TArray<AActor*>();
			AProjectOrganoidNodeZeroCore* Core = Found.Num() == 1 ? Cast<AProjectOrganoidNodeZeroCore>(Found[0]) : nullptr;
			if (!World || !Character || !Widget || !Objectives || !Power || !Saves || !Adaptations || !Core)
			{
				FailAndStop(Owner, Record, TEXT("Node Zero actors or subsystems missing."));
				return;
			}
			const EProjectOrganoidPowerState ReactorBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor);
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			const bool bLoaded = Mission && Objectives->LoadMission(Mission, false);
			AssertTrue(Record, TEXT("mission.active"), bLoaded && Objectives->GetActiveMissionId() == FName(MissionId) && !Saves->HasNewGamePlus(), MissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.f);
			const bool bRefusedNone = !Core->CraftVaccine(Character);
			Adaptations->UnlockAdaptation(UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve());
			const bool bRefusedOne = !Core->CraftVaccine(Character);
			AssertTrue(Record, TEXT("vaccine.needs_two"), bRefusedNone && bRefusedOne && !Core->IsVaccineCrafted(), TEXT("refused"), Core->IsVaccineCrafted() ? TEXT("crafted") : TEXT("refused"), CoreLabel);
			Adaptations->UnlockAdaptation(UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve());
			AssertTrue(Record, TEXT("vaccine.crafted"), Core->CraftVaccine(Character) && Core->IsVaccineCrafted(), TEXT("crafted"), Core->IsVaccineCrafted() ? TEXT("crafted") : TEXT("refused"), CoreLabel);
			AssertTrue(Record, TEXT("choice.early"), !Core->ChooseFate(Character, EProjectOrganoidNodeZeroFate::Destroy) && Core->GetChosenFate() == EProjectOrganoidNodeZeroFate::None, TEXT("refused"), TEXT("checked"), CoreLabel);
			AssertTrue(Record, TEXT("shutdown.one"), Core->Interact(Character) && Core->GetShutdownStage() == 1, TEXT("1"), FString::FromInt(Core->GetShutdownStage()), CoreLabel);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			AssertTrue(Record, TEXT("sterling.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Sterling:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("sterling.duration"), Widget->GetTransientNotificationSecondsRemaining() > 6.f && Widget->GetTransientNotificationSecondsRemaining() <= 7.f, TEXT("7"), FString::SanitizeFloat(Widget->GetTransientNotificationSecondsRemaining()), TEXT("HUD"));
			AssertTrue(Record, TEXT("shutdown.three"), Core->Interact(Character) && Core->Interact(Character) && Core->GetShutdownStage() == 3 && Core->GetSterlingPresentationCount() == 1 && !Core->Interact(Character), TEXT("3"), FString::FromInt(Core->GetShutdownStage()), CoreLabel);

			// Destroy on a temporary core: completes both objectives, does not grant NG+.
			AProjectOrganoidNodeZeroCore* DestroyCore = World->SpawnActor<AProjectOrganoidNodeZeroCore>(CoreLocation + FVector(200.f, 0.f, 0.f), FRotator::ZeroRotator);
			AssertTrue(Record, TEXT("choice.destroy_setup"), DestroyCore != nullptr, TEXT("spawned"), DestroyCore ? TEXT("spawned") : TEXT("missing"), CoreLabel);
			if (DestroyCore)
			{
				AssertTrue(Record, TEXT("choice.destroy_vaccine"), DestroyCore->CraftVaccine(Character) && DestroyCore->IsVaccineCrafted(), TEXT("crafted"), DestroyCore->IsVaccineCrafted() ? TEXT("crafted") : TEXT("refused"), CoreLabel);
				AssertTrue(Record, TEXT("choice.destroy_shutdown"), DestroyCore->Interact(Character) && DestroyCore->Interact(Character) && DestroyCore->Interact(Character) && DestroyCore->GetShutdownStage() == 3, TEXT("3"), FString::FromInt(DestroyCore->GetShutdownStage()), CoreLabel);
				AssertTrue(Record, TEXT("choice.destroy"), DestroyCore->ChooseFate(Character, EProjectOrganoidNodeZeroFate::Destroy) && DestroyCore->GetChosenFate() == EProjectOrganoidNodeZeroFate::Destroy && !DestroyCore->HasGrantedNewGamePlus() && !Saves->HasNewGamePlus(), TEXT("Destroy"), TEXT("checked"), CoreLabel);
			}

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.f);
			AssertTrue(Record, TEXT("choice.extract"), Core->ChooseFate(Character, EProjectOrganoidNodeZeroFate::Extract) && Core->GetChosenFate() == EProjectOrganoidNodeZeroFate::Extract && !Core->ChooseFate(Character, EProjectOrganoidNodeZeroFate::Destroy), TEXT("Extract"), TEXT("checked"), CoreLabel);
			const FString ExtractLine = Widget->GetLastResourceNotification().ToString();
			AssertTrue(Record, TEXT("sterling.extract_line"), Core->WasExtractLineShown() && ExtractLine.Contains(ExpectedExtractLine), ExpectedExtractLine, ExtractLine, TEXT("HUD"));
			FProjectOrganoidObjective Reach;
			FProjectOrganoidObjective Fate;
			const bool bReach = Objectives->GetObjective(FName(ReachId), Reach);
			const bool bFate = Objectives->GetObjective(FName(FateId), Fate);
			UProjectOrganoidSaveGame* Captured = Saves->CaptureSaveFromCharacter(Character);
			AssertTrue(Record, TEXT("objective.reach"), bReach && Reach.State == EProjectOrganoidObjectiveState::Completed && Reach.CurrentProgress == 1, TEXT("1"), bReach ? FString::FromInt(Reach.CurrentProgress) : TEXT("missing"), ReachId);
			AssertTrue(Record, TEXT("objective.fate"), bFate && Fate.State == EProjectOrganoidObjectiveState::Completed && Fate.CurrentProgress == 1, TEXT("1"), bFate ? FString::FromInt(Fate.CurrentProgress) : TEXT("missing"), FateId);
			AssertTrue(Record, TEXT("ngplus"), Core->HasGrantedNewGamePlus() && Saves->HasNewGamePlus() && Captured && Captured->bNewGamePlus, TEXT("true"), Saves->HasNewGamePlus() ? TEXT("true") : TEXT("false"), TEXT("Save"));
			TArray<AActor*> EscapeCinematics = OrganoidPlaytestActions::FindActorsByLabel(World, EscapeCinematicLabel);
			AProjectOrganoidSterlingEscapeCinematic* Escape = EscapeCinematics.Num() == 1 ? Cast<AProjectOrganoidSterlingEscapeCinematic>(EscapeCinematics[0]) : nullptr;
			AssertTrue(Record, TEXT("escape.played"), Escape && Escape->GetLastPlayedFate() == EProjectOrganoidNodeZeroFate::Extract && Escape->GetPlayCount() >= 1, TEXT("Extract"), Escape ? TEXT("played") : TEXT("missing"), EscapeCinematicLabel);
			AssertTrue(Record, TEXT("mission.advances"), Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("power.unchanged"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == ReactorBefore, TEXT("unchanged"), TEXT("checked"), TEXT("Power"));
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
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FNodeZeroFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterNodeZeroFunctional;
}
