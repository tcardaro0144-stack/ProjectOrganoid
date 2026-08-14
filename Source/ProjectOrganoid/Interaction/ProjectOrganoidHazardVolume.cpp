// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardVolume.h"
#include "ProjectOrganoidHazardInterface.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidStatsSubsystem.h"
#include "Components/BrushComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Controller.h"

AProjectOrganoidHazardVolume::AProjectOrganoidHazardVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	HazardType = EProjectOrganoidHazardType::ToxicGas;
	ApplicationType = EProjectOrganoidHazardApplicationType::Continuous;
	HazardIntensity = 1.0f;
	BaseDamageAmount = 10.0f;
	BurstInterval = 1.5f;
	bScaleDamageByDistance = false;
	bIsActive = true;

	if (UBrushComponent* Brush = GetBrushComponent())
	{
		Brush->SetCollisionProfileName(TEXT("Trigger"));
		Brush->SetGenerateOverlapEvents(true);
	}
}

void AProjectOrganoidHazardVolume::BeginPlay()
{
	Super::BeginPlay();
	BindBrushOverlaps();
	RefreshBurstTimer();
}

void AProjectOrganoidHazardVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BurstTimerHandle);
	}

	for (AActor* Actor : OverlappingActors)
	{
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
		{
			IProjectOrganoidHazardInterface::Execute_OnExitedHazard(Actor, HazardType);
		}
	}
	OverlappingActors.Reset();

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidHazardVolume::BindBrushOverlaps()
{
	if (UBrushComponent* Brush = GetBrushComponent())
	{
		Brush->OnComponentBeginOverlap.AddUniqueDynamic(this, &AProjectOrganoidHazardVolume::OnHazardBeginOverlap);
		Brush->OnComponentEndOverlap.AddUniqueDynamic(this, &AProjectOrganoidHazardVolume::OnHazardEndOverlap);
	}
}

void AProjectOrganoidHazardVolume::RefreshBurstTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(BurstTimerHandle);

	if (bIsActive
		&& ApplicationType == EProjectOrganoidHazardApplicationType::Burst
		&& BurstInterval > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			BurstTimerHandle,
			this,
			&AProjectOrganoidHazardVolume::ProcessBurstDamage,
			FMath::Max(0.1f, BurstInterval),
			true);
	}
}

void AProjectOrganoidHazardVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive || ApplicationType != EProjectOrganoidHazardApplicationType::Continuous)
	{
		return;
	}

	for (int32 Index = OverlappingActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = OverlappingActors[Index];
		if (!IsValid(Actor))
		{
			OverlappingActors.RemoveAt(Index);
			continue;
		}

		float ScaledDamage = BaseDamageAmount * HazardIntensity * DeltaTime;
		if (bScaleDamageByDistance)
		{
			ScaledDamage *= CalculateDistanceScalingFactor(Actor);
		}

		ApplyDamageToActor(Actor, ScaledDamage, DeltaTime);
	}
}

void AProjectOrganoidHazardVolume::OnHazardBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	if (!OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		return;
	}

	OverlappingActors.AddUnique(OtherActor);

	if (bIsActive)
	{
		IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(OtherActor, HazardType, HazardIntensity);
	}
}

void AProjectOrganoidHazardVolume::OnHazardEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!IsValid(OtherActor) || !OverlappingActors.Contains(OtherActor))
	{
		return;
	}

	OverlappingActors.Remove(OtherActor);

	if (OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		IProjectOrganoidHazardInterface::Execute_OnExitedHazard(OtherActor, HazardType);
	}
}

void AProjectOrganoidHazardVolume::ProcessBurstDamage()
{
	if (!bIsActive || ApplicationType != EProjectOrganoidHazardApplicationType::Burst)
	{
		return;
	}

	const float PulseDelta = FMath::Max(0.1f, BurstInterval);

	for (int32 Index = OverlappingActors.Num() - 1; Index >= 0; --Index)
	{
		AActor* Actor = OverlappingActors[Index];
		if (!IsValid(Actor))
		{
			OverlappingActors.RemoveAt(Index);
			continue;
		}

		float ScaledDamage = BaseDamageAmount * HazardIntensity;
		if (bScaleDamageByDistance)
		{
			ScaledDamage *= CalculateDistanceScalingFactor(Actor);
		}

		ApplyDamageToActor(Actor, ScaledDamage, PulseDelta);
	}
}

void AProjectOrganoidHazardVolume::ApplyDamageToActor(AActor* TargetActor, float DamageToApply, float DeltaTime)
{
	if (!TargetActor || DamageToApply <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Avery / interface actors: single path via OnTickHazard (vitals + stats via ApplyHealthDelta).
	if (TargetActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		IProjectOrganoidHazardInterface::Execute_OnTickHazard(TargetActor, HazardType, DamageToApply, DeltaTime);
		return;
	}

	// Non-interface actors: standard UE damage event.
	FPointDamageEvent DamageEvent;
	DamageEvent.Damage = DamageToApply;
	DamageEvent.HitInfo = FHitResult();
	DamageEvent.ShotDirection = FVector::ZeroVector;
	TargetActor->TakeDamage(DamageToApply, DamageEvent, GetInstigatorController(), this);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->RecordHazardDamageTaken(TargetActor, HazardType, DamageToApply);
		}
	}
}

float AProjectOrganoidHazardVolume::CalculateDistanceScalingFactor(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return 1.0f;
	}

	const FVector VolumeCenter = GetActorLocation();
	const float Distance = FVector::Dist(VolumeCenter, TargetActor->GetActorLocation());

	float ApproximateRadius = 500.0f;
	if (const UBrushComponent* Brush = GetBrushComponent())
	{
		ApproximateRadius = FMath::Max(Brush->Bounds.SphereRadius, 1.0f);
	}

	const float Alpha = FMath::Clamp(Distance / ApproximateRadius, 0.0f, 1.0f);
	// Higher damage near center (1.0), falls off toward the edge (0.2).
	return FMath::Lerp(1.0f, 0.2f, Alpha);
}
