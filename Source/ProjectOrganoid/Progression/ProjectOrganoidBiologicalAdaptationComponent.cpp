// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Controller.h"
#include "HAL/PlatformTime.h"

UProjectOrganoidBiologicalAdaptationComponent::UProjectOrganoidBiologicalAdaptationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AProjectOrganoidCharacter* UProjectOrganoidBiologicalAdaptationComponent::GetOwnerCharacter() const
{
	return Cast<AProjectOrganoidCharacter>(GetOwner());
}

FVector UProjectOrganoidBiologicalAdaptationComponent::GetAimStartAndDirection(
	const AProjectOrganoidCharacter* Character,
	FVector& OutDirection)
{
	OutDirection = FVector::ForwardVector;
	if (!Character)
	{
		return FVector::ZeroVector;
	}

	if (const AController* Controller = Character->GetController())
	{
		FVector ViewLoc;
		FRotator ViewRot;
		Controller->GetPlayerViewPoint(ViewLoc, ViewRot);
		OutDirection = ViewRot.Vector();
		return ViewLoc;
	}

	if (const UCameraComponent* Camera = Character->GetFollowCamera())
	{
		OutDirection = Camera->GetForwardVector();
		return Camera->GetComponentLocation();
	}

	OutDirection = Character->GetActorForwardVector();
	return Character->GetActorLocation();
}

bool UProjectOrganoidBiologicalAdaptationComponent::PathsMatch(
	const FSoftObjectPath& Path,
	const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const
{
	if (Path.IsNull() || !AdaptationData)
	{
		return false;
	}
	if (Path == FSoftObjectPath(AdaptationData))
	{
		return true;
	}
	return Path.TryLoad() == AdaptationData;
}

bool UProjectOrganoidBiologicalAdaptationComponent::UnlockAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData)
{
	if (!AdaptationData)
	{
		return false;
	}

	const FSoftObjectPath Path(AdaptationData);
	if (Path.IsNull() || IsAdaptationUnlocked(AdaptationData))
	{
		return IsAdaptationUnlocked(AdaptationData);
	}

	UnlockedAdaptations.Add(Path);
	return true;
}

bool UProjectOrganoidBiologicalAdaptationComponent::IsAdaptationUnlocked(const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const
{
	if (!AdaptationData)
	{
		return false;
	}

	const FSoftObjectPath Path(AdaptationData);
	if (UnlockedAdaptations.Contains(Path))
	{
		return true;
	}

	for (const FSoftObjectPath& Existing : UnlockedAdaptations)
	{
		if (PathsMatch(Existing, AdaptationData))
		{
			return true;
		}
	}
	return false;
}

TArray<UProjectOrganoidBiologicalAdaptationData*> UProjectOrganoidBiologicalAdaptationComponent::GetUnlockedAdaptations() const
{
	TArray<UProjectOrganoidBiologicalAdaptationData*> Result;
	for (const FSoftObjectPath& Path : UnlockedAdaptations)
	{
		if (UProjectOrganoidBiologicalAdaptationData* Adaptation = Cast<UProjectOrganoidBiologicalAdaptationData>(Path.TryLoad()))
		{
			Result.AddUnique(Adaptation);
		}
	}
	return Result;
}

bool UProjectOrganoidBiologicalAdaptationComponent::EquipAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData)
{
	if (!AdaptationData || !IsAdaptationUnlocked(AdaptationData))
	{
		return false;
	}

	if (EquippedAdaptation == AdaptationData)
	{
		return true;
	}

	EquippedAdaptation = AdaptationData;
	OnAdaptationLoadoutChanged.Broadcast(EquippedAdaptation, true);
	return true;
}

bool UProjectOrganoidBiologicalAdaptationComponent::UnequipAdaptation()
{
	if (!EquippedAdaptation)
	{
		return true;
	}

	UProjectOrganoidBiologicalAdaptationData* Previous = EquippedAdaptation;
	EquippedAdaptation = nullptr;
	OnAdaptationLoadoutChanged.Broadcast(Previous, false);
	return true;
}

