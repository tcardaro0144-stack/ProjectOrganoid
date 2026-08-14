// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidSaveGame.generated.h"

/**
 *  Disk-serialized Avery progress: vitals, upgrades, inventory grid, keycards.
 */
UCLASS(BlueprintType)
class UProjectOrganoidSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FString SaveSlotName = TEXT("OrganoidSave0");

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 UserIndex = 0;

	// --- Suit vitals ---
	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float Health = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float MaxHealth = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float Toxicity = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float MaxToxicity = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float HeartRate = 72.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float PEEnergy = 100.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Vitals")
	float MaxPEEnergy = 100.0f;

	// --- Upgrade levels ---
	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 SuitHealthUpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 SuitToxicityUpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 SuitPEUpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 WeaponDamageUpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 WeaponFireRateUpgradeLevel = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Upgrades")
	int32 WeaponPenetrationUpgradeLevel = 0;

	// --- Weapon runtime stats (post-upgrade) ---
	UPROPERTY(BlueprintReadWrite, Category = "Save|Weapon")
	float WeaponDamage = 28.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Weapon")
	float WeaponFireRate = 4.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Weapon")
	float WeaponPenetration = 0.25f;

	/** Installed attachments on the equipped weapon */
	UPROPERTY(BlueprintReadWrite, Category = "Save|Weapon|Mods")
	TArray<FProjectOrganoidSavedWeaponMod> EquippedWeaponMods;

	// --- Inventory ---
	UPROPERTY(BlueprintReadWrite, Category = "Save|Inventory")
	int32 InventoryGridWidth = 8;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Inventory")
	int32 InventoryGridHeight = 6;

	UPROPERTY(BlueprintReadWrite, Category = "Save|Inventory")
	TArray<FProjectOrganoidSavedInventoryItem> InventoryItems;

	/** Distinct keycard tiers present in inventory at save time */
	UPROPERTY(BlueprintReadWrite, Category = "Save|Security")
	TArray<EProjectOrganoidSecurityTier> OwnedKeycardTiers;
};
