// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDoorLock.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AProjectOrganoidDoorLock::AProjectOrganoidDoorLock()
{
	InteractionPrompt = FText::FromString(TEXT("Use Keycard Lock"));

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(InteractionSphere);
	DoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AProjectOrganoidDoorLock::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->RegisterDoor(this);
			HandlePowerStateChanged(Power->GetSectorPowerState(PowerSector), EProjectOrganoidPowerState::Online);
		}
	}
}

void AProjectOrganoidDoorLock::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->UnregisterDoor(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool AProjectOrganoidDoorLock::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	// Always allow interact attempts so we can show access-denied feedback when locked
	return true;
}

bool AProjectOrganoidDoorLock::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (!bIsLocked)
	{
		SetOpen(!bIsOpen);
		OnInteracted.Broadcast(this, Interactor);
		return true;
	}

	UProjectOrganoidInventoryComponent* Inventory = Interactor->GetInventoryComponent();
	if (!Inventory || !Inventory->HasKeycardOfTier(RequiredSecurityTier))
	{
		BP_OnAccessDenied(Interactor, RequiredSecurityTier);
		return false;
	}

	if (bConsumeKeycardOnUnlock)
	{
		Inventory->ConsumeKeycardOfTier(RequiredSecurityTier);
	}

	SetLocked(false);
	SetOpen(true);
	BP_OnDoorUnlocked(Interactor);
	OnInteracted.Broadcast(this, Interactor);
	return true;
}

void AProjectOrganoidDoorLock::SetLocked(bool bNewLocked)
{
	if (bIsLocked == bNewLocked)
	{
		return;
	}

	bIsLocked = bNewLocked;
	OnDoorStateChanged.Broadcast(!bIsLocked);

	InteractionPrompt = bIsLocked
		? FText::FromString(TEXT("Use Keycard Lock"))
		: FText::FromString(TEXT("Open / Close Door"));
}

void AProjectOrganoidDoorLock::SetOpen(bool bNewOpen)
{
	bIsOpen = bNewOpen;
}

void AProjectOrganoidDoorLock::HandlePowerStateChanged(EProjectOrganoidPowerState NewState, EProjectOrganoidPowerState PreviousState)
{
	if (bDisableInteractDuringBlackout)
	{
		bIsInteractable = NewState != EProjectOrganoidPowerState::Blackout;
	}

	BP_OnPowerStateChanged(NewState);
}
