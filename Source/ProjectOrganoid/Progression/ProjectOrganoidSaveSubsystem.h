// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidSaveSubsystem.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidSaveGame;

/**
 *  Save / load Avery's vitals, inventory grid, and keycard tiers.
 *  Also queues load-on-travel for main-menu slot selection.
 */
UCLASS()
class UProjectOrganoidSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString DefaultSaveSlot = TEXT("OrganoidSave0");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString SlotNamePrefix = TEXT("OrganoidSave");

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SavePlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadPlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName);

	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSaveExist(const FString& SlotName) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DeleteSave(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	UProjectOrganoidSaveGame* CaptureSaveFromCharacter(AProjectOrganoidCharacter* Character) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool ApplySaveToCharacter(UProjectOrganoidSaveGame* SaveGame, AProjectOrganoidCharacter* Character) const;

	UFUNCTION(BlueprintPure, Category = "Save")
	FString GetSlotNameForIndex(int32 SlotIndex) const;

	/** Summaries for main-menu load UI (empty slots still returned with bExists=false). */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FProjectOrganoidSaveSlotInfo> GetSaveSlotInfos(int32 NumSlots = 3) const;

	UFUNCTION(BlueprintCallable, Category = "Save")
	UProjectOrganoidSaveGame* LoadSaveGameObject(const FString& SlotName) const;

	/** Queue a slot to apply after the next level travel (main-menu Load Game). */
	UFUNCTION(BlueprintCallable, Category = "Save|Travel")
	void RequestLoadOnNextTravel(const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save|Travel")
	void ClearPendingLoad();

	UFUNCTION(BlueprintPure, Category = "Save|Travel")
	bool HasPendingLoad() const { return bHasPendingLoad; }

	UFUNCTION(BlueprintPure, Category = "Save|Travel")
	FString GetPendingLoadSlot() const { return PendingLoadSlotName; }

	/** Apply and clear pending load if one was queued. */
	UFUNCTION(BlueprintCallable, Category = "Save|Travel")
	bool TryApplyPendingLoad(AProjectOrganoidCharacter* Character);

protected:

	UPROPERTY()
	FString PendingLoadSlotName;

	UPROPERTY()
	bool bHasPendingLoad = false;
};
