// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidCheckpoint.generated.h"

class UStaticMeshComponent;
class AProjectOrganoidCharacter;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidCheckpointUsed, AProjectOrganoidCheckpoint*, Checkpoint, AProjectOrganoidCharacter*, Character, bool, bSaveSucceeded);

/**
 *  Facility checkpoint — serializes full game state (vitals, inventory, weapon mods,
 *  objectives, stats) when Avery interacts or walks into the volume.
 */
UCLASS(Blueprintable)
class AProjectOrganoidCheckpoint : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidCheckpoint();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;

	/** Optional walk-in trigger (in addition to interact) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> AutosaveVolume;

	/** Stable id written into the save for restore / UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	FName CheckpointId = TEXT("Checkpoint_Unnamed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	FText CheckpointDisplayName = FText::FromString(TEXT("Facility Checkpoint"));

	/** Empty = use SaveSubsystem AutosaveSlotName */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint|Save")
	FString SaveSlotOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint|Save")
	bool bSaveOnInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint|Save")
	bool bSaveOnOverlapEnter = false;

	/** Seconds before the same pawn can retrigger overlap autosave */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint|Save", meta = (ClampMin = "0.0"))
	float OverlapAutosaveCooldownSeconds = 30.0f;

	/** Heal Avery to MaxHealth when a successful checkpoint save completes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint")
	bool bRestoreHealthOnSave = true;

	UPROPERTY(BlueprintAssignable, Category = "Checkpoint")
	FOnProjectOrganoidCheckpointUsed OnCheckpointUsed;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	bool TriggerCheckpointSave(AProjectOrganoidCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Checkpoint")
	void BP_OnCheckpointSaved(AProjectOrganoidCharacter* Character, bool bSucceeded);

protected:

	float LastOverlapAutosaveTime = -1000.0f;

	UFUNCTION()
	void OnAutosaveVolumeBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	FString ResolveSaveSlot() const;
};
