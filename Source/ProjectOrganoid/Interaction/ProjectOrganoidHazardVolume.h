// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidHazardVolume.generated.h"

/**
 *  Brush-based facility hazard volume (Continuous DPS or Burst pulses).
 *  Dispatches IProjectOrganoidHazardInterface enter / tick / exit callbacks.
 */
UCLASS(Blueprintable, BlueprintType)
class AProjectOrganoidHazardVolume : public AVolume
{
	GENERATED_BODY()

public:

	AProjectOrganoidHazardVolume();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
	virtual void OnHazardBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	virtual void OnHazardEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard")
	EProjectOrganoidHazardType HazardType = EProjectOrganoidHazardType::ToxicGas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard")
	EProjectOrganoidHazardApplicationType ApplicationType = EProjectOrganoidHazardApplicationType::Continuous;

	/** Enter-callback intensity passed to OnEnteredHazard */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float HazardIntensity = 1.0f;

	/** Damage per second (Continuous) or per pulse (Burst), before distance scaling */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard", meta = (ClampMin = "0.0"))
	float BaseDamageAmount = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard", meta = (EditCondition = "ApplicationType == EProjectOrganoidHazardApplicationType::Burst", ClampMin = "0.1"))
	float BurstInterval = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard")
	bool bScaleDamageByDistance = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ProjectOrganoid|Hazard")
	bool bIsActive = true;

private:

	UPROPERTY()
	TArray<TObjectPtr<AActor>> OverlappingActors;

	FTimerHandle BurstTimerHandle;

	void ProcessBurstDamage();
	void ApplyDamageToActor(AActor* TargetActor, float DamageToApply, float DeltaTime);
	float CalculateDistanceScalingFactor(AActor* TargetActor) const;
	void BindBrushOverlaps();
	void RefreshBurstTimer();
};
