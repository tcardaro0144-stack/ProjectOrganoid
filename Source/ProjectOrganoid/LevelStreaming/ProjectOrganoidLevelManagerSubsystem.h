// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidLevelManagerSubsystem.generated.h"

class AProjectOrganoidCharacter;
class AProjectOrganoidHazardZone;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidSubLevelChanged, EProjectOrganoidSubLevelTag, NewTag, EProjectOrganoidSubLevelTag, PreviousTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidLevelTransitionFinished, EProjectOrganoidSubLevelTag, ActiveTag);

/**
 *  World subsystem — async sub-level streaming, transition autosave,
 *  and global hazard context for the active Epitope floor.
 */
UCLASS()
class UProjectOrganoidLevelManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Save")
	FString TransitionSaveSlot = TEXT("OrganoidSave0");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Save")
	bool bAutoSaveOnTransition = true;

	/** Registered sub-level definitions (tag → streaming name + ambient hazards) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TArray<FProjectOrganoidSubLevelDefinition> SubLevelDefinitions;

	UPROPERTY(BlueprintAssignable, Category = "Level")
	FOnProjectOrganoidSubLevelChanged OnSubLevelChanged;

	UPROPERTY(BlueprintAssignable, Category = "Level")
	FOnProjectOrganoidLevelTransitionFinished OnLevelTransitionFinished;

	UFUNCTION(BlueprintCallable, Category = "Level")
	void RegisterSubLevelDefinition(const FProjectOrganoidSubLevelDefinition& Definition);

	UFUNCTION(BlueprintPure, Category = "Level")
	EProjectOrganoidSubLevelTag GetActiveSubLevelTag() const { return ActiveSubLevelTag; }

	UFUNCTION(BlueprintPure, Category = "Level")
	bool IsTransitioning() const { return bIsTransitioning; }

	UFUNCTION(BlueprintPure, Category = "Level")
	bool GetSubLevelDefinition(EProjectOrganoidSubLevelTag Tag, FProjectOrganoidSubLevelDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	TArray<EProjectOrganoidHazardType> GetActiveAmbientHazards() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	float GetActiveDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	float GetActiveToxicityMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	bool IsHazardTypeAmbient(EProjectOrganoidHazardType HazardType) const;

	/**
	 *  Seamless async transition: autosave → load TargetLevel → unload UnloadLevels →
	 *  set active tag / hazard context → optional teleport.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level")
	bool RequestSubLevelTransition(
		AProjectOrganoidCharacter* Character,
		EProjectOrganoidSubLevelTag TargetTag,
		FName TargetStreamingLevelName,
		const TArray<FName>& StreamingLevelsToUnload,
		bool bMakeVisibleAfterLoad,
		bool bTeleportToDestination,
		FTransform DestinationTransform);

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void RegisterHazardZone(AProjectOrganoidHazardZone* HazardZone);

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void UnregisterHazardZone(AProjectOrganoidHazardZone* HazardZone);

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void RefreshHazardZonesForActiveContext();

protected:

	UPROPERTY(VisibleAnywhere, Category = "Level")
	EProjectOrganoidSubLevelTag ActiveSubLevelTag = EProjectOrganoidSubLevelTag::None;

	UPROPERTY(VisibleAnywhere, Category = "Level")
	bool bIsTransitioning = false;

	UPROPERTY()
	TArray<TWeakObjectPtr<AProjectOrganoidHazardZone>> RegisteredHazardZones;

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> PendingTeleportCharacter;

	FTransform PendingTeleportTransform = FTransform::Identity;
	bool bPendingTeleport = false;
	EProjectOrganoidSubLevelTag PendingTargetTag = EProjectOrganoidSubLevelTag::None;
	TArray<FName> PendingUnloadLevels;
	int32 PendingLoadCallbackId = 0;

	bool AutoSaveCharacter(AProjectOrganoidCharacter* Character);
	void SetActiveSubLevelTag(EProjectOrganoidSubLevelTag NewTag);
	void FinishTransition();

	UFUNCTION()
	void OnStreamLevelLoaded();
};
