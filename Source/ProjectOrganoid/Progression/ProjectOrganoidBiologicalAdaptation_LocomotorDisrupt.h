// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.generated.h"

/**
 * Syringe-kit adaptation. Impairs locomotor nerves.
 * Distinct from Neural Slow: 50% speed for 5 seconds, not 40% for 4.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt : public UProjectOrganoidBiologicalAdaptationData
{
	GENERATED_BODY()

public:

	UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|LocomotorDisrupt")
	FText EffectDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|LocomotorDisrupt", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float LocomotorSpeedMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|LocomotorDisrupt", meta = (ClampMin = "0.1"))
	float DurationSeconds = 5.0f;

	static const TCHAR* ContentPath();
	static UProjectOrganoidBiologicalAdaptationData* Resolve();

	virtual bool TryResolveTarget(
		AProjectOrganoidCharacter* Character,
		AActor*& OutTarget,
		EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const override;

	virtual bool ExecuteOnTarget(AProjectOrganoidCharacter* Character, AActor* Target) const override;
};
