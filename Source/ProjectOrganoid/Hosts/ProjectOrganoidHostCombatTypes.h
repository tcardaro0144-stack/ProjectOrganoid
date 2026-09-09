// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidHostCombatTypes.generated.h"

/** Reusable Host combat loop. Not a StateTree. Not sector-specific. */
UENUM(BlueprintType)
enum class EProjectOrganoidHostCombatState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Investigate UMETA(DisplayName = "Investigate"),
	Pursue UMETA(DisplayName = "Pursue"),
	Attack UMETA(DisplayName = "Attack"),
	Search UMETA(DisplayName = "Search"),
	Return UMETA(DisplayName = "Return"),
	Dead UMETA(DisplayName = "Dead")
};
