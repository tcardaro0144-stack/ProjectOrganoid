// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidTrapBase.h"
#include "ProjectOrganoidHazardEmitterTrap.generated.h"

class USphereComponent;

/**
 *  Proximity hazard emitter — pulses toxic / frost / biohazard in a radius when armed.
 */
UCLASS(Blueprintable)
class AProjectOrganoidHazardEmitterTrap : public AProjectOrganoidTrapBase
{
	GENERATED_BODY()

public:

	AProjectOrganoidHazardEmitterTrap();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> EmissionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Emitter", meta = (ClampMin = "50.0"))
	float EmissionRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Emitter", meta = (ClampMin = "0.1"))
	float PulseInterval = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Emitter")
	bool bAutoPulseWhileArmed = true;

protected:

	virtual void BeginPlay() override;
	virtual void ApplyTrapEffects(AActor* TriggeringActor) override;

	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	float PulseAccumulator = 0.0f;
	void PulseEmission();
};
