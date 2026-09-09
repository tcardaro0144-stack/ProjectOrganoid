// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidEncounterPresenceSubsystem.h"

void UProjectOrganoidEncounterPresenceSubsystem::Deinitialize()
{
	ClearAllEncounterSources();
	Super::Deinitialize();
}

bool UProjectOrganoidEncounterPresenceSubsystem::DoesCombatStateLockStations(EProjectOrganoidHostCombatState State)
{
	return State == EProjectOrganoidHostCombatState::Pursue
		|| State == EProjectOrganoidHostCombatState::Attack;
}

bool UProjectOrganoidEncounterPresenceSubsystem::IsEncounterActive() const
{
	return ActiveSourceIds.Num() > 0;
}

void UProjectOrganoidEncounterPresenceSubsystem::SetEncounterSourceActive(FName SourceId, bool bActive)
{
	if (SourceId.IsNone())
	{
		return;
	}

	if (bActive)
	{
		ActiveSourceIds.AddUnique(SourceId);
		return;
	}

	ActiveSourceIds.Remove(SourceId);
}

void UProjectOrganoidEncounterPresenceSubsystem::NotifySourceCombatState(FName SourceId, EProjectOrganoidHostCombatState State)
{
	SetEncounterSourceActive(SourceId, DoesCombatStateLockStations(State));
}

void UProjectOrganoidEncounterPresenceSubsystem::ClearAllEncounterSources()
{
	ActiveSourceIds.Reset();
}
