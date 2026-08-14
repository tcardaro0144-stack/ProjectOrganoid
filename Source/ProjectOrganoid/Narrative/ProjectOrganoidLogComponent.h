// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidLogComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidLogEntryCollected, const FProjectOrganoidLogEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidLogEntryRead, const FProjectOrganoidLogEntry&, Entry);

/**
 *  Avery's facility lore archive — collects data-pad entries.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidLogComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidLogComponent();

	UPROPERTY(BlueprintAssignable, Category = "Log")
	FOnProjectOrganoidLogEntryCollected OnLogEntryCollected;

	UPROPERTY(BlueprintAssignable, Category = "Log")
	FOnProjectOrganoidLogEntryRead OnLogEntryRead;

	/** Add or refresh a lore entry from a facility data pad */
	UFUNCTION(BlueprintCallable, Category = "Log")
	bool CollectLogEntry(const FProjectOrganoidLogEntry& Entry);

	UFUNCTION(BlueprintCallable, Category = "Log")
	bool MarkEntryRead(FName EntryId);

	UFUNCTION(BlueprintPure, Category = "Log")
	bool HasEntry(FName EntryId) const;

	UFUNCTION(BlueprintPure, Category = "Log")
	bool GetEntry(FName EntryId, FProjectOrganoidLogEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Log")
	TArray<FProjectOrganoidLogEntry> GetAllEntries() const { return CollectedEntries; }

	UFUNCTION(BlueprintPure, Category = "Log")
	TArray<FProjectOrganoidLogEntry> GetUnreadEntries() const;

	UFUNCTION(BlueprintPure, Category = "Log")
	int32 GetCollectedCount() const { return CollectedEntries.Num(); }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Log")
	TArray<FProjectOrganoidLogEntry> CollectedEntries;

	int32 FindEntryIndex(FName EntryId) const;
};
