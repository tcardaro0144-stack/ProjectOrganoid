// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class AProjectOrganoidWeapon;
class AController;

/**
 *  Physical projectile for Cryo Lancer / Denaturing Launcher style weapons.
 */
UCLASS()
class AProjectOrganoidProjectile : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidProjectile();

	/** Collision sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Projectile movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** Initialize damage payload from the firing weapon */
	UFUNCTION(BlueprintCallable, Category = "Ballistics")
	void InitFromWeapon(AProjectOrganoidWeapon* InWeapon, APawn* InInstigatorPawn, AController* InInstigatorController);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	float Damage = 25.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	float Penetration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	int32 RemainingPenetrations = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
	TObjectPtr<AProjectOrganoidWeapon> SourceWeapon;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
