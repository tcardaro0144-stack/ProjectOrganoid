// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidInventoryTypes.generated.h"

/**
 *  A placed item occupying one or more cells in the inventory grid.
 *  Origin is the top-left slot (column X, row Y). StackCount supports ammo / SOT stacks.
 */
USTRUCT(BlueprintType)
struct FProjectOrganoidPlacedItem
{
	GENERATED_BODY()

	/** Stable id for move / remove / UMG selection */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FGuid InstanceId;

	/** Item definition (footprint, icon, type) */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UProjectOrganoidItemData> ItemData = nullptr;

	/** Top-left column */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 OriginX = 0;

	/** Top-left row */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 OriginY = 0;

	/** Units in this cell (ammo / consumables). Always >= 1 when valid. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Stack")
	int32 StackCount = 1;

	bool IsValid() const
	{
		return InstanceId.IsValid() && ItemData != nullptr && StackCount > 0;
	}

	float GetTotalWeight() const
	{
		return IsValid() ? ItemData->ItemWeight * static_cast<float>(StackCount) : 0.0f;
	}
};
