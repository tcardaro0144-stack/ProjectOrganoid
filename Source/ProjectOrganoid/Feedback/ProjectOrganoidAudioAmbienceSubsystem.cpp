// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/ReverbEffect.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace ProjectOrganoidAmbience
{
	static const FName ReverbTag(TEXT("ProjectOrganoidAmbience"));
}

void UProjectOrganoidAudioAmbienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UpdateTargetsForState(CurrentState);
}

void UProjectOrganoidAudioAmbienceSubsystem::Deinitialize()
{
	UnbindLocalPlayerCharacter(nullptr);
	PopActiveSoundMix();
	ClearActiveReverb();

	auto DestroyLayer = [](TObjectPtr<UAudioComponent>& Comp)
	{
		if (Comp)
		{
			Comp->Stop();
			Comp->DestroyComponent();
			Comp = nullptr;
		}
	};

	DestroyLayer(AmbientLayerAudio);
	DestroyLayer(TensionLayerAudio);
	DestroyLayer(CombatLayerAudio);
	DestroyLayer(CriticalLayerAudio);

	Super::Deinitialize();
}

TStatId UProjectOrganoidAudioAmbienceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectOrganoidAudioAmbienceSubsystem, STATGROUP_Tickables);
}

void UProjectOrganoidAudioAmbienceSubsystem::Tick(float DeltaTime)
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

	if (bCombatActive)
	{
		CombatTimerRemaining = FMath::Max(0.0f, CombatTimerRemaining - DeltaTime);
		if (CombatTimerRemaining <= KINDA_SMALL_NUMBER)
		{
			bCombatActive = false;
			CombatIntensity = 0.0f;
		}
		else
		{
			CombatIntensity = FMath::FInterpTo(CombatIntensity, 0.35f, DeltaTime, 0.35f);
		}
	}

	const EProjectOrganoidAmbienceState Desired = EvaluateDesiredState();
	if (Desired != CurrentState)
	{
		ApplyAmbienceState(Desired);
	}

	EnsureMusicLayers(Character);
	UpdateLayerVolumes(DeltaTime);
	UpdateMixParameters(DeltaTime);

	const float Intensity = ComputeMusicIntensity();
	if (!FMath::IsNearlyEqual(Intensity, LastBroadcastIntensity, IntensityBroadcastEpsilon))
	{
		CurrentMusicIntensity = Intensity;
		LastBroadcastIntensity = Intensity;
		OnMusicIntensityChanged.Broadcast(CurrentMusicIntensity);
	}
	else
	{
		CurrentMusicIntensity = Intensity;
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::BindLocalPlayerCharacter(AProjectOrganoidCharacter* Character)
{
	BoundCharacter = Character;
	if (Character)
	{
		NotifyHealthChanged(Character->GetHealth(), Character->GetMaxHealth());
		EnsureMusicLayers(Character);
		ApplyAmbienceState(EvaluateDesiredState(), true);
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::UnbindLocalPlayerCharacter(AProjectOrganoidCharacter* Character)
{
	if (Character && BoundCharacter.Get() != Character)
	{
		return;
	}

	BoundCharacter.Reset();
}

void UProjectOrganoidAudioAmbienceSubsystem::NotifyCombatStimulus(float Intensity)
{
	const float Clamped = FMath::Max(0.0f, Intensity) * CombatStimulusGain;
	CombatIntensity = FMath::Clamp(CombatIntensity + Clamped, 0.0f, 1.0f);
	CombatTimerRemaining = CombatLingerSeconds;
	bCombatActive = CombatIntensity > KINDA_SMALL_NUMBER || Clamped > KINDA_SMALL_NUMBER;

	if (bCombatActive)
	{
		ApplyAmbienceState(EvaluateDesiredState());
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::SetCombatActive(bool bActive)
{
	bCombatActive = bActive;
	if (bActive)
	{
		CombatTimerRemaining = CombatLingerSeconds;
		CombatIntensity = FMath::Max(CombatIntensity, 0.75f);
	}
	else
	{
		CombatTimerRemaining = 0.0f;
		CombatIntensity = 0.0f;
	}

	ApplyAmbienceState(EvaluateDesiredState());
}

void UProjectOrganoidAudioAmbienceSubsystem::NotifyHazardEntered(EProjectOrganoidHazardType HazardType, float Intensity)
{
	if (HazardType == EProjectOrganoidHazardType::None)
	{
		return;
	}

	++ActiveHazardCount;
	PrimaryHazardType = HazardType;
	HazardIntensity = FMath::Max(HazardIntensity, FMath::Clamp(Intensity, 0.0f, 5.0f));
	ApplyAmbienceState(EvaluateDesiredState());
}

void UProjectOrganoidAudioAmbienceSubsystem::NotifyHazardExited(EProjectOrganoidHazardType HazardType)
{
	ActiveHazardCount = FMath::Max(0, ActiveHazardCount - 1);
	if (ActiveHazardCount == 0)
	{
		PrimaryHazardType = EProjectOrganoidHazardType::None;
		HazardIntensity = 0.0f;
	}
	else if (PrimaryHazardType == HazardType)
	{
		// Keep last known type until count drains; intensity softens.
		HazardIntensity *= 0.65f;
	}

	ApplyAmbienceState(EvaluateDesiredState());
}

void UProjectOrganoidAudioAmbienceSubsystem::NotifyHealthChanged(float CurrentHealth, float MaxHealth)
{
	const float SafeMax = FMath::Max(MaxHealth, 1.0f);
	HealthNormalized = FMath::Clamp(CurrentHealth / SafeMax, 0.0f, 1.0f);
	ApplyAmbienceState(EvaluateDesiredState());
}

AProjectOrganoidCharacter* UProjectOrganoidAudioAmbienceSubsystem::ResolveLocalCharacter() const
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

EProjectOrganoidAmbienceState UProjectOrganoidAudioAmbienceSubsystem::EvaluateDesiredState() const
{
	if (HealthNormalized <= CriticalHealthThreshold)
	{
		return EProjectOrganoidAmbienceState::CriticalHealth;
	}

	if (bCombatActive)
	{
		return EProjectOrganoidAmbienceState::Combat;
	}

	if (ActiveHazardCount > 0)
	{
		return EProjectOrganoidAmbienceState::Hazard;
	}

	if (HealthNormalized <= TensionHealthThreshold)
	{
		return EProjectOrganoidAmbienceState::Tension;
	}

	return EProjectOrganoidAmbienceState::Exploration;
}

void UProjectOrganoidAudioAmbienceSubsystem::ApplyAmbienceState(EProjectOrganoidAmbienceState NewState, bool bForce)
{
	if (!bForce && NewState == CurrentState)
	{
		UpdateTargetsForState(NewState);
		return;
	}

	const EProjectOrganoidAmbienceState Previous = CurrentState;
	CurrentState = NewState;
	UpdateTargetsForState(NewState);
	PushStateSoundMix(NewState);
	ApplyStateReverb(NewState);

	if (Previous != NewState || bForce)
	{
		OnAmbienceStateChanged.Broadcast(NewState, Previous);
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::UpdateTargetsForState(EProjectOrganoidAmbienceState State)
{
	switch (State)
	{
	case EProjectOrganoidAmbienceState::Exploration:
		TargetAmbientVolume = 1.0f;
		TargetTensionVolume = 0.0f;
		TargetCombatVolume = 0.0f;
		TargetCriticalVolume = 0.0f;
		TargetMixPitch = 1.0f;
		TargetMixVolume = 1.0f;
		break;

	case EProjectOrganoidAmbienceState::Tension:
		TargetAmbientVolume = 0.55f;
		TargetTensionVolume = 0.85f;
		TargetCombatVolume = 0.0f;
		TargetCriticalVolume = 0.15f * (1.0f - HealthNormalized);
		TargetMixPitch = 0.97f;
		TargetMixVolume = 1.05f;
		break;

	case EProjectOrganoidAmbienceState::Combat:
		TargetAmbientVolume = 0.25f;
		TargetTensionVolume = 0.45f;
		TargetCombatVolume = FMath::Clamp(0.55f + CombatIntensity * 0.45f, 0.55f, 1.0f);
		TargetCriticalVolume = HealthNormalized <= TensionHealthThreshold ? 0.25f : 0.0f;
		TargetMixPitch = 1.04f;
		TargetMixVolume = 1.15f;
		break;

	case EProjectOrganoidAmbienceState::Hazard:
		TargetAmbientVolume = 0.35f;
		TargetTensionVolume = FMath::Clamp(0.4f + HazardIntensity * 0.1f, 0.4f, 0.9f);
		TargetCombatVolume = bCombatActive ? 0.35f : 0.0f;
		TargetCriticalVolume = 0.1f;
		TargetMixPitch = 0.94f;
		TargetMixVolume = 1.1f;
		break;

	case EProjectOrganoidAmbienceState::CriticalHealth:
		TargetAmbientVolume = 0.15f;
		TargetTensionVolume = 0.55f;
		TargetCombatVolume = bCombatActive ? 0.7f : 0.2f;
		TargetCriticalVolume = 1.0f;
		TargetMixPitch = 0.90f;
		TargetMixVolume = 1.2f;
		break;
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::UpdateLayerVolumes(float DeltaTime)
{
	AmbientLayerVolume = FMath::FInterpTo(AmbientLayerVolume, TargetAmbientVolume, DeltaTime, LayerInterpSpeed);
	TensionLayerVolume = FMath::FInterpTo(TensionLayerVolume, TargetTensionVolume, DeltaTime, LayerInterpSpeed);
	CombatLayerVolume = FMath::FInterpTo(CombatLayerVolume, TargetCombatVolume, DeltaTime, LayerInterpSpeed);
	CriticalLayerVolume = FMath::FInterpTo(CriticalLayerVolume, TargetCriticalVolume, DeltaTime, LayerInterpSpeed);

	if (AmbientLayerAudio)
	{
		AmbientLayerAudio->SetVolumeMultiplier(AmbientLayerVolume);
	}
	if (TensionLayerAudio)
	{
		TensionLayerAudio->SetVolumeMultiplier(TensionLayerVolume);
	}
	if (CombatLayerAudio)
	{
		CombatLayerAudio->SetVolumeMultiplier(CombatLayerVolume);
	}
	if (CriticalLayerAudio)
	{
		CriticalLayerAudio->SetVolumeMultiplier(CriticalLayerVolume);
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::UpdateMixParameters(float DeltaTime)
{
	const float PrevPitch = CurrentMixPitch;
	const float PrevVolume = CurrentMixVolume;

	CurrentMixPitch = FMath::FInterpTo(CurrentMixPitch, TargetMixPitch, DeltaTime, MixInterpSpeed);
	CurrentMixVolume = FMath::FInterpTo(CurrentMixVolume, TargetMixVolume, DeltaTime, MixInterpSpeed);

	UWorld* World = GetWorld();
	USoundMix* Mix = ActivePushedMix.IsValid() ? ActivePushedMix.Get() : ExplorationSoundMix.LoadSynchronous();
	USoundClass* MusicClass = MusicSoundClass.LoadSynchronous();
	if (World && Mix && MusicClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(
			World,
			Mix,
			MusicClass,
			CurrentMixVolume,
			CurrentMixPitch,
			0.0f,
			true);
	}

	if (!FMath::IsNearlyEqual(PrevPitch, CurrentMixPitch, 0.01f) || !FMath::IsNearlyEqual(PrevVolume, CurrentMixVolume, 0.01f))
	{
		OnAmbienceMixChanged.Broadcast(CurrentMixPitch, CurrentMixVolume);
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::EnsureMusicLayers(AProjectOrganoidCharacter* Character)
{
	if (!Character)
	{
		return;
	}

	SyncLayerComponent(AmbientLayerAudio, Character, AmbientLayerSound, TEXT("OrganoidAmbientLayer"), AmbientLayerVolume);
	SyncLayerComponent(TensionLayerAudio, Character, TensionLayerSound, TEXT("OrganoidTensionLayer"), TensionLayerVolume);
	SyncLayerComponent(CombatLayerAudio, Character, CombatLayerSound, TEXT("OrganoidCombatLayer"), CombatLayerVolume);
	SyncLayerComponent(CriticalLayerAudio, Character, CriticalLayerSound, TEXT("OrganoidCriticalLayer"), CriticalLayerVolume);
}

void UProjectOrganoidAudioAmbienceSubsystem::SyncLayerComponent(
	TObjectPtr<UAudioComponent>& Component,
	AProjectOrganoidCharacter* Character,
	const TSoftObjectPtr<USoundBase>& SoftSound,
	const TCHAR* ComponentName,
	float Volume)
{
	USoundBase* Sound = SoftSound.LoadSynchronous();
	if (!Sound)
	{
		if (Component && Component->IsPlaying())
		{
			Component->Stop();
		}
		return;
	}

	if (!Component)
	{
		Component = NewObject<UAudioComponent>(Character, ComponentName);
		Component->SetupAttachment(Character->GetRootComponent());
		Component->bAutoActivate = false;
		Component->bUISound = true;
		Component->bAllowSpatialization = false;
		Component->RegisterComponent();
	}

	if (Component->GetSound() != Sound)
	{
		Component->SetSound(Sound);
	}

	Component->SetVolumeMultiplier(Volume);

	if (!Component->IsPlaying())
	{
		Component->Play();
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::PushStateSoundMix(EProjectOrganoidAmbienceState State)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PopActiveSoundMix();

	TSoftObjectPtr<USoundMix> SoftMix;
	switch (State)
	{
	case EProjectOrganoidAmbienceState::Combat:
		SoftMix = CombatSoundMix;
		break;
	case EProjectOrganoidAmbienceState::Hazard:
		SoftMix = HazardSoundMix;
		break;
	case EProjectOrganoidAmbienceState::CriticalHealth:
		SoftMix = CriticalSoundMix;
		break;
	case EProjectOrganoidAmbienceState::Tension:
	case EProjectOrganoidAmbienceState::Exploration:
	default:
		SoftMix = ExplorationSoundMix;
		break;
	}

	if (USoundMix* Mix = SoftMix.LoadSynchronous())
	{
		UGameplayStatics::PushSoundMixModifier(World, Mix);
		ActivePushedMix = Mix;
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::PopActiveSoundMix()
{
	UWorld* World = GetWorld();
	if (World && ActivePushedMix.IsValid())
	{
		UGameplayStatics::PopSoundMixModifier(World, ActivePushedMix.Get());
	}
	ActivePushedMix.Reset();
}

void UProjectOrganoidAudioAmbienceSubsystem::ApplyStateReverb(EProjectOrganoidAmbienceState State)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ClearActiveReverb();

	TSoftObjectPtr<UReverbEffect> SoftReverb;
	switch (State)
	{
	case EProjectOrganoidAmbienceState::Combat:
		SoftReverb = CombatReverb;
		break;
	case EProjectOrganoidAmbienceState::Hazard:
		SoftReverb = HazardReverb;
		break;
	case EProjectOrganoidAmbienceState::CriticalHealth:
		SoftReverb = CriticalReverb;
		break;
	case EProjectOrganoidAmbienceState::Tension:
	case EProjectOrganoidAmbienceState::Exploration:
	default:
		SoftReverb = ExplorationReverb;
		break;
	}

	if (UReverbEffect* Reverb = SoftReverb.LoadSynchronous())
	{
		UGameplayStatics::ActivateReverbEffect(World, Reverb, ProjectOrganoidAmbience::ReverbTag, ReverbVolume, 1.0f, ReverbFadeTime);
		ActiveReverbTag = ProjectOrganoidAmbience::ReverbTag;
	}
}

void UProjectOrganoidAudioAmbienceSubsystem::ClearActiveReverb()
{
	UWorld* World = GetWorld();
	if (World && ActiveReverbTag != NAME_None)
	{
		UGameplayStatics::DeactivateReverbEffect(World, ActiveReverbTag);
	}
	ActiveReverbTag = NAME_None;
}

float UProjectOrganoidAudioAmbienceSubsystem::ComputeMusicIntensity() const
{
	const float LayerWeighted =
		AmbientLayerVolume * 0.15f +
		TensionLayerVolume * 0.35f +
		CombatLayerVolume * 0.55f +
		CriticalLayerVolume * 0.75f;

	const float Stimulus =
		(bCombatActive ? 0.35f + CombatIntensity * 0.35f : 0.0f) +
		(ActiveHazardCount > 0 ? 0.2f + FMath::Clamp(HazardIntensity, 0.0f, 1.0f) * 0.15f : 0.0f) +
		(1.0f - HealthNormalized) * 0.35f;

	return FMath::Clamp(FMath::Max(LayerWeighted, Stimulus), 0.0f, 1.0f);
}
