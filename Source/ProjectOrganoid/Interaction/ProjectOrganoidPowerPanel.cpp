// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AProjectOrganoidPowerPanel::AProjectOrganoidPowerPanel()
{
	InteractionPrompt = FText::FromString(TEXT("Engage Backup Power"));
	InteractionRange = 220.0f;

	PanelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PanelMesh"));
	PanelMesh->SetupAttachment(InteractionSphere.Get());
	PanelMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PanelMesh->SetRelativeScale3D(FVector(0.35f, 0.12f, 0.55f));
	PanelMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 40.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PanelMesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
	StatusLight->SetupAttachment(PanelMesh);
	StatusLight->SetRelativeLocation(FVector(20.0f, 0.0f, 20.0f));
	StatusLight->SetIntensity(350.0f);
	StatusLight->SetAttenuationRadius(450.0f);
	StatusLight->SetLightColor(FLinearColor(1.0f, 0.55f, 0.12f));
	StatusLight->SetCastShadows(false);
}

bool AProjectOrganoidPowerPanel::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	return !(bSingleUse && bHasBeenEngaged);
}

bool AProjectOrganoidPowerPanel::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
	{
		Power->SetSectorPowerState(PowerSector, RestoredState);
	}

	bHasBeenEngaged = true;
	RefreshPrompt();
	NotifyObjectiveEvent(SuccessObjectiveEventId);
	BP_OnPowerPanelEngaged(Interactor, RestoredState);
	return true;
}

void AProjectOrganoidPowerPanel::RefreshPrompt()
{
	if (bSingleUse && bHasBeenEngaged)
	{
		bIsInteractable = false;
		InteractionPrompt = FText::FromString(TEXT("Breaker Engaged"));
		if (StatusLight)
		{
			StatusLight->SetLightColor(FLinearColor(0.2f, 0.85f, 0.35f));
		}
	}
}

void AProjectOrganoidPowerPanel::NotifyObjectiveEvent(FName EventId) const
{
	if (EventId.IsNone())
	{
		return;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(EventId);
		}
	}
}
