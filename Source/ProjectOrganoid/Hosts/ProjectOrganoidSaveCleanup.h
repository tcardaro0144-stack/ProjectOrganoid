// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidSaveCleanup.generated.h"

/**
 * Conclusion save wipe. Deletes campaign test/autosave slots.
 * Destroy clears NG+. Extract keeps / grants NG+ for the harder loop.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidSaveCleanup : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidSaveCleanup();

	UFUNCTION(BlueprintCallable, Category = "Conclusion|Save")
	bool ApplyCleanup(EProjectOrganoidNodeZeroFate Fate);

	UFUNCTION(BlueprintPure, Category = "Conclusion|Save")
	int32 GetDeletedSlotCount() const { return DeletedSlotCount; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Save")
	EProjectOrganoidNodeZeroFate GetLastCleanupFate() const { return LastCleanupFate; }

	UFUNCTION(BlueprintPure, Category = "Conclusion|Save")
	bool WasNewGamePlusKept() const { return bKeptNewGamePlus; }

	bool UsesPursuerAI() const { return false; }

	static const TArray<FString>& GetCleanupSlotNames();

private:
	EProjectOrganoidNodeZeroFate LastCleanupFate = EProjectOrganoidNodeZeroFate::None;
	int32 DeletedSlotCount = 0;
	bool bKeptNewGamePlus = false;
};
