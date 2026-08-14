// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidSaveGame.h"

void UProjectOrganoidObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!LoadDefaultMission())
	{
		SeedDefaultCampaignObjectives();
	}
}

bool UProjectOrganoidObjectiveSubsystem::LoadDefaultMission()
{
	if (DefaultMissionAsset.IsNull())
	{
		return false;
	}

	UProjectOrganoidObjectiveDataAsset* Mission = DefaultMissionAsset.LoadSynchronous();
	return LoadMission(Mission, true);
}

bool UProjectOrganoidObjectiveSubsystem::LoadMission(UProjectOrganoidObjectiveDataAsset* MissionAsset, bool bClearExisting)
{
	if (!MissionAsset || MissionAsset->MissionId.IsNone())
	{
		return false;
	}

	if (bClearExisting)
	{
		Objectives.Reset();
		EventTriggers.Reset();
		ActiveMissionObjectiveIds.Reset();
		bActiveMissionCompletionNotified = false;
	}

	ActiveMissionId = MissionAsset->MissionId;
	ActiveMissionTitle = MissionAsset->MissionTitle.IsEmpty()
		? FText::FromName(MissionAsset->MissionId)
		: MissionAsset->MissionTitle;

	for (const FProjectOrganoidMissionTaskDefinition& Task : MissionAsset->Tasks)
	{
		if (Task.Objective.ObjectiveId.IsNone())
		{
			continue;
		}

		FProjectOrganoidObjective ObjectiveCopy = Task.Objective;
		if (ObjectiveCopy.State == EProjectOrganoidObjectiveState::Inactive)
		{
			ObjectiveCopy.CurrentProgress = FMath::Clamp(ObjectiveCopy.CurrentProgress, 0, ObjectiveCopy.TargetProgress);
		}

		RegisterObjective(ObjectiveCopy);
		ActiveMissionObjectiveIds.AddUnique(ObjectiveCopy.ObjectiveId);

		for (FProjectOrganoidObjectiveEventTrigger Trigger : Task.EventTriggers)
		{
			if (Trigger.ObjectiveId.IsNone())
			{
				Trigger.ObjectiveId = ObjectiveCopy.ObjectiveId;
			}
			RegisterEventTrigger(Trigger);
		}

		if (Task.bAutoActivate)
		{
			ActivateObjective(ObjectiveCopy.ObjectiveId);
		}
	}

	OnMissionLoaded.Broadcast(ActiveMissionId, ActiveMissionTitle);
	BroadcastJournalState();
	return true;
}

