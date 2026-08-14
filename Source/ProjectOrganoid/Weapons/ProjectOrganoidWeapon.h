// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidWeapon.generated.h"

class USkeletalMeshComponent;
class AProjectOrganoidProjectile;
class AProjectOrganoidCharacter;
class UProjectOrganoidWeaponModComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidWeaponFired, const FProjectOrganoidBallisticHit&, PrimaryHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidWeakPointReaction, const FProjectOrganoidBallisticHit&, HitInfo);

/**
 *  Base firearm for ProjectOrganoid — hitscan or projectile ballistics
 *  with PE Tactical weak-point multipliers and reaction triggers.
 */
UCLASS(Abstract, Blueprintable)
class AProjectOrganoidWeapon : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidWeapon();

	virtual void BeginPlay() override;

	/** Visual mesh (optional — assign in Blueprint) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	/** Attachment / suppressor manager */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectOrganoidWeaponModComponent> WeaponModComponent;

	/** Base damage per successful hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	float Damage = 28.0f;

	/** Fraction of damage retained after each pierce (0–1). Also gates MaxPenetrations usefulness. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Penetration = 0.35f;

	/** Extra actors a hitscan/projectile may pierce after the first */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics", meta = (ClampMin = "0"))
	int32 MaxPenetrations = 0;

	/** Ammo family required to fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	EProjectOrganoidAmmoType AmmoType = EProjectOrganoidAmmoType::Pistol;

	/** Shots per second */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics", meta = (ClampMin = "0.1"))
	float FireRate = 4.0f;

	/** Hitscan trace vs spawned projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	EProjectOrganoidBallisticsMode BallisticsMode = EProjectOrganoidBallisticsMode::Hitscan;

	/** Max hitscan range (uu) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics", meta = (ClampMin = "100.0"))
	float HitscanRange = 12000.0f;

	/** Projectile class when BallisticsMode == Projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	TSubclassOf<AProjectOrganoidProjectile> ProjectileClass;

	/** Muzzle socket on WeaponMesh (falls back to actor forward) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ballistics")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Damage multiplier when striking Locomotor Nerves / Organoid Core during Tactical Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Tactical", meta = (ClampMin = "1.0"))
	float TacticalWeakPointDamageMultiplier = 2.5f;

	/** If true, Locomotor Nerve tactical hits request dismemberment */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Tactical")
	bool bTacticalLocomotorTriggersDismemberment = true;

	/** If true, Organoid Core tactical hits request incapacitation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Tactical")
	bool bTacticalCoreTriggersIncapacitation = true;

	UPROPERTY(BlueprintAssignable, Category = "Weapon")
	FOnProjectOrganoidWeaponFired OnWeaponFired;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Tactical")
	FOnProjectOrganoidWeakPointReaction OnWeakPointReaction;

	/** Overcharged pulse radius (uu) — strips bio-shields and clears toxic volumes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AltFire", meta = (ClampMin = "100.0"))
	float OverchargedPulseRadius = 750.0f;

	/** PE Energy spent to fire the overcharged pulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AltFire", meta = (ClampMin = "0.0"))
	float OverchargedPulsePECost = 28.0f;

	/** Cooldown between overcharged pulses (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AltFire", meta = (ClampMin = "0.1"))
	float OverchargedPulseCooldown = 3.5f;

	/** Flat damage applied to hosts hit by the pulse (after shield strip) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AltFire", meta = (ClampMin = "0.0"))
	float OverchargedPulseDamage = 18.0f;

	/** Noise loudness reported for AI hearing on primary / alt fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AI", meta = (ClampMin = "0.0"))
	float GunfireNoiseLoudness = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|AI", meta = (ClampMin = "100.0"))
	float GunfireNoiseMaxRange = 3500.0f;

	/** Attempt to fire; respects fire-rate cooldown */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool Fire();

	/** Secondary overcharged pulse — strip bio-shields + clear toxic hazard volumes */
	UFUNCTION(BlueprintCallable, Category = "Weapon|AltFire")
	bool FireOverchargedPulse();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	bool CanFire() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|AltFire")
	bool CanFireOverchargedPulse() const;

	/** Build and apply a hit (used by weapon hitscan and projectiles) */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ballistics")
	FProjectOrganoidBallisticHit ProcessBallisticHit(const FHitResult& Hit, float InDamage, bool bIsTacticalMode);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWeaponOwnerCharacter(AProjectOrganoidCharacter* InOwnerCharacter);

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AProjectOrganoidCharacter* GetWeaponOwnerCharacter() const { return OwnerCharacter; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	UProjectOrganoidWeaponModComponent* GetWeaponModComponent() const { return WeaponModComponent; }

	/** Base damage × attachment multipliers */
	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	float GetEffectiveDamage() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	float GetEffectiveFireRate() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	float GetEffectivePenetration() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	float GetEffectiveGunfireNoiseLoudness() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	float GetEffectiveGunfireNoiseMaxRange() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Mods")
	bool HasSuppressorInstalled() const;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AProjectOrganoidCharacter> OwnerCharacter;

	float LastFireTimeSeconds = -BIG_NUMBER;
	float LastPulseFireTimeSeconds = -BIG_NUMBER;

	bool FireHitscan();
	bool FireProjectile();
	void ReportGunfireNoise() const;
	int32 ApplyOverchargedPulseEffects(const FVector& Origin);

	void GetMuzzleTransform(FTransform& OutTransform) const;
	void GetAimVectors(FVector& OutStart, FVector& OutDirection) const;
	bool IsOwnerInTacticalMode() const;
};
