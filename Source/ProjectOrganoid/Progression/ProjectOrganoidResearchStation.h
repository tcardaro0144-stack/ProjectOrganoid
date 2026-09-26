// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidResearchStation.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidResearchStationWidget;
class UProjectOrganoidWeaponModData;
class UStaticMeshComponent;

/**
 *  Research Station — free remount of already-unlocked build elements.
 *  Unlock and install are separate. No SOT charge. No heal. No checkpoint.
 *  Interaction is locked only by encounter presence (Pursue / Attack).
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidResearchStation : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidResearchStation();

	/** Provisional blockout console. Not final Research Station art. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|UI")
	TSubclassOf<UProjectOrganoidResearchStationWidget> StationWidgetClass;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "ResearchStation")
	bool IsLockedByEncounter() const;

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	bool IsStationUIOpen() const;

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	UProjectOrganoidResearchStationWidget* GetActiveStationWidget() const { return ActiveStationWidget; }

	UFUNCTION(BlueprintPure, Category = "ResearchStation")
	int32 GetConfigurationSOTCost() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	UProjectOrganoidResearchStationWidget* OpenResearchStationUI(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void CloseResearchStationUI();

	void NotifyStationUIClosed();

	/** Install an already-unlocked mod. Costs 0 SOT. Does not consume items. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Loadout")
	bool TryInstallUnlockedMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData);

	/** Remove an installed mod. Ownership is retained. Costs 0 SOT. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Loadout")
	bool TryRemoveInstalledMod(AProjectOrganoidCharacter* Character, EProjectOrganoidWeaponModSlot Slot);

	/** Equip an already-unlocked adaptation. Costs 0 SOT. Ownership is retained on unequip. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool TryEquipUnlockedAdaptation(AProjectOrganoidCharacter* Character, UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool TryUnequipAdaptation(AProjectOrganoidCharacter* Character);

	/**
	 * Called by the station widget only after TryEquipUnlockedAdaptation returns success.
	 * Default-off. No credit unless the campaign contract is configured and the equipped
	 * adaptation matches CampaignCreditAdaptation.
	 */
	void NotifyCampaignAdaptationEquipped(
		AProjectOrganoidCharacter* Character,
		UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	/**
	 * Optional campaign contract (default off). When CampaignRequiredActiveObjectiveId is None,
	 * the station keeps its existing free-remount behavior: no unlock, no objective event.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	FName CampaignRequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	TSoftObjectPtr<UProjectOrganoidBiologicalAdaptationData> CampaignUnlockAdaptation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	TSoftObjectPtr<UProjectOrganoidBiologicalAdaptationData> CampaignCreditAdaptation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	FName CampaignSuccessObjectiveEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	FName CampaignReplayGuardObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	FText CampaignSuccessNotificationSpeaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign")
	FText CampaignSuccessNotificationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Campaign", meta = (ClampMin = "0.0"))
	float CampaignSuccessNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|Campaign")
	int32 CampaignSuccessNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|Campaign")
	int32 CampaignSuccessEventFireCount = 0;

	/**
	 * Beat 19 presentation contract. Default off, so the Neural Slow intro contract is unchanged.
	 * When configured, a successful interact credits Obj_UseResearchStation once. No currency and no shop.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec")
	FName RespecRequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec")
	FName RespecSuccessObjectiveEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec")
	FName RespecReplayGuardObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec")
	FText RespecNotificationSpeaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec")
	FText RespecNotificationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|Respec", meta = (ClampMin = "0.0"))
	float RespecNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|Respec")
	int32 RespecNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|Respec")
	int32 RespecEventFireCount = 0;

	/**
	 * Beat 20 syringe-kit contract. Default off, so the intro and free-respec contracts stay intact.
	 * Two successful interacts while Obj_RecoverSyringeKit is active each advance the objective by 1.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FName SyringeRequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FName SyringeSuccessObjectiveEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FName SyringeReplayGuardObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FText SyringePrompt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FText SyringeNotificationSpeaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit")
	FText SyringeNotificationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|SyringeKit", meta = (ClampMin = "0.0"))
	float SyringeNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|SyringeKit")
	int32 SyringeNotificationCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation|SyringeKit")
	int32 SyringeEventFireCount = 0;

	virtual FText GetInteractionPrompt() const override;

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidResearchStationWidget> ActiveStationWidget;

	bool IsCampaignContractConfigured() const;
	bool IsCampaignObjectiveInState(FName ObjectiveId, EProjectOrganoidObjectiveState State) const;
	bool IsCampaignObjectiveActive() const;
	bool IsCampaignReplayGuardCompleted() const;
	bool AdaptationMatchesSoft(
		const TSoftObjectPtr<UProjectOrganoidBiologicalAdaptationData>& Soft,
		const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const;
	void ApplyCampaignUnlockIfActive(AProjectOrganoidCharacter* Interactor);
	void TryReconcileCampaignIfAlreadyEquipped(AProjectOrganoidCharacter* Interactor);
	void AwardCampaignEquipCredit(AProjectOrganoidCharacter* Interactor);
	void PresentCampaignSuccessNotification(AProjectOrganoidCharacter* Interactor);
	bool IsRespecContractConfigured() const;
	bool IsRespecObjectiveActive() const;
	bool IsRespecReplayGuardCompleted() const;
	void TryAwardRespecUseCredit(AProjectOrganoidCharacter* Interactor);
	void PresentRespecNotification(AProjectOrganoidCharacter* Interactor);
	bool IsSyringeContractConfigured() const;
	bool IsSyringeObjectiveActive() const;
	bool IsSyringeReplayGuardCompleted() const;
	void TryAwardSyringeKitCredit(AProjectOrganoidCharacter* Interactor);
	void PresentSyringeNotification(AProjectOrganoidCharacter* Interactor);
};
