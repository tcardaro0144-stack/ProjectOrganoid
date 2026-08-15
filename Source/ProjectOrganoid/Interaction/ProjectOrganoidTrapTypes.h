// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidTrapTypes.generated.h"

/** Trap archetypes spawned into facility corridors */
UENUM(BlueprintType)
enum class EProjectOrganoidTrapType : uint8
{
	LaserTripwire UMETA(DisplayName = "Laser Tripwire"),
	PressurePlate UMETA(DisplayName = "Pressure Plate"),
	HazardEmitter UMETA(DisplayName = "Hazard Emitter")
};

/** One procedural placement request inside a corridor volume */
USTRUCT(BlueprintType)
struct FProjectOrganoidTrapSpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	EProjectOrganoidTrapType TrapType = EProjectOrganoidTrapType::LaserTripwire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	EProjectOrganoidHazardType LinkedHazard = EProjectOrganoidHazardType::UVCRadiation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	FName CorridorId = NAME_None;
};

/** Weighted mix used when procedurally filling a corridor */
USTRUCT(BlueprintType)
struct FProjectOrganoidTrapSpawnWeights
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float LaserTripwireWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float PressurePlateWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float HazardEmitterWeight = 0.75f;
};
