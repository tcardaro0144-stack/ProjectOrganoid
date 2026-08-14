// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidProjectile.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectOrganoidProjectile::AProjectOrganoidProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(6.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	SetRootComponent(CollisionSphere);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionSphere;
	ProjectileMovement->InitialSpeed = 6000.0f;
	ProjectileMovement->MaxSpeed = 6000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.05f;

	CollisionSphere->OnComponentHit.AddDynamic(this, &AProjectOrganoidProjectile::OnProjectileHit);
}

void AProjectOrganoidProjectile::InitFromWeapon(
	AProjectOrganoidWeapon* InWeapon,
	APawn* InInstigatorPawn,
	AController* InInstigatorController)
{
	SourceWeapon = InWeapon;
	if (InWeapon)
	{
		Damage = InWeapon->Damage;
		Penetration = InWeapon->Penetration;
		RemainingPenetrations = InWeapon->MaxPenetrations;
	}

	if (InInstigatorPawn)
	{
		SetInstigator(InInstigatorPawn);
	}

	if (InInstigatorController)
	{
		SetOwner(InInstigatorController->GetPawn() ? InInstigatorController->GetPawn() : InInstigatorPawn);
	}

	CollisionSphere->IgnoreActorWhenMoving(GetOwner(), true);
	if (InInstigatorPawn)
	{
		CollisionSphere->IgnoreActorWhenMoving(InInstigatorPawn, true);
	}
}

void AProjectOrganoidProjectile::OnProjectileHit(
	UPrimitiveComponent* HitComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}

	const bool bTactical = SourceWeapon && SourceWeapon->GetWeaponOwnerCharacter()
		&& SourceWeapon->GetWeaponOwnerCharacter()->IsTacticalModeActive();

	if (SourceWeapon)
	{
		SourceWeapon->ProcessBallisticHit(Hit, Damage, bTactical);
	}

	if (RemainingPenetrations > 0 && Penetration > KINDA_SMALL_NUMBER)
	{
		RemainingPenetrations--;
		Damage *= Penetration;
		CollisionSphere->IgnoreActorWhenMoving(OtherActor, true);
		return;
	}

	Destroy();
}
