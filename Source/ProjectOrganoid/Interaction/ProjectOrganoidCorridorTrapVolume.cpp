// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCorridorTrapVolume.h"
#include "ProjectOrganoidTrapSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AProjectOrganoidCorridorTrapVolume::AProjectOrganoidCorridorTrapVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	CorridorBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CorridorBounds"));
	SetRootComponent(CorridorBounds);
	CorridorBounds->InitBoxExtent(FVector(600.0f, 180.0f, 140.0f));
	CorridorBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CorridorBounds->SetHiddenInGame(true);
}

void AProjectOrganoidCorridorTrapVolume::BeginPlay()
{
	Super::BeginPlay();

	if (CorridorId.IsNone())
	{
		CorridorId = GetFName();
	}

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidTrapSubsystem* Traps = World->GetSubsystem<UProjectOrganoidTrapSubsystem>())
		{
			Traps->RegisterCorridor(this);
			if (bPopulateOnBeginPlay)
			{
				Traps->PopulateCorridor(this);
			}
		}
	}
}

void AProjectOrganoidCorridorTrapVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidTrapSubsystem* Traps = World->GetSubsystem<UProjectOrganoidTrapSubsystem>())
		{
			Traps->UnregisterCorridor(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

int32 AProjectOrganoidCorridorTrapVolume::PopulateTraps()
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidTrapSubsystem* Traps = World->GetSubsystem<UProjectOrganoidTrapSubsystem>())
		{
			return Traps->PopulateCorridor(this);
		}
	}
	return 0;
}

FBox AProjectOrganoidCorridorTrapVolume::GetCorridorWorldBounds() const
{
	if (CorridorBounds)
	{
		return CorridorBounds->Bounds.GetBox();
	}
	return FBox(GetActorLocation() - FVector(100.0f), GetActorLocation() + FVector(100.0f));
}
