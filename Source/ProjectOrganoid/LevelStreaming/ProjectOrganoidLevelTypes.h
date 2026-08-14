// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidLevelTypes.generated.h"

/** Epitope facility sub-level tags used by streaming volumes */
UENUM(BlueprintType)
enum class EProjectOrganoidSubLevelTag : uint8
{
	None UMETA(DisplayName = "None / Persistent"),
	SubLevel1_Admin UMETA(DisplayName = "Sub-Level 1 — Admin & Decontamination"),
	SubLevel2_NeuroGenetics UMETA(DisplayName = "Sub-Level 2 — BSL-4 Neuro-Genetics"),
	SubLevel3_Cryo UMETA(DisplayName = "Sub-Level 3 — Cryogenic Storage"),
	SubLevel4_Compute UMETA(DisplayName = "Sub-Level 4 — Bio-Neural Compute"),
	SubLevel5_Reactor UMETA(DisplayName = "Sub-Level 5 — Core Reactor & Incubator")
};

/** Designer-authored streaming target + ambient hazard profile */
USTRUCT(BlueprintType)
struct FProjectOrganoidSubLevelDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	EProjectOrganoidSubLevelTag Tag = EProjectOrganoidSubLevelTag::None;

	/** Streaming level name (matches Level Streaming volume / sub-level short name) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	FName StreamingLevelName = NAME_None;

	/** Ambient hazards emphasized while this sub-level is active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Hazards")
	TArray<EProjectOrganoidHazardType> AmbientHazardTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Hazards", meta = (ClampMin = "0.0"))
	float AmbientDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Hazards", meta = (ClampMin = "0.0"))
	float AmbientToxicityMultiplier = 1.0f;
};
