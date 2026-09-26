// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostBase.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	constexpr float SyringeAimConeCosine = 0.9063f;

	enum class ESyringeWeakPoint : uint8
	{
		LocomotorNerves,
		OpticalNodes
	};

	bool WeakPointIntact(const AProjectOrganoidHostBase* Host, ESyringeWeakPoint WeakPoint)
	{
		if (!Host)
		{
			return false;
		}
		return WeakPoint == ESyringeWeakPoint::LocomotorNerves
			? !Host->bLocomotorNervesDestroyed
			: !Host->bOpticalNodesDestroyed;
	}

	bool ResolveSyringeTarget(
		const UProjectOrganoidBiologicalAdaptationData* Adaptation,
		AProjectOrganoidCharacter* Character,
		ESyringeWeakPoint WeakPoint,
		AActor*& OutTarget,
		EProjectOrganoidBiologicalAdaptationFailReason& FailReason)
	{
		OutTarget = nullptr;
		if (!Character || !Adaptation)
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
		const FVector End = Start + Direction * Adaptation->MaxTargetRange;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SyringeKitAim), false, Character);
		FHitResult Hit;
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params))
		{
			if (AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Hit.GetActor()))
			{
				const bool bLiving = !Host->bIsDead && !Host->bIsIncapacitated && WeakPointIntact(Host, WeakPoint);
				const bool bInRange = FVector::Dist(Character->GetActorLocation(), Host->GetActorLocation()) <= Adaptation->MaxTargetRange;
				if (bLiving && bInRange)
				{
					OutTarget = Host;
					FailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
					return true;
				}
			}
		}

		AProjectOrganoidHostBase* BestHost = nullptr;
		float BestDot = SyringeAimConeCosine;
		bool bSawInvalid = false;
		bool bSawOutOfRange = false;
		for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
		{
			AProjectOrganoidHostBase* Host = *It;
			if (!Host)
			{
				continue;
			}
			const FVector ToHost = Host->GetActorLocation() - Start;
			const float Dist = ToHost.Size();
			if (Dist <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const float Dot = FVector::DotProduct(Direction, ToHost / Dist);
			if (Dot < SyringeAimConeCosine)
			{
				continue;
			}
			if (Host->bIsDead || Host->bIsIncapacitated || !WeakPointIntact(Host, WeakPoint))
			{
				bSawInvalid = true;
				continue;
			}
			if (Dist > Adaptation->MaxTargetRange)
			{
				bSawOutOfRange = true;
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
		FailReason = bSawInvalid
			? EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget
			: (bSawOutOfRange
				? EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange
				: EProjectOrganoidBiologicalAdaptationFailReason::NoTarget);
		return false;
	}
}

UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt()
{
	AdaptationId = TEXT("Adaptation_LocomotorDisrupt");
	DisplayName = FText::FromString(TEXT("Locomotor Disrupt"));
	EffectDescription = FText::FromString(TEXT("Disrupts locomotor nerve clusters to impair movement"));
	PECost = 20.0f;
	CooldownSeconds = 8.0f;
	MaxTargetRange = 800.0f;
	TargetCount = 1;
	bRequiresTarget = true;
	LocomotorSpeedMultiplier = 0.5f;
	DurationSeconds = 5.0f;
}

const TCHAR* UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::ContentPath()
{
	return TEXT("/Game/Data/Adaptations/DA_Adaptation_LocomotorDisrupt.DA_Adaptation_LocomotorDisrupt");
}

UProjectOrganoidBiologicalAdaptationData* UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve()
{
	if (UProjectOrganoidBiologicalAdaptationData* Asset = LoadObject<UProjectOrganoidBiologicalAdaptationData>(nullptr, ContentPath()))
	{
		return Asset;
	}
	return GetMutableDefault<UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt>();
}

bool UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::TryResolveTarget(
	AProjectOrganoidCharacter* Character,
	AActor*& OutTarget,
	EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const
{
	return ResolveSyringeTarget(this, Character, ESyringeWeakPoint::LocomotorNerves, OutTarget, FailReason);
}

bool UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::ExecuteOnTarget(
	AProjectOrganoidCharacter* Character,
	AActor* Target) const
{
	AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Target);
	if (!Host || Host->bIsDead || Host->bIsIncapacitated || Host->bLocomotorNervesDestroyed)
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

UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::UProjectOrganoidBiologicalAdaptation_OpticalDisrupt()
{
	AdaptationId = TEXT("Adaptation_OpticalDisrupt");
	DisplayName = FText::FromString(TEXT("Optical Disrupt"));
	EffectDescription = FText::FromString(TEXT("Disrupts optical nodes to impair vision"));
	PECost = 20.0f;
	CooldownSeconds = 8.0f;
	MaxTargetRange = 800.0f;
	TargetCount = 1;
	bRequiresTarget = true;
	DurationSeconds = 5.0f;
}

const TCHAR* UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::ContentPath()
{
	return TEXT("/Game/Data/Adaptations/DA_Adaptation_OpticalDisrupt.DA_Adaptation_OpticalDisrupt");
}

UProjectOrganoidBiologicalAdaptationData* UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve()
{
	if (UProjectOrganoidBiologicalAdaptationData* Asset = LoadObject<UProjectOrganoidBiologicalAdaptationData>(nullptr, ContentPath()))
	{
		return Asset;
	}
	return GetMutableDefault<UProjectOrganoidBiologicalAdaptation_OpticalDisrupt>();
}

bool UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::TryResolveTarget(
	AProjectOrganoidCharacter* Character,
	AActor*& OutTarget,
	EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const
{
	return ResolveSyringeTarget(this, Character, ESyringeWeakPoint::OpticalNodes, OutTarget, FailReason);
}

bool UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::ExecuteOnTarget(
	AProjectOrganoidCharacter* Character,
	AActor* Target) const
{
	AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(Target);
	if (!Host || Host->bIsDead || Host->bIsIncapacitated || Host->bOpticalNodesDestroyed)
	{
		return false;
	}
	if (!Host->ApplyBiologicalOpticalBlind(DurationSeconds))
	{
		return false;
	}
	Host->NotifySuccessfulBiologicalAdaptation(Character);
	return true;
}
