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

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	bool bTriggeredDismemberment = false;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	bool bTriggeredIncapacitation = false;
};
