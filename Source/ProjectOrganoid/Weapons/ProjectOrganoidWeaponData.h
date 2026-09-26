// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidWeaponData.generated.h"

/** Distinct jobs for the five roster weapons. None of these is the Lytic Cannon. */
UENUM(BlueprintType)
enum class EProjectOrganoidWeaponRosterEffect : uint8
{
	None UMETA(DisplayName = "None"),
	Stun UMETA(DisplayName = "Stun"),
	WeakPoint UMETA(DisplayName = "Weak Point"),
	CryoSlow UMETA(DisplayName = "Cryo Slow"),
	Stagger UMETA(DisplayName = "Stagger"),
	Burn UMETA(DisplayName = "Burn")
};

/**
 *  One class for every roster firearm. Identity, role, and scarce ammo live on the asset.
 *  The opening pistol stays AProjectOrganoidDefaultWeapon. This is not an Arc Gun and not the Lytic Cannon.
 */
UCLASS(BlueprintType)
class PROJECTORGANOID_API UProjectOrganoidWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText Role;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ballistics", meta = (ClampMin = "0.0"))
	float Damage = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ballistics", meta = (ClampMin = "100.0"))
	float HitscanRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ballistics", meta = (ClampMin = "0.1"))
	float FireRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	EProjectOrganoidAmmoType AmmoType = EProjectOrganoidAmmoType::Special;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	EProjectOrganoidWeaponRosterEffect Effect = EProjectOrganoidWeaponRosterEffect::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ProjectOrganoidWeapon"), WeaponId.IsNone() ? GetFName() : WeaponId);
	}

	static UProjectOrganoidWeaponData* ResolveById(FName Id);
};