FSoftObjectPath UProjectOrganoidBiologicalAdaptationComponent::GetEquippedAdaptationPath() const
{
	return EquippedAdaptation ? FSoftObjectPath(EquippedAdaptation) : FSoftObjectPath();
}

bool UProjectOrganoidBiologicalAdaptationComponent::IsOnCooldown() const
{
	return GetCooldownRemaining() > KINDA_SMALL_NUMBER;
}

float UProjectOrganoidBiologicalAdaptationComponent::GetCooldownDuration() const
{
	return EquippedAdaptation ? EquippedAdaptation->CooldownSeconds : 0.0f;
}

float UProjectOrganoidBiologicalAdaptationComponent::GetCooldownRemaining() const
{
	const float Duration = GetCooldownDuration();
	if (Duration <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const double Elapsed = FPlatformTime::Seconds() - LastActivationRealTimeSeconds;
	return FMath::Max(0.0f, Duration - static_cast<float>(Elapsed));
}

FName UProjectOrganoidBiologicalAdaptationComponent::GetUnavailableReason() const
{
	if (!EquippedAdaptation)
	{
		return TEXT("Unequipped");
	}
	if (const AProjectOrganoidCharacter* Character = GetOwnerCharacter())
	{
		if (Character->IsPlayerDead())
		{
			return TEXT("Dead");
		}
		if (Character->GetPEEnergy() < EquippedAdaptation->PECost)
		{
			return TEXT("InsufficientPE");
		}
	}
	if (IsOnCooldown())
	{
		return TEXT("Cooldown");
	}
	return NAME_None;
}

bool UProjectOrganoidBiologicalAdaptationComponent::TryActivateEquipped()
{
	LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::None;

	AProjectOrganoidCharacter* Character = GetOwnerCharacter();
	if (!Character || Character->IsPlayerDead())
	{
		LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::Dead;
		return false;
	}

	if (!EquippedAdaptation)
	{
		LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::Unequipped;
		return false;
	}

	if (IsOnCooldown())
	{
		LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::Cooldown;
		return false;
	}

	if (Character->GetPEEnergy() < EquippedAdaptation->PECost)
	{
		LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::InsufficientPE;
		return false;
	}

	AActor* Target = nullptr;
	EProjectOrganoidBiologicalAdaptationFailReason TargetFail = EProjectOrganoidBiologicalAdaptationFailReason::None;
	if (!EquippedAdaptation->TryResolveTarget(Character, Target, TargetFail) || (EquippedAdaptation->bRequiresTarget && !Target))
	{
		LastFailReason = TargetFail;
		if (LastFailReason == EProjectOrganoidBiologicalAdaptationFailReason::None)
		{
			LastFailReason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
		}
		return false;
	}

	Character->ApplyPEEnergyDelta(-EquippedAdaptation->PECost);
	LastActivationRealTimeSeconds = FPlatformTime::Seconds();
	EquippedAdaptation->ExecuteOnTarget(Character, Target);
	return true;
}

void UProjectOrganoidBiologicalAdaptationComponent::ApplyUnlockedAdaptations(const TArray<FSoftObjectPath>& Paths)
{
	UnlockedAdaptations.Reset();
	for (const FSoftObjectPath& Path : Paths)
	{
		if (!Path.IsNull())
		{
			UnlockedAdaptations.AddUnique(Path);
		}
	}

	if (EquippedAdaptation && !IsAdaptationUnlocked(EquippedAdaptation))
	{
		EquippedAdaptation = nullptr;
	}
}

void UProjectOrganoidBiologicalAdaptationComponent::ApplyEquippedAdaptation(const FSoftObjectPath& Path)
{
	EquippedAdaptation = nullptr;
	if (Path.IsNull())
	{
		return;
	}

	if (UProjectOrganoidBiologicalAdaptationData* Adaptation = Cast<UProjectOrganoidBiologicalAdaptationData>(Path.TryLoad()))
	{
		if (IsAdaptationUnlocked(Adaptation))
		{
			EquippedAdaptation = Adaptation;
		}
	}
}
