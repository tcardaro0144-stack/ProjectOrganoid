// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"

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
	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		OccupyingCharacters.Add(Character);
	}
}

void AProjectOrganoidHazardZone::OnHazardEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		OccupyingCharacters.Remove(Character);
	}
}

void AProjectOrganoidHazardZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsActive || OccupyingCharacters.Num() == 0)
	{
		return;
	}

	// Copy keys in case a character is destroyed mid-tick
	TArray<AProjectOrganoidCharacter*> Characters = OccupyingCharacters.Array();
	for (AProjectOrganoidCharacter* Character : Characters)
	{
		if (IsValid(Character))
		{
			ApplyHazardToCharacter(Character, DeltaSeconds);
		}
		else
		{
			OccupyingCharacters.Remove(Character);
		}
	}
}

void AProjectOrganoidHazardZone::ApplyHazardToCharacter(AProjectOrganoidCharacter* Character, float DeltaSeconds)
{
	if (!Character)
	{
		return;
	}

	float HealthDamage = DamagePerSecond * DeltaSeconds;
	float ToxicityGain = ToxicityPerSecond * DeltaSeconds;
	float HeartRateGain = HeartRateSpikePerSecond * DeltaSeconds;

	// Hazard-specific emphasis
	switch (HazardType)
	{
	case EProjectOrganoidHazardType::UVCRadiation:
		// Sterilizing UV primarily burns suit integrity
		HealthDamage *= 1.0f;
		break;
	case EProjectOrganoidHazardType::LiquidN2Frost:
		// Cryo frost is pure thermal damage
		ToxicityGain = 0.0f;
		break;
	case EProjectOrganoidHazardType::ToxicGas:
		// Contaminant load dominates
		ToxicityGain *= 1.0f;
		break;
	default:
		break;
	}

	Character->ApplyHealthDelta(-HealthDamage);
	Character->ApplyToxicityDelta(ToxicityGain);
	Character->ApplyHeartRateDelta(HeartRateGain);

	OnHazardApplied.Broadcast(Character, HazardType);
}
