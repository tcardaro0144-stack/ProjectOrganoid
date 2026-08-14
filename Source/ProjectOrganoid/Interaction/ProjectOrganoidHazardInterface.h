// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidHazardInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UProjectOrganoidHazardInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implemented by Avery (and future pawns) that react to facility hazard volumes.
 *  Uses the shared EProjectOrganoidHazardType from InteractionTypes.
 */
class IProjectOrganoidHazardInterface
{
	GENERATED_BODY()

public:

	/** Called when the actor first enters a hazard volume */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ProjectOrganoid|Hazards")
	void OnEnteredHazard(EProjectOrganoidHazardType HazardType, float Intensity);

	/** Called continuously while inside the hazard volume */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ProjectOrganoid|Hazards")
	void OnTickHazard(EProjectOrganoidHazardType HazardType, float DamageAmount, float DeltaTime);

	/** Called when the actor leaves the hazard volume */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ProjectOrganoid|Hazards")
	void OnExitedHazard(EProjectOrganoidHazardType HazardType);
};
