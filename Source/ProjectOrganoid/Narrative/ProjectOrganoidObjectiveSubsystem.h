// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidObjectiveSubsystem.generated.h"

class UProjectOrganoidObjectiveDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidObjectiveChanged, const FProjectOrganoidObjective&, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidObjectivePopup, const FProjectOrganoidObjective&, Objective, FName, PopupReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidMissionLoaded, FName, MissionId, const FText&, MissionTitle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidMissionCompleted, FName, MissionId);

/**
 *  Mission / objective tracker — active & completed quest state, DataAsset missions,
 *  and gameplay-event driven task advancement (gates, data pads, hosts, etc.).
 */
UCLASS()
class UProjectOrganoidObjectiveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Optional designer mission loaded on startup (falls back to seeded campaign if empty) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objectives|Mission")
	TSoftObjectPtr<UProjectOrganoidObjectiveDataAsset> DefaultMissionAsset;

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

	UPROPERTY(BlueprintAssignable, Category = "Objectives|Mission")
	FOnProjectOrganoidMissionLoaded OnMissionLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Objectives|Mission")
	FOnProjectOrganoidMissionCompleted OnMissionCompleted;

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

	/** Fire a named gameplay event (gate opened, data pad read, host slain, etc.) */
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	int32 TriggerEvent(FName EventId);

	/** Register all tasks + event triggers from a mission DataAsset */
	UFUNCTION(BlueprintCallable, Category = "Objectives|Mission")
	bool LoadMission(UProjectOrganoidObjectiveDataAsset* MissionAsset, bool bClearExisting = false);

	UFUNCTION(BlueprintCallable, Category = "Objectives|Mission")
	bool LoadDefaultMission();

	UFUNCTION(BlueprintPure, Category = "Objectives")
	bool GetObjective(FName ObjectiveId, FProjectOrganoidObjective& OutObjective) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetActiveObjectives() const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetCompletedObjectives() const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetObjectivesByType(EProjectOrganoidObjectiveType Type) const;

	UFUNCTION(BlueprintPure, Category = "Objectives")
	TArray<FProjectOrganoidObjective> GetObjectivesByState(EProjectOrganoidObjectiveState State) const;

	UFUNCTION(BlueprintPure, Category = "Objectives|Mission")
	FName GetActiveMissionId() const { return ActiveMissionId; }

	UFUNCTION(BlueprintPure, Category = "Objectives|Mission")
	FText GetActiveMissionTitle() const { return ActiveMissionTitle; }

	UFUNCTION(BlueprintPure, Category = "Objectives|Mission")
	bool IsMissionComplete(FName MissionId) const;

protected:

	UPROPERTY()
	TArray<FProjectOrganoidObjective> Objectives;

	UPROPERTY()
	TArray<FProjectOrganoidObjectiveEventTrigger> EventTriggers;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives|Mission")
	FName ActiveMissionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Objectives|Mission")
	FText ActiveMissionTitle;

	/** Objective ids that belong to the active mission (for mission-complete checks) */
	UPROPERTY()
	TArray<FName> ActiveMissionObjectiveIds;

	int32 FindObjectiveIndex(FName ObjectiveId) const;
	void RequestPopup(const FProjectOrganoidObjective& Objective, FName Reason);
	void SeedDefaultCampaignObjectives();
	void EvaluateActiveMissionCompletion();
};
