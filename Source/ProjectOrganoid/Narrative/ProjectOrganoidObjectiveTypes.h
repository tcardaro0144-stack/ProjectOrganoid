// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidObjectiveTypes.generated.h"

UENUM(BlueprintType)
enum class EProjectOrganoidObjectiveType : uint8
{
	Main UMETA(DisplayName = "Main Objective"),
	Side UMETA(DisplayName = "Side Objective")
};

UENUM(BlueprintType)
enum class EProjectOrganoidObjectiveState : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EProjectOrganoidObjectiveEventAction : uint8
{
	Activate UMETA(DisplayName = "Activate"),
	Advance UMETA(DisplayName = "Advance Progress"),
	Complete UMETA(DisplayName = "Complete"),
	Fail UMETA(DisplayName = "Fail")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EProjectOrganoidObjectiveType Type = EProjectOrganoidObjectiveType::Main;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EProjectOrganoidObjectiveState State = EProjectOrganoidObjectiveState::Inactive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "0"))
	int32 CurrentProgress = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "1"))
	int32 TargetProgress = 1;

	/** Multi-stage quest ordering (0 = first stage). Used by journal grouping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective|Journal", meta = (ClampMin = "0"))
	int32 StageIndex = 0;

	/** Must be Completed before this objective can activate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective|Journal")
	TArray<FName> PrerequisiteObjectiveIds;

	/** Extra journal flavor text shown in the quest log */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective|Journal")
	FText JournalNotes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective|Journal")
	bool bShowInJournal = true;

	/** When all prerequisites complete, auto-activate this task */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective|Journal")
	bool bAutoUnlockWhenPrerequisitesMet = true;

	bool IsComplete() const
	{
		return State == EProjectOrganoidObjectiveState::Completed
			|| (TargetProgress > 0 && CurrentProgress >= TargetProgress);
	}
};

/** Maps a gameplay event tag to an objective mutation */
USTRUCT(BlueprintType)
struct FProjectOrganoidObjectiveEventTrigger
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName EventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective")
	EProjectOrganoidObjectiveEventAction Action = EProjectOrganoidObjectiveEventAction::Advance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Objective", meta = (ClampMin = "1"))
	int32 ProgressDelta = 1;
};

USTRUCT(BlueprintType)
struct FProjectOrganoidLogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FName EntryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FText Author;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FName Category = TEXT("Facility");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	bool bIsRead = false;
};
