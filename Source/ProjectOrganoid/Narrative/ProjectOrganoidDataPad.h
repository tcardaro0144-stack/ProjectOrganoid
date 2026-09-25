// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidDataPad.generated.h"

class UStaticMeshComponent;
class UProjectOrganoidObjectiveSubsystem;

/**
 *  Facility data pad — pushes lore into Avery's log and optionally fires objective events.
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidDataPad : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidDataPad();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FProjectOrganoidLogEntry LogEntry;

	/** Optional pad-specific objective event (e.g. Event_DataPad_AdminMemo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	FName ObjectiveEventId = NAME_None;

	/**
	 * Optional interaction gate. Unset (NAME_None) preserves legacy always-interactable pads.
	 * When set, interaction requires that objective to be Active or Completed (Failed/Inactive/missing fail closed).
	 * Read-only check — does not mutate objectives, events, logs, or power.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	FName RequiredObjectiveIdForInteraction = NAME_None;

	/** If true, also fires Event_DataPadRead on first pickup (advances mission tasks) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	bool bBroadcastGenericDataPadEvent = true;

	/**
	 * Optional line shown once, only when this read is the interact that completes
	 * RequiredObjectiveIdForInteraction. Empty text leaves every other pad silent.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	FText CompletionNotificationSpeaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	FText CompletionNotificationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives", meta = (ClampMin = "0.0"))
	float CompletionNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Log|Objectives", Transient)
	int32 CompletionNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Log")
	bool bHasBeenRead = false;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void BP_OnDataPadRead(AProjectOrganoidCharacter* Interactor, const FProjectOrganoidLogEntry& Entry);

private:
	bool IsRequiredObjectiveCompleted(const UProjectOrganoidObjectiveSubsystem* Objectives) const;
	void PresentCompletionNotification(AProjectOrganoidCharacter* Interactor);
};
