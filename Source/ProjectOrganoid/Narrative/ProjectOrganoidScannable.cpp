// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidScannable.h"

bool IProjectOrganoidScannable::CanBeScanned_Implementation(AActor* /*Scanner*/) const
{
	return true;
}

FProjectOrganoidLogEntry IProjectOrganoidScannable::GetScanLoreEntry_Implementation() const
{
	return FProjectOrganoidLogEntry();
}

FName IProjectOrganoidScannable::GetScanObjectiveEventId_Implementation() const
{
	return NAME_None;
}

FText IProjectOrganoidScannable::GetScanDisplayName_Implementation() const
{
	return FText::FromString(TEXT("Unknown Specimen"));
}

bool IProjectOrganoidScannable::HasBeenScanned_Implementation() const
{
	return false;
}

void IProjectOrganoidScannable::NotifyScanned_Implementation(AActor* /*Scanner*/)
{
}
