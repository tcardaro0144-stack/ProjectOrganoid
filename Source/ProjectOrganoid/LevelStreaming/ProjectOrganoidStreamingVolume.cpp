// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidStreamingVolume.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AProjectOrganoidStreamingVolume::AProjectOrganoidStreamingVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->InitBoxExtent(FVector(600.0f, 600.0f, 300.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(TriggerVolume);

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidStreamingVolume::OnVolumeBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AProjectOrganoidStreamingVolume::OnVolumeEndOverlap);
}

UProjectOrganoidLevelManagerSubsystem* AProjectOrganoidStreamingVolume::GetLevelManager() const
{
	UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
}

TArray<FName> AProjectOrganoidStreamingVolume::GetRequestedPartitions() const
{
	TArray<FName> Partitions;

	if (const UProjectOrganoidLevelManagerSubsystem* Levels = GetLevelManager())
	{
		for (const EProjectOrganoidSubLevelTag Tag : RequestedRegions)
		{
			const FName Resolved = Levels->ResolveStreamingLevelName(Tag);
			if (!Resolved.IsNone())
			{
				Partitions.AddUnique(Resolved);
			}
		}
	}

	for (const FName& Explicit : RequestedStreamingLevels)
	{
		if (!Explicit.IsNone())
		{
			Partitions.AddUnique(Explicit);
		}
	}

	return Partitions;
}

void AProjectOrganoidStreamingVolume::BeginPlay()
{
	Super::BeginPlay();

	// A volume wrapping the player start will not receive a begin-overlap event, so adopt
	// anyone already inside us on the first frame.
	TArray<AActor*> Overlapping;
	TriggerVolume->GetOverlappingActors(Overlapping, AProjectOrganoidCharacter::StaticClass());

	for (AActor* Actor : Overlapping)
	{
		if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Actor))
		{
			AcquireRequests(Character);
			break;
		}
	}
}

void AProjectOrganoidStreamingVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UProjectOrganoidLevelManagerSubsystem* Levels = GetLevelManager())
	{
		Levels->RemoveAllStreamRequestsFrom(this);
		Levels->ClearPlayerRegion(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidStreamingVolume::OnVolumeBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		AcquireRequests(Character);
	}
}

void AProjectOrganoidStreamingVolume::OnVolumeEndOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/)
{
	AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	// Ignore the tail of a multi-component pawn while any part of it is still inside.
	if (TriggerVolume->IsOverlappingActor(Character))
	{
		return;
	}

	ReleaseRequests(Character);
}

void AProjectOrganoidStreamingVolume::AcquireRequests(AProjectOrganoidCharacter* Character)
{
	if (bPlayerInside)
	{
		return;
	}

	UProjectOrganoidLevelManagerSubsystem* Levels = GetLevelManager();
	if (!Levels)
	{
		return;
	}

	bPlayerInside = true;

	for (const FName& Partition : GetRequestedPartitions())
	{
		Levels->AddStreamRequest(Partition, this);
	}

	if (RegionContextTag != EProjectOrganoidSubLevelTag::None)
	{
		Levels->SetPlayerRegion(RegionContextTag, this);
	}

	OnPlayerEntered.Broadcast(Character);
}

void AProjectOrganoidStreamingVolume::ReleaseRequests(AProjectOrganoidCharacter* Character)
{
	if (!bPlayerInside)
	{
		return;
	}

	UProjectOrganoidLevelManagerSubsystem* Levels = GetLevelManager();
	if (!Levels)
	{
		return;
	}

	bPlayerInside = false;

	for (const FName& Partition : GetRequestedPartitions())
	{
		Levels->RemoveStreamRequest(Partition, this);
	}

	if (RegionContextTag != EProjectOrganoidSubLevelTag::None)
	{
		Levels->ClearPlayerRegion(this);
	}

	OnPlayerExited.Broadcast(Character);
}
