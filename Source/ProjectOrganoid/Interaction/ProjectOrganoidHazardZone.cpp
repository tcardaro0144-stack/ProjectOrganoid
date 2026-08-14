// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidAudioSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

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
		// Untagged zones follow ambient hazard type for the active floor
		bIsActive = bHazardTypeIsAmbient || ActiveTag == EProjectOrganoidSubLevelTag::None;
	}
	else
	{
		bIsActive = (AssociatedSubLevelTag == ActiveTag) || bHazardTypeIsAmbient;
	}
}

void AProjectOrganoidHazardZone::ClearHazardVolume()
{
	const bool bWasToxicGas = (HazardType == EProjectOrganoidHazardType::ToxicGas) && OccupyingCharacters.Num() > 0;

	bIsActive = false;
	OccupyingCharacters.Reset();
	BP_OnHazardCleared();

	if (HazardVolume)
	{
		HazardVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HazardVolume->SetHiddenInGame(true);
	}

	if (bWasToxicGas)
	{
		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
			{
				AudioSubsystem->SetToxicGasDistortion(0.0f);
			}
		}
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

		if (bIsActive && HazardType == EProjectOrganoidHazardType::ToxicGas)
		{
			if (UWorld* World = GetWorld())
			{
				if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
				{
					AudioSubsystem->SetToxicGasDistortion(1.0f);
				}
			}
		}
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

		if (HazardType == EProjectOrganoidHazardType::ToxicGas)
		{
			if (UWorld* World = GetWorld())
			{
				if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
				{
					// Clear unless another active toxic-gas volume still contains Avery.
					bool bStillInToxicGas = false;
					TArray<AActor*> ToxicZones;
					UGameplayStatics::GetAllActorsOfClass(World, AProjectOrganoidHazardZone::StaticClass(), ToxicZones);
					for (AActor* ZoneActor : ToxicZones)
					{
						AProjectOrganoidHazardZone* Zone = Cast<AProjectOrganoidHazardZone>(ZoneActor);
						if (Zone && Zone != this && Zone->bIsActive
							&& Zone->HazardType == EProjectOrganoidHazardType::ToxicGas
							&& Zone->HazardVolume
							&& Zone->HazardVolume->IsOverlappingActor(Character))
						{
							bStillInToxicGas = true;
							break;
						}
					}

					if (!bStillInToxicGas)
					{
						AudioSubsystem->SetToxicGasDistortion(0.0f);
					}
				}
			}
		}
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

	float HealthDamage = DamagePerSecond * EnvironmentDamageMultiplier * DeltaSeconds;
	float ToxicityGain = ToxicityPerSecond * EnvironmentToxicityMultiplier * DeltaSeconds;
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
