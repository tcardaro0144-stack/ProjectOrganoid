// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidObjectiveSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidObjectiveChanged, const FProjectOrganoidObjective&, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidObjectivePopup, const FProjectOrganoidObjective&, Objective, FName, PopupReason);

/**
 *  Tracks main / side objectives and resolves gameplay event triggers.
 */
UCLASS()
class UProjectOrganoidObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnProjectOrganoidObjectiveChanged OnObjectiveActivated;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnProjectOrganoidObjectiveChanged OnObjectiveUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnProjectOrganoidObjectiveChanged OnObjectiveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnProjectOrganoidObjectiveChanged OnObjectiveFailed;

	/** Fired for HUD pop-up notifications (Activated / Updated / Completed / Failed) */
	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnProjectOrganoidObjectivePopup OnObjectivePopupRequested;

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool RegisterObjective(const FProjectOrganoidObjective& Objective);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool ActivateObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool AdvanceObjective(FName ObjectiveId, int32 ProgressDelta = 1);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool CompleteObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	bool FailObjective(FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void RegisterEventTrigger(const FProjectOrganoidObjectiveEventTrigger& Trigger);

	/** Fire a named gameplay event (door unlocked, host slain, pad read, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	int32 TriggerEvent(FName EventId);

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetObjective(FName ObjectiveId, FProjectOrganoidObjective& OutObjective) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetActiveObjectives() const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetObjectivesByType(EProjectOrganoidObjectiveType Type) const;

protected:

	UPROPERTY()
	TArray<FProjectOrganoidObjective> Objectives;

	UPROPERTY()
	TArray<FProjectOrganoidObjectiveEventTrigger> EventTriggers;

	int32 FindObjectiveIndex(FName ObjectiveId) const;
	void RequestPopup(const FProjectOrganoidObjective& Objective, FName Reason);
	void SeedDefaultCampaignObjectives();
};
