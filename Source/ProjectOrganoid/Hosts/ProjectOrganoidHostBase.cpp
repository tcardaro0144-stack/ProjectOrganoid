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
#include "TimerManager.h"

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
}

void AProjectOrganoidHostBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

	HearingConfig->HearingRange = HearingRange;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(2.0f);

	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->ConfigureSense(*HearingConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
}

EProjectOrganoidWeakPointType AProjectOrganoidHostBase::ResolveWeakPoint_Implementation(const FHitResult& Hit) const
{
	const UPrimitiveComponent* HitComp = Hit.GetComponent();
	if (HitComp)
	{
		if (HitComp == BioCoreHitbox || HitComp->ComponentHasTag(TEXT("OrganoidCore")) || HitComp->ComponentHasTag(TEXT("BioCore")))
		{
			return EProjectOrganoidWeakPointType::OrganoidCore;
		}
		if (HitComp == LocomotorNervesHitbox || HitComp->ComponentHasTag(TEXT("LocomotorNerves")))
		{
			return EProjectOrganoidWeakPointType::LocomotorNerves;
		}
		if (HitComp == OpticalNodesHitbox || HitComp->ComponentHasTag(TEXT("OpticalNodes")))
		{
			return EProjectOrganoidWeakPointType::OpticalNodes;
		}
	}

	if (Hit.BoneName == TEXT("OrganoidCore") || Hit.BoneName == TEXT("BioCore") || Hit.BoneName == TEXT("core"))
	{
		return EProjectOrganoidWeakPointType::OrganoidCore;
	}
	if (Hit.BoneName == TEXT("LocomotorNerves") || Hit.BoneName == TEXT("spine_01") || Hit.BoneName == TEXT("pelvis"))
	{
		return EProjectOrganoidWeakPointType::LocomotorNerves;
	}
	if (Hit.BoneName == TEXT("OpticalNodes") || Hit.BoneName == TEXT("head") || Hit.BoneName == TEXT("face"))
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

	const float AppliedDamage = FMath::Max(0.0f, HitInfo.FinalDamage);
	Health = FMath::Max(0.0f, Health - AppliedDamage);
	Toxicity = FMath::Min(MaxToxicity, Toxicity + (AppliedDamage * ToxicityGainPerDamage));

	OnHostDamaged.Broadcast(HitInfo, DamageCauser);

	switch (HitInfo.WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		ApplyLocomotorNerveReaction();
		break;
	case EProjectOrganoidWeakPointType::OpticalNodes:
		ApplyOpticalNodeReaction();
		break;
	case EProjectOrganoidWeakPointType::OrganoidCore:
		ApplyBioCoreReaction(HitInfo.bTriggeredIncapacitation);
		break;
	default:
		SetStaggered(true);
		break;
	}

	if (HitInfo.bTriggeredDismemberment)
	{
		SetDismembered(true);
		ApplyLocomotorNerveReaction();
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

void AProjectOrganoidHostBase::ApplyLocomotorNerveReaction()
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = CachedWalkSpeed * LocomotorSlowMultiplier;
	}

	SetStaggered(true);

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

void AProjectOrganoidHostBase::RestoreLocomotorSpeed()
{
	if (bIsIncapacitated || bIsDead)
	{
		return;
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const float SpeedScale = bIsDismembered ? LocomotorSlowMultiplier : 1.0f;
		MoveComp->MaxWalkSpeed = CachedWalkSpeed * SpeedScale;
	}
}

void AProjectOrganoidHostBase::RestoreOpticalSight()
{
	if (bIsIncapacitated || bIsDead)
	{
		return;
	}

	SetBlinded(false);
	if (AIPerception)
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
	}

	bIsStaggered = false;
	bIsBlinded = false;

	if (!bIsIncapacitated && !bIsDead)
	{
		RestoreLocomotorSpeed();
		if (AIPerception)
		{
			AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), true);
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
	SetIncapacitated(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	OnHostDied.Broadcast();
	BP_OnHostDied();
}
