// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.generated.h"

/** One mission task row — objective definition plus the events that advance it */
USTRUCT(BlueprintType)
struct FProjectOrganoidMissionTaskDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Task")
	FProjectOrganoidObjective Objective;

	/**
	 *  Gameplay events that mutate this task (e.g. Event_SecurityGateOpened → Advance).
	 *  ObjectiveId on each trigger is filled from Objective.ObjectiveId when the mission loads
	 *  if left None.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Task")
	TArray<FProjectOrganoidObjectiveEventTrigger> EventTriggers;

	/** If true, activate this task when the mission is loaded */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Task")
	bool bAutoActivate = true;
};

/**
 *  Designer-authored mission / quest package for UProjectOrganoidObjectiveSubsystem.
 */
UCLASS(BlueprintType)
class UProjectOrganoidObjectiveDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FName MissionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FText MissionTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	FText MissionDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Tasks")
	TArray<FProjectOrganoidMissionTaskDefinition> Tasks;

	/** Loaded when every task on this mission completes. Journal history is kept. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Chain")
	TSoftObjectPtr<UProjectOrganoidObjectiveDataAsset> NextMissionAsset;
};
