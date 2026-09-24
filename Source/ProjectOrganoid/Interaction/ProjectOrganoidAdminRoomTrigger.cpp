// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminRoomTrigger.h"
#include "ProjectOrganoidAdminSectorController.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AProjectOrganoidAdminRoomTrigger::AProjectOrganoidAdminRoomTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 130.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetHiddenInGame(true);
	TriggerBox->SetSimulatePhysics(false);
	SetActorHiddenInGame(true);
}

void AProjectOrganoidAdminRoomTrigger::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &AProjectOrganoidAdminRoomTrigger::OnTriggerBeginOverlap);
	}
}

void AProjectOrganoidAdminRoomTrigger::OnTriggerBeginOverlap(
	UPrimitiveComponent* /*OverlappedComponent*/,
	AActor* OtherActor,
	UPrimitiveComponent* /*OtherComp*/,
	int32 /*OtherBodyIndex*/,
	bool /*bFromSweep*/,
	const FHitResult& /*SweepResult*/)
{
	if (!Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		return;
	}

	if (bTriggerOnce && bHasTriggered)
	{
		return;
	}

	if (RoomID.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AProjectOrganoidAdminSectorController* Found = nullptr;
	int32 Count = 0;
	for (TActorIterator<AProjectOrganoidAdminSectorController> It(World); It; ++It)
	{
		AProjectOrganoidAdminSectorController* Controller = *It;
		if (!IsValid(Controller))
		{
			continue;
		}
		++Count;
		Found = Controller;
	}

	if (Count != 1 || !Found)
	{
		return;
	}

	Found->SetCurrentRoom(RoomID);
	bHasTriggered = true;
}
