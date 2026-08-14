// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidPerceptionTypes.generated.h"

/** Avery's locomotion noise profile reported to AI hearing */
UENUM(BlueprintType)
enum class EProjectOrganoidPlayerMovementNoiseState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Crouch UMETA(DisplayName = "Crouch"),
	Walk UMETA(DisplayName = "Walk"),
	Run UMETA(DisplayName = "Run")
};

/** Classified hearing stimulus kinds for organoid hosts */
UENUM(BlueprintType)
enum class EProjectOrganoidHearingStimulusKind : uint8
{
	Unknown UMETA(DisplayName = "Unknown"),
	FootstepIdle UMETA(DisplayName = "Footstep Idle"),
	FootstepCrouch UMETA(DisplayName = "Footstep Crouch"),
	FootstepWalk UMETA(DisplayName = "Footstep Walk"),
	FootstepRun UMETA(DisplayName = "Footstep Run"),
	Gunfire UMETA(DisplayName = "Gunfire"),
	GenericNoise UMETA(DisplayName = "Generic Noise")
};

namespace ProjectOrganoidNoiseTags
{
	inline const FName Footstep(TEXT("Footstep"));
	inline const FName FootstepIdle(TEXT("Footstep_Idle"));
	inline const FName FootstepCrouch(TEXT("Footstep_Crouch"));
	inline const FName FootstepWalk(TEXT("Footstep_Walk"));
	inline const FName FootstepRun(TEXT("Footstep_Run"));
	inline const FName Gunfire(TEXT("Gunfire"));
}
