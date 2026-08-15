// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidTrapBase.h"
#include "ProjectOrganoidLaserTripwire.generated.h"

class UBoxComponent;

/**
 *  Corridor laser tripwire — overlapping the beam triggers UV-C / heat damage.
 */
UCLASS(Blueprintable)
class AProjectOrganoidLaserTripwire : public AProjectOrganoidTrapBase
{
	GENERATED_BODY()

public:

	AProjectOrganoidLaserTripwire();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BeamVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Laser", meta = (ClampMin = "10.0"))
	float BeamLength = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Laser", meta = (ClampMin = "1.0"))
	float BeamThickness = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Laser")
	bool bContinuousWhileOverlapping = true;

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> OverlappingActors;

	void RefreshBeamExtent();
};
