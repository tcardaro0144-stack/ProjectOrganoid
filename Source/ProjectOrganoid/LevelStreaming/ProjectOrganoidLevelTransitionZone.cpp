// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLevelTransitionZone.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

AProjectOrganoidLevelTransitionZone::AProjectOrganoidLevelTransitionZone()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->InitBoxExtent(FVector(150.0f, 150.0f, 120.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(TriggerVolume);

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidLevelTransitionZone::OnTriggerBeginOverlap);

	ArrivalTransform = FTransform::Identity;
}

void AProjectOrganoidLevelTransitionZone::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	TriggerTransition(Character);
}

bool AProjectOrganoidLevelTransitionZone::TriggerTransition(AProjectOrganoidCharacter* Character)
{
	if (!Character || (bTriggerOnce && bHasTriggered))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UProjectOrganoidLevelManagerSubsystem* LevelManager = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>();
	if (!LevelManager || LevelManager->IsTransitioning())
	{
		return false;
	}

	const FTransform Dest = bTeleportOnArrival
		? (ArrivalTransform.Equals(FTransform::Identity) ? GetActorTransform() : ArrivalTransform)
		: FTransform::Identity;

	const bool bStarted = LevelManager->RequestSubLevelTransition(
		Character,
		TargetSubLevelTag,
		TargetStreamingLevelName,
		StreamingLevelsToUnload,
		bMakeVisibleAfterLoad,
		bTeleportOnArrival,
		Dest);

	if (bStarted)
	{
		bHasTriggered = true;
		OnTransitionTriggered.Broadcast(Character, TargetSubLevelTag);
	}

	return bStarted;
}
