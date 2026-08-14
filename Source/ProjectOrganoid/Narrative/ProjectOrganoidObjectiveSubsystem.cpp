// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidObjectiveSubsystem.h"

void UProjectOrganoidObjectiveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SeedDefaultCampaignObjectives();
}

void UProjectOrganoidObjectiveSubsystem::SeedDefaultCampaignObjectives()
{
	if (Objectives.Num() > 0)
	{
		return;
	}

	FProjectOrganoidObjective ReachSterling;
	ReachSterling.ObjectiveId = TEXT("Main_ReachSterlingTerminal");
	ReachSterling.Title = FText::FromString(TEXT("Locate Dr. Sterling's Terminal"));
	ReachSterling.Description = FText::FromString(TEXT("Find an operational Sterling upgrade terminal in Sub-Level 1 Administration."));
	ReachSterling.Type = EProjectOrganoidObjectiveType::Main;
	ReachSterling.TargetProgress = 1;
	RegisterObjective(ReachSterling);

	FProjectOrganoidObjective ClearHosts;
	ClearHosts.ObjectiveId = TEXT("Side_ClearNeuroHosts");
	ClearHosts.Title = FText::FromString(TEXT("Neutralize Mutated Hosts"));
	ClearHosts.Description = FText::FromString(TEXT("Eliminate organoid hosts in the Neuro-Genetics wing."));
	ClearHosts.Type = EProjectOrganoidObjectiveType::Side;
	ClearHosts.TargetProgress = 2;
	RegisterObjective(ClearHosts);

	FProjectOrganoidObjectiveEventTrigger UnlockAdmin;
	UnlockAdmin.EventId = TEXT("Event_SterlingTerminalUsed");
	UnlockAdmin.ObjectiveId = TEXT("Main_ReachSterlingTerminal");
	UnlockAdmin.Action = EProjectOrganoidObjectiveEventAction::Complete;
	RegisterEventTrigger(UnlockAdmin);

	FProjectOrganoidObjectiveEventTrigger HostDown;
	HostDown.EventId = TEXT("Event_HostNeutralized");
	HostDown.ObjectiveId = TEXT("Side_ClearNeuroHosts");
	HostDown.Action = EProjectOrganoidObjectiveEventAction::Advance;
	HostDown.ProgressDelta = 1;
	RegisterEventTrigger(HostDown);

	ActivateObjective(TEXT("Main_ReachSterlingTerminal"));
	ActivateObjective(TEXT("Side_ClearNeuroHosts"));
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

	Objective.State = EProjectOrganoidObjectiveState::Active;
	Objective.CurrentProgress = FMath::Clamp(Objective.CurrentProgress, 0, Objective.TargetProgress);
	OnObjectiveActivated.Broadcast(Objective);
	RequestPopup(Objective, TEXT("Activated"));
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
	TArray<FProjectOrganoidObjective> Result;
	for (const FProjectOrganoidObjective& Objective : Objectives)
	{
		if (Objective.State == EProjectOrganoidObjectiveState::Active)
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
