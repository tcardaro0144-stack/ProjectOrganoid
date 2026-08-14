// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidFeedbackComponent.generated.h"

class UAudioComponent;
class UPostProcessComponent;
class USoundBase;
class UMaterialInterface;
class UParticleSystem;
class AProjectOrganoidCharacter;
class AProjectOrganoidWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidFeedbackWeakPoint, const FProjectOrganoidBallisticHit&, HitInfo);

/**
 *  Avery Vance diegetic audio / visual feedback:
 *  BPM-scaled heartbeat & breathing, toxicity / low-PE post-process,
 *  and weak-point hit SFX / VFX.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// -------------------------------------------------------------------------
	// Audio — heartbeat / breathing
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio")
	TObjectPtr<USoundBase> HeartbeatSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio")
	TObjectPtr<USoundBase> BreathingSound;

	/** BPM mapped to this pitch at resting heart rate */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "40.0", ClampMax = "120.0"))
	float RestingBPMReference = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "0.25", ClampMax = "1.0"))
	float MinHeartbeatPitch = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float MaxHeartbeatPitch = 1.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "0.25", ClampMax = "1.0"))
	float MinBreathingPitch = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "1.0", ClampMax = "2.5"))
	float MaxBreathingPitch = 1.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HeartbeatVolume = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BreathingVolume = 0.35f;

	// -------------------------------------------------------------------------
	// Post-process — toxicity / low PE
	// -------------------------------------------------------------------------

	/** Screen effect material blended in as toxicity rises */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess")
	TObjectPtr<UMaterialInterface> HighToxicityPostProcessMaterial;

	/** Screen effect material blended in as PE energy drops */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess")
	TObjectPtr<UMaterialInterface> LowPEPostProcessMaterial;

	/** Toxicity % (0–1 of MaxToxicity) where PP begins */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ToxicityEffectStartNormalized = 0.35f;

	/** PE energy % (0–1) at/below which low-PE PP begins */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LowPEEffectStartNormalized = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxToxicityBlendWeight = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxLowPEBlendWeight = 0.70f;

	// -------------------------------------------------------------------------
	// Weak-point impact SFX / VFX
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<USoundBase> LocomotorNerveHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<USoundBase> OpticalNodeHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<USoundBase> BioCoreHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<USoundBase> DefaultWeakPointHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<UParticleSystem> LocomotorNerveHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<UParticleSystem> OpticalNodeHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<UParticleSystem> BioCoreHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	TObjectPtr<UParticleSystem> DefaultWeakPointHitVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Feedback|WeakPoint")
	float WeakPointVFXScale = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Feedback|WeakPoint")
	FOnProjectOrganoidFeedbackWeakPoint OnWeakPointFeedbackPlayed;

	/** Manually fire weak-point feedback (also bound to equipped weapon) */
	UFUNCTION(BlueprintCallable, Category = "Feedback|WeakPoint")
	void PlayWeakPointImpact(const FProjectOrganoidBallisticHit& HitInfo);

	UFUNCTION(BlueprintCallable, Category = "Feedback")
	void RefreshWeaponBinding();

	UFUNCTION(BlueprintImplementableEvent, Category = "Feedback|WeakPoint")
	void BP_OnWeakPointImpact(const FProjectOrganoidBallisticHit& HitInfo);

protected:

	UPROPERTY()
	TObjectPtr<AProjectOrganoidCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAudioComponent> HeartbeatAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BreathingAudio;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> FeedbackPostProcess;

	UPROPERTY()
	TObjectPtr<AProjectOrganoidWeapon> BoundWeapon;

	int32 ToxicityBlendableIndex = INDEX_NONE;
	int32 LowPEBlendableIndex = INDEX_NONE;

	void EnsureAudioComponents();
	void EnsurePostProcessComponent();
	void UpdateVitalAudio(float DeltaTime);
	void UpdatePostProcessEffects();
	void UnbindWeaponFeedback();

	UFUNCTION()
	void HandleWeakPointReaction(const FProjectOrganoidBallisticHit& HitInfo);

	float ComputePitchFromBPM(float BPM, float MinPitch, float MaxPitch) const;
	USoundBase* SelectWeakPointSound(EProjectOrganoidWeakPointType WeakPoint) const;
	UParticleSystem* SelectWeakPointVFX(EProjectOrganoidWeakPointType WeakPoint) const;
};
