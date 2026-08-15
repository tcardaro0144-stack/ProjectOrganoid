// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidTrapBase.h"
#include "ProjectOrganoidPressurePlate.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 *  Floor pressure plate — triggers when a pawn steps on it.
 */
UCLASS(Blueprintable)
class AProjectOrganoidPressurePlate : public AProjectOrganoidTrapBase
{
	GENERATED_BODY()

public:

	AProjectOrganoidPressurePlate();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PlateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Plate", meta = (ClampMin = "1.0"))
	float TriggerHalfHeight = 12.0f;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
