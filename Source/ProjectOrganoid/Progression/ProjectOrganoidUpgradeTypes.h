// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidUpgradeTypes.generated.h"

/** Sterling terminal upgrade categories (SOT-funded) */
UENUM(BlueprintType)
enum class EProjectOrganoidUpgradeType : uint8
{
	SuitMaxHealth UMETA(DisplayName = "Suit — Max Health"),
	SuitToxicityThreshold UMETA(DisplayName = "Suit — Toxicity Threshold"),
	SuitPEEnergyMax UMETA(DisplayName = "Suit — PE Energy Max"),
	WeaponDamage UMETA(DisplayName = "Weapon — Damage"),
	WeaponFireRate UMETA(DisplayName = "Weapon — Fire Rate"),
	WeaponPenetration UMETA(DisplayName = "Weapon — Penetration")
};

/** Serialized inventory cell for save games */
USTRUCT(BlueprintType)
struct FProjectOrganoidSavedInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FSoftObjectPath ItemDataPath;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 OriginX = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 OriginY = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	int32 StackCount = 1;

	/** Denormalized keycard tier for quick restore / UI */
	UPROPERTY(BlueprintReadWrite, Category = "Save")
	EProjectOrganoidSecurityTier SecurityTier = EProjectOrganoidSecurityTier::None;
};
