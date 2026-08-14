// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidSecurityTypes.generated.h"

/** How Avery cleared (or failed to clear) a security gate */
UENUM(BlueprintType)
enum class EProjectOrganoidSecurityOverrideMethod : uint8
{
	None UMETA(DisplayName = "None"),
	Keycard UMETA(DisplayName = "Keycard"),
	HackingTool UMETA(DisplayName = "Terminal Hacking Tool"),
	SubsystemOverride UMETA(DisplayName = "Facility Override"),
	Manual UMETA(DisplayName = "Manual / Scripted")
};

/** Per-gate seal state under facility lockdown */
UENUM(BlueprintType)
enum class EProjectOrganoidSecurityGateState : uint8
{
	Sealed UMETA(DisplayName = "Sealed"),
	Unlocked UMETA(DisplayName = "Unlocked"),
	Open UMETA(DisplayName = "Open")
};
