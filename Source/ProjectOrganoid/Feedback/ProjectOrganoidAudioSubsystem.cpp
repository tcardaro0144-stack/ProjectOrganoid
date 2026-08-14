// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAudioSubsystem.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPerceptionComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Sound/SoundBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"

void UProjectOrganoidAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentManagedBPM = RestingBPM;
}

void UProjectOrganoidAudioSubsystem::Deinitialize()
{
	UnbindLocalPlayerCharacter(nullptr);

	if (HeartbeatAudio)
	{
		HeartbeatAudio->Stop();
		HeartbeatAudio->DestroyComponent();
		HeartbeatAudio = nullptr;
	}

	if (ManagedPostProcess)
	{
		ManagedPostProcess->DestroyComponent();
		ManagedPostProcess = nullptr;
	}

	Super::Deinitialize();
}

TStatId UProjectOrganoidAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectOrganoidAudioSubsystem, STATGROUP_Tickables);
}

void UProjectOrganoidAudioSubsystem::Tick(float DeltaTime)
{
	AProjectOrganoidCharacter* Character = BoundCharacter.IsValid()
		? BoundCharacter.Get()
		: ResolveLocalCharacter();

	if (!Character)
	{
		return;
	}

	if (!BoundCharacter.IsValid())
	{
		BindLocalPlayerCharacter(Character);
	}

	UpdateBPMFromCharacter(Character, DeltaTime);
	UpdatePlayerFootsteps(Character, DeltaTime);
	UpdatePostProcessSettings(DeltaTime);
}

void UProjectOrganoidAudioSubsystem::BindLocalPlayerCharacter(AProjectOrganoidCharacter* Character)
{
	BoundCharacter = Character;
	if (Character)
	{
		EnsurePostProcessComponent(Character);
		EnsureHeartbeatAudio(Character);
		CurrentManagedBPM = Character->GetHeartRate();
		NotifyTacticalModeChanged(Character->IsTacticalModeActive());
	}
}

void UProjectOrganoidAudioSubsystem::UnbindLocalPlayerCharacter(AProjectOrganoidCharacter* Character)
{
	if (Character && BoundCharacter.Get() != Character)
	{
		return;
	}

	BoundCharacter.Reset();
}

AProjectOrganoidCharacter* UProjectOrganoidAudioSubsystem::ResolveLocalCharacter() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
	{
		return Cast<AProjectOrganoidCharacter>(Pawn);
	}
	return nullptr;
}

float UProjectOrganoidAudioSubsystem::ComputeTargetBPM(const AProjectOrganoidCharacter* Character) const
{
	if (!Character)
	{
		return RestingBPM;
	}

	const float MaxHealth = FMath::Max(Character->GetMaxHealth(), 1.0f);
	const float MaxTox = FMath::Max(Character->GetMaxToxicity(), 1.0f);
	const float HealthNorm = FMath::Clamp(Character->GetHealth() / MaxHealth, 0.0f, 1.0f);
	const float ToxNorm = FMath::Clamp(Character->GetToxicity() / MaxTox, 0.0f, 1.0f);

	// Low health and high toxicity both push heart rate up.
	const float HealthStress = (1.0f - HealthNorm) * HealthStressBPM;
	const float ToxStress = ToxNorm * ToxicityStressBPM;
	const float TacticalBoost = Character->IsTacticalModeActive() ? TacticalModeBPMBoost : 0.0f;

	return FMath::Clamp(RestingBPM + HealthStress + ToxStress + TacticalBoost, RestingBPM, MaxStressBPM);
}

void UProjectOrganoidAudioSubsystem::UpdateBPMFromCharacter(AProjectOrganoidCharacter* Character, float DeltaTime)
{
	if (!Character)
	{
		return;
	}

	const float TargetBPM = ComputeTargetBPM(Character);
	CurrentManagedBPM = FMath::FInterpTo(CurrentManagedBPM, TargetBPM, DeltaTime, BPMInterpSpeed);

	const float Current = Character->GetHeartRate();
	const float Next = FMath::FInterpTo(Current, CurrentManagedBPM, DeltaTime, BPMInterpSpeed);
	Character->ApplyHeartRateDelta(Next - Current);

	if (!FMath::IsNearlyEqual(LastBroadcastBPM, CurrentManagedBPM, 0.5f))
	{
		LastBroadcastBPM = CurrentManagedBPM;
		OnBPMChanged.Broadcast(CurrentManagedBPM);
	}

	EnsureHeartbeatAudio(Character);
	UpdateHeartbeatAudio();
}

void UProjectOrganoidAudioSubsystem::EnsureHeartbeatAudio(AProjectOrganoidCharacter* Character)
{
	if (!Character || !HeartbeatLoopSound)
	{
		return;
	}

	if (!HeartbeatAudio)
	{
		HeartbeatAudio = NewObject<UAudioComponent>(Character, TEXT("OrganoidManagedHeartbeat"));
		HeartbeatAudio->SetupAttachment(Character->GetRootComponent());
		HeartbeatAudio->bAutoActivate = false;
		HeartbeatAudio->bUISound = true;
		HeartbeatAudio->RegisterComponent();
	}

	if (HeartbeatAudio->GetSound() != HeartbeatLoopSound)
	{
		HeartbeatAudio->SetSound(HeartbeatLoopSound);
	}

	if (!HeartbeatAudio->IsPlaying())
	{
		HeartbeatAudio->Play();
	}
}

