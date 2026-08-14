// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/SphereComponent.h"

AProjectOrganoidInteractable::AProjectOrganoidInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->InitSphereRadius(InteractionRange);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(InteractionSphere);
}

bool AProjectOrganoidInteractable::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	return bIsInteractable && Interactor != nullptr;
}

bool AProjectOrganoidInteractable::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}

	OnInteracted.Broadcast(this, Interactor);
	return true;
}
