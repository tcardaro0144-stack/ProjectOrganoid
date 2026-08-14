// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHazardInterface.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AProjectOrganoidHazardZone::AProjectOrganoidHazardZone()
{
	PrimaryActorTick.bCanEverTick = true;

	HazardVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardVolume"));
	HazardVolume->InitBoxExtent(FVector(200.0f, 200.0f, 150.0f));
	HazardVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HazardVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	HazardVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HazardVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(HazardVolume);

	HazardVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidHazardZone::OnHazardBeginOverlap);
	HazardVolume->OnComponentEndOverlap.AddDynamic(this, &AProjectOrganoidHazardZone::OnHazardEndOverlap);

	ApplyHazardDefaultsForType();
}

void AProjectOrganoidHazardZone::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidLevelManagerSubsystem* LevelManager = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			LevelManager->RegisterHazardZone(this);
		}
	}
}

void AProjectOrganoidHazardZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidLevelManagerSubsystem* LevelManager = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			LevelManager->UnregisterHazardZone(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidHazardZone::ApplySubLevelEnvironmentContext(
	EProjectOrganoidSubLevelTag ActiveTag,
	float DamageMultiplier,
	float ToxicityMultiplier,
	bool bHazardTypeIsAmbient)
{
	EnvironmentDamageMultiplier = DamageMultiplier;
	EnvironmentToxicityMultiplier = ToxicityMultiplier;

	if (bIgnoreSubLevelContext)
	{
		return;
	}

	if (AssociatedSubLevelTag == EProjectOrganoidSubLevelTag::None)
	{
		bIsActive = bHazardTypeIsAmbient || ActiveTag == EProjectOrganoidSubLevelTag::None;
	}
	else
	{
		bIsActive = (AssociatedSubLevelTag == ActiveTag) || bHazardTypeIsAmbient;
	}
}

void AProjectOrganoidHazardZone::ClearHazardVolume()
{
	TArray<AActor*> Occupants = OccupyingActors.Array();
	for (AActor* Actor : Occupants)
	{
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
		{
			IProjectOrganoidHazardInterface::Execute_OnExitedHazard(Actor, HazardType);
		}
	}

	bIsActive = false;
	OccupyingActors.Reset();
	BP_OnHazardCleared();

	if (HazardVolume)
	{
		HazardVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HazardVolume->SetHiddenInGame(true);
	}
}

void AProjectOrganoidHazardZone::ApplyHazardDefaultsForType()
{
	switch (HazardType)
	{
	case EProjectOrganoidHazardType::UVCRadiation:
		DamagePerSecond = 12.0f;
		ToxicityPerSecond = 2.0f;
		HeartRateSpikePerSecond = 6.0f;
		break;
	case EProjectOrganoidHazardType::LiquidN2Frost:
		DamagePerSecond = 15.0f;
		ToxicityPerSecond = 0.0f;
		HeartRateSpikePerSecond = 8.0f;
		break;
	case EProjectOrganoidHazardType::ToxicGas:
		DamagePerSecond = 4.0f;
		ToxicityPerSecond = 12.0f;
		HeartRateSpikePerSecond = 5.0f;
		break;
	case EProjectOrganoidHazardType::Biohazard:
		DamagePerSecond = 6.0f;
		ToxicityPerSecond = 14.0f;
		HeartRateSpikePerSecond = 7.0f;
		break;
	case EProjectOrganoidHazardType::ExtremeHeat:
		DamagePerSecond = 18.0f;
		ToxicityPerSecond = 0.0f;
		HeartRateSpikePerSecond = 10.0f;
		break;
	default:
		break;
	}
}

void AProjectOrganoidHazardZone::OnHazardBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		return;
	}

	OccupyingActors.Add(OtherActor);

	if (bIsActive)
	{
		IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(OtherActor, HazardType, HazardIntensity);
	}
}

void AProjectOrganoidHazardZone::OnHazardEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	OccupyingActors.Remove(OtherActor);

	if (OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		IProjectOrganoidHazardInterface::Execute_OnExitedHazard(OtherActor, HazardType);
	}
}

void AProjectOrganoidHazardZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsActive || OccupyingActors.Num() == 0)
	{
		return;
	}

	TArray<AActor*> Actors = OccupyingActors.Array();
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			ApplyHazardToActor(Actor, DeltaSeconds);
		}
		else
		{
			OccupyingActors.Remove(Actor);
		}
	}
}

float AProjectOrganoidHazardZone::ComputeTickDamageAmount(float DeltaSeconds) const
{
	return DamagePerSecond * EnvironmentDamageMultiplier * HazardIntensity * DeltaSeconds;
}

void AProjectOrganoidHazardZone::ApplyHazardToActor(AActor* Actor, float DeltaSeconds)
{
	if (!Actor || !Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		return;
	}

	const float TickDamage = ComputeTickDamageAmount(DeltaSeconds);
	IProjectOrganoidHazardInterface::Execute_OnTickHazard(Actor, HazardType, TickDamage, DeltaSeconds);

	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Actor))
	{
		OnHazardApplied.Broadcast(Character, HazardType);
	}
}
