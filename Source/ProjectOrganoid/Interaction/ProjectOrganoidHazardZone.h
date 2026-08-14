// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidHazardZone.generated.h"

class UBoxComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHazardApplied, AProjectOrganoidCharacter*, Character, EProjectOrganoidHazardType, HazardType);

/**
 *  Environmental hazard volume — UV-C, Liquid N2 frost, or toxic gas.
 *  Applies health damage and/or toxicity to Avery while overlapping.
 */
UCLASS(Blueprintable)
class AProjectOrganoidHazardZone : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidHazardZone();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> HazardVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	EProjectOrganoidHazardType HazardType = EProjectOrganoidHazardType::ToxicGas;

	/** Health lost per second while inside the zone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
	float DamagePerSecond = 8.0f;

	/** Toxicity gained per second while inside the zone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
	float ToxicityPerSecond = 5.0f;

	/** Heart-rate spike applied per second (diegetic stress) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (ClampMin = "0.0"))
	float HeartRateSpikePerSecond = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	bool bIsActive = true;

	/**
	 *  If not None, this zone only stays active while the matching sub-level is current
	 *  (or when its HazardType is listed as ambient for that floor).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Level")
	EProjectOrganoidSubLevelTag AssociatedSubLevelTag = EProjectOrganoidSubLevelTag::None;

	/** If true, ignore sub-level gating and remain controllable only via bIsActive */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Level")
	bool bIgnoreSubLevelContext = false;

	UPROPERTY(BlueprintReadOnly, Category = "Hazard|Level")
	float EnvironmentDamageMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Hazard|Level")
	float EnvironmentToxicityMultiplier = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Hazard")
	FOnProjectOrganoidHazardApplied OnHazardApplied;

	/** Called by LevelManager when the active Epitope floor changes */
	UFUNCTION(BlueprintCallable, Category = "Hazard|Level")
	void ApplySubLevelEnvironmentContext(
		EProjectOrganoidSubLevelTag ActiveTag,
		float DamageMultiplier,
		float ToxicityMultiplier,
		bool bHazardTypeIsAmbient);

protected:

	UPROPERTY()
	TSet<TObjectPtr<AProjectOrganoidCharacter>> OccupyingCharacters;

	UFUNCTION()
	void OnHazardBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnHazardEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyHazardDefaultsForType();
	void ApplyHazardToCharacter(AProjectOrganoidCharacter* Character, float DeltaSeconds);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
