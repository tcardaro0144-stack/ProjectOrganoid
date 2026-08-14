// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidHostBase.generated.h"

class USphereComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHostDamaged, const FProjectOrganoidBallisticHit&, HitInfo, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHostStateChanged, FName, StateName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidHostDied);

/**
 *  Mutated organoid host base — weak-point hitboxes, vitals, status flags,
 *  and AI sight/hearing perception for Epitope facility enemies.
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

	/** Locomotor nerve cluster (movement / stagger target) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> LocomotorNervesHitbox;

	/** Optical node cluster (vision / blind target) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> OpticalNodesHitbox;

	/** Bio-Core / Organoid Core (incapacitation target) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|WeakPoints")
	TObjectPtr<USphereComponent> BioCoreHitbox;

	/** Sight + hearing perception */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

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

	/** Toxicity gained per point of organoid damage received */
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

	// -------------------------------------------------------------------------
	// Reaction tuning
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI")
	float SightRadius = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI")
	float LoseSightRadius = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI")
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI")
	float HearingRange = 1800.0f;

	// -------------------------------------------------------------------------
	// Events
	// -------------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostDamaged OnHostDamaged;

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostStateChanged OnHostStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Host|Events")
	FOnProjectOrganoidHostDied OnHostDied;

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

protected:

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	float CachedWalkSpeed = 350.0f;

	FTimerHandle LocomotorSlowTimer;
	FTimerHandle OpticalBlindTimer;
	FTimerHandle StaggerTimer;

	void ConfigureWeakPointHitbox(USphereComponent* Hitbox, FName Tag, float Radius, FVector RelativeLocation);
	void ConfigureAIPerception();
	void ApplyLocomotorNerveReaction();
	void ApplyOpticalNodeReaction();
	void ApplyBioCoreReaction(bool bForceIncapacitate);
	void SetStaggered(bool bNewStaggered);
	void SetBlinded(bool bNewBlinded);
	void SetDismembered(bool bNewDismembered);
	void SetIncapacitated(bool bNewIncapacitated);
	void RestoreLocomotorSpeed();
	void RestoreOpticalSight();
	void ClearStagger();
	void HandleDeath();

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Reactions")
	void BP_OnWeakPointReaction(EProjectOrganoidWeakPointType WeakPoint, const FProjectOrganoidBallisticHit& HitInfo);

	UFUNCTION(BlueprintImplementableEvent, Category = "Host|Reactions")
	void BP_OnHostDied();
};
