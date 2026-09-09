// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidBiologicalAdaptationTypes.h"

bool UProjectOrganoidBiologicalAdaptationData::TryResolveTarget(
	AProjectOrganoidCharacter* /*Character*/,
	AActor*& OutTarget,
	EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const
{
	OutTarget = nullptr;
	if (bRequiresTarget)
	{
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
		return false;
	}
	FailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
	return true;
}

bool UProjectOrganoidBiologicalAdaptationData::ExecuteOnTarget(
	AProjectOrganoidCharacter* /*Character*/,
	AActor* /*Target*/) const
{
	return false;
}
