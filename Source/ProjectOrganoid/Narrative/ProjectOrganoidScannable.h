// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidScannable.generated.h"

class AActor;

UINTERFACE(MinimalAPI, Blueprintable)
class UProjectOrganoidScannable : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Actors that Avery can photograph / scan for lore data points.
 */
class IProjectOrganoidScannable
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	bool CanBeScanned(AActor* Scanner) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	FProjectOrganoidLogEntry GetScanLoreEntry() const;

	/** Optional objective event fired on first successful extract (e.g. Event_DataPadRead) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	FName GetScanObjectiveEventId() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	FText GetScanDisplayName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	bool HasBeenScanned() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scan")
	void NotifyScanned(AActor* Scanner);
};
