// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.generated.h"

class AProjectOrganoidCharacter;
class AProjectOrganoidAmbienceZone;
class UAudioComponent;
class USoundBase;
class USoundMix;
class USoundClass;
class UReverbEffect;

/** High-level music / ambient bed driven by combat, hazards, and vitals */
UENUM(BlueprintType)
enum class EProjectOrganoidAmbienceState : uint8
{
	Exploration UMETA(DisplayName = "Exploration"),
	Tension UMETA(DisplayName = "Tension"),
	Combat UMETA(DisplayName = "Combat"),
	Hazard UMETA(DisplayName = "Hazard"),
	CriticalHealth UMETA(DisplayName = "Critical Health")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidActiveAmbienceZone
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Audio|Zone")
	TWeakObjectPtr<AProjectOrganoidAmbienceZone> Zone;

	UPROPERTY(BlueprintReadOnly, Category = "Audio|Zone")
	FName ZoneId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Audio|Zone")
	int32 Priority = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidAmbienceStateChanged, EProjectOrganoidAmbienceState, NewState, EProjectOrganoidAmbienceState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidMusicIntensityChanged, float, Intensity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidAmbienceMixChanged, float, MixPitch, float, MixVolume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidAmbienceZoneChanged, FName, ZoneId, bool, bEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidOcclusionUpdated, float, OcclusionFactor);

/**
 *  Dynamic music + ambient director:
 *  - Combat stimuli raise intensity and push combat SoundMix / reverb
 *  - Hazard enter/exit swaps ambient beds and wet reverb
 *  - Health thresholds escalate tension → critical beds
 *  Layer volumes (Ambient / Tension / Combat / Critical) interpolate toward targets each tick.
 */
