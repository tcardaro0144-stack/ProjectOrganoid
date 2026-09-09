// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.generated.h"

class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnProjectOrganoidBiologicalAdaptationChanged,
	UProjectOrganoidBiologicalAdaptationData*,
	AdaptationData,
	bool,
	bEquipped);

/**
 *  Character-owned Biological Adaptation ownership, loadout, activation, cooldown.
 *  Cooldowns use wall-clock time. PE spend uses AProjectOrganoidCharacter::ApplyPEEnergyDelta.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class PROJECTORGANOID_API UProjectOrganoidBiologicalAdaptationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidBiologicalAdaptationComponent();

	UPROPERTY(BlueprintAssignable, Category = "Progression|Adaptations")
	FOnProjectOrganoidBiologicalAdaptationChanged OnAdaptationLoadoutChanged;

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations")
	bool UnlockAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	bool IsAdaptationUnlocked(const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const;

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	TArray<UProjectOrganoidBiologicalAdaptationData*> GetUnlockedAdaptations() const;

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	TArray<FSoftObjectPath> GetUnlockedAdaptationPaths() const { return UnlockedAdaptations; }

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations")
	bool EquipAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations")
	bool UnequipAdaptation();

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	UProjectOrganoidBiologicalAdaptationData* GetEquippedAdaptation() const { return EquippedAdaptation; }

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	FSoftObjectPath GetEquippedAdaptationPath() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations")
	bool TryActivateEquipped();

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	bool IsOnCooldown() const;

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	float GetCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	float GetCooldownDuration() const;

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	EProjectOrganoidBiologicalAdaptationFailReason GetLastFailReason() const { return LastFailReason; }

	UFUNCTION(BlueprintPure, Category = "Progression|Adaptations")
	FName GetUnavailableReason() const;

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations|Save")
	void ApplyUnlockedAdaptations(const TArray<FSoftObjectPath>& Paths);

	UFUNCTION(BlueprintCallable, Category = "Progression|Adaptations|Save")
	void ApplyEquippedAdaptation(const FSoftObjectPath& Path);

	static FVector GetAimStartAndDirection(const AProjectOrganoidCharacter* Character, FVector& OutDirection);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Adaptations")
	TArray<FSoftObjectPath> UnlockedAdaptations;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|Adaptations")
	TObjectPtr<UProjectOrganoidBiologicalAdaptationData> EquippedAdaptation;

	double LastActivationRealTimeSeconds = -1.0e9;
	EProjectOrganoidBiologicalAdaptationFailReason LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;

	AProjectOrganoidCharacter* GetOwnerCharacter() const;
	bool PathsMatch(const FSoftObjectPath& Path, const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const;
};
