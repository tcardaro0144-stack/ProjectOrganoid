// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminFacilityStateSubsystem.h"

void UProjectOrganoidAdminFacilityStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentState = EProjectOrganoidAdminFacilityState::Normal;
}

void UProjectOrganoidAdminFacilityStateSubsystem::SetAdminFacilityState(EProjectOrganoidAdminFacilityState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	const EProjectOrganoidAdminFacilityState PreviousState = CurrentState;
	CurrentState = NewState;
	OnAdminFacilityStateChanged.Broadcast(NewState, PreviousState);
}
