// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidUpgradeTerminal.generated.h"

class UStaticMeshComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidUpgradePurchased, AProjectOrganoidCharacter*, Character, EProjectOrganoidUpgradeType, UpgradeType, int32, NewLevel);

/**
 *  Dr. Sterling upgrade terminal — spend Synthetic Organoid Tissue (SOT)
 *  to improve Avery's suit vitals and weapon stats. Can also trigger save.
 */
UCLASS(Blueprintable)
class AProjectOrganoidUpgradeTerminal : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidUpgradeTerminal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TerminalMesh;

	/** Base SOT cost at level 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Cost", meta = (ClampMin = "1"))
	int32 BaseSOTCost = 2;

	/** Extra SOT cost added per existing upgrade level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Cost", meta = (ClampMin = "0"))
	int32 SOTCostPerLevel = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Suit")
	float MaxHealthPerLevel = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Suit")
	float ToxicityThresholdPerLevel = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Suit")
	float PEEnergyMaxPerLevel = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Weapon")
	float WeaponDamagePerLevel = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Weapon")
	float WeaponFireRatePerLevel = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Weapon")
	float WeaponPenetrationPerLevel = 0.05f;

	/** If true, successful interact also writes a save slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Save")
	bool bAutoSaveOnInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Save")
	FString SaveSlotName = TEXT("OrganoidSave0");

	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnProjectOrganoidUpgradePurchased OnUpgradePurchased;

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetUpgradeCost(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool CanAffordUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const;

	/** Spend SOT and apply the selected upgrade */
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool TryPurchaseUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnTerminalOpened(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnUpgradeSucceeded(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnUpgradeFailed(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType);
};
