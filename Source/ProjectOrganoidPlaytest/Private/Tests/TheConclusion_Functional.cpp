#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidTerminal.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace TheConclusionFunctional
{
	constexpr TCHAR TestId[] = TEXT("TheConclusion_Functional");
	constexpr TCHAR DisplayName[] = TEXT("The Conclusion Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR ReactorPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor");
	constexpr TCHAR ConclusionSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion.DA_Mission_TheConclusion");
	constexpr TCHAR ResearchStationSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ResearchStation.DA_Mission_ResearchStation");
	constexpr TCHAR ResearchStationMissionId[] = TEXT("Mission_ResearchStation");
	constexpr TCHAR HandoverSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeHandover.DA_Mission_ComputeHandover");
	constexpr TCHAR EntrySoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeEntry.DA_Mission_ComputeEntry");
	constexpr TCHAR ConclusionMissionId[] = TEXT("Mission_TheConclusion");
	constexpr TCHAR HandoverMissionId[] = TEXT("Mission_ComputeHandover");
	constexpr TCHAR ConclusionObjectiveId[] = TEXT("Obj_ReachControlSpine");
	constexpr TCHAR HackObjectiveId[] = TEXT("Obj_HackComputeCore");
	constexpr TCHAR ConclusionEvent[] = TEXT("Event_ReactorControlUsed");
	constexpr TCHAR HackEvent[] = TEXT("Event_ComputeCoreHacked");
	constexpr TCHAR ConfessionEvent[] = TEXT("Event_SterlingConfessionRead");
	constexpr TCHAR EntryEvent[] = TEXT("Event_ComputeEntered");
	constexpr TCHAR TerminalLabel[] = TEXT("Terminal_ControlSpine");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidTheConclusionTest");
	constexpr TCHAR ExpectedLine[] = TEXT("The incubator is awake. Every document leads here.");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Reach Control Spine");
	const FVector TerminalLocation(-1950.f, -1650.f, -4700.f);

	FString BoolText(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }
	const TCHAR* PowerText(EProjectOrganoidPowerState State)
	{
		switch (State)
		{
		case EProjectOrganoidPowerState::Online: return TEXT("Online");
		case EProjectOrganoidPowerState::Emergency: return TEXT("Emergency");
		case EProjectOrganoidPowerState::Blackout: return TEXT("Blackout");
		default: return TEXT("Unknown");
		}
	}
	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path)) return Package->IsDirty();
		return false;
	}
	int32 CountActiveId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives) return 0;
		for (const FProjectOrganoidObjective& Entry : Objectives->GetActiveObjectives())
		{
			if (Entry.ObjectiveId == Id) ++Count;
		}
		return Count;
	}
	int32 CountCompletedId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives) return 0;
		for (const FProjectOrganoidObjective& Entry : Objectives->GetCompletedObjectives())
		{
			if (Entry.ObjectiveId == Id) ++Count;
		}
		return Count;
	}

	class FTheConclusionFunctional : public IOrganoidPlaytestCase
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
			bRequestedReactorStream = false;
			bSecondSession = false;
			bCampaignDone = false;
			bReloadDone = false;
			Owner.SetStage(TEXT("Preflight"));
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
		}
		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
			Owner.SetStage(TEXT("Abort"));
		}
		virtual void Tick(UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) override
		{
			FOrganoidPlaytestRecord* Record = Owner.GetActiveRecord();
			if (!Record) return;
			switch (Stage)
			{
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie: TickStartPie(Owner, *Record); break;
			case EStage::WaitReady: TickWaitReady(Owner, *Record, DeltaTime); break;
			case EStage::Proof: TickProof(Owner, *Record, DeltaTime); break;
			case EStage::EndSession:
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitBetween;
				break;
			case EStage::WaitBetween:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress())
				{
					if (bAnyAssertFailed)
					{
						Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record->FailureReason);
						return;
					}
					bSecondSession = true;
					bRequestedReactorStream = false;
					Stage = EStage::StartPie;
					return;
				}
				if (WaitSeconds > 20.f) Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("First PIE session did not end."));
				break;
			case EStage::EndPie:
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitStopped;
				break;
			case EStage::WaitStopped:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress() || WaitSeconds > 20.f)
				{
					UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
					if (UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty()) Record->FailureReason = TEXT("Test save slot remained after cleanup.");
					}
					TArray<UPackage*> WorldDirty;
					TArray<UPackage*> ContentDirty;
					FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
					FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
					if (WorldDirty.Num() + ContentDirty.Num() > 0)
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty()) Record->FailureReason = TEXT("A package was dirty after the test.");
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, EndSession, WaitBetween, EndPie, WaitStopped };
		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty()) Record.FailureReason = Reason;
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
				if (Record.FailureReason.IsEmpty()) Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
			}
			return bPassed;
		}
		void StopIfFailed() { if (bAnyAssertFailed) Stage = EStage::EndPie; }
		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUD ? HUD->GetBoundHUDWidget() : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Conclusion = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConclusionSoftPath);
			UProjectOrganoidObjectiveDataAsset* Handover = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, HandoverSoftPath);
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Conclusion && Conclusion->Tasks.Num() == 1 ? &Conclusion->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.conclusion_id"), Conclusion && Conclusion->MissionId == FName(ConclusionMissionId), ConclusionMissionId, Conclusion ? Conclusion->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.conclusion_title"), Conclusion && Conclusion->MissionTitle.ToString() == TEXT("The Conclusion"), TEXT("The Conclusion"), Conclusion ? Conclusion->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.conclusion_description"), Conclusion && Conclusion->MissionDescription.ToString().Contains(TEXT("incubator")), TEXT("incubator"), Conclusion ? Conclusion->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.conclusion_next_research_station"), Conclusion && Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() == ResearchStationSoftPath, ResearchStationSoftPath, Conclusion ? Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), Conclusion && Conclusion->Tasks.Num() == 1, TEXT("1"), Conclusion ? FString::FromInt(Conclusion->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(ConclusionObjectiveId), ConclusionObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Reach Control Spine"), TEXT("Reach Control Spine"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("control spine")), TEXT("control spine"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(ConclusionEvent), ConclusionEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.handover_next_conclusion"), Handover && Handover->NextMissionAsset.ToSoftObjectPath().ToString() == ConclusionSoftPath, ConclusionSoftPath, Handover ? Handover->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_next_handover"), Entry && Entry->NextMissionAsset.ToSoftObjectPath().ToString() == HandoverSoftPath, HandoverSoftPath, Entry ? Entry->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Found = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, TerminalLabel) : TArray<AActor*>();
			AProjectOrganoidTerminal* Terminal = Found.Num() == 1 ? Cast<AProjectOrganoidTerminal>(Found[0]) : nullptr;
			const FString PackageName = Terminal && Terminal->GetOutermost() ? Terminal->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("terminal.count"), Found.Num() == 1, TEXT("1"), FString::FromInt(Found.Num()), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.class"), Terminal != nullptr, TEXT("ProjectOrganoidTerminal"), Found.Num() == 1 && Found[0] ? Found[0]->GetClass()->GetName() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.package"), PackageName.Contains(TEXT("SL_Epitope_Reactor")), ReactorPackage, PackageName, TerminalLabel);
			AssertTrue(Record, TEXT("terminal.location"), Terminal && Terminal->GetActorLocation().Equals(TerminalLocation, 1.f), TEXT("-1950,-1650,-4700"), Terminal ? Terminal->GetActorLocation().ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.objective"), Terminal && Terminal->RequiredActiveObjectiveId == FName(ConclusionObjectiveId), ConclusionObjectiveId, Terminal ? Terminal->RequiredActiveObjectiveId.ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.event"), Terminal && Terminal->SuccessObjectiveEventId == FName(ConclusionEvent), ConclusionEvent, Terminal ? Terminal->SuccessObjectiveEventId.ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.prompt"), Terminal && Terminal->CampaignHackPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Terminal ? Terminal->CampaignHackPrompt.ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.speaker"), Terminal && Terminal->CompletionNotificationSpeaker.ToString() == TEXT("Nathan"), TEXT("Nathan"), Terminal ? Terminal->CompletionNotificationSpeaker.ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.line"), Terminal && Terminal->CompletionNotificationText.ToString() == ExpectedLine, ExpectedLine, Terminal ? Terminal->CompletionNotificationText.ToString() : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.duration"), Terminal && FMath::IsNearlyEqual(Terminal->CompletionNotificationDurationSeconds, 7.f), TEXT("7"), Terminal ? FString::SanitizeFloat(Terminal->CompletionNotificationDurationSeconds) : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.no_power_change"), Terminal && !Terminal->bApplyPowerChangeOnSuccess && Terminal->PowerSector == EProjectOrganoidPowerSector::Reactor, TEXT("false"), Terminal ? BoolText(Terminal->bApplyPowerChangeOnSuccess) : TEXT("missing"), TerminalLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.reactor_clean"), !PackageIsDirty(ReactorPackage), TEXT("clean"), PackageIsDirty(ReactorPackage) ? TEXT("dirty") : TEXT("clean"), ReactorPackage);
			if (bAnyAssertFailed || !Conclusion || !Handover || !Entry || !Terminal)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 18 mission contract missing.") : Record.FailureReason);
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
			bRequestedReactorStream = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedReactorStream)
			{
				const FName ReactorName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel5_Reactor);
				if (!ReactorName.IsNone())
				{
					Levels->AddStreamRequest(ReactorName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedReactorStream = true;
				}
			}
			if (Character && bRequestedReactorStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with Terminal_ControlSpine."));
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
			if (!Widget)
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds > 10.f) FailAndStop(Owner, Record, TEXT("HUD was not ready."));
				return;
			}
			if (bSecondSession)
			{
				if (!bReloadDone) TickReload(Owner, Record, World, Character, Widget);
				return;
			}
			if (!bCampaignDone)
			{
				bCampaignDone = true;
				TickCampaign(Owner, Record, World, Character, Widget);
			}
		}

		void TickCampaign(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel) : TArray<AActor*>();
			AProjectOrganoidTerminal* Terminal = Found.Num() == 1 ? Cast<AProjectOrganoidTerminal>(Found[0]) : nullptr;
			if (!World || !Character || !Objectives || !Saves || !Power || !Terminal || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 18 actors or subsystems missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Emergency);
			AssertTrue(Record, TEXT("power.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("terminal.unmoved"), Terminal->GetActorLocation().Equals(TerminalLocation, 5.f), TEXT("-1950,-1650,-4700"), Terminal->GetActorLocation().ToString(), TerminalLabel);
			AssertTrue(Record, TEXT("terminal.prompt_live"), Terminal->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Terminal->InteractionPrompt.ToString(), TerminalLabel);

			UProjectOrganoidObjectiveDataAsset* Handover = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, HandoverSoftPath);
			const bool bHandoverLoaded = Handover && Objectives->LoadMission(Handover, false);
			AssertTrue(Record, TEXT("reject.handover_loaded"), bHandoverLoaded && Objectives->GetActiveMissionId() == FName(HandoverMissionId), HandoverMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Objectives->TriggerEvent(FName(EntryEvent));
			AssertTrue(Record, TEXT("reject.still_handover"), Objectives->GetActiveMissionId() == FName(HandoverMissionId), HandoverMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.spine_not_active"), CountActiveId(Objectives, FName(ConclusionObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(ConclusionObjectiveId))), ConclusionObjectiveId);
			const bool bCanInteract = Terminal->CanInteract_Implementation(Character);
			AssertTrue(Record, TEXT("reject.interact"), !bCanInteract, TEXT("false"), BoolText(bCanInteract), TerminalLabel);
			AssertTrue(Record, TEXT("reject.no_credit"), !Terminal->bHasBeenHacked && Terminal->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Terminal->CompletionNotificationCount), TerminalLabel);
			AssertTrue(Record, TEXT("reject.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));

			int32 Hacked = 0;
			for (int32 HackIndex = 0; HackIndex < 3; ++HackIndex)
			{
				Hacked += Objectives->TriggerEvent(FName(HackEvent));
			}
			Objectives->TriggerEvent(FName(ConfessionEvent));
			AssertTrue(Record, TEXT("chain.hack_events"), Hacked > 0, TEXT(">0"), FString::FromInt(Hacked), HackEvent);
			AssertTrue(Record, TEXT("chain.conclusion_current"), Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.objective_active"), CountActiveId(Objectives, FName(ConclusionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(ConclusionObjectiveId))), ConclusionObjectiveId);
			AssertTrue(Record, TEXT("chain.hack_completed"), CountCompletedId(Objectives, FName(HackObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("chain.opening_not_complete"), !Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation"))), TEXT("false"), BoolText(Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation")))), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Terminal->ApplyHackRewards(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.hacked"), Terminal->bHasBeenHacked, TEXT("true"), BoolText(Terminal->bHasBeenHacked), TerminalLabel);
			AssertTrue(Record, TEXT("success.fire"), Terminal->CompletionNotificationCount == 1, TEXT("1"), FString::FromInt(Terminal->CompletionNotificationCount), TerminalLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConclusionObjectiveId))), ConclusionObjectiveId);
			AssertTrue(Record, TEXT("success.mission"), Objectives->GetActiveMissionId() == FName(ResearchStationMissionId) && CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, ResearchStationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.notification"), Terminal->CompletionNotificationCount == 1, TEXT("1"), FString::FromInt(Terminal->CompletionNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.no_door"), Found.Num() == 1 && Terminal->GetActorLocation().Equals(TerminalLocation, 5.f) && Terminal->LinkedDoorLock.IsNull() && Terminal->LinkedSecurityGate.IsNull(), TEXT("terminal"), Terminal->GetActorLocation().ToString(), TerminalLabel);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Terminal->ApplyHackRewards(Character);
			AssertTrue(Record, TEXT("replay.fire_once"), Terminal->bHasBeenHacked && Terminal->CompletionNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("hacked=%s notes=%d"), *BoolText(Terminal->bHasBeenHacked), Terminal->CompletionNotificationCount), TerminalLabel);
			AssertTrue(Record, TEXT("replay.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("replay.line_cleared"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("cleared"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->GetActiveMissionId() == FName(ResearchStationMissionId) && CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, ResearchStationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.reactor_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel) : TArray<AActor*>();
			AProjectOrganoidTerminal* Terminal = Found.Num() == 1 ? Cast<AProjectOrganoidTerminal>(Found[0]) : nullptr;
			if (!Objectives || !Saves || !Power || !Terminal || !Character || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Blackout);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(ResearchStationMissionId) && CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, ResearchStationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConclusionObjectiveId))), ConclusionObjectiveId);
			AssertTrue(Record, TEXT("save.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("save"));
			AssertTrue(Record, TEXT("save.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("save"));
			Terminal->ApplyHackRewards(Character);
			AssertTrue(Record, TEXT("save.replay_no_line"), Terminal->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Terminal->CompletionNotificationCount), TerminalLabel);
			AssertTrue(Record, TEXT("save.unmoved"), Terminal->GetActorLocation().Equals(TerminalLocation, 5.f), TEXT("-1950,-1650,-4700"), Terminal->GetActorLocation().ToString(), TerminalLabel);
			AssertTrue(Record, TEXT("dirty.reload_reactor"), !PackageIsDirty(ReactorPackage), TEXT("clean"), PackageIsDirty(ReactorPackage) ? TEXT("dirty") : TEXT("clean"), ReactorPackage);
			AssertTrue(Record, TEXT("dirty.reload_admin"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedReactorStream = false;
		bool bSecondSession = false;
		bool bCampaignDone = false;
		bool bReloadDone = false;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FTheConclusionFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterTheConclusionFunctional;
}
