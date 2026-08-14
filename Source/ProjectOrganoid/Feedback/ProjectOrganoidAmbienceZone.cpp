// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAmbienceZone.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AProjectOrganoidAmbienceZone::AProjectOrganoidAmbienceZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ZoneVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ZoneVolume"));
	SetRootComponent(ZoneVolume);
	ZoneVolume->InitBoxExtent(FVector(400.0f, 400.0f, 200.0f));
	ZoneVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ZoneVolume->SetCollisionObjectType(ECC_WorldDynamic);
	ZoneVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ZoneVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ZoneVolume->SetGenerateOverlapEvents(true);
}

void AProjectOrganoidAmbienceZone::BeginPlay()
{
	Super::BeginPlay();

	if (ZoneId.IsNone())
	{
		ZoneId = GetFName();
	}

	ZoneVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidAmbienceZone::HandleBeginOverlap);
	ZoneVolume->OnComponentEndOverlap.AddDynamic(this, &AProjectOrganoidAmbienceZone::HandleEndOverlap);
}

void AProjectOrganoidAmbienceZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bLocalPlayerInside)
	{
		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
			{
				Ambience->UnregisterAmbienceZone(this);
			}
		}
		bLocalPlayerInside = false;
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidAmbienceZone::HandleBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		NotifyEnter(Character);
	}
}

void AProjectOrganoidAmbienceZone::HandleEndOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		NotifyExit(Character);
	}
}

void AProjectOrganoidAmbienceZone::NotifyEnter(AProjectOrganoidCharacter* Character)
{
	if (!Character || bLocalPlayerInside)
	{
		return;
	}

	bLocalPlayerInside = true;

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->RegisterAmbienceZone(this);
		}
	}
}

void AProjectOrganoidAmbienceZone::NotifyExit(AProjectOrganoidCharacter* Character)
{
	if (!Character || !bLocalPlayerInside)
	{
		return;
	}

	bLocalPlayerInside = false;

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->UnregisterAmbienceZone(this);
		}
	}
}
