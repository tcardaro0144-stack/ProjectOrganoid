// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidWeaponModTypes.generated.h"

/** Attachment socket / exclusivity slot on a firearm */
UENUM(BlueprintType)
enum class EProjectOrganoidWeaponModSlot : uint8
{
	Barrel UMETA(DisplayName = "Barrel"),
	Optic UMETA(DisplayName = "Optic"),
	Magazine UMETA(DisplayName = "Magazine"),
	Underbarrel UMETA(DisplayName = "Underbarrel"),
	Grip UMETA(DisplayName = "Grip")
};

/**
 *  Designer-authored weapon attachment (suppressor, compensator, etc.).
 *  Installed via Sterling terminal / WeaponModComponent.
 */
UCLASS(BlueprintType)
class UProjectOrganoidWeaponModData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod")
	FName ModId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod")
	EProjectOrganoidWeaponModSlot Slot = EProjectOrganoidWeaponModSlot::Barrel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Stats", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Stats", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float FireRateMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Stats", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float PenetrationMultiplier = 1.0f;

	/**
	 *  Multiplies gunfire AI loudness. Suppressors should be well below 1.0
	 *  (e.g. 0.3). When bSuppressesSoundEmission is true, loudness is also
	 *  clamped by the mod component's suppressor floor.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Audio", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float NoiseLoudnessMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Audio", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float NoiseRangeMultiplier = 1.0f;

	/** Marks this attachment as a suppressed barrel for AI / UI treatment */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Audio")
	bool bSuppressesSoundEmission = false;

	/** SOT spent at Sterling terminal to install this mod */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mod|Cost", meta = (ClampMin = "0"))
	int32 SOTInstallCost = 2;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ProjectOrganoidWeaponMod"), ModId.IsNone() ? GetFName() : ModId);
	}
};

/** Disk record for one installed attachment */
USTRUCT(BlueprintType)
struct FProjectOrganoidSavedWeaponMod
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FSoftObjectPath ModDataPath;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	EProjectOrganoidWeaponModSlot Slot = EProjectOrganoidWeaponModSlot::Barrel;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName ModId = NAME_None;
};
