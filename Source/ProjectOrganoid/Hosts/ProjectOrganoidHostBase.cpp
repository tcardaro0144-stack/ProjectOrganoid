// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHostBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidStatsSubsystem.h"

AProjectOrganoidHostBase::AProjectOrganoidHostBase()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	bUseControllerRotationYaw = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;
	CachedWalkSpeed = DefaultWalkSpeed;

	Health = MaxHealth;
	Toxicity = 0.0f;

	LocomotorNervesHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("LocomotorNervesHitbox"));
	LocomotorNervesHitbox->SetupAttachment(GetMesh());
	ConfigureWeakPointHitbox(LocomotorNervesHitbox, TEXT("LocomotorNerves"), 22.0f, FVector(0.0f, 0.0f, 40.0f));

	OpticalNodesHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("OpticalNodesHitbox"));
	OpticalNodesHitbox->SetupAttachment(GetMesh());
	ConfigureWeakPointHitbox(OpticalNodesHitbox, TEXT("OpticalNodes"), 18.0f, FVector(0.0f, 0.0f, 75.0f));

	BioCoreHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("BioCoreHitbox"));
	BioCoreHitbox->SetupAttachment(GetMesh());
	ConfigureWeakPointHitbox(BioCoreHitbox, TEXT("OrganoidCore"), 20.0f, FVector(0.0f, 0.0f, 50.0f));
	BioCoreHitbox->ComponentTags.AddUnique(TEXT("BioCore"));

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
}

void AProjectOrganoidHostBase::ConfigureWeakPointHitbox(USphereComponent* Hitbox, FName Tag, float Radius, FVector RelativeLocation)
{
	if (!Hitbox)
	{
		return;
	}

	Hitbox->InitSphereRadius(Radius);
	Hitbox->SetRelativeLocation(RelativeLocation);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hitbox->SetCollisionObjectType(ECC_Pawn);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Hitbox->SetGenerateOverlapEvents(false);
	Hitbox->SetCanEverAffectNavigation(false);
	Hitbox->ComponentTags.AddUnique(Tag);
}

void AProjectOrganoidHostBase::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	CachedWalkSpeed = DefaultWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = DefaultWalkSpeed;

	ConfigureAIPerception();

	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AProjectOrganoidHostBase::OnTargetPerceptionUpdated);
	}
}

void AProjectOrganoidHostBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.RemoveDynamic(this, &AProjectOrganoidHostBase::OnTargetPerceptionUpdated);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidHostBase::ConfigureAIPerception()
{
	if (!AIPerception || !SightConfig || !HearingConfig)
	{
		return;
	}

	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(3.0f);

	const float EffectiveHearing = HearingRange + (bIsEnraged ? RageHearingBonus : 0.0f);
	HearingConfig->HearingRange = EffectiveHearing;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(2.5f);

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

void AProjectOrganoidHostBase::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed() || !Actor)
	{
		return;
	}

	// Footstep / gunfire arrive through hearing sense
	const FAISenseID HearingID = UAISense::GetSenseID<UAISense_Hearing>();
	if (Stimulus.Type == HearingID)
	{
		LastHeardNoiseLocation = Stimulus.StimulusLocation;
		LastHeardNoiseInstigator = Actor;
		LastHeardNoiseTag = Stimulus.Tag;
		bHasRecentNoiseStimulus = true;

		OnNoiseHeard.Broadcast(Actor, Stimulus.Tag);
		OnHostStateChanged.Broadcast(
			Stimulus.Tag == FName(TEXT("Gunfire")) ? TEXT("HeardGunfire") : TEXT("HeardFootstep"));

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(NoiseStimulusTimer);
			World->GetTimerManager().SetTimer(
				NoiseStimulusTimer,
				this,
				&AProjectOrganoidHostBase::ClearNoiseStimulus,
				3.0f,
				false);
		}
	}
}

void AProjectOrganoidHostBase::ClearNoiseStimulus()
{
	bHasRecentNoiseStimulus = false;
}