void UProjectOrganoidAudioSubsystem::UpdateHeartbeatAudio()
{
	if (!HeartbeatAudio)
	{
		return;
	}

	const float SafeRest = FMath::Max(RestingBPM, 1.0f);
	const float Normalized = FMath::Clamp(CurrentManagedBPM / SafeRest, 0.5f, 2.5f);
	const float Pitch = FMath::Lerp(0.75f, 1.85f, FMath::Clamp((Normalized - 0.5f) / 2.0f, 0.0f, 1.0f));
	const float Stress = FMath::Clamp((CurrentManagedBPM - RestingBPM) / FMath::Max(MaxStressBPM - RestingBPM, 1.0f), 0.0f, 1.0f);

	HeartbeatAudio->SetPitchMultiplier(Pitch);
	HeartbeatAudio->SetVolumeMultiplier(HeartbeatVolume * (0.55f + Stress * 0.85f));
}

bool UProjectOrganoidAudioSubsystem::PlayFootstepAtLocation(
	const FVector& Location,
	AActor* Instigator,
	float LoudnessOverride,
	bool bIgnoreInterval,
	FName NoiseTag,
	float MaxRangeOverride)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!bIgnoreInterval)
	{
		if (FootstepCooldownRemaining > 0.0f)
		{
			return false;
		}
		FootstepCooldownRemaining = FootstepIntervalSeconds;
	}

	const float Loudness = LoudnessOverride >= 0.0f ? LoudnessOverride : FootstepNoiseLoudness;
	const float MaxRange = MaxRangeOverride >= 0.0f ? MaxRangeOverride : FootstepNoiseMaxRange;
	const FName ResolvedTag = NoiseTag.IsNone() ? ProjectOrganoidNoiseTags::Footstep : NoiseTag;

	if (FootstepSound)
	{
		float Volume = 1.0f;
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			if (AProjectOrganoidCharacter* Listener = BoundCharacter.Get())
			{
				const float Occlusion = Ambience->EvaluateSoundOcclusion(
					Listener->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f),
					Location,
					Instigator);
				Volume = 1.0f - (Occlusion * Ambience->MaxOcclusionAttenuation);
			}
		}
		UGameplayStatics::PlaySoundAtLocation(World, FootstepSound, Location, Volume, 1.0f, 0.0f);
	}

	ReportSpatialNoise(Location, Instigator, Loudness, MaxRange, ResolvedTag);
	OnSpatialAudioTriggered.Broadcast(Location, ResolvedTag, Instigator);
	return true;
}

void UProjectOrganoidAudioSubsystem::PlayGunfireAtLocation(
	const FVector& Location,
	AActor* Instigator,
	float LoudnessOverride,
	float MaxRangeOverride)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Loudness = LoudnessOverride >= 0.0f ? LoudnessOverride : DefaultGunfireNoiseLoudness;
	const float MaxRange = MaxRangeOverride >= 0.0f ? MaxRangeOverride : DefaultGunfireNoiseMaxRange;

	if (GunfireSound)
	{
		float Volume = 1.0f;
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			if (AProjectOrganoidCharacter* Listener = BoundCharacter.Get())
			{
				const float Occlusion = Ambience->EvaluateSoundOcclusion(
					Listener->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f),
					Location,
					Instigator);
				Volume = 1.0f - (Occlusion * Ambience->MaxOcclusionAttenuation);
			}
		}
		UGameplayStatics::PlaySoundAtLocation(World, GunfireSound, Location, Volume, 1.0f, 0.0f);
	}

	ReportSpatialNoise(Location, Instigator, Loudness, MaxRange, ProjectOrganoidNoiseTags::Gunfire);
	OnSpatialAudioTriggered.Broadcast(Location, ProjectOrganoidNoiseTags::Gunfire, Instigator);
}

