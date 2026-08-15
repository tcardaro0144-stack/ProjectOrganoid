// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardEmitterTrap.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"

AProjectOrganoidHazardEmitterTrap::AProjectOrganoidHazardEmitterTrap()
{
	PrimaryActorTick.bCanEverTick = true;
	TrapType = EProjectOrganoidTrapType::HazardEmitter;
	LinkedHazard = EProjectOrganoidHazardType::ToxicGas;
	TriggerDamage = 12.0f;
	RetriggerCooldown = 0.25f;

	EmissionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("EmissionSphere"));
	EmissionSphere->SetupAttachment(TrapRoot);
	EmissionSphere->InitSphereRadius(EmissionRadius);
	EmissionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EmissionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	EmissionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EmissionSphere->SetGenerateOverlapEvents(true);
}

void AProjectOrganoidHazardEmitterTrap::BeginPlay()
{
	Super::BeginPlay();
	EmissionSphere->SetSphereRadius(EmissionRadius);
	EmissionSphere->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidHazardEmitterTrap::HandleBeginOverlap);
}

void AProjectOrganoidHazardEmitterTrap::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bArmed || !bAutoPulseWhileArmed)
	{
		return;
	}

	PulseAccumulator += DeltaSeconds;
	if (PulseAccumulator >= PulseInterval)
	{
		PulseAccumulator = 0.0f;
		PulseEmission();
	}
}

void AProjectOrganoidHazardEmitterTrap::HandleBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	TriggerTrap(OtherActor);
}

void AProjectOrganoidHazardEmitterTrap::PulseEmission()
{
	TArray<AActor*> Overlaps;
	EmissionSphere->GetOverlappingActors(Overlaps);
	for (AActor* Actor : Overlaps)
	{
		if (Actor && Actor != this)
		{
			TriggerTrap(Actor);
		}
	}
}

void AProjectOrganoidHazardEmitterTrap::ApplyTrapEffects(AActor* TriggeringActor)
{
	ApplyHazardToActor(TriggeringActor, TriggerDamage * TriggerIntensity);
}
