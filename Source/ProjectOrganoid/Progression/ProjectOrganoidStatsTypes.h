// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidStatsTypes.generated.h"

/** Tracked gameplay counters for Avery's run / profile */
UENUM(BlueprintType)
enum class EProjectOrganoidStatMetric : uint8
{
	HostKills UMETA(DisplayName = "Host Kills"),
	DataPadsRead UMETA(DisplayName = "Data Pads Read"),
	SecurityGatesOverridden UMETA(DisplayName = "Security Gates Overridden"),
	DamageTaken UMETA(DisplayName = "Damage Taken")
};

/** Runtime + serialized player statistics */
USTRUCT(BlueprintType)
struct FProjectOrganoidPlayerStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	int32 HostKills = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	int32 DataPadsRead = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	int32 SecurityGatesOverridden = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Stats")
	float DamageTaken = 0.0f;

	int32 GetMetricInt(EProjectOrganoidStatMetric Metric) const
	{
		switch (Metric)
		{
		case EProjectOrganoidStatMetric::HostKills: return HostKills;
		case EProjectOrganoidStatMetric::DataPadsRead: return DataPadsRead;
		case EProjectOrganoidStatMetric::SecurityGatesOverridden: return SecurityGatesOverridden;
		case EProjectOrganoidStatMetric::DamageTaken: return FMath::FloorToInt(DamageTaken);
		default: return 0;
		}
	}

	float GetMetricFloat(EProjectOrganoidStatMetric Metric) const
	{
		switch (Metric)
		{
		case EProjectOrganoidStatMetric::DamageTaken: return DamageTaken;
		default: return static_cast<float>(GetMetricInt(Metric));
		}
	}
};

/** One achievement row inside UProjectOrganoidAchievementDataAsset */
USTRUCT(BlueprintType)
struct FProjectOrganoidAchievementDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
	FName AchievementId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
	EProjectOrganoidStatMetric Metric = EProjectOrganoidStatMetric::HostKills;

	/** Unlock when the tracked metric reaches this value (DamageTaken uses float compare). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement", meta = (ClampMin = "1.0"))
	float TargetValue = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
	bool bEnabled = true;
};

/**
 *  Designer-authored achievement definition list.
 *  Assign on UProjectOrganoidStatsSubsystem::AchievementCatalog.
 */
UCLASS(BlueprintType)
class UProjectOrganoidAchievementDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievements")
	TArray<FProjectOrganoidAchievementDefinition> Achievements;
};
