// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.generated.h"

/**
 *  Reusable encounter-presence authority for interaction locks.
 *  Research Stations (and future bosses) register named sources.
 *  Stations must not iterate Hosts or read audio combat linger.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidEncounterPresenceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	/** Pursue and Attack lock Research Stations. All other Host states allow them. */
	UFUNCTION(BlueprintPure, Category = "Encounter|Presence")
	static bool DoesCombatStateLockStations(EProjectOrganoidHostCombatState State);

	UFUNCTION(BlueprintPure, Category = "Encounter|Presence")
	bool IsEncounterActive() const;

	UFUNCTION(BlueprintPure, Category = "Encounter|Presence")
	int32 GetActiveSourceCount() const { return ActiveSourceIds.Num(); }

	/**
	 *  Named source register for Hosts, bosses, or tests.
	 *  The station never needs the concrete encounter class.
	 */
	UFUNCTION(BlueprintCallable, Category = "Encounter|Presence")
	void SetEncounterSourceActive(FName SourceId, bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Encounter|Presence")
	void NotifySourceCombatState(FName SourceId, EProjectOrganoidHostCombatState State);

	UFUNCTION(BlueprintCallable, Category = "Encounter|Presence")
	void ClearAllEncounterSources();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Encounter|Presence")
	TArray<FName> ActiveSourceIds;
};
