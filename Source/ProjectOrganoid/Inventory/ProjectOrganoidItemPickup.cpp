// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidItemPickup::AProjectOrganoidItemPickup()
{
	InteractionPrompt = FText::FromString(TEXT("Pick Up"));
	InteractionRange = 180.0f;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(InteractionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickupMesh->SetRelativeScale3D(FVector(0.25f, 0.35f, 0.08f));
}

void AProjectOrganoidItemPickup::BeginPlay()
{
	Super::BeginPlay();
	RefreshPrompt();
}

void AProjectOrganoidItemPickup::RefreshPrompt()
{
	if (ItemData && !ItemData->ItemName.IsEmpty())
	{
		InteractionPrompt = FText::Format(
			NSLOCTEXT("ProjectOrganoid", "PickupPrompt", "Pick Up {0}"),
			ItemData->ItemName);
	}
}

bool AProjectOrganoidItemPickup::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	// Temporary opening-progression infrastructure. Not final architecture.
	// Discriminator matches the authored Research Wing card only:
	// KeyItem + Level2_Lab + no generic keycard objective broadcast.
	// Later opening/campaign progression must replace this with an explicit unlock.
	if (ItemData
		&& ItemData->ItemType == EProjectOrganoidItemType::KeyItem
		&& ItemData->SecurityTier == EProjectOrganoidSecurityTier::Level2_Lab
		&& !ItemData->bBroadcastGenericKeycardObjectiveEvent)
	{
		return false;
	}

	return Super::CanInteract_Implementation(Interactor) && ItemData != nullptr && Quantity > 0;
}

bool AProjectOrganoidItemPickup::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Interactor->GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	FGuid InstanceId;
	if (!Inventory->TryAddItem(ItemData, InstanceId, Quantity))
	{
		InteractionPrompt = FText::FromString(TEXT("Inventory Full"));
		return false;
	}

	RefreshPrompt();
	BP_OnPickedUp(Interactor, ItemData, Quantity);
	NotifyPickupEvents();
	OnInteracted.Broadcast(this, Interactor);

	if (bDestroyOnPickup)
	{
		Destroy();
	}
	else
	{
		bIsInteractable = false;
	}

	return true;
}

void AProjectOrganoidItemPickup::NotifyPickupEvents()
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI)
	{
		return;
	}

	UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
	if (!Objectives)
	{
		return;
	}

	Objectives->TriggerEvent(TEXT("Event_ItemPickedUp"));
	if (ItemData
		&& ItemData->ItemType == EProjectOrganoidItemType::KeyItem
		&& ItemData->bBroadcastGenericKeycardObjectiveEvent)
	{
		Objectives->TriggerEvent(TEXT("Event_KeycardPickedUp"));
	}
	if (!PickupObjectiveEventId.IsNone())
	{
		Objectives->TriggerEvent(PickupObjectiveEventId);
	}
}
