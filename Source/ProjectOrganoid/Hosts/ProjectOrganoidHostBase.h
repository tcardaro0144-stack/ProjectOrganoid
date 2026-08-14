// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidPerceptionTypes.h"
#include "ProjectOrganoidHostBase.generated.h"

class USphereComponent;
class UProjectOrganoidPerceptionComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHostDamaged, const FProjectOrganoidBallisticHit&, HitInfo, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHostStateChanged, FName, StateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidHostDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHostNoiseHeard, AActor*, NoiseInstigator, FName, NoiseTag);

/**
 *  Mutated organoid host — weak points, phase-shift mutations (rage / bio-shield),
 *  and AI sight + hearing for footstep / gunfire noise.
 */
UCLASS(Abstract, Blueprintable)
class AProjectOrganoidHostBase : public ACharacter, public IProjectOrganoidDamageable
{
	GENERATED_BODY()

public:

	AProjectOrganoidHostBase();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// -------------------------------------------------------------------------
	// Components
	// -------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> LocomotorNervesHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> OpticalNodesHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> BioCoreHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|AI")
	TObjectPtr<UProjectOrganoidPerceptionComponent> HostPerception;

	// -------------------------------------------------------------------------
	// Vitals
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Vitals", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Vitals")
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Vitals", meta = (ClampMin = "0.0"))
	float MaxToxicity = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Vitals")
	float Toxicity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Vitals", meta = (ClampMin = "0.0"))
	float ToxicityGainPerDamage = 0.15f;

	// -------------------------------------------------------------------------
	// State flags
	// -------------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|State")
	bool bIsStaggered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|State")
	bool bIsDismembered = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|State")
	bool bIsIncapacitated = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|State")
	bool bIsBlinded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|State")
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Mutation")
	bool bIsEnraged = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Mutation")
	bool bHasBioShield = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|WeakPoints")
	bool bLocomotorNervesDestroyed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|WeakPoints")
	bool bOpticalNodesDestroyed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|WeakPoints")
	bool bBioCoreDestroyed = false;

	// -------------------------------------------------------------------------
	// Reaction / mutation tuning
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Reactions", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float LocomotorSlowMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Reactions", meta = (ClampMin = "0.1"))
	float LocomotorSlowDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Reactions", meta = (ClampMin = "0.1"))
	float OpticalBlindDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Reactions", meta = (ClampMin = "0.1"))
	float StaggerDuration = 1.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Reactions", meta = (ClampMin = "0.1"))
	float DefaultWalkSpeed = 350.0f;

	/** Walk speed multiplier while enraged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation", meta = (ClampMin = "1.0"))
	float RageSpeedMultiplier = 1.55f;

	/** Incoming damage multiplier while enraged (hosts glass-cannon) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation", meta = (ClampMin = "0.1"))
	float RageIncomingDamageMultiplier = 1.25f;

	/** Fraction of damage absorbed while bio-shield is up (0–1) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BioShieldAbsorption = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation", meta = (ClampMin = "0.1"))
	float BioShieldDuration = 8.0f;

	/** Weak-point hits that count as "destruction" for phase-shift */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation")
	bool bDestroyWeakPointOnCriticalHit = true;

	/** Extra hearing range while enraged (applied to HostPerception) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "0.0"))
	float RageHearingBonus = 600.0f;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	FVector GetLastHeardNoiseLocation() const;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	FName GetLastHeardNoiseTag() const;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	AActor* GetLastHeardNoiseInstigator() const;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	bool HasRecentNoiseStimulus() const;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	bool HasSightOnPlayer() const;

	// -------------------------------------------------------------------------
	// Events
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostDamaged OnHostDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostStateChanged OnHostStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostDied OnHostDied;

	UPROPERTY(BlueprintAssignable, Category = "Host|AI")
	FOnProjectOrganoidHostNoiseHeard OnNoiseHeard;

	// -------------------------------------------------------------------------
	// IProjectOrganoidDamageable
	// -------------------------------------------------------------------------

	virtual EProjectOrganoidWeakPointType ResolveWeakPoint_Implementation(const FHitResult& Hit) const override;
	virtual void ApplyOrganoidHit_Implementation(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Host|Vitals")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? Health / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Host|Vitals")
	float GetToxicityPercent() const { return MaxToxicity > 0.0f ? Toxicity / MaxToxicity : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Host|State")
	void ClearStatusEffects();

	UFUNCTION(BlueprintCallable, Category = "Host|Mutation")
	void EnterRageState();

	UFUNCTION(BlueprintCallable, Category = "Host|Mutation")
	void ActivateBioShield();

	/** Overcharged pulse / denature — drops bio-shield immediately */
	UFUNCTION(BlueprintCallable, Category = "Host|Mutation")
	bool StripBioShield();

	UFUNCTION(BlueprintPure, Category = "Host|Mutation")
	bool HasBioShield() const { return bHasBioShield; }

	UFUNCTION(BlueprintPure, Category = "Host|Mutation")
	bool IsEnraged() const { return bIsEnraged; }

protected:

	float CachedWalkSpeed = 350.0f;

	FTimerHandle LocomotorSlowTimer;
	FTimerHandle OpticalBlindTimer;
	FTimerHandle StaggerTimer;
	FTimerHandle BioShieldTimer;

	void ConfigureWeakPointHitbox(USphereComponent* Hitbox, FName Tag, float Radius, FVector RelativeLocation);
	void SyncHostPerception();
	void ApplyLocomotorNerveReaction();
	void ApplyOpticalNodeReaction();
	void ApplyBioCoreReaction(bool bForceIncapacitate);
	void DestroyWeakPoint(EProjectOrganoidWeakPointType WeakPoint);
	void EvaluatePhaseShiftMutation(EProjectOrganoidWeakPointType DestroyedWeakPoint);
	void SetStaggered(bool bNewStaggered);
	void SetBlinded(bool bNewBlinded);
	void SetDismembered(bool bNewDismembered);
	void SetIncapacitated(bool bNewIncapacitated);
	void RestoreLocomotorSpeed();
	void RestoreOpticalSight();
	void ClearStagger();
	void ExpireBioShield();
	void RefreshMovementSpeed();
	void HandleDeath();

	UFUNCTION()
	void HandleHearingStimulus(AActor* Instigator, FName NoiseTag, EProjectOrganoidHearingStimulusKind Kind, FVector StimulusLocation, float Strength);

	UFUNCTION()
	void HandleSightStimulus(AActor* Target, bool bSensed, FVector StimulusLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Reactions")
	void BP_OnWeakPointReaction(EProjectOrganoidWeakPointType WeakPoint, const FProjectOrganoidBallisticHit& HitInfo);

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Mutation")
	void BP_OnRageStateEntered();

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Mutation")
	void BP_OnBioShieldChanged(bool bActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Reactions")
	void BP_OnHostDied();
};
