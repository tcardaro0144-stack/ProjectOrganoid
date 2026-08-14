// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractionTypes.generated.h"

/** OSHA / BSL facility keycard clearance for door locks */
UENUM(BlueprintType)
enum class EProjectOrganoidSecurityTier : uint8
{
	None UMETA(DisplayName = "None"),
	Level1_Admin UMETA(DisplayName = "Level 1 — Administrative"),
	Level2_Lab UMETA(DisplayName = "Level 2 — BSL Lab"),
	Level3_Cryo UMETA(DisplayName = "Level 3 — Cryogenic"),
	Level4_Compute UMETA(DisplayName = "Level 4 — Bio-Neural Compute"),
	Level5_Core UMETA(DisplayName = "Level 5 — Reactor / Incubator")
};

/** Environmental hazard types in the Epitope complex */
UENUM(BlueprintType)
enum class EProjectOrganoidHazardType : uint8
{
	None UMETA(DisplayName = "None"),
	UVCRadiation UMETA(DisplayName = "UV-C Radiation"),
	LiquidN2Frost UMETA(DisplayName = "Liquid N2 Frost"),
	ToxicGas UMETA(DisplayName = "Toxic Gas / Toxicity"),
	Biohazard UMETA(DisplayName = "Biohazard"),
	ExtremeHeat UMETA(DisplayName = "Extreme Heat")
};

/** How a brush hazard volume applies damage over time */
UENUM(BlueprintType)
enum class EProjectOrganoidHazardApplicationType : uint8
{
	Continuous UMETA(DisplayName = "Continuous (Over Time)"),
	Burst UMETA(DisplayName = "Burst (Interval Pulses)")
};
