// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "ProjectOrganoidAdminLightingLibrary.generated.h"

/**
 * Section 20 Admin lighting helpers. Affects only APointLight actors labeled
 * Admin_ZoneLight_* that are owned by SL_Epitope_Admin. Does not touch
 * FacilityLight, terminals, hologram, Access Door, or global sky/directional lights.
 * Intensity remains S20 (AuthoredIntensity / InactiveMultiplier). Section 21C
 * composes facility-state color on the same fixtures.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidAdminLightingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Admin|Lighting")
	static bool IsApprovedAdminLightingZone(FName ZoneName);

	/** Linear light color for Admin facility posture. Converts approved sRGB hex targets. */
	UFUNCTION(BlueprintPure, Category = "Admin|Lighting")
	static FLinearColor GetAdminFacilityStateLightColor(EProjectOrganoidAdminFacilityState State);

	/** None CurrentZone leaves intensities at AuthoredIntensity (100%). Unknown zones return 0 and change nothing. */
	UFUNCTION(BlueprintCallable, Category = "Admin|Lighting", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static int32 ApplyAdminZoneLighting(UObject* WorldContextObject, FName CurrentZone, float AuthoredIntensity, float InactiveMultiplier);

	/**
	 * Shared S20 dispatch for BP_AdminRoomTrigger ActorBeginOverlap.
	 * Does not call SetCurrentRoom. Finds the unique Admin_LightController and calls SetLightingZone.
	 */
	UFUNCTION(BlueprintCallable, Category = "Admin|Lighting", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static void RequestAdminLightingZone(UObject* WorldContextObject, AActor* OtherActor, FName ZoneName);
};
