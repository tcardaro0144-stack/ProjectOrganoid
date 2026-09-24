// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAmbienceZone.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/ReverbEffect.h"
#include "Sound/SoundBase.h"

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

	if (RoomToneSound.IsNull())
	{
		RoomToneSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/Ambient/SW_FacilityBed.SW_FacilityBed")));
	}

	if (ZoneReverb.IsNull())
	{
		ZoneReverb = TSoftObjectPtr<UReverbEffect>(FSoftObjectPath(TEXT("/Engine/EngineSounds/ReverbSettings/BunkerHall.BunkerHall")));
	}

	ZoneVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AProjectOrganoidAmbienceZone::HandleBeginOverlap);
	ZoneVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &AProjectOrganoidAmbienceZone::HandleEndOverlap);
	SynchronizeOverlappingLocalCharacter();
}

void AProjectOrganoidAmbienceZone::SynchronizeOverlappingLocalCharacter()
{
	if (!ZoneVolume)
	{
		return;
	}

	ZoneVolume->UpdateOverlaps();

	TArray<AActor*> Overlapping;
	ZoneVolume->GetOverlappingActors(Overlapping, AProjectOrganoidCharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Actor))
		{
			NotifyEnter(Character);
			return;
		}
	}

	if (bLocalPlayerInside)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Pawn);
	if (!Character)
	{
		return;
	}

	const FBox Bounds = ZoneVolume->Bounds.GetBox();
	if (Bounds.IsInsideOrOn(Character->GetActorLocation()))
	{
		NotifyEnter(Character);
	}
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
