// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidStatsSubsystem.h"
#include "Kismet/GameplayStatics.h"

UProjectOrganoidLogComponent::UProjectOrganoidLogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

int32 UProjectOrganoidLogComponent::FindEntryIndex(FName EntryId) const
{
	for (int32 Index = 0; Index < CollectedEntries.Num(); ++Index)
	{
		if (CollectedEntries[Index].EntryId == EntryId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool UProjectOrganoidLogComponent::CollectLogEntry(const FProjectOrganoidLogEntry& Entry)
{
	if (Entry.EntryId.IsNone())
	{
		return false;
	}

	const int32 Existing = FindEntryIndex(Entry.EntryId);
	if (Existing != INDEX_NONE)
	{
		CollectedEntries[Existing] = Entry;
		OnLogEntryCollected.Broadcast(CollectedEntries[Existing]);
		return true;
	}

	CollectedEntries.Add(Entry);
	OnLogEntryCollected.Broadcast(CollectedEntries.Last());

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->RecordDataPadRead(1);
		}
	}

	return true;
}

bool UProjectOrganoidLogComponent::MarkEntryRead(FName EntryId)
{
	const int32 Index = FindEntryIndex(EntryId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	CollectedEntries[Index].bIsRead = true;
	OnLogEntryRead.Broadcast(CollectedEntries[Index]);
	return true;
}

bool UProjectOrganoidLogComponent::HasEntry(FName EntryId) const
{
	return FindEntryIndex(EntryId) != INDEX_NONE;
}

bool UProjectOrganoidLogComponent::GetEntry(FName EntryId, FProjectOrganoidLogEntry& OutEntry) const
{
	const int32 Index = FindEntryIndex(EntryId);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutEntry = CollectedEntries[Index];
	return true;
}

TArray<FProjectOrganoidLogEntry> UProjectOrganoidLogComponent::GetUnreadEntries() const
{
	TArray<FProjectOrganoidLogEntry> Result;
	for (const FProjectOrganoidLogEntry& Entry : CollectedEntries)
	{
		if (!Entry.bIsRead)
		{
			Result.Add(Entry);
		}
	}
	return Result;
}
