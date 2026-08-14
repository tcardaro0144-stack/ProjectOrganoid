// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidScannable.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidScannableActor.generated.h"

class UStaticMeshComponent;

/**
 *  Placeable scan target — organoid sample, console readout, corpse evidence, etc.
 */
UCLASS(Blueprintable)
class AProjectOrganoidScannableActor : public AActor, public IProjectOrganoidScannable
{
	GENERATED_BODY()

public:

	AProjectOrganoidScannableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	FText ScanDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan|Lore")
	FProjectOrganoidLogEntry LoreEntry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan|Objectives")
	FName ObjectiveEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan")
	bool bAllowRescan = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Scan")
	bool bHasBeenScanned = false;

	virtual bool CanBeScanned_Implementation(AActor* Scanner) const override;
	virtual FProjectOrganoidLogEntry GetScanLoreEntry_Implementation() const override;
	virtual FName GetScanObjectiveEventId_Implementation() const override;
	virtual FText GetScanDisplayName_Implementation() const override;
	virtual bool HasBeenScanned_Implementation() const override;
	virtual void NotifyScanned_Implementation(AActor* Scanner) override;
};