EProjectOrganoidWeakPointType AProjectOrganoidHostBase::ResolveWeakPoint_Implementation(const FHitResult& Hit) const
{
	const UPrimitiveComponent* HitComp = Hit.GetComponent();
	if (HitComp)
	{
		if (!bBioCoreDestroyed && (HitComp == BioCoreHitbox || HitComp->ComponentHasTag(TEXT("OrganoidCore")) || HitComp->ComponentHasTag(TEXT("BioCore"))))
		{
			return EProjectOrganoidWeakPointType::OrganoidCore;
		}
		if (!bLocomotorNervesDestroyed && (HitComp == LocomotorNervesHitbox || HitComp->ComponentHasTag(TEXT("LocomotorNerves"))))
		{
			return EProjectOrganoidWeakPointType::LocomotorNerves;
		}
		if (!bOpticalNodesDestroyed && (HitComp == OpticalNodesHitbox || HitComp->ComponentHasTag(TEXT("OpticalNodes"))))
		{
			return EProjectOrganoidWeakPointType::OpticalNodes;
		}
	}

	if (!bBioCoreDestroyed && (Hit.BoneName == TEXT("OrganoidCore") || Hit.BoneName == TEXT("BioCore") || Hit.BoneName == TEXT("core")))
	{
		return EProjectOrganoidWeakPointType::OrganoidCore;
	}
	if (!bLocomotorNervesDestroyed && (Hit.BoneName == TEXT("LocomotorNerves") || Hit.BoneName == TEXT("spine_01") || Hit.BoneName == TEXT("pelvis")))
	{
		return EProjectOrganoidWeakPointType::LocomotorNerves;
	}
	if (!bOpticalNodesDestroyed && (Hit.BoneName == TEXT("OpticalNodes") || Hit.BoneName == TEXT("head") || Hit.BoneName == TEXT("face")))
	{
		return EProjectOrganoidWeakPointType::OpticalNodes;
	}

	return EProjectOrganoidWeakPointType::None;
}

void AProjectOrganoidHostBase::ApplyOrganoidHit_Implementation(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser)
{
	if (bIsDead || bIsIncapacitated)
	{
		return;
	}

	float AppliedDamage = FMath::Max(0.0f, HitInfo.FinalDamage);

	if (bHasBioShield)
	{
		AppliedDamage *= (1.0f - BioShieldAbsorption);
	}

	if (bIsEnraged)
	{
		AppliedDamage *= RageIncomingDamageMultiplier;
	}

	Health = FMath::Max(0.0f, Health - AppliedDamage);
	Toxicity = FMath::Min(MaxToxicity, Toxicity + (AppliedDamage * ToxicityGainPerDamage));

	OnHostDamaged.Broadcast(HitInfo, DamageCauser);

	switch (HitInfo.WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		ApplyLocomotorNerveReaction();
		if (bDestroyWeakPointOnCriticalHit && (HitInfo.bTacticalModeHit || HitInfo.bTriggeredDismemberment))
		{
			DestroyWeakPoint(EProjectOrganoidWeakPointType::LocomotorNerves);
		}
		break;
	case EProjectOrganoidWeakPointType::OpticalNodes:
		ApplyOpticalNodeReaction();
		if (bDestroyWeakPointOnCriticalHit && HitInfo.bTacticalModeHit)
		{
			DestroyWeakPoint(EProjectOrganoidWeakPointType::OpticalNodes);
		}
		break;
	case EProjectOrganoidWeakPointType::OrganoidCore:
		ApplyBioCoreReaction(HitInfo.bTriggeredIncapacitation);
		if (bDestroyWeakPointOnCriticalHit && (HitInfo.bTacticalModeHit || HitInfo.bTriggeredIncapacitation))
		{
			DestroyWeakPoint(EProjectOrganoidWeakPointType::OrganoidCore);
		}
		break;
	default:
		SetStaggered(true);
		break;
	}

	if (HitInfo.bTriggeredDismemberment)
	{
		SetDismembered(true);
		ApplyLocomotorNerveReaction();
		DestroyWeakPoint(EProjectOrganoidWeakPointType::LocomotorNerves);
	}

	if (HitInfo.bTriggeredIncapacitation)
	{
		SetIncapacitated(true);
	}

	BP_OnWeakPointReaction(HitInfo.WeakPoint, HitInfo);

	if (Health <= 0.0f)
	{
		HandleDeath();
	}
}

