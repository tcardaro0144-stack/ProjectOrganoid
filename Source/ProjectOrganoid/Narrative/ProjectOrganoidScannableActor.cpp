// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidScannableActor.h"
#include "Components/StaticMeshComponent.h"

AProjectOrganoidScannableActor::AProjectOrganoidScannableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ScanDisplayName = FText::FromString(TEXT("Facility Specimen"));
	LoreEntry.Category = TEXT("Scan");
}

bool AProjectOrganoidScannableActor::CanBeScanned_Implementation(AActor* /*Scanner*/) const
{
	return bAllowRescan || !bHasBeenScanned;
}

FProjectOrganoidLogEntry AProjectOrganoidScannableActor::GetScanLoreEntry_Implementation() const
{
	return LoreEntry;
}

FName AProjectOrganoidScannableActor::GetScanObjectiveEventId_Implementation() const
{
	return ObjectiveEventId;
}

FText AProjectOrganoidScannableActor::GetScanDisplayName_Implementation() const
{
	return ScanDisplayName;
}

bool AProjectOrganoidScannableActor::HasBeenScanned_Implementation() const
{
	return bHasBeenScanned;
}

void AProjectOrganoidScannableActor::NotifyScanned_Implementation(AActor* /*Scanner*/)
{
	bHasBeenScanned = true;
}
