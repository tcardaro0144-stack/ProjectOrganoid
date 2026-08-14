// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSecuritySubsystem.h"
#include "ProjectOrganoidSecurityGate.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UProjectOrganoidSecuritySubsystem::Deinitialize()
{
	RegisteredGates.Reset();
	Super::Deinitialize();
}

void UProjectOrganoidSecuritySubsystem::PruneInvalidGates()
{
	RegisteredGates.RemoveAll([](const TObjectPtr<AProjectOrganoidSecurityGate>& Gate)
	{
		return !IsValid(Gate);
	});
}

void UProjectOrganoidSecuritySubsystem::RegisterSecurityGate(AProjectOrganoidSecurityGate* Gate)
{
	if (!Gate)
	{
		return;
	}

	PruneInvalidGates();
	RegisteredGates.AddUnique(Gate);

	if (bFacilityLockdownActive && Gate->bResealOnFacilityLockdown)
	{
		if (Gate->LockdownGroupId == ActiveLockdownId || ActiveLockdownId == TEXT("FacilityWide") || Gate->LockdownGroupId == TEXT("FacilityWide"))
		{
			Gate->ApplyFacilityLockdown();
		}
	}
}

void UProjectOrganoidSecuritySubsystem::UnregisterSecurityGate(AProjectOrganoidSecurityGate* Gate)
{
	RegisteredGates.Remove(Gate);
}

void UProjectOrganoidSecuritySubsystem::EngageFacilityLockdown(FName LockdownId)
{
	ActiveLockdownId = LockdownId.IsNone() ? FName(TEXT("FacilityWide")) : LockdownId;
	bFacilityLockdownActive = true;

	PruneInvalidGates();
	for (AProjectOrganoidSecurityGate* Gate : RegisteredGates)
	{
		if (!IsValid(Gate) || !Gate->bResealOnFacilityLockdown)
		{
			continue;
		}

		const bool bMatchesGroup =
			Gate->LockdownGroupId == ActiveLockdownId
			|| ActiveLockdownId == TEXT("FacilityWide")
			|| Gate->LockdownGroupId == TEXT("FacilityWide");

		if (bMatchesGroup)
		{
			Gate->ApplyFacilityLockdown();
		}
	}

	OnFacilityLockdownChanged.Broadcast(true, ActiveLockdownId);

	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(TEXT("Event_FacilityLockdownEngaged"));
		}
	}
}

void UProjectOrganoidSecuritySubsystem::LiftFacilityLockdown(bool bUnlockAllGates)
{
	const FName PreviousId = ActiveLockdownId;
	bFacilityLockdownActive = false;
	ActiveLockdownId = NAME_None;

	PruneInvalidGates();
	for (AProjectOrganoidSecurityGate* Gate : RegisteredGates)
	{
		if (!IsValid(Gate))
		{
			continue;
		}

		const bool bMatchesGroup =
			Gate->LockdownGroupId == PreviousId
			|| PreviousId == TEXT("FacilityWide")
			|| Gate->LockdownGroupId == TEXT("FacilityWide")
			|| PreviousId.IsNone();

		if (bMatchesGroup)
		{
			Gate->ClearFacilityLockdownSeal(bUnlockAllGates);
		}
	}

	OnFacilityLockdownChanged.Broadcast(false, PreviousId);

	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(TEXT("Event_FacilityLockdownLifted"));
		}
	}
}

bool UProjectOrganoidSecuritySubsystem::OverrideGateById(FName GateId, EProjectOrganoidSecurityOverrideMethod Method)
{
	return OverrideGate(FindGateById(GateId), Method);
}

bool UProjectOrganoidSecuritySubsystem::OverrideGate(AProjectOrganoidSecurityGate* Gate, EProjectOrganoidSecurityOverrideMethod Method)
{
	if (!IsValid(Gate))
	{
		return false;
	}

	Gate->SetGateState(EProjectOrganoidSecurityGateState::Open, Method);
	return true;
}

AProjectOrganoidSecurityGate* UProjectOrganoidSecuritySubsystem::FindGateById(FName GateId) const
{
	for (AProjectOrganoidSecurityGate* Gate : RegisteredGates)
	{
		if (IsValid(Gate) && Gate->GateId == GateId)
		{
			return Gate;
		}
	}
	return nullptr;
}

TArray<AProjectOrganoidSecurityGate*> UProjectOrganoidSecuritySubsystem::GetRegisteredGates() const
{
	TArray<AProjectOrganoidSecurityGate*> Result;
	for (AProjectOrganoidSecurityGate* Gate : RegisteredGates)
	{
		if (IsValid(Gate))
		{
			Result.Add(Gate);
		}
	}
	return Result;
}

void UProjectOrganoidSecuritySubsystem::NotifyGateOverrideAttempt(AProjectOrganoidSecurityGate* Gate, AProjectOrganoidCharacter* Character, bool bSucceeded)
{
	OnSecurityGateOverrideAttempt.Broadcast(Gate, Character, bSucceeded);
}

void UProjectOrganoidSecuritySubsystem::NotifyGateStateChanged(
	AProjectOrganoidSecurityGate* Gate,
	EProjectOrganoidSecurityGateState NewState,
	EProjectOrganoidSecurityOverrideMethod Method)
{
	OnSecurityGateStateChanged.Broadcast(Gate, NewState, Method);
}
