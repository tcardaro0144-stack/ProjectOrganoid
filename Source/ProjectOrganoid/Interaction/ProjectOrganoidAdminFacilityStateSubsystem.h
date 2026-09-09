// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnProjectOrganoidAdminFacilityStateChanged,
	EProjectOrganoidAdminFacilityState, NewState,
	EProjectOrganoidAdminFacilityState, PreviousState);

/**
 * Authoritative Admin-local emergency posture (Section 21A).
 * Does not own power, global gate lockdown, room lighting, doors, or audio.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidAdminFacilityStateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Admin|FacilityState")
	FOnProjectOrganoidAdminFacilityStateChanged OnAdminFacilityStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Admin|FacilityState")
	void SetAdminFacilityState(EProjectOrganoidAdminFacilityState NewState);

	UFUNCTION(BlueprintPure, Category = "Admin|FacilityState")
	EProjectOrganoidAdminFacilityState GetAdminFacilityState() const { return CurrentState; }

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Admin|FacilityState")
	EProjectOrganoidAdminFacilityState CurrentState = EProjectOrganoidAdminFacilityState::Normal;
};
