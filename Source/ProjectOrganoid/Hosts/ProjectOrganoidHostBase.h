// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidPerceptionTypes.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidHostBase.generated.h"

class USphereComponent;
class UProjectOrganoidPerceptionComponent;
class UProjectOrganoidHitReactionComponent;
class AActor;
class AProjectOrganoidCharacter;
class AProjectOrganoidHostAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHostDamaged, const FProjectOrganoidBallisticHit&, HitInfo, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHostStateChanged, FName, StateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidHostDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHostNoiseHeard, AActor*, NoiseInstigator, FName, NoiseTag);

/**
 *  Mutated organoid host — weak points, phase-shift mutations (rage / bio-shield),
 *  and AI sight + hearing for footstep / gunfire noise.
 */
UCLASS(Abstract, Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidHostBase : public ACharacter, public IProjectOrganoidDamageable
{
	GENERATED_BODY()

public:

	AProjectOrganoidHostBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Combat")
	TObjectPtr<UProjectOrganoidHitReactionComponent> HitReaction;

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

	/** Per-instance opt-out for authored Hosts that must never rage or raise a bio-shield. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Mutation")
	bool bAllowPhaseShiftMutations = true;

	/** Extra hearing range while enraged (applied to HostPerception) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "0.0"))
	float RageHearingBonus = 600.0f;

	// -------------------------------------------------------------------------
	// Authored encounter activation
	// -------------------------------------------------------------------------

	/** Opt-in gate. False preserves the legacy Host behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Activation")
	bool bRequiresEncounterActivation = false;

	/** A currently perceived player at or inside this 2D range permanently activates the Host. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Activation", meta = (ClampMin = "50.0", EditCondition = "bRequiresEncounterActivation"))
	float ProximityActivationRange = 200.0f;

	/** Runtime state. Activation is intentionally one-way for the lifetime of this Host. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Host|Activation")
	bool bEncounterActivated = false;

	UFUNCTION(BlueprintPure, Category = "Host|Activation")
	bool IsEncounterActivated() const { return !bRequiresEncounterActivation || bEncounterActivated; }

	/** Permanently opens the authored activation gate. Returns true only on the transition. */
	UFUNCTION(BlueprintCallable, Category = "Host|Activation")
	bool ActivateEncounter();

	/** Activates only for a player-controlled Project Organoid character inside the configured range. */
	bool TryActivateEncounterFromProximity(const AActor* Target);

	/** Gunfire and explicitly tagged generic noise activate; locomotion footsteps do not. */
	bool IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind Kind) const;

	/**
	 * Optional campaign lesson (default off). When RequiredActiveObjectiveId is set:
	 * encounter activation requires that objective Active; completed objective never rearms;
	 * a matching tactical weak-point hit that actually applies the existing impairment effect
	 * fires LessonSuccessObjectiveEventId once and presents the one-shot response.
	 * Legacy Hosts with RequiredActiveObjectiveId=None are unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	FName RequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	EProjectOrganoidWeakPointType RequiredLessonWeakPoint = EProjectOrganoidWeakPointType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	bool bLessonRequiresTacticalMode = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	FName LessonSuccessObjectiveEventId = NAME_None;

	/** When this objective is Completed, lesson credit/response will not re-fire or rearm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	FName LessonCompletedObjectiveReplayGuardId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	FText LessonSuccessNotificationSpeaker = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson")
	FText LessonSuccessNotificationText = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Lesson", meta = (ClampMin = "0.0"))
	float LessonSuccessNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Lesson|Diagnostics", Transient)
	int32 LessonSuccessNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|Lesson|Diagnostics", Transient)
	int32 LessonSuccessEventFireCount = 0;

	/**
	 * Optional adaptation lesson (default off). Empty required objective preserves existing Hosts.
	 * Encounter activation requires that objective Active. A successful biological effect on this Host,
	 * while that objective is Active and the equipped adaptation matches, fires the event once.
	 * A completed replay-guard objective never rearms the encounter or the response.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	FName AdaptationCampaignRequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	TSoftObjectPtr<UProjectOrganoidBiologicalAdaptationData> AdaptationCampaignRequiredAdaptation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	FName AdaptationCampaignSuccessEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	FName AdaptationCampaignReplayGuardObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	FText AdaptationCampaignNotificationSpeaker = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign")
	FText AdaptationCampaignNotificationText = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AdaptationCampaign", meta = (ClampMin = "0.0"))
	float AdaptationCampaignNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|AdaptationCampaign|Diagnostics", Transient)
	int32 AdaptationCampaignNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|AdaptationCampaign|Diagnostics", Transient)
	int32 AdaptationCampaignEventFireCount = 0;

	/** Called only after a biological adaptation successfully applies its effect to this Host. */
	void NotifySuccessfulBiologicalAdaptation(AProjectOrganoidCharacter* Character);

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

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	AActor* GetCurrentSightTarget() const;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	FVector GetLastKnownPlayerLocation() const { return LastKnownPlayerLocation; }

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	bool HasLastKnownPlayerLocation() const { return bHasLastKnownPlayerLocation; }

	void RememberPerceivedPlayerLocation(const FVector& WorldLocation);
	void BroadcastCombatState(EProjectOrganoidHostCombatState NewState);
	void BindHostPerceptionToController();

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	EProjectOrganoidHostCombatState GetCombatState() const;

	UFUNCTION(BlueprintPure, Category = "Host|Combat")
	bool CanAttemptMelee() const;

	UFUNCTION(BlueprintPure, Category = "Host|Combat")
	bool IsTargetInMeleeRange(const AActor* Target, float ExtraRange = 0.0f) const;

	UFUNCTION(BlueprintPure, Category = "Host|Combat")
	bool HasValidMeleeLineOfSight(const AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category = "Host|Combat")
	bool TryBeginMeleeAttack(AProjectOrganoidCharacter* Target);

	UFUNCTION(BlueprintCallable, Category = "Host|Combat")
	void CancelMeleeAttack();

	UFUNCTION(BlueprintPure, Category = "Host|Combat")
	bool IsMeleeWindupActive() const { return bMeleeWindupActive; }

	UFUNCTION(BlueprintPure, Category = "Host|Combat")
	bool DidLastMeleeCommitDamage() const { return bLastMeleeCommitDealtDamage; }

	// -------------------------------------------------------------------------
	// Melee (provisional, designer-tunable)
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "50.0"))
	float MeleeAttackRange = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "1.0"))
	float MeleeCommitRangeSlack = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "0.05"))
	float MeleeWindupSeconds = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "0.1"))
	float MeleeCooldownSeconds = 1.60f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "1.0"))
	float MeleeDamage = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|Combat", meta = (ClampMin = "10.0"))
	float MeleeSweepRadius = 48.0f;

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

	/** Direct hit application for automation. Same path as IProjectOrganoidDamageable. */
	UFUNCTION(BlueprintCallable, Category = "Host|Combat")
	void ApplyResolvedOrganoidHit(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser);

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

	/**
	 *  Temporary locomotor reduction for Biological Adaptations (Neural Slow).
	 *  Does not stagger, strip bio-shield, destroy weak points, or change AI.
	 *  Duration is wall-clock. SpeedMultiplier 0.6 = 40% reduction.
	 */
	UFUNCTION(BlueprintCallable, Category = "Host|Biological")
	bool ApplyBiologicalLocomotorSlow(float SpeedMultiplier, float DurationSeconds);

	UFUNCTION(BlueprintPure, Category = "Host|Biological")
	bool IsBiologicalLocomotorSlowActive() const { return bBiologicalLocomotorSlowActive; }

	UFUNCTION(BlueprintPure, Category = "Host|Biological")
	float GetBiologicalLocomotorSlowMultiplier() const { return BiologicalLocomotorSlowMultiplier; }

	/**
	 * Temporary optical-node blind for Biological Adaptations.
	 * Does not stagger, damage, strip bio-shield, or destroy the optical nodes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Host|Biological")
	bool ApplyBiologicalOpticalBlind(float DurationSeconds);

	UFUNCTION(BlueprintPure, Category = "Host|Biological")
	bool IsBiologicalOpticalBlindActive() const { return bBiologicalOpticalBlindActive; }

protected:

	float CachedWalkSpeed = 350.0f;

	FTimerHandle LocomotorSlowTimer;
	FTimerHandle OpticalBlindTimer;
	bool bBiologicalLocomotorSlowActive = false;
	bool bBiologicalOpticalBlindActive = false;
	float BiologicalLocomotorSlowMultiplier = 1.0f;
	double BiologicalLocomotorSlowExpireRealTime = 0.0;
	FTimerHandle StaggerTimer;
	FTimerHandle BioShieldTimer;
	FTimerHandle MeleeWindupTimer;
	FTimerHandle MeleeCooldownTimer;

	FVector LastKnownPlayerLocation = FVector::ZeroVector;
	bool bHasLastKnownPlayerLocation = false;
	bool bMeleeWindupActive = false;
	bool bMeleeOnCooldown = false;
	bool bLastMeleeCommitDealtDamage = false;
	TWeakObjectPtr<AProjectOrganoidCharacter> MeleeTarget;

	void EnsureHostAIController();
	void NotifyAIPreempt();
	void CommitMeleeAttack();
	AProjectOrganoidCharacter* ResolveMeleeSweepTarget() const;

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
	void ClearBiologicalLocomotorSlow();
	void RestoreOpticalSight();
	void ClearStagger();
	void ExpireBioShield();
	void RefreshMovementSpeed();
	void HandleDeath();

	bool IsLessonContractConfigured() const;
	bool IsLessonObjectiveActive() const;
	bool IsLessonReplayGuardCompleted() const;
	bool CanOpenEncounterActivationGate() const;
	bool HasAppliedLessonWeakPointEffect(const FProjectOrganoidBallisticHit& HitInfo) const;
	void TryAwardBiologicalTargetingLesson(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser);
	void NotifyLessonObjectiveEvent(FName EventId) const;
	void PresentLessonSuccessNotification(AActor* DamageCauser);
	void SyncLessonCompletedFromObjectives();
	bool IsAdaptationCampaignConfigured() const;
	bool IsAdaptationCampaignObjectiveActive() const;
	bool IsAdaptationCampaignReplayGuardCompleted() const;
	void PresentAdaptationCampaignNotification(AProjectOrganoidCharacter* Character);
	void SyncAdaptationCampaignCompletedFromObjectives();

	UFUNCTION()
	void HandleHearingStimulus(AActor* NoiseInstigator, FName NoiseTag, EProjectOrganoidHearingStimulusKind Kind, FVector StimulusLocation, float Strength);

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
