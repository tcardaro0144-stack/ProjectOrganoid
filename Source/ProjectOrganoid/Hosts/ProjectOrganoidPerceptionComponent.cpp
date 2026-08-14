// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPerceptionComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense.h"
#include "TimerManager.h"
#include "Engine/World.h"

UProjectOrganoidPerceptionComponent::UProjectOrganoidPerceptionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

void UProjectOrganoidPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();

	ConfigureHostSenses();
	OnTargetPerceptionUpdated.AddDynamic(this, &UProjectOrganoidPerceptionComponent::HandleTargetPerceptionUpdated);
}

void UProjectOrganoidPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnTargetPerceptionUpdated.RemoveDynamic(this, &UProjectOrganoidPerceptionComponent::HandleTargetPerceptionUpdated);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoiseStimulusTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidPerceptionComponent::ConfigureHostSenses()
{
	if (!SightConfig || !HearingConfig)
	{
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(SightMaxAge);

	HearingConfig->HearingRange = GetEffectiveHearingRange();
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(HearingMaxAge);

	ConfigureSense(*SightConfig);
	ConfigureSense(*HearingConfig);
	SetDominantSense(UAISense_Sight::StaticClass());
}

void UProjectOrganoidPerceptionComponent::SetHearingRangeBonus(float Bonus)
{
	HearingRangeBonus = FMath::Max(0.0f, Bonus);
	ConfigureHostSenses();
}

void UProjectOrganoidPerceptionComponent::SetSightEnabled(bool bEnabled)
{
	SetSenseEnabled(UAISense_Sight::StaticClass(), bEnabled);
}

void UProjectOrganoidPerceptionComponent::SetHearingEnabled(bool bEnabled)
{
	SetSenseEnabled(UAISense_Hearing::StaticClass(), bEnabled);
}

void UProjectOrganoidPerceptionComponent::SetAllSensesEnabled(bool bEnabled)
{
	SetSightEnabled(bEnabled);
	SetHearingEnabled(bEnabled);
}

float UProjectOrganoidPerceptionComponent::GetEffectiveHearingRange() const
{
	return HearingRange + HearingRangeBonus;
}

void UProjectOrganoidPerceptionComponent::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor)
	{
		return;
	}

	const FAISenseID SightID = UAISense::GetSenseID<UAISense_Sight>();
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();

	if (Stimulus.Type == SightID)
	{
		ProcessSightStimulus(Actor, Stimulus);
	}
	else if (Stimulus.Type == HearingID)
	{
		ProcessHearingStimulus(Actor, Stimulus);
	}
}

void UProjectOrganoidPerceptionComponent::ProcessSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	const bool bSensed = Stimulus.WasSuccessfullySensed();
	if (bSensed)
	{
		CurrentSightTarget = Actor;
		bHasSightOnTarget = true;
	}
	else if (CurrentSightTarget == Actor)
	{
		CurrentSightTarget = nullptr;
		bHasSightOnTarget = false;
	}

	OnSightStimulus.Broadcast(Actor, bSensed, Stimulus.StimulusLocation);
}

void UProjectOrganoidPerceptionComponent::ProcessHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	const EProjectOrganoidHearingStimulusKind Kind = ClassifyNoiseTag(Stimulus.Tag);
	const float Strength = Stimulus.Strength;

	if (Kind == EProjectOrganoidHearingStimulusKind::FootstepCrouch && Strength < CrouchNoiseMinStrength)
	{
		return;
	}

	const bool bIsGunfire = Kind == EProjectOrganoidHearingStimulusKind::Gunfire;
	const bool bReplace =
		!bHasRecentNoiseStimulus ||
		bIsGunfire ||
		!bPrioritizeGunfireOverFootsteps ||
		LastHeardStimulusKind != EProjectOrganoidHearingStimulusKind::Gunfire;

	if (!bReplace)
	{
		return;
	}

	LastHeardNoiseLocation = Stimulus.StimulusLocation;
	LastHeardNoiseInstigator = Actor;
	LastHeardNoiseTag = Stimulus.Tag;
	LastHeardStimulusKind = Kind;
	LastHeardNoiseStrength = Strength;
	LastHeardMovementState = MovementStateFromNoiseTag(Stimulus.Tag);
	bHasRecentNoiseStimulus = true;

	OnHearingStimulus.Broadcast(Actor, Stimulus.Tag, Kind, Stimulus.StimulusLocation, Strength);

	if (Kind == EProjectOrganoidHearingStimulusKind::FootstepCrouch ||
		Kind == EProjectOrganoidHearingStimulusKind::FootstepWalk ||
		Kind == EProjectOrganoidHearingStimulusKind::FootstepRun ||
		Kind == EProjectOrganoidHearingStimulusKind::FootstepIdle)
	{
		OnPlayerMovementHeard.Broadcast(Actor, LastHeardMovementState);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(NoiseStimulusTimer);
		World->GetTimerManager().SetTimer(
			NoiseStimulusTimer,
			this,
			&UProjectOrganoidPerceptionComponent::ClearNoiseStimulus,
			NoiseStimulusHoldSeconds,
			false);
	}
}

void UProjectOrganoidPerceptionComponent::ClearNoiseStimulus()
{
	bHasRecentNoiseStimulus = false;
	LastHeardNoiseStrength = 0.0f;
}

