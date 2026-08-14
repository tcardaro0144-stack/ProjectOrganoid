// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidPerceptionTypes.h"
#include "ProjectOrganoidPerceptionComponent.generated.h"

class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidSightStimulus, AActor*, Target, bool, bSensed, FVector, StimulusLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnProjectOrganoidHearingStimulus, AActor*, Instigator, FName, NoiseTag, EProjectOrganoidHearingStimulusKind, Kind, FVector, StimulusLocation, float, Strength);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidPlayerMovementHeard, AActor*, Instigator, EProjectOrganoidPlayerMovementNoiseState, MovementState);

/**
 *  Host sensory stack built on UAIPerceptionComponent.
 *  Configures sight + hearing, classifies weapon Gunfire noise and player movement Footstep_* tags.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidPerceptionComponent : public UAIPerceptionComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidPerceptionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintAssignable, Category = "Hosts|Perception")
	FOnProjectOrganoidSightStimulus OnSightStimulus;

	UPROPERTY(BlueprintAssignable, Category = "Hosts|Perception")
	FOnProjectOrganoidHearingStimulus OnHearingStimulus;

	UPROPERTY(BlueprintAssignable, Category = "Hosts|Perception")
	FOnProjectOrganoidPlayerMovementHeard OnPlayerMovementHeard;

	// -------------------------------------------------------------------------
	// Sense tuning
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Sight", meta = (ClampMin = "100.0"))
	float SightRadius = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Sight", meta = (ClampMin = "100.0"))
	float LoseSightRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Sight", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Sight", meta = (ClampMin = "0.1"))
	float SightMaxAge = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing", meta = (ClampMin = "100.0"))
	float HearingRange = 1800.0f;

	/** Added to HearingRange while ApplyHearingRangeBonus is active (rage, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing", meta = (ClampMin = "0.0"))
	float HearingRangeBonus = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing", meta = (ClampMin = "0.1"))
	float HearingMaxAge = 2.5f;

	/** How long a heard noise stays "recent" for AI queries */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing", meta = (ClampMin = "0.1"))
	float NoiseStimulusHoldSeconds = 3.0f;

	/** Minimum strength for crouch footsteps to register (filters quiet movement) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing", meta = (ClampMin = "0.0"))
	float CrouchNoiseMinStrength = 0.15f;

	/** Prefer gunfire over older footsteps when both are recent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hosts|Perception|Hearing")
	bool bPrioritizeGunfireOverFootsteps = true;

	// -------------------------------------------------------------------------
	// API
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Hosts|Perception")
	void ConfigureHostSenses();

	UFUNCTION(BlueprintCallable, Category = "Hosts|Perception|Hearing")
	void SetHearingRangeBonus(float Bonus);

	UFUNCTION(BlueprintCallable, Category = "Hosts|Perception")
	void SetSightEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Hosts|Perception")
	void SetHearingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Hosts|Perception")
	void SetAllSensesEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Sight")
	bool HasSightOnTarget() const { return bHasSightOnTarget; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Sight")
	AActor* GetCurrentSightTarget() const { return CurrentSightTarget; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	bool HasRecentNoiseStimulus() const { return bHasRecentNoiseStimulus; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	FVector GetLastHeardNoiseLocation() const { return LastHeardNoiseLocation; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	FName GetLastHeardNoiseTag() const { return LastHeardNoiseTag; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	AActor* GetLastHeardNoiseInstigator() const { return LastHeardNoiseInstigator; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	EProjectOrganoidHearingStimulusKind GetLastHeardStimulusKind() const { return LastHeardStimulusKind; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	EProjectOrganoidPlayerMovementNoiseState GetLastHeardMovementState() const { return LastHeardMovementState; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception|Hearing")
	float GetLastHeardNoiseStrength() const { return LastHeardNoiseStrength; }

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static EProjectOrganoidHearingStimulusKind ClassifyNoiseTag(FName NoiseTag);

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static EProjectOrganoidPlayerMovementNoiseState MovementStateFromNoiseTag(FName NoiseTag);

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static FName NoiseTagFromMovementState(EProjectOrganoidPlayerMovementNoiseState MovementState);

	/** Resolve Avery's current locomotion noise tag from speed / crouch. */
	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static EProjectOrganoidPlayerMovementNoiseState ResolvePlayerMovementNoiseState(const ACharacter* Character, float WalkSpeedThreshold = 220.0f, float RunSpeedThreshold = 420.0f);

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static float GetMovementNoiseLoudnessScale(EProjectOrganoidPlayerMovementNoiseState MovementState);

	UFUNCTION(BlueprintPure, Category = "Hosts|Perception")
	static float GetMovementNoiseRangeScale(EProjectOrganoidPlayerMovementNoiseState MovementState);

protected:

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Sight")
	TObjectPtr<AActor> CurrentSightTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Sight")
	bool bHasSightOnTarget = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	FVector LastHeardNoiseLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	FName LastHeardNoiseTag = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	TObjectPtr<AActor> LastHeardNoiseInstigator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	bool bHasRecentNoiseStimulus = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	EProjectOrganoidHearingStimulusKind LastHeardStimulusKind = EProjectOrganoidHearingStimulusKind::Unknown;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	EProjectOrganoidPlayerMovementNoiseState LastHeardMovementState = EProjectOrganoidPlayerMovementNoiseState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hosts|Perception|Hearing")
	float LastHeardNoiseStrength = 0.0f;

	FTimerHandle NoiseStimulusTimer;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ProcessSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void ProcessHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void ClearNoiseStimulus();
	float GetEffectiveHearingRange() const;
};
