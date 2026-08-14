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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission|Task")
	FProjectOrganoidObjective Objective;

	/**
	 *  Gameplay events that mutate this task (e.g. Event_SecurityGateOpened → Advance).
	 *  ObjectiveId on each trigger is filled from Objective.ObjectiveId when the mission loads
	 *  if left None.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission|Task")
	TArray<FProjectOrganoidObjectiveEventTrigger> EventTriggers;

	/** If true, activate this task when the mission is loaded */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission|Task")
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FName MissionId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FText MissionTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission")
	FText MissionDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mission|Tasks")
	TArray<FProjectOrganoidMissionTaskDefinition> Tasks;
};
