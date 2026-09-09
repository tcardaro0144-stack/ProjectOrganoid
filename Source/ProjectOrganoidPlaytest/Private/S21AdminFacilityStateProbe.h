#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "S21AdminFacilityStateProbe.generated.h"

UCLASS()
class UOrganoidAdminFacilityStateProbe : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleAdminFacilityStateChanged(
		EProjectOrganoidAdminFacilityState NewState,
		EProjectOrganoidAdminFacilityState PreviousState)
	{
		++BroadcastCount;
		LastNewState = NewState;
		LastPreviousState = PreviousState;
	}

	int32 BroadcastCount = 0;
	EProjectOrganoidAdminFacilityState LastNewState = EProjectOrganoidAdminFacilityState::Normal;
	EProjectOrganoidAdminFacilityState LastPreviousState = EProjectOrganoidAdminFacilityState::Normal;
};
