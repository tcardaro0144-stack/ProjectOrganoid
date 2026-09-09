// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.generated.h"

class AProjectOrganoidCharacter;
class AActor;

/** Why TryActivateEquipped failed. None means the last call succeeded or has not run. */
UENUM(BlueprintType)
enum class EProjectOrganoidBiologicalAdaptationFailReason : uint8
{
	None UMETA(DisplayName = "None"),
	Unequipped UMETA(DisplayName = "Unequipped"),
	InsufficientPE UMETA(DisplayName = "Insufficient PE"),
	Cooldown UMETA(DisplayName = "Cooldown"),
	NoTarget UMETA(DisplayName = "No Target"),
	OutOfRange UMETA(DisplayName = "Out Of Range"),
	InvalidTarget UMETA(DisplayName = "Invalid Target"),
	Dead UMETA(DisplayName = "Dead")
};

/**
 *  Designer-authored Biological Adaptation definition.
 *  Ownership lives on the Character component. Equipped loadout is a single slot.
 *  Not a weapon mod.
 */
UCLASS(BlueprintType, Abstract)
class PROJECTORGANOID_API UProjectOrganoidBiologicalAdaptationData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation")
	FName AdaptationId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|PE", meta = (ClampMin = "0.0"))
	float PECost = 0.0f;

	/** Wall-clock cooldown seconds. Not dilation-affected. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|Timing", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|Target", meta = (ClampMin = "0.0"))
	float MaxTargetRange = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|Target", meta = (ClampMin = "0"))
	int32 TargetCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Adaptation|Target")
	bool bRequiresTarget = true;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("ProjectOrganoidBiologicalAdaptation"), AdaptationId.IsNone() ? GetFName() : AdaptationId);
	}

	/** Resolve a valid target without spending PE. Sets FailReason on reject. */
	virtual bool TryResolveTarget(
		AProjectOrganoidCharacter* Character,
		AActor*& OutTarget,
		EProjectOrganoidBiologicalAdaptationFailReason& FailReason) const;

	/** Apply the committed effect. Called only after PE is spent. */
	virtual bool ExecuteOnTarget(AProjectOrganoidCharacter* Character, AActor* Target) const;
};

/** Disk record for the single equipped adaptation slot. */
USTRUCT(BlueprintType)
struct FProjectOrganoidSavedBiologicalAdaptation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FSoftObjectPath AdaptationDataPath;

	UPROPERTY(BlueprintReadWrite, Category = "Save")
	FName AdaptationId = NAME_None;
};