void UProjectOrganoidObjectiveSubsystem::SeedDefaultCampaignObjectives()
{
	if (Objectives.Num() > 0)
	{
		return;
	}

	ActiveMissionId = TEXT("Mission_EpitopeLockdown");
	ActiveMissionTitle = FText::FromString(TEXT("Epitope Lockdown"));

	// --- Main: override a sealed security gate ---
	FProjectOrganoidObjective OverrideGate;
	OverrideGate.ObjectiveId = TEXT("Main_OverrideSecurityGate");
	OverrideGate.Title = FText::FromString(TEXT("Override Security Gate"));
	OverrideGate.Description = FText::FromString(TEXT("Use a keycard or hacking tool to open a sealed facility gate."));
	OverrideGate.Type = EProjectOrganoidObjectiveType::Main;
	OverrideGate.TargetProgress = 1;
	OverrideGate.StageIndex = 0;
	OverrideGate.JournalNotes = FText::FromString(TEXT("Admin decon — HEPA seals still engaged."));
	RegisterObjective(OverrideGate);
	ActiveMissionObjectiveIds.Add(OverrideGate.ObjectiveId);

	FProjectOrganoidObjectiveEventTrigger GateOpened;
	GateOpened.EventId = TEXT("Event_SecurityGateOpened");
	GateOpened.ObjectiveId = TEXT("Main_OverrideSecurityGate");
	GateOpened.Action = EProjectOrganoidObjectiveEventAction::Complete;
	RegisterEventTrigger(GateOpened);

	// --- Main: recover facility intel from data pads ---
	FProjectOrganoidObjective ReadPads;
	ReadPads.ObjectiveId = TEXT("Main_ReadFacilityDataPads");
	ReadPads.Title = FText::FromString(TEXT("Recover Facility Logs"));
	ReadPads.Description = FText::FromString(TEXT("Read data pads scattered through the Epitope complex."));
	ReadPads.Type = EProjectOrganoidObjectiveType::Main;
	ReadPads.TargetProgress = 2;
	ReadPads.StageIndex = 0;
	ReadPads.JournalNotes = FText::FromString(TEXT("Scan or collect pads — photography mode extracts lore."));
	RegisterObjective(ReadPads);
	ActiveMissionObjectiveIds.Add(ReadPads.ObjectiveId);

	FProjectOrganoidObjectiveEventTrigger PadRead;
	PadRead.EventId = TEXT("Event_DataPadRead");
	PadRead.ObjectiveId = TEXT("Main_ReadFacilityDataPads");
	PadRead.Action = EProjectOrganoidObjectiveEventAction::Advance;
	PadRead.ProgressDelta = 1;
	RegisterEventTrigger(PadRead);

	// --- Stage 1: Sterling terminal (gated behind security override) ---
	FProjectOrganoidObjective ReachSterling;
	ReachSterling.ObjectiveId = TEXT("Main_ReachSterlingTerminal");
	ReachSterling.Title = FText::FromString(TEXT("Locate Dr. Sterling's Terminal"));
	ReachSterling.Description = FText::FromString(TEXT("Find an operational Sterling upgrade terminal in Sub-Level 1 Administration."));
	ReachSterling.Type = EProjectOrganoidObjectiveType::Main;
	ReachSterling.TargetProgress = 1;
	ReachSterling.StageIndex = 1;
	ReachSterling.PrerequisiteObjectiveIds.Add(TEXT("Main_OverrideSecurityGate"));
	ReachSterling.bAutoUnlockWhenPrerequisitesMet = true;
	ReachSterling.JournalNotes = FText::FromString(TEXT("Unlocks once the Admin gate is overridden."));
	RegisterObjective(ReachSterling);
	ActiveMissionObjectiveIds.Add(ReachSterling.ObjectiveId);

	FProjectOrganoidObjectiveEventTrigger UnlockAdmin;
	UnlockAdmin.EventId = TEXT("Event_SterlingTerminalUsed");
	UnlockAdmin.ObjectiveId = TEXT("Main_ReachSterlingTerminal");
	UnlockAdmin.Action = EProjectOrganoidObjectiveEventAction::Complete;
	RegisterEventTrigger(UnlockAdmin);

	// --- Side: clear hosts ---
	FProjectOrganoidObjective ClearHosts;
	ClearHosts.ObjectiveId = TEXT("Side_ClearNeuroHosts");
	ClearHosts.Title = FText::FromString(TEXT("Neutralize Mutated Hosts"));
	ClearHosts.Description = FText::FromString(TEXT("Eliminate organoid hosts in the Neuro-Genetics wing."));
	ClearHosts.Type = EProjectOrganoidObjectiveType::Side;
	ClearHosts.TargetProgress = 2;
	ClearHosts.StageIndex = 0;
	RegisterObjective(ClearHosts);
	ActiveMissionObjectiveIds.Add(ClearHosts.ObjectiveId);

	FProjectOrganoidObjectiveEventTrigger HostDown;
	HostDown.EventId = TEXT("Event_HostNeutralized");
	HostDown.ObjectiveId = TEXT("Side_ClearNeuroHosts");
	HostDown.Action = EProjectOrganoidObjectiveEventAction::Advance;
	HostDown.ProgressDelta = 1;
	RegisterEventTrigger(HostDown);

	ActivateObjective(TEXT("Main_OverrideSecurityGate"));
	ActivateObjective(TEXT("Main_ReadFacilityDataPads"));
	ActivateObjective(TEXT("Side_ClearNeuroHosts"));
	// Sterling waits for gate prereq — TryUnlockDependentObjectives handles it later

	OnMissionLoaded.Broadcast(ActiveMissionId, ActiveMissionTitle);
	BroadcastJournalState();
}