EProjectOrganoidHearingStimulusKind UProjectOrganoidPerceptionComponent::ClassifyNoiseTag(FName NoiseTag)
{
	if (NoiseTag == ProjectOrganoidNoiseTags::Gunfire)
	{
		return EProjectOrganoidHearingStimulusKind::Gunfire;
	}
	if (NoiseTag == ProjectOrganoidNoiseTags::FootstepCrouch)
	{
		return EProjectOrganoidHearingStimulusKind::FootstepCrouch;
	}
	if (NoiseTag == ProjectOrganoidNoiseTags::FootstepWalk)
	{
		return EProjectOrganoidHearingStimulusKind::FootstepWalk;
	}
	if (NoiseTag == ProjectOrganoidNoiseTags::FootstepRun)
	{
		return EProjectOrganoidHearingStimulusKind::FootstepRun;
	}
	if (NoiseTag == ProjectOrganoidNoiseTags::FootstepIdle)
	{
		return EProjectOrganoidHearingStimulusKind::FootstepIdle;
	}
	if (NoiseTag == ProjectOrganoidNoiseTags::Footstep)
	{
		return EProjectOrganoidHearingStimulusKind::FootstepWalk;
	}
	if (NoiseTag != NAME_None)
	{
		return EProjectOrganoidHearingStimulusKind::GenericNoise;
	}
	return EProjectOrganoidHearingStimulusKind::Unknown;
}

EProjectOrganoidPlayerMovementNoiseState UProjectOrganoidPerceptionComponent::MovementStateFromNoiseTag(FName NoiseTag)
{
	switch (ClassifyNoiseTag(NoiseTag))
	{
	case EProjectOrganoidHearingStimulusKind::FootstepCrouch:
		return EProjectOrganoidPlayerMovementNoiseState::Crouch;
	case EProjectOrganoidHearingStimulusKind::FootstepWalk:
		return EProjectOrganoidPlayerMovementNoiseState::Walk;
	case EProjectOrganoidHearingStimulusKind::FootstepRun:
		return EProjectOrganoidPlayerMovementNoiseState::Run;
	case EProjectOrganoidHearingStimulusKind::FootstepIdle:
		return EProjectOrganoidPlayerMovementNoiseState::Idle;
	default:
		return EProjectOrganoidPlayerMovementNoiseState::Idle;
	}
}

FName UProjectOrganoidPerceptionComponent::NoiseTagFromMovementState(EProjectOrganoidPlayerMovementNoiseState MovementState)
{
	switch (MovementState)
	{
	case EProjectOrganoidPlayerMovementNoiseState::Crouch:
		return ProjectOrganoidNoiseTags::FootstepCrouch;
	case EProjectOrganoidPlayerMovementNoiseState::Walk:
		return ProjectOrganoidNoiseTags::FootstepWalk;
	case EProjectOrganoidPlayerMovementNoiseState::Run:
		return ProjectOrganoidNoiseTags::FootstepRun;
	case EProjectOrganoidPlayerMovementNoiseState::Idle:
	default:
		return ProjectOrganoidNoiseTags::FootstepIdle;
	}
}

EProjectOrganoidPlayerMovementNoiseState UProjectOrganoidPerceptionComponent::ResolvePlayerMovementNoiseState(
	const ACharacter* Character,
	float WalkSpeedThreshold,
	float RunSpeedThreshold)
{
	if (!Character)
	{
		return EProjectOrganoidPlayerMovementNoiseState::Idle;
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const float Speed = Character->GetVelocity().Size2D();

	if (Movement && Movement->IsCrouching())
	{
		return Speed > KINDA_SMALL_NUMBER
			? EProjectOrganoidPlayerMovementNoiseState::Crouch
			: EProjectOrganoidPlayerMovementNoiseState::Idle;
	}

	if (Speed < WalkSpeedThreshold)
	{
		return Speed > KINDA_SMALL_NUMBER
			? EProjectOrganoidPlayerMovementNoiseState::Walk
			: EProjectOrganoidPlayerMovementNoiseState::Idle;
	}

	if (Speed >= RunSpeedThreshold)
	{
		return EProjectOrganoidPlayerMovementNoiseState::Run;
	}

	return EProjectOrganoidPlayerMovementNoiseState::Walk;
}

float UProjectOrganoidPerceptionComponent::GetMovementNoiseLoudnessScale(EProjectOrganoidPlayerMovementNoiseState MovementState)
{
	switch (MovementState)
	{
	case EProjectOrganoidPlayerMovementNoiseState::Crouch:
		return 0.35f;
	case EProjectOrganoidPlayerMovementNoiseState::Walk:
		return 0.75f;
	case EProjectOrganoidPlayerMovementNoiseState::Run:
		return 1.25f;
	case EProjectOrganoidPlayerMovementNoiseState::Idle:
	default:
		return 0.0f;
	}
}

float UProjectOrganoidPerceptionComponent::GetMovementNoiseRangeScale(EProjectOrganoidPlayerMovementNoiseState MovementState)
{
	switch (MovementState)
	{
	case EProjectOrganoidPlayerMovementNoiseState::Crouch:
		return 0.4f;
	case EProjectOrganoidPlayerMovementNoiseState::Walk:
		return 0.85f;
	case EProjectOrganoidPlayerMovementNoiseState::Run:
		return 1.35f;
	case EProjectOrganoidPlayerMovementNoiseState::Idle:
	default:
		return 0.0f;
	}
}
