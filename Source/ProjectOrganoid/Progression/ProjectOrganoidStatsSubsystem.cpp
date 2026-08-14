// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidStatsSubsystem.h"
#include "ProjectOrganoidSaveGame.h"

void UProjectOrganoidStatsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureCatalogLoaded();
}

void UProjectOrganoidStatsSubsystem::EnsureCatalogLoaded()
{
	if (LoadedCatalog)
	{
		return;
	}

	if (!AchievementCatalog.IsNull())
	{
		LoadedCatalog = AchievementCatalog.LoadSynchronous();
	}
}

void UProjectOrganoidStatsSubsystem::SetAchievementCatalog(UProjectOrganoidAchievementDataAsset* Catalog)
{
	LoadedCatalog = Catalog;
	if (Catalog)
	{
		AchievementCatalog = Catalog;
	}
}

float UProjectOrganoidStatsSubsystem::GetStatValue(EProjectOrganoidStatMetric Metric) const
{
	return PlayerStats.GetMetricFloat(Metric);
}

void UProjectOrganoidStatsSubsystem::ResetStatsAndAchievements()
{
	PlayerStats = FProjectOrganoidPlayerStats();
	UnlockedAchievementIds.Reset();
}

void UProjectOrganoidStatsSubsystem::RecordHostKill(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	PlayerStats.HostKills += Count;
	NotifyStatChanged(EProjectOrganoidStatMetric::HostKills);
}

void UProjectOrganoidStatsSubsystem::RecordDataPadRead(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	PlayerStats.DataPadsRead += Count;
	NotifyStatChanged(EProjectOrganoidStatMetric::DataPadsRead);
}

void UProjectOrganoidStatsSubsystem::RecordSecurityGateOverridden(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	PlayerStats.SecurityGatesOverridden += Count;
	NotifyStatChanged(EProjectOrganoidStatMetric::SecurityGatesOverridden);
}

void UProjectOrganoidStatsSubsystem::RecordDamageTaken(float Amount)
{
	if (Amount <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	PlayerStats.DamageTaken += Amount;
	NotifyStatChanged(EProjectOrganoidStatMetric::DamageTaken);
}

bool UProjectOrganoidStatsSubsystem::IsAchievementUnlocked(FName AchievementId) const
{
	return UnlockedAchievementIds.Contains(AchievementId);
}

TArray<FProjectOrganoidAchievementDefinition> UProjectOrganoidStatsSubsystem::GetAchievementDefinitions() const
{
	if (LoadedCatalog)
	{
		return LoadedCatalog->Achievements;
	}

	if (const UProjectOrganoidAchievementDataAsset* Catalog = AchievementCatalog.Get())
	{
		return Catalog->Achievements;
	}

	return TArray<FProjectOrganoidAchievementDefinition>();
}

void UProjectOrganoidStatsSubsystem::NotifyStatChanged(EProjectOrganoidStatMetric Metric)
{
	OnStatChanged.Broadcast(Metric, GetStatValue(Metric));
	EvaluateAchievementsForMetric(Metric);
}

void UProjectOrganoidStatsSubsystem::EvaluateAchievementsForMetric(EProjectOrganoidStatMetric Metric)
{
	EnsureCatalogLoaded();
	if (!LoadedCatalog)
	{
		return;
	}

	const float CurrentValue = GetStatValue(Metric);
	for (const FProjectOrganoidAchievementDefinition& Definition : LoadedCatalog->Achievements)
	{
		if (!Definition.bEnabled || Definition.AchievementId.IsNone() || Definition.Metric != Metric)
		{
			continue;
		}

		if (IsAchievementUnlocked(Definition.AchievementId))
		{
			continue;
		}

		if (CurrentValue + KINDA_SMALL_NUMBER >= Definition.TargetValue)
		{
			UnlockAchievement(Definition);
		}
	}
}

void UProjectOrganoidStatsSubsystem::UnlockAchievement(const FProjectOrganoidAchievementDefinition& Definition)
{
	if (Definition.AchievementId.IsNone() || IsAchievementUnlocked(Definition.AchievementId))
	{
		return;
	}

	UnlockedAchievementIds.Add(Definition.AchievementId);
	OnAchievementUnlocked.Broadcast(Definition.AchievementId);
	OnAchievementUnlockedDetailed.Broadcast(Definition);
}

void UProjectOrganoidStatsSubsystem::CaptureStatsToSaveGame(UProjectOrganoidSaveGame* SaveGame) const
{
	if (!SaveGame)
	{
		return;
	}

	SaveGame->PlayerStats = PlayerStats;
	SaveGame->UnlockedAchievementIds = UnlockedAchievementIds;
}

void UProjectOrganoidStatsSubsystem::ApplyStatsFromSaveGame(const UProjectOrganoidSaveGame* SaveGame)
{
	if (!SaveGame)
	{
		return;
	}

	PlayerStats = SaveGame->PlayerStats;
	UnlockedAchievementIds = SaveGame->UnlockedAchievementIds;
}
