// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerPanel.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AProjectOrganoidCharacter;
class UProjectOrganoidObjectiveSubsystem;

/**
 *  Self-powered breaker. Default path restores a sector on interact.
 *  Optional discovery-before-restore path reports emergency status and fires a
 *  one-shot discovery event without changing sector power (Neuro campaign).
 *  When RequiredActiveObjectiveId is set, discovery-only applies until that
 *  objective is Active; then one interact restores power and fires SuccessObjectiveEventId.
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidPowerPanel : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidPowerPanel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PanelMesh;

	/** Always-on indicator so the breaker stays findable in a blackout */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerSector PowerSector = EProjectOrganoidPowerSector::Cryo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerState RestoredState = EProjectOrganoidPowerState::Emergency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	bool bSingleUse = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power")
	bool bHasBeenEngaged = false;

	/** Fired only on successful restore-on-interact (default path / gated restore). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	FName SuccessObjectiveEventId = NAME_None;

	/**
	 * When true, interact reports FailureStatusReport and fires DiscoveryObjectiveEventId
	 * once without SetSectorPowerState / restore engagement — unless RequiredActiveObjectiveId
	 * is Active, in which case a single interact restores power directly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Discovery")
	bool bDiscoverPowerFailureBeforeRestore = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Discovery")
	FText FailureStatusReport = FText::FromString(TEXT("PRIMARY FEED OFFLINE — EMERGENCY BACKUP ACTIVE"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Discovery")
	FName DiscoveryObjectiveEventId = FName(TEXT("Event_NeuroPowerFailureDiscovered"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Discovery")
	FText ReviewPrompt = FText::FromString(TEXT("Review Power Status"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Discovery")
	FName FailureStatusLogEntryId = FName(TEXT("Log_NeuroPowerFailureStatus"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Discovery")
	bool bHasDiscoveredPowerFailure = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Discovery")
	FText LastStatusReport;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Discovery")
	int32 DiscoveryEventFireCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Discovery")
	int32 StatusReportCount = 0;

	/**
	 * Optional gate: when set, restore + SuccessObjectiveEventId require this objective Active.
	 * None preserves legacy restore-on-interact / discovery-only behavior.
	 * Completed objective re-applies RestoredState on BeginPlay (save/reload persistence).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Restore")
	FName RequiredActiveObjectiveId = NAME_None;

	/** Prompt while RequiredActiveObjectiveId is Active and restore has not engaged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Restore")
	FText ActiveRestorePrompt = FText::FromString(TEXT("Restore NeuroGenetics power"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Restore")
	FText RestoreSuccessNotificationSpeaker = FText::FromString(TEXT("Nathan"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Restore")
	FText RestoreSuccessNotificationText = FText::FromString(
		TEXT("NeuroGenetics is back online. Cryo is still dark, but I can work with this."));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Restore", meta = (ClampMin = "0.0"))
	float RestoreSuccessNotificationDurationSeconds = 6.0f;

	/** Test / diagnostics: how many first-restore HUD lines were presented. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Restore|Diagnostics", Transient)
	int32 RestoreSuccessNotificationCount = 0;

	/** Test / diagnostics: how many times SuccessObjectiveEventId was fired. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power|Restore|Diagnostics", Transient)
	int32 SuccessEventFireCount = 0;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Power")
	void BP_OnPowerPanelEngaged(AProjectOrganoidCharacter* Interactor, EProjectOrganoidPowerState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Power|Discovery")
	void BP_OnPowerFailureStatusReported(AProjectOrganoidCharacter* Interactor, const FText& StatusReport, bool bFirstDiscovery);

	void RefreshPrompt();

protected:

	void NotifyObjectiveEvent(FName EventId) const;
	bool InteractDiscoverPowerFailure(AProjectOrganoidCharacter* Interactor);
	bool InteractRestorePower(AProjectOrganoidCharacter* Interactor);
	bool InteractReviewOnly(AProjectOrganoidCharacter* Interactor);
	void ReportFailureStatus(AProjectOrganoidCharacter* Interactor, bool bFirstDiscovery);
	void PresentRestoreSuccessNotification(AProjectOrganoidCharacter* Interactor);
	void SyncCompletedRestoreFromObjectives();
	void BindObjectivePromptRefresh();
	void UnbindObjectivePromptRefresh();
	UFUNCTION()
	void HandleObjectiveChangedForPrompt(const FProjectOrganoidObjective& Objective);
	UProjectOrganoidObjectiveSubsystem* GetObjectiveSubsystem() const;
	bool IsRequiredObjectiveActive() const;
	bool IsRequiredObjectiveCompleted() const;

	bool bBoundObjectivePromptRefresh = false;
};
