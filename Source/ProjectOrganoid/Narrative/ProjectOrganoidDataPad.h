// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidDataPad.generated.h"

class UStaticMeshComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Log")
	bool bHasBeenRead = false;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void BP_OnDataPadRead(AProjectOrganoidCharacter* Interactor, const FProjectOrganoidLogEntry& Entry);
};
