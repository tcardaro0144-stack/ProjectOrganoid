// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidFeedbackComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystem.h"
#include "Materials/MaterialInterface.h"

UProjectOrganoidFeedbackComponent::UProjectOrganoidFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UProjectOrganoidFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AProjectOrganoidCharacter>(GetOwner());
	EnsureAudioComponents();
	EnsurePostProcessComponent();
	RefreshWeaponBinding();
}

void UProjectOrganoidFeedbackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindWeaponFeedback();
	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshWeaponBinding();
	UpdateVitalAudio(DeltaTime);
	UpdatePostProcessEffects();
}

void UProjectOrganoidFeedbackComponent::EnsureAudioComponents()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (!HeartbeatAudio)
	{
		HeartbeatAudio = NewObject<UAudioComponent>(OwnerActor, TEXT("HeartbeatAudio"));
		HeartbeatAudio->SetupAttachment(OwnerActor->GetRootComponent());
		HeartbeatAudio->bAutoActivate = false;
		HeartbeatAudio->bUISound = false;
		HeartbeatAudio->RegisterComponent();
	}

	if (!BreathingAudio)
	{
		BreathingAudio = NewObject<UAudioComponent>(OwnerActor, TEXT("BreathingAudio"));
		BreathingAudio->SetupAttachment(OwnerActor->GetRootComponent());
		BreathingAudio->bAutoActivate = false;
		BreathingAudio->bUISound = false;
		BreathingAudio->RegisterComponent();
	}

	if (HeartbeatSound && HeartbeatAudio->GetSound() != HeartbeatSound)
	{
		HeartbeatAudio->SetSound(HeartbeatSound);
	}
	if (BreathingSound && BreathingAudio->GetSound() != BreathingSound)
	{
		BreathingAudio->SetSound(BreathingSound);
	}

	if (HeartbeatSound && HeartbeatAudio && !HeartbeatAudio->IsPlaying())
	{
		HeartbeatAudio->Play();
	}
	if (BreathingSound && BreathingAudio && !BreathingAudio->IsPlaying())
	{
		BreathingAudio->Play();
	}
}

void UProjectOrganoidFeedbackComponent::EnsurePostProcessComponent()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (!FeedbackPostProcess)
	{
		FeedbackPostProcess = NewObject<UPostProcessComponent>(OwnerActor, TEXT("FeedbackPostProcess"));
		FeedbackPostProcess->SetupAttachment(OwnerActor->GetRootComponent());
		FeedbackPostProcess->bEnabled = true;
		FeedbackPostProcess->bUnbound = true;
		FeedbackPostProcess->Priority = 10.0f;
		FeedbackPostProcess->BlendWeight = 1.0f;
		FeedbackPostProcess->RegisterComponent();
	}

	FWeightedBlendables& Blendables = FeedbackPostProcess->Settings.WeightedBlendables;

	if (HighToxicityPostProcessMaterial && ToxicityBlendableIndex == INDEX_NONE)
	{
		ToxicityBlendableIndex = Blendables.Array.Num();
		Blendables.Array.Add(FWeightedBlendable(0.0f, HighToxicityPostProcessMaterial));
	}

	if (LowPEPostProcessMaterial && LowPEBlendableIndex == INDEX_NONE)
	{
		LowPEBlendableIndex = Blendables.Array.Num();
		Blendables.Array.Add(FWeightedBlendable(0.0f, LowPEPostProcessMaterial));
	}
}

float UProjectOrganoidFeedbackComponent::ComputePitchFromBPM(float BPM, float MinPitch, float MaxPitch) const
{
	const float SafeRest = FMath::Max(RestingBPMReference, 1.0f);
	const float Normalized = FMath::Clamp(BPM / SafeRest, 0.5f, 2.5f);
	const float Alpha = FMath::Clamp((Normalized - 0.5f) / 2.0f, 0.0f, 1.0f);
	return FMath::Lerp(MinPitch, MaxPitch, Alpha);
}

void UProjectOrganoidFeedbackComponent::UpdateVitalAudio(float DeltaTime)
{
	if (!OwnerCharacter)
	{
		return;
	}

	EnsureAudioComponents();

	const float BPM = OwnerCharacter->GetHeartRate();
	const float HeartPitch = ComputePitchFromBPM(BPM, MinHeartbeatPitch, MaxHeartbeatPitch);
	const float BreathPitch = ComputePitchFromBPM(BPM, MinBreathingPitch, MaxBreathingPitch);

	// Louder when stressed
	const float Stress = FMath::Clamp((BPM - 60.0f) / 120.0f, 0.0f, 1.0f);

	if (HeartbeatAudio)
	{
		HeartbeatAudio->SetPitchMultiplier(HeartPitch);
		HeartbeatAudio->SetVolumeMultiplier(HeartbeatVolume * (0.65f + Stress * 0.75f));
	}

	if (BreathingAudio)
	{
		BreathingAudio->SetPitchMultiplier(BreathPitch);
		BreathingAudio->SetVolumeMultiplier(BreathingVolume * (0.55f + Stress * 0.90f));
	}
}

