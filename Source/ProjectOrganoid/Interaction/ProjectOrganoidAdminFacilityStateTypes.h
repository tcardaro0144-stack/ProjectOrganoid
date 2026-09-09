// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidAdminFacilityStateTypes.generated.h"

/** Admin-local emergency / security posture. Separate from sector power and global gate lockdown. */
UENUM(BlueprintType)
enum class EProjectOrganoidAdminFacilityState : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Alert UMETA(DisplayName = "Alert"),
	Lockdown UMETA(DisplayName = "Lockdown")
};
