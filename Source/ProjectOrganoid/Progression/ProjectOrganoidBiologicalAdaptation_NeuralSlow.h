// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.generated.h"

/**
 *  First campaign Biological Adaptation.
 *  Temporary Host locomotor disruption. No direct damage.
 *  PROVISIONAL DESIGN TUNING: PE 20, cooldown 8s wall-clock, duration 4s,
 *  range 800uu, one target, ~40% movement reduction.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidBiologicalAdaptation_NeuralSlow : public UProjectOrganoidBiologicalAdaptationData
{
	GENERATED_BODY()

public:

	UProjectOrganoidBiologicalAdaptation_NeuralSlow();

	/** Speed multiplier applied to Host walk speed (0.6 = 40% reduction). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|NeuralSlow", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float LocomotorSpeedMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|NeuralSlow", meta = (ClampMin = "0.1"))
	float DurationSeconds = 4.0f;

	static const TCHAR* ContentPath();
	static UProjectOrganoidBiologicalAdaptationData* Resolve();

	virtual bool TryResolveTarget(
		AProjectOrganoidCharacter* Character,
		AActor*& OutTarget,
		EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const override;

	virtual bool ExecuteOnTarget(AProjectOrganoidCharacter* Character, AActor* Target) const override;
};
