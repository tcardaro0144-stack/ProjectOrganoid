// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidHUDWidget.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidObjectiveSubsystem;

/**
 *  Diegetic vitals overlay + objective pop-up notifications for Avery Vance.
 */
UCLASS()
class UProjectOrganoidHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Bind this HUD to Avery's character and start receiving vitals / tactical events */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void BindToCharacter(AProjectOrganoidCharacter* InCharacter);

	/** Clear the character binding and stop listening for tactical mode changes */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void UnbindFromCharacter();

	/** Subscribe to objective subsystem pop-up events */
	UFUNCTION(BlueprintCallable, Category = "HUD|Objectives")
	void BindToObjectiveSubsystem();

	UFUNCTION(BlueprintCallable, Category = "HUD|Objectives")
	void UnbindFromObjectiveSubsystem();

	/** Pull current Suit Vitals from the bound character into the overlay */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void UpdateVitalsFromCharacter();

	/** Push heart rate (BPM) into Blueprint visual/audio presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetHeartRateBPM(float BPM);

	/** Push toxicity percentage into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetToxicityPercent(float Percent);

	/** Push current health into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetHealth(float HealthValue);

	/** Push current / max PE Energy into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetPEEnergy(float CurrentEnergy, float MaxEnergy);

	/** Fired when Tactical Mode engages — implement audio/visual effects in Blueprint */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Tactical")
	void OnTacticalModeActivated();

	/** Fired when Tactical Mode disengages — implement audio/visual effects in Blueprint */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Tactical")
	void OnTacticalModeDeactivated();

	/**
	 *  Objective toast / pop-up. PopupReason: Activated, Updated, Completed, Failed.
	 *  Implement fade/slide animation in Blueprint.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Objectives")
	void ShowObjectivePopup(const FProjectOrganoidObjective& Objective, FName PopupReason);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Objectives")
	void RefreshObjectiveList(const TArray<FProjectOrganoidObjective>& ActiveObjectives);

	/** Full quest journal refresh (multi-stage board) */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Journal")
	void RefreshJournal(const TArray<FProjectOrganoidObjective>& JournalEntries);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Journal")
	void ShowJournalStage(int32 StageIndex, const TArray<FProjectOrganoidObjective>& StageObjectives);

	/** Photo / scan mode overlay hooks */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Photo")
	void OnPhotoModeChanged(bool bActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Photo")
	void OnScanFocusChanged(AActor* FocusedActor, const FText& DisplayName);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Photo")
	void OnScanLoreExtracted(const FProjectOrganoidLogEntry& LoreEntry, bool bNewLore);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Photo")
	void OnPhotoCaptured(const FString& ScreenshotPath);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Vitals")
	TObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidObjectiveSubsystem> BoundObjectiveSubsystem;

	UFUNCTION()
	void HandleTacticalModeChanged(bool bIsActive);

	UFUNCTION()
	void HandleObjectivePopup(const FProjectOrganoidObjective& Objective, FName PopupReason);

	UFUNCTION()
	void HandleJournalUpdated();

	UFUNCTION()
	void HandleJournalStageChanged(int32 StageIndex, const TArray<FProjectOrganoidObjective>& StageObjectives);

	UFUNCTION()
	void HandlePhotoModeChanged(bool bActive);

	UFUNCTION()
	void HandleScanFocusChanged(AActor* FocusedActor, FText DisplayName);

	UFUNCTION()
	void HandleScanCompleted(AActor* ScannedActor, const FProjectOrganoidLogEntry& LoreEntry, bool bNewLore);

	UFUNCTION()
	void HandlePhotoCaptured(const FString& ScreenshotPath);

	void RefreshActiveObjectiveList();
	void BindPhotoScanEvents();
	void UnbindPhotoScanEvents();
};