void UProjectOrganoidAudioSubsystem::UpdatePlayerFootsteps(AProjectOrganoidCharacter* Character, float DeltaTime)
{
	FootstepCooldownRemaining = FMath::Max(0.0f, FootstepCooldownRemaining - DeltaTime);

	if (!Character)
	{
		return;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement || Movement->IsFalling())
	{
		return;
	}

	const EProjectOrganoidPlayerMovementNoiseState MovementState =
		UProjectOrganoidPerceptionComponent::ResolvePlayerMovementNoiseState(Character);

	if (MovementState == EProjectOrganoidPlayerMovementNoiseState::Idle)
	{
		return;
	}

	const float Speed = Character->GetVelocity().Size2D();
	if (Speed < FootstepMinSpeed && MovementState != EProjectOrganoidPlayerMovementNoiseState::Crouch)
	{
		return;
	}

	const float SpeedAlpha = FMath::Clamp(Speed / 600.0f, 0.0f, 1.0f);
	const float Stress = FMath::Clamp((CurrentManagedBPM - RestingBPM) / FMath::Max(MaxStressBPM - RestingBPM, 1.0f), 0.0f, 1.0f);
	const float IntervalScale = FMath::Lerp(1.0f, 0.65f, SpeedAlpha * 0.7f + Stress * 0.3f);
	const float EffectiveInterval = FootstepIntervalSeconds * IntervalScale;

	if (FootstepCooldownRemaining <= 0.0f)
	{
		const float LoudnessScale = UProjectOrganoidPerceptionComponent::GetMovementNoiseLoudnessScale(MovementState);
		const float RangeScale = UProjectOrganoidPerceptionComponent::GetMovementNoiseRangeScale(MovementState);
		const float Loudness = FootstepNoiseLoudness * LoudnessScale * (0.85f + SpeedAlpha * 0.35f);
		const float MaxRange = FootstepNoiseMaxRange * RangeScale;
		const FName NoiseTag = UProjectOrganoidPerceptionComponent::NoiseTagFromMovementState(MovementState);

		if (PlayFootstepAtLocation(Character->GetActorLocation(), Character, Loudness, true, NoiseTag, MaxRange))
		{
			FootstepCooldownRemaining = EffectiveInterval;
		}
	}
}

void UProjectOrganoidAudioSubsystem::ReportSpatialNoise(
	const FVector& Location,
	AActor* Instigator,
	float Loudness,
	float MaxRange,
	FName NoiseTag) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UAISense_Hearing::ReportNoiseEvent(World, Location, Loudness, Instigator, MaxRange, NoiseTag);
}

void UProjectOrganoidAudioSubsystem::SetToxicGasDistortion(float Intensity)
{
	ToxicGasDistortionTarget = FMath::Clamp(Intensity, 0.0f, 1.0f);
}

void UProjectOrganoidAudioSubsystem::AddToxicGasDistortion(float DeltaIntensity)
{
	ToxicGasDistortionTarget = FMath::Clamp(ToxicGasDistortionTarget + DeltaIntensity, 0.0f, 1.0f);
}

void UProjectOrganoidAudioSubsystem::SetTacticalDesaturation(float Intensity)
{
	TacticalDesaturationTarget = FMath::Clamp(Intensity, 0.0f, 1.0f);
}

void UProjectOrganoidAudioSubsystem::NotifyTacticalModeChanged(bool bIsActive)
{
	SetTacticalDesaturation(bIsActive ? 1.0f : 0.0f);
}

void UProjectOrganoidAudioSubsystem::EnsurePostProcessComponent(AProjectOrganoidCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	if (!ManagedPostProcess)
	{
		ManagedPostProcess = NewObject<UPostProcessComponent>(Character, TEXT("OrganoidManagedPostProcess"));
		if (UCameraComponent* Camera = Character->GetFollowCamera())
		{
			ManagedPostProcess->SetupAttachment(Camera);
		}
		else
		{
			ManagedPostProcess->SetupAttachment(Character->GetRootComponent());
		}
		ManagedPostProcess->bEnabled = true;
		ManagedPostProcess->bUnbound = true;
		ManagedPostProcess->Priority = 20.0f;
		ManagedPostProcess->BlendWeight = 1.0f;
		ManagedPostProcess->RegisterComponent();
	}
}

void UProjectOrganoidAudioSubsystem::UpdatePostProcessSettings(float DeltaTime)
{
	AProjectOrganoidCharacter* Character = BoundCharacter.Get();
	if (!Character)
	{
		return;
	}

	EnsurePostProcessComponent(Character);
	if (!ManagedPostProcess)
	{
		return;
	}

	ToxicGasDistortion = FMath::FInterpTo(ToxicGasDistortion, ToxicGasDistortionTarget, DeltaTime, PostProcessInterpSpeed);
	TacticalDesaturation = FMath::FInterpTo(TacticalDesaturation, TacticalDesaturationTarget, DeltaTime, PostProcessInterpSpeed);

	FPostProcessSettings& Settings = ManagedPostProcess->Settings;

	// Toxic gas — chromatic fringe, vignette, grain
	Settings.bOverride_SceneFringeIntensity = true;
	Settings.SceneFringeIntensity = ToxicGasDistortion * MaxToxicGasFringeIntensity;

	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = ToxicGasDistortion * MaxToxicGasVignette;

	Settings.bOverride_FilmGrainIntensity = true;
	Settings.FilmGrainIntensity = ToxicGasDistortion * MaxToxicGasGrain;

	// Tactical time dilation — desaturate toward grey
	const float Saturation = FMath::Lerp(1.0f, TacticalMinColorSaturation, TacticalDesaturation);
	Settings.bOverride_ColorSaturation = true;
	Settings.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.0f);

	Settings.bOverride_ColorContrast = true;
	const float Contrast = FMath::Lerp(1.0f, 1.15f, TacticalDesaturation);
	Settings.ColorContrast = FVector4(Contrast, Contrast, Contrast, 1.0f);

	ManagedPostProcess->bEnabled = (ToxicGasDistortion > KINDA_SMALL_NUMBER) || (TacticalDesaturation > KINDA_SMALL_NUMBER);
}
