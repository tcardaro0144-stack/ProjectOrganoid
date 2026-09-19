#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.generated.h"

UCLASS()
class UOrganoidNeuroResearchFloorArrayMissionCompletionProbe : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION()
	void HandleMissionCompleted(FName MissionId)
	{
		if (MissionId == FName(TEXT("Mission_OpeningFoundation")))
		{
			++OpeningFoundationCompletedCount;
		}
	}

	int32 OpeningFoundationCompletedCount = 0;
};