int32 UProjectOrganoidObjectiveSubsystem::FindObjectiveIndex(FName ObjectiveId) const
{
	for (int32 Index = 0; Index < Objectives.Num(); ++Index)
	{
		if (Objectives[Index].ObjectiveId == ObjectiveId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UProjectOrganoidObjectiveSubsystem::RequestPopup(const FProjectOrganoidObjective& Objective, FName Reason)
{
	OnObjectivePopupRequested.Broadcast(Objective, Reason);
}

bool UProjectOrganoidObjectiveSubsystem::RegisterObjective(const FProjectOrganoidObjective& Objective)
{
	if (Objective.ObjectiveId.IsNone())
	{
		return false;
	}

	const int32 Existing = FindObjectiveIndex(Objective.ObjectiveId);
	if (Existing != INDEX_NONE)
	{
		Objectives[Existing] = Objective;
		return true;
	}

	Objectives.Add(Objective);
	return true;
}

bool UProjectOrganoidObjectiveSubsystem::ActivateObjective(FName ObjectiveId)
{
	const int32 Index = FindObjectiveIndex(ObjectiveId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	FProjectOrganoidObjective& Objective = Objectives[Index];
	if (Objective.State == EProjectOrganoidObjectiveState::Active
		|| Objective.State == EProjectOrganoidObjectiveState::Completed)
	{
		return false;
	}

	if (!ArePrerequisitesMetForObjective(Objective))
	{
		return false;
	}

	Objective.State = EProjectOrganoidObjectiveState::Active;
	Objective.CurrentProgress = FMath::Clamp(Objective.CurrentProgress, 0, Objective.TargetProgress);
	OnObjectiveActivated.Broadcast(Objective);
	RequestPopup(Objective, TEXT("Activated"));
	BroadcastJournalState();
	return true;
}

bool UProjectOrganoidObjectiveSubsystem::AdvanceObjective(FName ObjectiveId, int32 ProgressDelta)
{
	const int32 Index = FindObjectiveIndex(ObjectiveId);
	if (Index == INDEX_NONE || ProgressDelta == 0)
	{
		return false;
	}

	FProjectOrganoidObjective& Objective = Objectives[Index];
	if (Objective.State != EProjectOrganoidObjectiveState::Active)
	{
		return false;
	}

	Objective.CurrentProgress = FMath::Clamp(Objective.CurrentProgress + ProgressDelta, 0, Objective.TargetProgress);
	OnObjectiveUpdated.Broadcast(Objective);
	RequestPopup(Objective, TEXT("Updated"));
	BroadcastJournalState();

	if (Objective.CurrentProgress >= Objective.TargetProgress)
	{
		return CompleteObjective(ObjectiveId);
	}

	return true;
}

bool UProjectOrganoidObjectiveSubsystem::CompleteObjective(FName ObjectiveId)
{
	const int32 Index = FindObjectiveIndex(ObjectiveId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	FProjectOrganoidObjective& Objective = Objectives[Index];
	if (Objective.State == EProjectOrganoidObjectiveState::Completed)
	{
		return false;
	}

	Objective.State = EProjectOrganoidObjectiveState::Completed;
	Objective.CurrentProgress = Objective.TargetProgress;
	OnObjectiveCompleted.Broadcast(Objective);
	RequestPopup(Objective, TEXT("Completed"));
	TryUnlockDependentObjectives(ObjectiveId);
	BroadcastJournalState();
	EvaluateActiveMissionCompletion();
	return true;
}

bool UProjectOrganoidObjectiveSubsystem::FailObjective(FName ObjectiveId)
{
	const int32 Index = FindObjectiveIndex(ObjectiveId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	FProjectOrganoidObjective& Objective = Objectives[Index];
	if (Objective.State == EProjectOrganoidObjectiveState::Failed
		|| Objective.State == EProjectOrganoidObjectiveState::Completed)
	{
		return false;
	}

	Objective.State = EProjectOrganoidObjectiveState::Failed;
	OnObjectiveFailed.Broadcast(Objective);
	RequestPopup(Objective, TEXT("Failed"));
	BroadcastJournalState();
	return true;
}

void UProjectOrganoidObjectiveSubsystem::RegisterEventTrigger(const FProjectOrganoidObjectiveEventTrigger& Trigger)
{
	if (Trigger.EventId.IsNone() || Trigger.ObjectiveId.IsNone())
	{
		return;
	}
	EventTriggers.Add(Trigger);
}

int32 UProjectOrganoidObjectiveSubsystem::TriggerEvent(FName EventId)
{
	if (EventId.IsNone())
	{
		return 0;
	}

	int32 Handled = 0;
	for (const FProjectOrganoidObjectiveEventTrigger& Trigger : EventTriggers)
	{
		if (Trigger.EventId != EventId)
		{
			continue;
		}

		switch (Trigger.Action)
		{
		case EProjectOrganoidObjectiveEventAction::Activate:
			if (ActivateObjective(Trigger.ObjectiveId))
			{
				++Handled;
			}
			break;
		case EProjectOrganoidObjectiveEventAction::Advance:
			if (AdvanceObjective(Trigger.ObjectiveId, Trigger.ProgressDelta))
			{
				++Handled;
			}
			break;
		case EProjectOrganoidObjectiveEventAction::Complete:
			if (CompleteObjective(Trigger.ObjectiveId))
			{
				++Handled;
			}
			break;
		case EProjectOrganoidObjectiveEventAction::Fail:
			if (FailObjective(Trigger.ObjectiveId))
			{
				++Handled;
			}
			break;
		default:
			break;
		}
	}

	return Handled;
}

void UProjectOrganoidObjectiveSubsystem::EvaluateActiveMissionCompletion()
{
	if (ActiveMissionId.IsNone() || ActiveMissionObjectiveIds.Num() == 0 || bActiveMissionCompletionNotified)
	{
		return;
	}

	if (IsMissionComplete(ActiveMissionId))
	{
		bActiveMissionCompletionNotified = true;
		OnMissionCompleted.Broadcast(ActiveMissionId);
	}
}

bool UProjectOrganoidObjectiveSubsystem::IsMissionComplete(FName MissionId) const
{
	if (MissionId.IsNone() || MissionId != ActiveMissionId || ActiveMissionObjectiveIds.Num() == 0)
	{
		return false;
	}

	for (const FName& ObjectiveId : ActiveMissionObjectiveIds)
	{
		const int32 Index = FindObjectiveIndex(ObjectiveId);
		if (Index == INDEX_NONE || Objectives[Index].State != EProjectOrganoidObjectiveState::Completed)
		{
			return false;
		}
	}

	return true;
}

bool UProjectOrganoidObjectiveSubsystem::GetObjective(FName ObjectiveId, FProjectOrganoidObjective& OutObjective) const
{
	const int32 Index = FindObjectiveIndex(ObjectiveId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutObjective = Objectives[Index];
	return true;
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetActiveObjectives() const
{
	return GetObjectivesByState(EProjectOrganoidObjectiveState::Active);
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetCompletedObjectives() const
{
	return GetObjectivesByState(EProjectOrganoidObjectiveState::Completed);
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetObjectivesByState(EProjectOrganoidObjectiveState State) const
{
	TArray<FProjectOrganoidObjective> Result;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State == State)
		{
			Result.Add(Objective);
		}
	}
	return Result;
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetObjectivesByType(EProjectOrganoidObjectiveType Type) const
{
	TArray<FProjectOrganoidObjective> Result;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.Type == Type)
		{
			Result.Add(Objective);
		}
	}
	return Result;
}

void UProjectOrganoidObjectiveSubsystem::CaptureObjectivesToSaveGame(UProjectOrganoidSaveGame* SaveGame) const
{
	if (!SaveGame)
	{
		return;
	}

	SaveGame->ActiveMissionId = ActiveMissionId;
	SaveGame->ActiveMissionTitle = ActiveMissionTitle;
	SaveGame->ActiveMissionObjectiveIds = ActiveMissionObjectiveIds;
	SaveGame->Objectives = Objectives;
	SaveGame->ObjectiveEventTriggers = EventTriggers;

	SaveGame->CompletedObjectiveIds.Reset();
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State == EProjectOrganoidObjectiveState::Completed)
		{
			SaveGame->CompletedObjectiveIds.Add(Objective.ObjectiveId);
		}
	}
}

void UProjectOrganoidObjectiveSubsystem::ApplyObjectivesFromSaveGame(const UProjectOrganoidSaveGame* SaveGame)
{
	if (!SaveGame)
	{
		return;
	}

	ActiveMissionId = SaveGame->ActiveMissionId;
	ActiveMissionTitle = SaveGame->ActiveMissionTitle;
	ActiveMissionObjectiveIds = SaveGame->ActiveMissionObjectiveIds;
	Objectives = SaveGame->Objectives;
	EventTriggers = SaveGame->ObjectiveEventTriggers;
	bActiveMissionCompletionNotified = IsMissionComplete(ActiveMissionId);

	// Older saves may only have CompletedObjectiveIds — merge into board if needed.
	if (Objectives.Num() == 0 && SaveGame->CompletedObjectiveIds.Num() > 0)
	{
		for (const FName& CompletedId : SaveGame->CompletedObjectiveIds)
		{
			FProjectOrganoidObjective Stub;
			Stub.ObjectiveId = CompletedId;
			Stub.State = EProjectOrganoidObjectiveState::Completed;
			Stub.CurrentProgress = 1;
			Stub.TargetProgress = 1;
			Objectives.Add(Stub);
		}
	}

	BroadcastJournalState();
}

bool UProjectOrganoidObjectiveSubsystem::ArePrerequisitesMetForObjective(const FProjectOrganoidObjective& Objective) const
{
	for (const FName& PrereqId : Objective.PrerequisiteObjectiveIds)
	{
		if (PrereqId.IsNone())
		{
			continue;
		}

		FProjectOrganoidObjective Prereq;
		if (!GetObjective(PrereqId, Prereq) || Prereq.State != EProjectOrganoidObjectiveState::Completed)
		{
			return false;
		}
	}
	return true;
}

bool UProjectOrganoidObjectiveSubsystem::ArePrerequisitesMet(FName ObjectiveId) const
{
	FProjectOrganoidObjective Objective;
	if (!GetObjective(ObjectiveId, Objective))
	{
		return false;
	}
	return ArePrerequisitesMetForObjective(Objective);
}

void UProjectOrganoidObjectiveSubsystem::TryUnlockDependentObjectives(FName CompletedObjectiveId)
{
	if (CompletedObjectiveId.IsNone())
	{
		return;
	}

	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State != EProjectOrganoidObjectiveState::Inactive
			|| !Objective.bAutoUnlockWhenPrerequisitesMet
			|| !Objective.PrerequisiteObjectiveIds.Contains(CompletedObjectiveId))
		{
			continue;
		}

		if (ArePrerequisitesMetForObjective(Objective))
		{
			ActivateObjective(Objective.ObjectiveId);
		}
	}
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetJournalEntries() const
{
	TArray<FProjectOrganoidObjective> Entries;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (!Objective.bShowInJournal)
		{
			continue;
		}

		if (Objective.State == EProjectOrganoidObjectiveState::Active
			|| Objective.State == EProjectOrganoidObjectiveState::Completed
			|| Objective.State == EProjectOrganoidObjectiveState::Failed)
		{
			Entries.Add(Objective);
			continue;
		}

		if (Objective.State == EProjectOrganoidObjectiveState::Inactive
			&& ArePrerequisitesMetForObjective(Objective))
		{
			Entries.Add(Objective);
		}
	}

	Entries.Sort([](const FProjectOrganoidObjective& A, const FProjectOrganoidObjective& B)
	{
		if (A.StageIndex != B.StageIndex)
		{
			return A.StageIndex < B.StageIndex;
		}
		return A.ObjectiveId.LexicalLess(B.ObjectiveId);
	});

	return Entries;
}

TArray<FProjectOrganoidObjective> UProjectOrganoidObjectiveSubsystem::GetObjectivesForStage(int32 StageIndex) const
{
	TArray<FProjectOrganoidObjective> StageEntries;
	for (const FProjectOrganoidObjective& Entry : GetJournalEntries())
	{
		if (Entry.StageIndex == StageIndex)
		{
			StageEntries.Add(Entry);
		}
	}
	return StageEntries;
}

int32 UProjectOrganoidObjectiveSubsystem::GetCurrentJournalStage() const
{
	int32 LowestActive = TNumericLimits<int32>::Max();
	bool bFoundActive = false;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State == EProjectOrganoidObjectiveState::Active)
		{
			LowestActive = FMath::Min(LowestActive, Objective.StageIndex);
			bFoundActive = true;
		}
	}

	if (bFoundActive)
	{
		return LowestActive;
	}

	int32 HighestCompleted = 0;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State == EProjectOrganoidObjectiveState::Completed)
		{
			HighestCompleted = FMath::Max(HighestCompleted, Objective.StageIndex);
		}
	}
	return HighestCompleted;
}

void UProjectOrganoidObjectiveSubsystem::NotifyJournalUpdated()
{
	BroadcastJournalState();
}

void UProjectOrganoidObjectiveSubsystem::BroadcastJournalState()
{
	OnJournalUpdated.Broadcast();

	const int32 Stage = GetCurrentJournalStage();
	CachedJournalStage = Stage;
	OnJournalStageChanged.Broadcast(Stage, GetObjectivesForStage(Stage));
}
