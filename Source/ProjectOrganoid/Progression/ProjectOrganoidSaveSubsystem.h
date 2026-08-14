// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidSaveSubsystem.generated.h"

class AProjectOrganoidCharacter;
class AProjectOrganoidCheckpoint;
class UProjectOrganoidSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidGameSaved, const FString&, SlotName, EProjectOrganoidSaveReason, Reason, bool, bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidGameLoaded, const FString&, SlotName, bool, bSucceeded);

/**
 *  Full game-state serialization: vitals, inventory, weapon mods, objectives, stats.
 *  Checkpoints + objective autosaves write to AutosaveSlotName by default.
 */
UCLASS()
class UProjectOrganoidSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString DefaultSaveSlot = TEXT("OrganoidSave0");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString AutosaveSlotName = TEXT("OrganoidAutosave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FString SlotNamePrefix = TEXT("OrganoidSave");

	/** Autosave when a Main objective completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Autosave")
	bool bAutosaveOnMainObjectiveComplete = true;

	/** Autosave when any objective in AutosaveObjectiveIds completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Autosave")
	TArray<FName> AutosaveObjectiveIds;

	/** Also autosave when the active mission completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Autosave")
	bool bAutosaveOnMissionComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Load")
	bool bRestorePlayerTransformOnLoad = true;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnProjectOrganoidGameSaved OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnProjectOrganoidGameLoaded OnGameLoaded;

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SavePlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadPlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName);

	/** Full serialization with explicit reason + optional checkpoint metadata */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SerializeGameState(
		AProjectOrganoidCharacter* Character,
		const FString& SlotName,
		EProjectOrganoidSaveReason Reason,
		AProjectOrganoidCheckpoint* Checkpoint = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Save|Checkpoint")
	bool SaveAtCheckpoint(AProjectOrganoidCharacter* Character, AProjectOrganoidCheckpoint* Checkpoint, const FString& SlotName);

	UFUNCTION(BlueprintCallable, Category = "Save|Autosave")
	bool Autosave(AProjectOrganoidCharacter* Character, EProjectOrganoidSaveReason Reason = EProjectOrganoidSaveReason::ObjectiveAutosave);

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

	bool bBoundToObjectives = false;

	void BindObjectiveAutosave();
	void UnbindObjectiveAutosave();

	UFUNCTION()
	void HandleObjectiveCompleted(const FProjectOrganoidObjective& Objective);

	UFUNCTION()
	void HandleMissionCompleted(FName MissionId);

	AProjectOrganoidCharacter* ResolveLocalCharacter() const;
	void FillSaveMeta(
		UProjectOrganoidSaveGame* SaveGame,
		AProjectOrganoidCharacter* Character,
		EProjectOrganoidSaveReason Reason,
		AProjectOrganoidCheckpoint* Checkpoint) const;
};
