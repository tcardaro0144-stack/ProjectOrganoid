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
class AProjectOrganoidDataPad : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidDataPad();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log")
	FProjectOrganoidLogEntry LogEntry;

	/** Optional objective event fired on first successful read (e.g. Event_DataPad_AdminMemo) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Log|Objectives")
	FName ObjectiveEventId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Log")
	bool bHasBeenRead = false;

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Log")
	void BP_OnDataPadRead(AProjectOrganoidCharacter* Interactor, const FProjectOrganoidLogEntry& Entry);
};
