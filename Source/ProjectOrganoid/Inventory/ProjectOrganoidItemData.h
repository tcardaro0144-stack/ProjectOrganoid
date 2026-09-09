// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidWeaponTypes.h"
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
class PROJECTORGANOID_API UProjectOrganoidItemData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** Display name shown in inventory UMG */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText ItemName;

	/** Inventory flavor text only. Not a medical/scientific claim surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText Description;

	/** Inventory icon for UMG slots */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Gameplay / UI item classification */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EProjectOrganoidItemType ItemType = EProjectOrganoidItemType::None;

	/**
	 *  Ammo family for reserve stacks (used when ItemType == Ammo).
	 *  Reload / consume match this against the equipped weapon's AmmoType.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Ammo")
	EProjectOrganoidAmmoType AmmoType = EProjectOrganoidAmmoType::None;

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
	 *  When true, picking up this KeyItem fires Event_KeycardPickedUp.
	 *  Default stays true so DA_Item_AdminKeycard still completes
	 *  Main_ObtainAdminKeycard. Level-2+ campaign credentials set this false.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security")
	bool bBroadcastGenericKeycardObjectiveEvent = true;

	/**
	 *  Portable terminal / override spike. When true, Avery can lift security
	 *  gates up to OverrideClearsUpTo without a matching keycard.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security")
	bool bIsSecurityOverrideTool = false;

	/** Max gate clearance this hacking tool can override (inclusive). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Security", meta = (EditCondition = "bIsSecurityOverrideTool"))
	EProjectOrganoidSecurityTier OverrideClearsUpTo = EProjectOrganoidSecurityTier::Level2_Lab;

	/** Carry weight contribution per unit (stack counts multiply this) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Weight", meta = (ClampMin = "0.0"))
	float ItemWeight = 1.0f;

	/** Whether identical ItemData instances merge into one grid cell */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stack")
	bool bCanStack = false;

	/** Max units in a single stacked placement (1 = no stacking) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stack", meta = (ClampMin = "1", EditCondition = "bCanStack"))
	int32 MaxStackCount = 1;

	/**
	 *  Direct health restored on a successful consumable use (0 = not a healing item).
	 *  Applied through ApplyHealthDelta and clamped by MaxHealth. No toxicity/buffs.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Consumable", meta = (ClampMin = "0.0"))
	float HealAmount = 0.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ProjectOrganoidItem"), GetFName());
	}

	/** Transient pistol-ammo definition for PIE tests. Campaign content uses DA_Item_PistolAmmo. */
	static UProjectOrganoidItemData* CreateTransientPistolAmmo(UObject* Outer);
};
