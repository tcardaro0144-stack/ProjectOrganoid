// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidAudioSubsystem.generated.h"

class AProjectOrganoidCharacter;
class USoundBase;
class UPostProcessComponent;
class UAudioComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidSpatialAudioTriggered, FVector, Location, FName, NoiseTag, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidBPMChanged, float, NewBPM);

/**
 *  World audio / post-process director:
 *  - Dynamic BPM heart-rate scaling from Avery's health & toxicity
 *  - 3D footstep / gunfire cues that feed host AI hearing
 *  - Toxic-gas screen distortion + tactical desaturation
 */
UCLASS()
class UProjectOrganoidAudioSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableInEditor() const override { return false; }

	UPROPERTY(BlueprintAssignable, Category = "Audio")
	FOnProjectOrganoidSpatialAudioTriggered OnSpatialAudioTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Audio|BPM")
	FOnProjectOrganoidBPMChanged OnBPMChanged;

	// -------------------------------------------------------------------------
	// BPM / vitals audio
	// -------------------------------------------------------------------------

	/** Resting BPM when Avery is healthy and clean */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "40.0", ClampMax = "120.0"))
	float RestingBPM = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "80.0", ClampMax = "220.0"))
	float MaxStressBPM = 180.0f;

	/** How strongly low health raises BPM (0–1 health → this delta) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "0.0", ClampMax = "120.0"))
	float HealthStressBPM = 55.0f;

	/** How strongly toxicity raises BPM (0–1 toxicity → this delta) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "0.0", ClampMax = "120.0"))
	float ToxicityStressBPM = 45.0f;

	/** Extra BPM while tactical mode is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "0.0", ClampMax = "40.0"))
	float TacticalModeBPMBoost = 12.0f;

	/** Interp speed when drifting HeartRate toward the computed target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float BPMInterpSpeed = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM")
	TObjectPtr<USoundBase> HeartbeatLoopSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|BPM", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HeartbeatVolume = 0.6f;

	UFUNCTION(BlueprintPure, Category = "Audio|BPM")
	float ComputeTargetBPM(const AProjectOrganoidCharacter* Character) const;

	UFUNCTION(BlueprintPure, Category = "Audio|BPM")
	float GetCurrentManagedBPM() const { return CurrentManagedBPM; }

	UFUNCTION(BlueprintCallable, Category = "Audio|BPM")
	void BindLocalPlayerCharacter(AProjectOrganoidCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Audio|BPM")
	void UnbindLocalPlayerCharacter(AProjectOrganoidCharacter* Character = nullptr);

	// -------------------------------------------------------------------------
	// Spatialized AI-perceivable audio
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial")
	TObjectPtr<USoundBase> FootstepSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial")
	TObjectPtr<USoundBase> GunfireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Footstep", meta = (ClampMin = "0.0"))
	float FootstepNoiseLoudness = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Footstep", meta = (ClampMin = "100.0"))
	float FootstepNoiseMaxRange = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Footstep", meta = (ClampMin = "0.05"))
	float FootstepIntervalSeconds = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Footstep", meta = (ClampMin = "1.0"))
	float FootstepMinSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Gunfire", meta = (ClampMin = "0.0"))
	float DefaultGunfireNoiseLoudness = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Spatial|Gunfire", meta = (ClampMin = "100.0"))
	float DefaultGunfireNoiseMaxRange = 3500.0f;

	/**
	 *  Play a 3D footstep at Location and report AI hearing.
	 *  NoiseTag defaults to Footstep; pass Footstep_Walk / Footstep_Run / Footstep_Crouch for hosts.
	 *  Returns false if throttled by FootstepIntervalSeconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Spatial")
	bool PlayFootstepAtLocation(
		const FVector& Location,
		AActor* Instigator,
		float LoudnessOverride = -1.0f,
		bool bIgnoreInterval = false,
		FName NoiseTag = NAME_None,
		float MaxRangeOverride = -1.0f);

	/** Play 3D gunfire and report AI hearing (tag: Gunfire). */
	UFUNCTION(BlueprintCallable, Category = "Audio|Spatial")
	void PlayGunfireAtLocation(const FVector& Location, AActor* Instigator, float LoudnessOverride = -1.0f, float MaxRangeOverride = -1.0f);

	/** Drive footsteps from Avery's movement speed (call from character Tick). */
	UFUNCTION(BlueprintCallable, Category = "Audio|Spatial")
	void UpdatePlayerFootsteps(AProjectOrganoidCharacter* Character, float DeltaTime);

	// -------------------------------------------------------------------------
	// Post-process — toxic gas + tactical desaturation
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|PostProcess", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float MaxToxicGasFringeIntensity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxToxicGasVignette = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxToxicGasGrain = 0.35f;

	/** Color saturation at full tactical desaturation (0 = grey, 1 = full color) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|PostProcess", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TacticalMinColorSaturation = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|PostProcess", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float PostProcessInterpSpeed = 6.0f;

	UFUNCTION(BlueprintCallable, Category = "Audio|PostProcess")
	void SetToxicGasDistortion(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Audio|PostProcess")
	void AddToxicGasDistortion(float DeltaIntensity);

	UFUNCTION(BlueprintCallable, Category = "Audio|PostProcess")
	void SetTacticalDesaturation(float Intensity);

	UFUNCTION(BlueprintCallable, Category = "Audio|PostProcess")
	void NotifyTacticalModeChanged(bool bIsActive);

	UFUNCTION(BlueprintPure, Category = "Audio|PostProcess")
	float GetToxicGasDistortion() const { return ToxicGasDistortion; }

	UFUNCTION(BlueprintPure, Category = "Audio|PostProcess")
	float GetTacticalDesaturation() const { return TacticalDesaturation; }

protected:

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> ManagedPostProcess;

	UPROPERTY()
	TObjectPtr<UAudioComponent> HeartbeatAudio;

	float CurrentManagedBPM = 72.0f;
	float ToxicGasDistortion = 0.0f;
	float TacticalDesaturation = 0.0f;
	float ToxicGasDistortionTarget = 0.0f;
	float TacticalDesaturationTarget = 0.0f;
	float FootstepCooldownRemaining = 0.0f;
	float LastBroadcastBPM = -1.0f;

	void EnsurePostProcessComponent(AProjectOrganoidCharacter* Character);
	void EnsureHeartbeatAudio(AProjectOrganoidCharacter* Character);
	void UpdateBPMFromCharacter(AProjectOrganoidCharacter* Character, float DeltaTime);
	void UpdateHeartbeatAudio();
	void UpdatePostProcessSettings(float DeltaTime);
	void ReportSpatialNoise(const FVector& Location, AActor* Instigator, float Loudness, float MaxRange, FName NoiseTag) const;
	AProjectOrganoidCharacter* ResolveLocalCharacter() const;
};
