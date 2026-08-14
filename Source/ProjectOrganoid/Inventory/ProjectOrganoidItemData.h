// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidItemData.generated.h"

class UTexture2D;

/** Classification for survival-horror item handling and UI filters */
UENUM(BlueprintType)
enum class EProjectOrganoidItemType : uint8
{
	None UMETA(DisplayName = "None"),
	Weapon UMETA(DisplayName = "Weapon"),
	Ammo UMETA(DisplayName = "Ammo"),
	Consumable UMETA(DisplayName = "Consumable"),
	KeyItem UMETA(DisplayName = "Key Item"),
	SOT UMETA(DisplayName = "Synthetic Organoid Tissue"),
	Attachment UMETA(DisplayName = "Attachment")
};

/**
 *  Primary data definition for a grid-inventory item
 *  (e.g. Shotgun 2x4, Ammo 1x1, P226 1x2).
 */
UCLASS(BlueprintType)
class UProjectOrganoidItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Display name shown in inventory UMG */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	/** Inventory icon for UMG slots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Gameplay / UI item classification */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EProjectOrganoidItemType ItemType = EProjectOrganoidItemType::None;

	/** Grid footprint width in slots (columns) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 1;

	/** Grid footprint height in slots (rows) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 1;

	/**
	 *  Keycard clearance (used when ItemType == KeyItem).
	 *  Higher tiers satisfy lower-tier door locks / security gates.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security")
	EProjectOrganoidSecurityTier SecurityTier = EProjectOrganoidSecurityTier::None;

	/**
	 *  Portable terminal / override spike. When true, Avery can lift security
	 *  gates up to OverrideClearsUpTo without a matching keycard.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security")
	bool bIsSecurityOverrideTool = false;

	/** Max gate clearance this hacking tool can override (inclusive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security", meta = (EditCondition = "bIsSecurityOverrideTool"))
	EProjectOrganoidSecurityTier OverrideClearsUpTo = EProjectOrganoidSecurityTier::Level2_Lab;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ProjectOrganoidItem"), GetFName());
	}
};
