// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidPowerTypes.generated.h"

/** Facility power sectors (align with Epitope sub-levels) */
UENUM(BlueprintType)
enum class EProjectOrganoidPowerSector : uint8
{
	FacilityWide UMETA(DisplayName = "Facility-Wide"),
	Admin UMETA(DisplayName = "Admin & Decontamination"),
	NeuroGenetics UMETA(DisplayName = "BSL-4 Neuro-Genetics"),
	Cryo UMETA(DisplayName = "Cryogenic Storage"),
	Compute UMETA(DisplayName = "Bio-Neural Compute"),
	Reactor UMETA(DisplayName = "Core Reactor & Incubator")
};

/** Per-sector electrical state */
UENUM(BlueprintType)
enum class EProjectOrganoidPowerState : uint8
{
	Online UMETA(DisplayName = "Online"),
	Emergency UMETA(DisplayName = "Emergency Backup"),
	Blackout UMETA(DisplayName = "Blackout")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidSectorPowerStatus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerSector Sector = EProjectOrganoidPowerSector::FacilityWide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerState State = EProjectOrganoidPowerState::Online;
};
