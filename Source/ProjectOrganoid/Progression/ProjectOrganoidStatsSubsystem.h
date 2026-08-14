// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidStatsTypes.h"
#include "ProjectOrganoidStatsSubsystem.generated.h"

class UProjectOrganoidAchievementDataAsset;
class UProjectOrganoidSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidStatChanged, EProjectOrganoidStatMetric, Metric, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidAchievementUnlocked, FName, AchievementId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidAchievementUnlockedDetailed, const FProjectOrganoidAchievementDefinition&, Definition);

/**
 *  Tracks Avery's run metrics and evaluates the achievement catalog.
 *  Persist via UProjectOrganoidSaveSubsystem capture / apply.
 */
UCLASS()
class UProjectOrganoidStatsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Achievements")
	TSoftObjectPtr<UProjectOrganoidAchievementDataAsset> AchievementCatalog;

	UPROPERTY(BlueprintAssignable, Category = "Stats")
	FOnProjectOrganoidStatChanged OnStatChanged;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Achievements")
	FOnProjectOrganoidAchievementUnlocked OnAchievementUnlocked;

	UPROPERTY(BlueprintAssignable, Category = "Stats|Achievements")
	FOnProjectOrganoidAchievementUnlockedDetailed OnAchievementUnlockedDetailed;

	UFUNCTION(BlueprintPure, Category = "Stats")
	FProjectOrganoidPlayerStats GetPlayerStats() const { return PlayerStats; }

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetStatValue(EProjectOrganoidStatMetric Metric) const;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void SetAchievementCatalog(UProjectOrganoidAchievementDataAsset* Catalog);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetStatsAndAchievements();

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordHostKill(int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordDataPadRead(int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordSecurityGateOverridden(int32 Count = 1);

	UFUNCTION(BlueprintCallable, Category = "Stats")
	void RecordDamageTaken(float Amount);

	UFUNCTION(BlueprintPure, Category = "Stats|Achievements")
	bool IsAchievementUnlocked(FName AchievementId) const;

	UFUNCTION(BlueprintPure, Category = "Stats|Achievements")
	TArray<FName> GetUnlockedAchievementIds() const { return UnlockedAchievementIds; }

	UFUNCTION(BlueprintPure, Category = "Stats|Achievements")
	TArray<FProjectOrganoidAchievementDefinition> GetAchievementDefinitions() const;

	/** Write stats + unlocked ids into a save object */
	UFUNCTION(BlueprintCallable, Category = "Stats|Save")
	void CaptureStatsToSaveGame(UProjectOrganoidSaveGame* SaveGame) const;

	/** Restore stats + unlocked ids from a save object */
	UFUNCTION(BlueprintCallable, Category = "Stats|Save")
	void ApplyStatsFromSaveGame(const UProjectOrganoidSaveGame* SaveGame);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FProjectOrganoidPlayerStats PlayerStats;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Achievements")
	TArray<FName> UnlockedAchievementIds;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidAchievementDataAsset> LoadedCatalog;

	void EnsureCatalogLoaded();
	void NotifyStatChanged(EProjectOrganoidStatMetric Metric);
	void EvaluateAchievementsForMetric(EProjectOrganoidStatMetric Metric);
	void UnlockAchievement(const FProjectOrganoidAchievementDefinition& Definition);
};
