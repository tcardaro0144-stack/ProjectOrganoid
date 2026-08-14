// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidProjectile.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidAudioSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "CollisionQueryParams.h"
#include "Perception/AISense_Hearing.h"

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

	if (bFired)
	{
		ReportGunfireNoise();
	}

	return bFired;
}

bool AProjectOrganoidWeapon::CanFireOverchargedPulse() const
{
	if (!GetWorld() || !OwnerCharacter)
	{
		return false;
	}

	if (OwnerCharacter->GetPEEnergy() < OverchargedPulsePECost)
	{
		return false;
	}

	return (GetWorld()->GetTimeSeconds() - LastPulseFireTimeSeconds) >= OverchargedPulseCooldown;
}

bool AProjectOrganoidWeapon::FireOverchargedPulse()
{
	if (!CanFireOverchargedPulse())
	{
		return false;
	}

	LastPulseFireTimeSeconds = GetWorld()->GetTimeSeconds();

	OwnerCharacter->ApplyPEEnergyDelta(-OverchargedPulsePECost);
	OwnerCharacter->ApplyHeartRateDelta(8.0f);

	FTransform MuzzleTransform;
	GetMuzzleTransform(MuzzleTransform);
	const FVector Origin = MuzzleTransform.GetLocation();

	ApplyOverchargedPulseEffects(Origin);
	ReportGunfireNoise();

	FProjectOrganoidBallisticHit PulseHit;
	PulseHit.ImpactPoint = Origin;
	PulseHit.FinalDamage = OverchargedPulseDamage;
	PulseHit.bTacticalModeHit = IsOwnerInTacticalMode();
	OnWeaponFired.Broadcast(PulseHit);

	return true;
}

void AProjectOrganoidWeapon::ReportGunfireNoise() const
{
	if (!GetWorld() || !OwnerCharacter)
	{
		return;
	}

	const FVector NoiseLocation = OwnerCharacter->GetActorLocation();

	if (UProjectOrganoidAudioSubsystem* AudioSubsystem = GetWorld()->GetSubsystem<UProjectOrganoidAudioSubsystem>())
	{
		AudioSubsystem->PlayGunfireAtLocation(
			NoiseLocation,
			OwnerCharacter,
			GunfireNoiseLoudness,
			GunfireNoiseMaxRange);
		return;
	}

	// Fallback if subsystem is unavailable
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		NoiseLocation,
		GunfireNoiseLoudness,
		OwnerCharacter,
		GunfireNoiseMaxRange,
		FName(TEXT("Gunfire")));
}

int32 AProjectOrganoidWeapon::ApplyOverchargedPulseEffects(const FVector& Origin)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	if (OwnerCharacter)
	{
		ActorsToIgnore.Add(OwnerCharacter);
	}

	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		World,
		Origin,
		OverchargedPulseRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappingActors);

	int32 AffectedCount = 0;
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor)
		{
			continue;
		}

		if (AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Actor))
		{
			Host->StripBioShield();

			FProjectOrganoidBallisticHit PulseHit;
			PulseHit.HitActor = Host;
			PulseHit.ImpactPoint = Host->GetActorLocation();
			PulseHit.FinalDamage = OverchargedPulseDamage;
			PulseHit.bTacticalModeHit = IsOwnerInTacticalMode();
			IProjectOrganoidDamageable::Execute_ApplyOrganoidHit(Host, PulseHit, this);
			++AffectedCount;
		}
		else if (AProjectOrganoidHazardZone* Hazard = Cast<AProjectOrganoidHazardZone>(Actor))
		{
			if (Hazard->HazardType == EProjectOrganoidHazardType::ToxicGas)
			{
				Hazard->ClearHazardVolume();
				++AffectedCount;
			}
		}
	}

	return AffectedCount;
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
