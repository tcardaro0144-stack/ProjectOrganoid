// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "ProjectOrganoidWeaponTypes.generated.h"

/** Ammo family used by weapons and inventory matching */
UENUM(BlueprintType)
enum class EProjectOrganoidAmmoType : uint8
{
	None UMETA(DisplayName = "None"),
	Pistol UMETA(DisplayName = "Pistol"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Rifle UMETA(DisplayName = "Rifle"),
	Special UMETA(DisplayName = "Special / Denature")
};

/**
 *  Per-weapon loaded magazine snapshot.
 *  Loaded rounds are weapon state only — never mirrored into inventory.
 *  The save/holster map is keyed by WeaponClass so future multi-weapon
 *  ownership can persist each firearm's magazine without rewriting fire,
 *  reload, or reserve-consume logic. This block persists the equipped
 *  pistol only; do not treat a single EquippedMagazineCount as the model.
 */
USTRUCT(BlueprintType)
struct FProjectOrganoidWeaponMagazineState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Weapon|Ammo")
	FSoftClassPath WeaponClass;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon|Ammo")
	EProjectOrganoidAmmoType AmmoType = EProjectOrganoidAmmoType::None;

	UPROPERTY(BlueprintReadWrite, Category = "Weapon|Ammo")
	int32 LoadedMagazineCount = 0;

	bool IsValid() const
	{
		return WeaponClass.IsValid() && LoadedMagazineCount >= 0;
	}
};

/** Ballistic delivery mode */
UENUM(BlueprintType)
enum class EProjectOrganoidBallisticsMode : uint8
{
	Hitscan UMETA(DisplayName = "Hitscan"),
	Projectile UMETA(DisplayName = "Projectile")
};

/** Host weak points highlighted in PE Tactical Mode */
UENUM(BlueprintType)
enum class EProjectOrganoidWeakPointType : uint8
{
	None UMETA(DisplayName = "None"),
	LocomotorNerves UMETA(DisplayName = "Locomotor Nerves"),
	OpticalNodes UMETA(DisplayName = "Optical Nodes"),
	OrganoidCore UMETA(DisplayName = "Bio-Core / Organoid Core")
};

/** Resolved ballistic hit payload for damage / UMG / VFX */
USTRUCT(BlueprintType)
struct FProjectOrganoidBallisticHit
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	TObjectPtr<AActor> HitActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	FVector ImpactPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	FVector ImpactNormal = FVector::UpVector;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	FName HitBoneName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	EProjectOrganoidWeakPointType WeakPoint = EProjectOrganoidWeakPointType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	bool bTacticalModeHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	float FinalDamage = 0.0f;

	/** Shot / impulse direction used for hit reactions (usually toward impact) */
	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	FVector ImpactDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	float ImpulseStrength = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	bool bTriggeredDismemberment = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	bool bTriggeredIncapacitation = false;
};
