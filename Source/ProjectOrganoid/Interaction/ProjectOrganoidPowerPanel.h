// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidPowerPanel.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AProjectOrganoidCharacter;

/**
 *  Self-powered breaker. Default path restores a sector on interact.
 *  Optional discovery-before-restore path reports emergency status and fires a
 *  one-shot discovery event without changing sector power (Neuro campaign).
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

	/** Fired only on successful restore-on-interact (default path). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	FName SuccessObjectiveEventId = NAME_None;

	/**
	 * When true, interact reports FailureStatusReport and fires DiscoveryObjectiveEventId
	 * once without SetSectorPowerState / restore engagement. Default false preserves
	 * existing restore-on-interact panels.
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

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Power")
	void BP_OnPowerPanelEngaged(AProjectOrganoidCharacter* Interactor, EProjectOrganoidPowerState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Power|Discovery")
	void BP_OnPowerFailureStatusReported(AProjectOrganoidCharacter* Interactor, const FText& StatusReport, bool bFirstDiscovery);

protected:

	void NotifyObjectiveEvent(FName EventId) const;
	void RefreshPrompt();
	bool InteractDiscoverPowerFailure(AProjectOrganoidCharacter* Interactor);
	bool InteractRestorePower(AProjectOrganoidCharacter* Interactor);
	void ReportFailureStatus(AProjectOrganoidCharacter* Interactor, bool bFirstDiscovery);
};
