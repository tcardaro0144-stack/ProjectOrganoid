// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "ProjectOrganoidAdminAccessDoorFacilityStateListener.generated.h"

class UProjectOrganoidAdminFacilityStateSubsystem;

/**
 * Section 21B Access Door listener. Applies Admin facility posture to the
 * existing Blueprint bLocked flag as a lockdown override. Does not own
 * facility state, power, global lockdown, lighting, or the opening Timeline.
 */
UCLASS(ClassGroup = (Admin), meta = (BlueprintSpawnableComponent))
class PROJECTORGANOID_API UProjectOrganoidAdminAccessDoorFacilityStateListener : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidAdminAccessDoorFacilityStateListener();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Admin|FacilityState")
	bool bBaselineLocked = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Admin|FacilityState")
	bool bBaselineCaptured = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Admin|FacilityState")
	bool bFacilityLockOverrideActive = false;

protected:

	UFUNCTION()
	void HandleAdminFacilityStateChanged(
		EProjectOrganoidAdminFacilityState NewState,
		EProjectOrganoidAdminFacilityState PreviousState);

	void BindAndSynchronize();
	void Unbind();
	void ApplyAdminFacilityState(EProjectOrganoidAdminFacilityState State);

	static bool ReadOwnerLocked(AActor* Owner, bool& OutLocked);
	static bool WriteOwnerLocked(AActor* Owner, bool bLocked);

	TWeakObjectPtr<UProjectOrganoidAdminFacilityStateSubsystem> BoundSubsystem;
};
