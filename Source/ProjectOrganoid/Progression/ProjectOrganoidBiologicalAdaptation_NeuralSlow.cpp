// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostBase.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"

namespace
{
	constexpr float AimConeCosine = 0.9063f; // ~25 degrees

	AProjectOrganoidHostBase* AsLivingHost(AActor* Actor)
	{
		AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Actor);
		if (!Host || Host->bIsDead || Host->bIsIncapacitated)
		{
			return nullptr;
		}
		return Host;
	}

	float DistanceToHost(const AProjectOrganoidCharacter* Character, const AProjectOrganoidHostBase* Host)
	{
		if (!Character || !Host)
		{
			return TNumericLimits<float>::Max();
		}
		return FVector::Dist(Character->GetActorLocation(), Host->GetActorLocation());
	}
}

UProjectOrganoidBiologicalAdaptation_NeuralSlow::UProjectOrganoidBiologicalAdaptation_NeuralSlow()
{
	AdaptationId = TEXT("NeuralSlow");
	DisplayName = FText::FromString(TEXT("Neural Slow"));
	// PROVISIONAL DESIGN TUNING — first campaign adaptation. Not final balance.
	PECost = 20.0f;
	CooldownSeconds = 8.0f;
	MaxTargetRange = 800.0f;
	TargetCount = 1;
	bRequiresTarget = true;
	LocomotorSpeedMultiplier = 0.6f;
	DurationSeconds = 4.0f;
}

const TCHAR* UProjectOrganoidBiologicalAdaptation_NeuralSlow::ContentPath()
{
	return TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");
}

UProjectOrganoidBiologicalAdaptationData* UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve()
{
	if (UProjectOrganoidBiologicalAdaptationData* Asset = LoadObject<UProjectOrganoidBiologicalAdaptationData>(nullptr, ContentPath()))
	{
		return Asset;
	}
	return GetMutableDefault<UProjectOrganoidBiologicalAdaptation_NeuralSlow>();
}

bool UProjectOrganoidBiologicalAdaptation_NeuralSlow::TryResolveTarget(
	AProjectOrganoidCharacter* Character,
	AActor*& OutTarget,
	EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const
{
	OutTarget = nullptr;
	if (!Character)
	{
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
		return false;
	}

	UWorld* World = Character->GetWorld();
	if (!World)
	{
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
		return false;
	}

	FVector Direction = FVector::ForwardVector;
	const FVector Start = UProjectOrganoidBiologicalAdaptationComponent::GetAimStartAndDirection(Character, Direction);
	const FVector End = Start + Direction * MaxTargetRange;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuralSlowAim), false, Character);
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
	{
		if (AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Hit.GetActor()))
		{
			if (Host->bIsDead || Host->bIsIncapacitated)
			{
				FailReason = EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget;
				return false;
			}
			if (DistanceToHost(Character, Host) > MaxTargetRange)
			{
				FailReason = EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange;
				return false;
			}
			OutTarget = Host;
			FailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
			return true;
		}
	}

	AProjectOrganoidHostBase* BestHost = nullptr;
	float BestDot = AimConeCosine;
	bool bSawHostInWorld = false;
	bool bSawDeadAimedHost = false;
	bool bSawOutOfRangeAimedHost = false;

	for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
	{
		AProjectOrganoidHostBase* Host = *It;
		if (!Host)
		{
			continue;
		}
		bSawHostInWorld = true;

		const FVector ToHost = (Host->GetActorLocation() - Start);
		const float Dist = ToHost.Size();
		if (Dist <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Dot = FVector::DotProduct(Direction, ToHost / Dist);
		if (Dot < AimConeCosine)
		{
			continue;
		}

		if (Host->bIsDead || Host->bIsIncapacitated)
		{
			bSawDeadAimedHost = true;
			continue;
		}

		if (Dist > MaxTargetRange)
		{
			bSawOutOfRangeAimedHost = true;
			continue;
		}

		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestHost = Host;
		}
	}

	if (BestHost)
	{
		OutTarget = BestHost;
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
		return true;
	}

	if (bSawDeadAimedHost)
	{
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget;
		return false;
	}
	if (bSawOutOfRangeAimedHost)
	{
		FailReason = EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange;
		return false;
	}

	FailReason = bSawHostInWorld
		? EProjectOrganoidBiologicalAdaptationFailReason::NoTarget
		: EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
	return false;
}

bool UProjectOrganoidBiologicalAdaptation_NeuralSlow::ExecuteOnTarget(
	AProjectOrganoidCharacter* Character,
	AActor* Target) const
{
	AProjectOrganoidHostBase* Host = AsLivingHost(Target);
	if (!Host)
	{
		return false;
	}

	if (!Host->ApplyBiologicalLocomotorSlow(LocomotorSpeedMultiplier, DurationSeconds))
	{
		return false;
	}

	Host->NotifySuccessfulBiologicalAdaptation(Character);
	return true;
}