void AProjectOrganoidHostBase::DestroyWeakPoint(EProjectOrganoidWeakPointType WeakPoint)
{
	USphereComponent* Hitbox = nullptr;
	bool* DestroyedFlag = nullptr;

	switch (WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		Hitbox = LocomotorNervesHitbox;
		DestroyedFlag = &bLocomotorNervesDestroyed;
		break;
	case EProjectOrganoidWeakPointType::OpticalNodes:
		Hitbox = OpticalNodesHitbox;
		DestroyedFlag = &bOpticalNodesDestroyed;
		break;
	case EProjectOrganoidWeakPointType::OrganoidCore:
		Hitbox = BioCoreHitbox;
		DestroyedFlag = &bBioCoreDestroyed;
		break;
	default:
		return;
	}

	if (!DestroyedFlag || *DestroyedFlag)
	{
		return;
	}

	*DestroyedFlag = true;
	if (Hitbox)
	{
		Hitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Hitbox->SetHiddenInGame(true);
	}

	OnHostStateChanged.Broadcast(TEXT("WeakPointDestroyed"));
	EvaluatePhaseShiftMutation(WeakPoint);
}

void AProjectOrganoidHostBase::EvaluatePhaseShiftMutation(EProjectOrganoidWeakPointType DestroyedWeakPoint)
{
	// Optical destruction → emergency bio-shield
	if (DestroyedWeakPoint == EProjectOrganoidWeakPointType::OpticalNodes)
	{
		ActivateBioShield();
	}

	// Locomotor / Bio-Core destruction → rage phase-shift
	if (DestroyedWeakPoint == EProjectOrganoidWeakPointType::LocomotorNerves
		|| DestroyedWeakPoint == EProjectOrganoidWeakPointType::OrganoidCore)
	{
		EnterRageState();
	}

	// Two+ weak points gone → force both mutations
	const int32 DestroyedCount =
		(bLocomotorNervesDestroyed ? 1 : 0)
		+ (bOpticalNodesDestroyed ? 1 : 0)
		+ (bBioCoreDestroyed ? 1 : 0);

	if (DestroyedCount >= 2)
	{
		EnterRageState();
		ActivateBioShield();
	}
}

void AProjectOrganoidHostBase::EnterRageState()
{
	if (bIsEnraged || bIsDead || bIsIncapacitated)
	{
		return;
	}

	bIsEnraged = true;
	RefreshMovementSpeed();
	ConfigureAIPerception();
	OnHostStateChanged.Broadcast(TEXT("Rage"));
	BP_OnRageStateEntered();
}

void AProjectOrganoidHostBase::ActivateBioShield()
{
	if (bIsDead || bIsIncapacitated)
	{
		return;
	}

	bHasBioShield = true;
	OnHostStateChanged.Broadcast(TEXT("BioShieldOn"));
	BP_OnBioShieldChanged(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BioShieldTimer);
		World->GetTimerManager().SetTimer(
			BioShieldTimer,
			this,
			&AProjectOrganoidHostBase::ExpireBioShield,
			BioShieldDuration,
			false);
	}
}

bool AProjectOrganoidHostBase::StripBioShield()
{
	if (!bHasBioShield)
	{
		return false;
	}

	bHasBioShield = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BioShieldTimer);
	}

	OnHostStateChanged.Broadcast(TEXT("BioShieldStripped"));
	BP_OnBioShieldChanged(false);
	SetStaggered(true);
	return true;
}

void AProjectOrganoidHostBase::ExpireBioShield()
{
	if (!bHasBioShield)
	{
		return;
	}

	bHasBioShield = false;
	OnHostStateChanged.Broadcast(TEXT("BioShieldExpired"));
	BP_OnBioShieldChanged(false);
}

void AProjectOrganoidHostBase::ApplyLocomotorNerveReaction()
{
	SetStaggered(true);
	RefreshMovementSpeed();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocomotorSlowTimer);
		World->GetTimerManager().SetTimer(
			LocomotorSlowTimer,
			this,
			&AProjectOrganoidHostBase::RestoreLocomotorSpeed,
			LocomotorSlowDuration,
			false);
	}

	OnHostStateChanged.Broadcast(TEXT("LocomotorSlowed"));
}

void AProjectOrganoidHostBase::ApplyOpticalNodeReaction()
{
	SetBlinded(true);
	SetStaggered(true);

	if (AIPerception)
	{
		AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OpticalBlindTimer);
		World->GetTimerManager().SetTimer(
			OpticalBlindTimer,
			this,
			&AProjectOrganoidHostBase::RestoreOpticalSight,
			OpticalBlindDuration,
			false);
	}

	OnHostStateChanged.Broadcast(TEXT("Blinded"));
}

