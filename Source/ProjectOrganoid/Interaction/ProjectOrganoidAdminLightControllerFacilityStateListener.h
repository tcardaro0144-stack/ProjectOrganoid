// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "ProjectOrganoidAdminLightControllerFacilityStateListener.generated.h"

class UProjectOrganoidAdminFacilityStateSubsystem;

/**
 * Section 21C LightController listener. Reapplies S20 zone lighting when Admin
 * facility posture changes. Does not own facility state, power, CurrentLightingZone,
 * intensity, doors, hologram, terminals, or audio.
 */
UCLASS(ClassGroup = (Admin), meta = (BlueprintSpawnableComponent))
class PROJECTORGANOID_API UProjectOrganoidAdminLightControllerFacilityStateListener : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidAdminLightControllerFacilityStateListener();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:

	UFUNCTION()
	void HandleAdminFacilityStateChanged(
		EProjectOrganoidAdminFacilityState NewState,
		EProjectOrganoidAdminFacilityState PreviousState);

	void BindAndSynchronize();
	void Unbind();
	void ReapplyOwnerLighting();

	TWeakObjectPtr<UProjectOrganoidAdminFacilityStateSubsystem> BoundSubsystem;
};
