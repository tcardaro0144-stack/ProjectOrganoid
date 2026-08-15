// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLaserTripwire.h"
#include "Components/BoxComponent.h"

AProjectOrganoidLaserTripwire::AProjectOrganoidLaserTripwire()
{
	PrimaryActorTick.bCanEverTick = true;
	TrapType = EProjectOrganoidTrapType::LaserTripwire;
	LinkedHazard = EProjectOrganoidHazardType::UVCRadiation;
	TriggerDamage = 18.0f;

	BeamVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BeamVolume"));
	BeamVolume->SetupAttachment(TrapRoot);
	BeamVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BeamVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	BeamVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BeamVolume->SetGenerateOverlapEvents(true);
	BeamVolume->ComponentTags.AddUnique(TEXT("Laser"));
	RefreshBeamExtent();
}

void AProjectOrganoidLaserTripwire::BeginPlay()
{
	Super::BeginPlay();
	RefreshBeamExtent();
	BeamVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidLaserTripwire::HandleBeginOverlap);
}

void AProjectOrganoidLaserTripwire::RefreshBeamExtent()
{
	if (!BeamVolume)
	{
		return;
	}

	BeamVolume->SetBoxExtent(FVector(BeamLength * 0.5f, BeamThickness, BeamThickness));
	BeamVolume->SetRelativeLocation(FVector(BeamLength * 0.5f, 0.0f, 0.0f));
}

void AProjectOrganoidLaserTripwire::HandleBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	OverlappingActors.Add(OtherActor);
	TriggerTrap(OtherActor);
}

void AProjectOrganoidLaserTripwire::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bArmed || !bContinuousWhileOverlapping)
	{
		return;
	}

	TArray<AActor*> Overlaps;
	BeamVolume->GetOverlappingActors(Overlaps);
	for (AActor* Actor : Overlaps)
	{
		if (Actor && Actor != this)
		{
			TriggerTrap(Actor);
		}
	}
}