void AProjectOrganoidHostBase::ApplyBioCoreReaction(bool bForceIncapacitate)
{
	SetStaggered(true);
	Toxicity = FMath::Min(MaxToxicity, Toxicity + 20.0f);

	if (bForceIncapacitate)
	{
		SetIncapacitated(true);
	}

	OnHostStateChanged.Broadcast(TEXT("BioCoreStruck"));
}

void AProjectOrganoidHostBase::SetStaggered(bool bNewStaggered)
{
	if (bIsStaggered == bNewStaggered)
	{
		return;
	}

	bIsStaggered = bNewStaggered;
	OnHostStateChanged.Broadcast(bIsStaggered ? TEXT("Staggered") : TEXT("StaggerCleared"));

	if (bIsStaggered)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(StaggerTimer);
			World->GetTimerManager().SetTimer(
				StaggerTimer,
				this,
				&AProjectOrganoidHostBase::ClearStagger,
				StaggerDuration,
				false);
		}
	}
}

void AProjectOrganoidHostBase::SetBlinded(bool bNewBlinded)
{
	if (bIsBlinded == bNewBlinded)
	{
		return;
	}

	bIsBlinded = bNewBlinded;
	OnHostStateChanged.Broadcast(bIsBlinded ? TEXT("Blinded") : TEXT("SightRestored"));
}

void AProjectOrganoidHostBase::SetDismembered(bool bNewDismembered)
{
	if (bIsDismembered == bNewDismembered)
	{
		return;
	}

	bIsDismembered = bNewDismembered;
	if (bIsDismembered)
	{
		OnHostStateChanged.Broadcast(TEXT("Dismembered"));
		RefreshMovementSpeed();
	}
}

void AProjectOrganoidHostBase::SetIncapacitated(bool bNewIncapacitated)
{
	if (bIsIncapacitated == bNewIncapacitated)
	{
		return;
	}

	bIsIncapacitated = bNewIncapacitated;
	if (bIsIncapacitated)
	{
		bHasBioShield = false;
		bIsEnraged = false;

		if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}

		if (AIPerception)
		{
			AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
			AIPerception->SetSenseEnabled(UAISense_Hearing::StaticClass(), false);
		}

		OnHostStateChanged.Broadcast(TEXT("Incapacitated"));
	}
}

void AProjectOrganoidHostBase::RefreshMovementSpeed()
{
	if (bIsIncapacitated || bIsDead)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	float Speed = CachedWalkSpeed;
	if (bIsDismembered || bLocomotorNervesDestroyed)
	{
		Speed *= LocomotorSlowMultiplier;
	}
	if (bIsEnraged)
	{
		Speed *= RageSpeedMultiplier;
	}

	MoveComp->MaxWalkSpeed = Speed;
}

void AProjectOrganoidHostBase::RestoreLocomotorSpeed()
{
	RefreshMovementSpeed();
}

void AProjectOrganoidHostBase::RestoreOpticalSight()
{
	if (bIsIncapacitated || bIsDead)
	{
		return;
	}

	SetBlinded(false);
	if (AIPerception && !bOpticalNodesDestroyed)
	{
		AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), true);
	}
}

void AProjectOrganoidHostBase::ClearStagger()
{
	if (!bIsIncapacitated && !bIsDead)
	{
		SetStaggered(false);
	}
}

void AProjectOrganoidHostBase::ClearStatusEffects()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocomotorSlowTimer);
		World->GetTimerManager().ClearTimer(OpticalBlindTimer);
		World->GetTimerManager().ClearTimer(StaggerTimer);
		World->GetTimerManager().ClearTimer(BioShieldTimer);
		World->GetTimerManager().ClearTimer(NoiseStimulusTimer);
	}

	bIsStaggered = false;
	bIsBlinded = false;
	bHasRecentNoiseStimulus = false;

	if (!bIsIncapacitated && !bIsDead)
	{
		RefreshMovementSpeed();
		if (AIPerception)
		{
			AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), !bOpticalNodesDestroyed);
			AIPerception->SetSenseEnabled(UAISense_Hearing::StaticClass(), true);
		}
	}
}

void AProjectOrganoidHostBase::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	bHasBioShield = false;
	bIsEnraged = false;
	SetIncapacitated(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	OnHostDied.Broadcast();
	BP_OnHostDied();

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(TEXT("Event_HostNeutralized"));
		}

		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->RecordHostKill(1);
		}
	}
}
