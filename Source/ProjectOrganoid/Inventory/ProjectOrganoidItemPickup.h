// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidItemPickup.generated.h"

class UStaticMeshComponent;
class UProjectOrganoidItemData;

/**
 *  World pickup that adds ItemData to Avery's grid inventory.
 */
UCLASS(Blueprintable)
class AProjectOrganoidItemPickup : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidItemPickup();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	TObjectPtr<UProjectOrganoidItemData> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup")
	bool bDestroyOnPickup = true;

	/** Fired after a successful add, in addition to Event_ItemPickedUp / Event_KeycardPickedUp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Objectives")
	FName PickupObjectiveEventId = NAME_None;

	virtual void BeginPlay() override;
	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Pickup")
	void BP_OnPickedUp(AProjectOrganoidCharacter* Interactor, UProjectOrganoidItemData* PickedItem, int32 PickedQuantity);

protected:

	void NotifyPickupEvents();
	void RefreshPrompt();
};