UCLASS()
class UProjectOrganoidAudioAmbienceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableInEditor() const override { return false; }

	UPROPERTY(BlueprintAssignable, Category = "Audio|Ambience")
	FOnProjectOrganoidAmbienceStateChanged OnAmbienceStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Audio|Ambience")
	FOnProjectOrganoidMusicIntensityChanged OnMusicIntensityChanged;

	UPROPERTY(BlueprintAssignable, Category = "Audio|Ambience")
	FOnProjectOrganoidAmbienceMixChanged OnAmbienceMixChanged;

	UPROPERTY(BlueprintAssignable, Category = "Audio|Ambience|Zone")
	FOnProjectOrganoidAmbienceZoneChanged OnAmbienceZoneChanged;

	UPROPERTY(BlueprintAssignable, Category = "Audio|Ambience|Occlusion")
	FOnProjectOrganoidOcclusionUpdated OnListenerOcclusionUpdated;

	// -------------------------------------------------------------------------
	// Binding / external stimuli
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience")
	void BindLocalPlayerCharacter(AProjectOrganoidCharacter* Character);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience")
	void UnbindLocalPlayerCharacter(AProjectOrganoidCharacter* Character = nullptr);

	/** Gunfire / host aggro — keeps combat state alive for CombatLingerSeconds */
	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Combat")
	void NotifyCombatStimulus(float Intensity = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Combat")
	void SetCombatActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Hazard")
	void NotifyHazardEntered(EProjectOrganoidHazardType HazardType, float Intensity = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Hazard")
	void NotifyHazardExited(EProjectOrganoidHazardType HazardType);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Vitals")
	void NotifyHealthChanged(float CurrentHealth, float MaxHealth);

	// -------------------------------------------------------------------------
	// Tunables
	// -------------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Combat", meta = (ClampMin = "0.5", ClampMax = "30.0"))
	float CombatLingerSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Combat", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CombatStimulusGain = 1.0f;

	/** Health fraction at or below which Tension state is forced (if not already Combat/Hazard/Critical) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Vitals", meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float TensionHealthThreshold = 0.55f;

	/** Health fraction at or below which CriticalHealth wins priority */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Vitals", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float CriticalHealthThreshold = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float LayerInterpSpeed = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience", meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float MixInterpSpeed = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float IntensityBroadcastEpsilon = 0.02f;

	// Soft content — assign in BP defaults / DataAsset overrides when available
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Layers")
	TSoftObjectPtr<USoundBase> AmbientLayerSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Layers")
	TSoftObjectPtr<USoundBase> TensionLayerSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Layers")
	TSoftObjectPtr<USoundBase> CombatLayerSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Layers")
	TSoftObjectPtr<USoundBase> CriticalLayerSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Mix")
	TSoftObjectPtr<USoundMix> ExplorationSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Mix")
	TSoftObjectPtr<USoundMix> CombatSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Mix")
	TSoftObjectPtr<USoundMix> HazardSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Mix")
	TSoftObjectPtr<USoundMix> CriticalSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Mix")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb")
	TSoftObjectPtr<UReverbEffect> ExplorationReverb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb")
	TSoftObjectPtr<UReverbEffect> CombatReverb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb")
	TSoftObjectPtr<UReverbEffect> HazardReverb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb")
	TSoftObjectPtr<UReverbEffect> CriticalReverb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReverbVolume = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Reverb", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReverbFadeTime = 0.75f;

	/** WorldStatic / WorldDynamic channels block sound when tracing listener → source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Occlusion")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxOcclusionAttenuation = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Occlusion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxOcclusionLowPass = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Ambience|Occlusion", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float OcclusionTickInterval = 0.15f;

	// -------------------------------------------------------------------------
	// Environment zones / occlusion
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Zone")
	void RegisterAmbienceZone(AProjectOrganoidAmbienceZone* Zone);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Zone")
	void UnregisterAmbienceZone(AProjectOrganoidAmbienceZone* Zone);

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Zone")
	FName GetActiveEnvironmentZoneId() const { return ActiveEnvironmentZoneId; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Zone")
	AProjectOrganoidAmbienceZone* GetActiveEnvironmentZone() const;

	/**
	 *  Trace listener → source through room geometry.
	 *  Returns 0 (clear) … 1 (fully occluded).
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Occlusion")
	float EvaluateSoundOcclusion(FVector ListenerLocation, FVector SourceLocation, AActor* IgnoreActor = nullptr) const;

	/** Apply volume / LPF scaling to a spatial audio component from an occlusion factor */
	UFUNCTION(BlueprintCallable, Category = "Audio|Ambience|Occlusion")
	void ApplyOcclusionToAudioComponent(UAudioComponent* AudioComponent, float OcclusionFactor, float BaseVolume = 1.0f) const;

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Occlusion")
	float GetListenerOcclusionFactor() const { return ListenerOcclusionFactor; }

	// -------------------------------------------------------------------------
	// Queries
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	EProjectOrganoidAmbienceState GetAmbienceState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetMusicIntensity() const { return CurrentMusicIntensity; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Combat")
	bool IsInCombat() const { return bCombatActive; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Hazard")
	bool IsInHazard() const { return ActiveHazardCount > 0; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Hazard")
	EProjectOrganoidHazardType GetPrimaryHazardType() const { return PrimaryHazardType; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience|Vitals")
	float GetHealthNormalized() const { return HealthNormalized; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetAmbientLayerVolume() const { return AmbientLayerVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetTensionLayerVolume() const { return TensionLayerVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetCombatLayerVolume() const { return CombatLayerVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetCriticalLayerVolume() const { return CriticalLayerVolume; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetMixPitch() const { return CurrentMixPitch; }

	UFUNCTION(BlueprintPure, Category = "Audio|Ambience")
	float GetMixVolume() const { return CurrentMixVolume; }

protected:

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY()
	TObjectPtr<UAudioComponent> AmbientLayerAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> TensionLayerAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CombatLayerAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CriticalLayerAudio;

	EProjectOrganoidAmbienceState CurrentState = EProjectOrganoidAmbienceState::Exploration;
	EProjectOrganoidHazardType PrimaryHazardType = EProjectOrganoidHazardType::None;

	bool bCombatActive = false;
	float CombatTimerRemaining = 0.0f;
	float CombatIntensity = 0.0f;
	int32 ActiveHazardCount = 0;
	float HazardIntensity = 0.0f;
	float HealthNormalized = 1.0f;

	float AmbientLayerVolume = 0.0f;
	float TensionLayerVolume = 0.0f;
	float CombatLayerVolume = 0.0f;
	float CriticalLayerVolume = 0.0f;

	float TargetAmbientVolume = 1.0f;
	float TargetTensionVolume = 0.0f;
	float TargetCombatVolume = 0.0f;
	float TargetCriticalVolume = 0.0f;

	float CurrentMusicIntensity = 0.0f;
	float LastBroadcastIntensity = -1.0f;

	float CurrentMixPitch = 1.0f;
	float CurrentMixVolume = 1.0f;
	float TargetMixPitch = 1.0f;
	float TargetMixVolume = 1.0f;

	FName ActiveReverbTag = NAME_None;
	TWeakObjectPtr<USoundMix> ActivePushedMix;

	UPROPERTY()
	TArray<FProjectOrganoidActiveAmbienceZone> ActiveZones;

	FName ActiveEnvironmentZoneId = NAME_None;
	FName ActiveEnvironmentReverbTag = NAME_None;
	float ListenerOcclusionFactor = 0.0f;
	float OcclusionTickAccumulator = 0.0f;

	UPROPERTY()
	TObjectPtr<UAudioComponent> RoomToneAudio;

	AProjectOrganoidCharacter* ResolveLocalCharacter() const;
	EProjectOrganoidAmbienceState EvaluateDesiredState() const;
	void ApplyAmbienceState(EProjectOrganoidAmbienceState NewState, bool bForce = false);
	void UpdateTargetsForState(EProjectOrganoidAmbienceState State);
	void UpdateLayerVolumes(float DeltaTime);
	void UpdateMixParameters(float DeltaTime);
	void EnsureMusicLayers(AProjectOrganoidCharacter* Character);
	void SyncLayerComponent(TObjectPtr<UAudioComponent>& Component, AProjectOrganoidCharacter* Character, const TSoftObjectPtr<USoundBase>& SoftSound, const TCHAR* ComponentName, float Volume);
	void PushStateSoundMix(EProjectOrganoidAmbienceState State);
	void PopActiveSoundMix();
	void ApplyStateReverb(EProjectOrganoidAmbienceState State);
	void ClearActiveReverb();
	void RefreshEnvironmentReverbFromZones();
	void ClearEnvironmentReverb();
	void UpdateRoomToneForActiveZone();
	void UpdateListenerOcclusion(float DeltaTime);
	float ComputeMusicIntensity() const;
};
