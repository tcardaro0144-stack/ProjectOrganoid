// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.generated.h"

/**
 * Syringe-kit adaptation. Impairs optical nodes without staggering or destroying them.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidBiologicalAdaptation_OpticalDisrupt : public UProjectOrganoidBiologicalAdaptationData
{
	GENERATED_BODY()

public:

	UProjectOrganoidBiologicalAdaptation_OpticalDisrupt();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|OpticalDisrupt")
	FText EffectDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|OpticalDisrupt", meta = (ClampMin = "0.1"))
	float DurationSeconds = 5.0f;

	static const TCHAR* ContentPath();
	static UProjectOrganoidBiologicalAdaptationData* Resolve();

	virtual bool TryResolveTarget(
		AProjectOrganoidCharacter* Character,
		AActor*& OutTarget,
		EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const override;

	virtual bool ExecuteOnTarget(AProjectOrganoidCharacter* Character, AActor* Target) const override;
};
