// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidProjectile.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "CollisionQueryParams.h"

AProjectOrganoidWeapon::AProjectOrganoidWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AProjectOrganoidWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AProjectOrganoidCharacter>(GetOwner());
		if (!OwnerCharacter)
		{
			OwnerCharacter = Cast<AProjectOrganoidCharacter>(GetInstigator());
		}
	}
}

void AProjectOrganoidWeapon::SetWeaponOwnerCharacter(AProjectOrganoidCharacter* InOwnerCharacter)
{
	OwnerCharacter = InOwnerCharacter;
	if (OwnerCharacter)
	{
		SetOwner(OwnerCharacter);
		SetInstigator(OwnerCharacter);
	}
}

bool AProjectOrganoidWeapon::CanFire() const
{
	if (!GetWorld())
	{
		return false;
	}

	const float MinInterval = 1.0f / FMath::Max(FireRate, 0.1f);
	return (GetWorld()->GetTimeSeconds() - LastFireTimeSeconds) >= MinInterval;
}

bool AProjectOrganoidWeapon::Fire()
{
	if (!CanFire())
	{
		return false;
	}

	LastFireTimeSeconds = GetWorld()->GetTimeSeconds();

	const bool bFired = (BallisticsMode == EProjectOrganoidBallisticsMode::Projectile)
		? FireProjectile()
		: FireHitscan();

	return bFired;
}

void AProjectOrganoidWeapon::GetMuzzleTransform(FTransform& OutTransform) const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		OutTransform = WeaponMesh->GetSocketTransform(MuzzleSocketName);
		return;
	}

	OutTransform = GetActorTransform();
}

void AProjectOrganoidWeapon::GetAimVectors(FVector& OutStart, FVector& OutDirection) const
{
	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);
	OutStart = MuzzleTransform.GetLocation();
	OutDirection = MuzzleTransform.GetRotation().GetForwardVector();

	if (AController* Controller = OwnerCharacter ? OwnerCharacter->GetController() : nullptr)
	{
		FVector ViewLoc;
		FRotator ViewRot;
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		OutStart = ViewLoc;
		OutDirection = ViewRot.Vector();
	}
}

bool AProjectOrganoidWeapon::IsOwnerInTacticalMode() const
{
	return OwnerCharacter && OwnerCharacter->IsTacticalModeActive();
}

bool AProjectOrganoidWeapon::FireHitscan()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector Start;
	FVector Direction;
	GetAimVectors(Start, Direction);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectOrganoidHitscan), true, this);
	Params.AddIgnoredActor(this);
	if (OwnerCharacter)
	{
		Params.AddIgnoredActor(OwnerCharacter);
	}
	Params.bReturnPhysicalMaterial = true;

	float RemainingDamage = Damage;
	int32 PenetrationsLeft = MaxPenetrations;
	FVector TraceStart = Start;
	FProjectOrganoidBallisticHit PrimaryHit;
	bool bGotPrimary = false;

	const bool bTactical = IsOwnerInTacticalMode();

	for (int32 ShotIndex = 0; ShotIndex <= MaxPenetrations; ++ShotIndex)
	{
		const FVector TraceEnd = TraceStart + (Direction * HitscanRange);
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			break;
		}

		const FProjectOrganoidBallisticHit Resolved = ProcessBallisticHit(Hit, RemainingDamage, bTactical);
		if (!bGotPrimary)
		{
			PrimaryHit = Resolved;
			bGotPrimary = true;
			OnWeaponFired.Broadcast(PrimaryHit);
		}

		if (PenetrationsLeft <= 0 || Penetration <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		Params.AddIgnoredActor(Hit.GetActor());
		RemainingDamage *= Penetration;
		PenetrationsLeft--;
		TraceStart = Hit.ImpactPoint + (Direction * 2.0f);
	}

	if (!bGotPrimary)
	{
		FProjectOrganoidBallisticHit Miss;
		Miss.FinalDamage = 0.0f;
		Miss.bTacticalModeHit = bTactical;
		OnWeaponFired.Broadcast(Miss);
	}

	return true;
}

bool AProjectOrganoidWeapon::FireProjectile()
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return FireHitscan();
	}

	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectOrganoidProjectile* Projectile = World->SpawnActor<AProjectOrganoidProjectile>(
		ProjectileClass,
		MuzzleTransform.GetLocation(),
		MuzzleTransform.GetRotation().Rotator(),
		SpawnParams);

	if (!Projectile)
	{
		return false;
	}

	Projectile->InitFromWeapon(this, OwnerCharacter, OwnerCharacter ? OwnerCharacter->GetController() : nullptr);

	FProjectOrganoidBallisticHit SpawnHit;
	SpawnHit.bTacticalModeHit = IsOwnerInTacticalMode();
	SpawnHit.FinalDamage = Damage;
	OnWeaponFired.Broadcast(SpawnHit);
	return true;
}

FProjectOrganoidBallisticHit AProjectOrganoidWeapon::ProcessBallisticHit(const FHitResult& Hit, float InDamage, bool bIsTacticalMode)
{
	FProjectOrganoidBallisticHit Result;
	Result.HitActor = Hit.GetActor();
	Result.HitComponent = Hit.GetComponent();
	Result.ImpactPoint = Hit.ImpactPoint;
	Result.ImpactNormal = Hit.ImpactNormal;
	Result.HitBoneName = Hit.BoneName;
	Result.bTacticalModeHit = bIsTacticalMode;
	Result.FinalDamage = InDamage;
	Result.WeakPoint = EProjectOrganoidWeakPointType::None;

	AActor* HitActor = Hit.GetActor();
	if (HitActor && HitActor->GetClass()->ImplementsInterface(UProjectOrganoidDamageable::StaticClass()))
	{
		Result.WeakPoint = IProjectOrganoidDamageable::Execute_ResolveWeakPoint(HitActor, Hit);

		const bool bCriticalWeakPoint =
			Result.WeakPoint == EProjectOrganoidWeakPointType::LocomotorNerves
			|| Result.WeakPoint == EProjectOrganoidWeakPointType::OrganoidCore;

		if (bIsTacticalMode && bCriticalWeakPoint)
		{
			Result.FinalDamage *= TacticalWeakPointDamageMultiplier;

			if (Result.WeakPoint == EProjectOrganoidWeakPointType::LocomotorNerves && bTacticalLocomotorTriggersDismemberment)
			{
				Result.bTriggeredDismemberment = true;
			}
			if (Result.WeakPoint == EProjectOrganoidWeakPointType::OrganoidCore && bTacticalCoreTriggersIncapacitation)
			{
				Result.bTriggeredIncapacitation = true;
			}

			OnWeakPointReaction.Broadcast(Result);
		}

		IProjectOrganoidDamageable::Execute_ApplyOrganoidHit(HitActor, Result, this);
	}
	else if (HitActor)
	{
		UGameplayStatics::ApplyPointDamage(
			HitActor,
			Result.FinalDamage,
			-Hit.ImpactNormal,
			Hit,
			OwnerCharacter ? OwnerCharacter->GetController() : nullptr,
			this,
			UDamageType::StaticClass());
	}

	return Result;
}
