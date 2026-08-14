// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidSaveSubsystem.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidSaveGame;

/**
 *  Save / load Avery's vitals, inventory grid, and keycard tiers.
 */
UCLASS()
class UProjectOrganoidSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString DefaultSaveSlot = TEXT("OrganoidSave0");

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
};