void UProjectOrganoidFeedbackComponent::UpdatePostProcessEffects()
{
	if (!OwnerCharacter || !FeedbackPostProcess)
	{
		EnsurePostProcessComponent();
		if (!FeedbackPostProcess)
		{
			return;
		}
	}

	FWeightedBlendables& Blendables = FeedbackPostProcess->Settings.WeightedBlendables;

	float ToxicityWeight = 0.0f;
	const float MaxTox = FMath::Max(OwnerCharacter->GetMaxToxicity(), 1.0f);
	const float ToxNorm = FMath::Clamp(OwnerCharacter->GetToxicity() / MaxTox, 0.0f, 1.0f);
	if (ToxNorm > ToxicityEffectStartNormalized)
	{
		const float Span = FMath::Max(1.0f - ToxicityEffectStartNormalized, KINDA_SMALL_NUMBER);
		ToxicityWeight = ((ToxNorm - ToxicityEffectStartNormalized) / Span) * MaxToxicityBlendWeight;
	}

	float LowPEWeight = 0.0f;
	const float MaxPE = FMath::Max(OwnerCharacter->GetMaxPEEnergy(), 1.0f);
	const float PENorm = FMath::Clamp(OwnerCharacter->GetPEEnergy() / MaxPE, 0.0f, 1.0f);
	if (PENorm < LowPEEffectStartNormalized)
	{
		const float Span = FMath::Max(LowPEEffectStartNormalized, KINDA_SMALL_NUMBER);
		LowPEWeight = ((LowPEEffectStartNormalized - PENorm) / Span) * MaxLowPEBlendWeight;
	}

	if (Blendables.Array.IsValidIndex(ToxicityBlendableIndex))
	{
		Blendables.Array[ToxicityBlendableIndex].Weight = ToxicityWeight;
		if (HighToxicityPostProcessMaterial)
		{
			Blendables.Array[ToxicityBlendableIndex].Object = HighToxicityPostProcessMaterial;
		}
	}

	if (Blendables.Array.IsValidIndex(LowPEBlendableIndex))
	{
		Blendables.Array[LowPEBlendableIndex].Weight = LowPEWeight;
		if (LowPEPostProcessMaterial)
		{
			Blendables.Array[LowPEBlendableIndex].Object = LowPEPostProcessMaterial;
		}
	}
}

void UProjectOrganoidFeedbackComponent::RefreshWeaponBinding()
{
	AProjectOrganoidWeapon* CurrentWeapon = nullptr;
	if (OwnerCharacter && OwnerCharacter->GetWeaponComponent())
	{
		CurrentWeapon = OwnerCharacter->GetWeaponComponent()->GetEquippedWeapon();
	}

	if (CurrentWeapon == BoundWeapon)
	{
		return;
	}

	UnbindWeaponFeedback();

	BoundWeapon = CurrentWeapon;
	if (BoundWeapon)
	{
		BoundWeapon->OnWeakPointReaction.AddDynamic(this, &UProjectOrganoidFeedbackComponent::HandleWeakPointReaction);
	}
}

void UProjectOrganoidFeedbackComponent::UnbindWeaponFeedback()
{
	if (BoundWeapon)
	{
		BoundWeapon->OnWeakPointReaction.RemoveDynamic(this, &UProjectOrganoidFeedbackComponent::HandleWeakPointReaction);
		BoundWeapon = nullptr;
	}
}

void UProjectOrganoidFeedbackComponent::HandleWeakPointReaction(const FProjectOrganoidBallisticHit& HitInfo)
{
	PlayWeakPointImpact(HitInfo);
}

USoundBase* UProjectOrganoidFeedbackComponent::SelectWeakPointSound(EProjectOrganoidWeakPointType WeakPoint) const
{
	switch (WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		return LocomotorNerveHitSound ? LocomotorNerveHitSound.Get() : DefaultWeakPointHitSound.Get();
	case EProjectOrganoidWeakPointType::OpticalNodes:
		return OpticalNodeHitSound ? OpticalNodeHitSound.Get() : DefaultWeakPointHitSound.Get();
	case EProjectOrganoidWeakPointType::OrganoidCore:
		return BioCoreHitSound ? BioCoreHitSound.Get() : DefaultWeakPointHitSound.Get();
	default:
		return DefaultWeakPointHitSound.Get();
	}
}

UParticleSystem* UProjectOrganoidFeedbackComponent::SelectWeakPointVFX(EProjectOrganoidWeakPointType WeakPoint) const
{
	switch (WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		return LocomotorNerveHitVFX ? LocomotorNerveHitVFX.Get() : DefaultWeakPointHitVFX.Get();
	case EProjectOrganoidWeakPointType::OpticalNodes:
		return OpticalNodeHitVFX ? OpticalNodeHitVFX.Get() : DefaultWeakPointHitVFX.Get();
	case EProjectOrganoidWeakPointType::OrganoidCore:
		return BioCoreHitVFX ? BioCoreHitVFX.Get() : DefaultWeakPointHitVFX.Get();
	default:
		return DefaultWeakPointHitVFX.Get();
	}
}

void UProjectOrganoidFeedbackComponent::PlayWeakPointImpact(const FProjectOrganoidBallisticHit& HitInfo)
{
	if (USoundBase* Sound = SelectWeakPointSound(HitInfo.WeakPoint))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, HitInfo.ImpactPoint);
	}

	if (UParticleSystem* VFX = SelectWeakPointVFX(HitInfo.WeakPoint))
	{
		const FRotator Rotation = HitInfo.ImpactNormal.Rotation();
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			VFX,
			HitInfo.ImpactPoint,
			Rotation,
			FVector(WeakPointVFXScale),
			true,
			EPSCPoolMethod::AutoRelease);
	}

	OnWeakPointFeedbackPlayed.Broadcast(HitInfo);
	BP_OnWeakPointImpact(HitInfo);
}
