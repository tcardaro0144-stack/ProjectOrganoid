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
 *  objectives, stats) when Nathan interacts or walks into the volume.
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidCheckpoint : public AProjectOrganoidInteractable
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

	/**
	 * Minimum health floor applied immediately before a checkpoint save is serialized.
	 * 0.25 = if current health is below 25% of MaxHealth, raise it to exactly 25%.
	 * Health already at or above the floor is unchanged. This is not +25%, not +25 HP,
	 * and not full healing. 0 disables the floor. Death/restart does not use this property.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Checkpoint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthStabilizationFloorPercent = 0.25f;

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
	void ApplyHealthStabilizationFloor(AProjectOrganoidCharacter* Character) const;
};
