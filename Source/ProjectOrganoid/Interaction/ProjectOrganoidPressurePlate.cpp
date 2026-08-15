// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPressurePlate.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AProjectOrganoidPressurePlate::AProjectOrganoidPressurePlate()
{
	TrapType = EProjectOrganoidTrapType::PressurePlate;
	LinkedHazard = EProjectOrganoidHazardType::ExtremeHeat;
	TriggerDamage = 30.0f;
	bSingleUse = false;
	RetriggerCooldown = 2.0f;

	PlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlateMesh"));
	PlateMesh->SetupAttachment(TrapRoot);
	PlateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(TrapRoot);
	TriggerVolume->SetBoxExtent(FVector(50.0f, 50.0f, TriggerHalfHeight));
	TriggerVolume->SetRelativeLocation(FVector(0.0f, 0.0f, TriggerHalfHeight));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
}

void AProjectOrganoidPressurePlate::BeginPlay()
{
	Super::BeginPlay();
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidPressurePlate::HandleBeginOverlap);
}

void AProjectOrganoidPressurePlate::HandleBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	TriggerTrap(OtherActor);
}
