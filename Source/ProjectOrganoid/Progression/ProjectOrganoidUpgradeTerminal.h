// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidUpgradeTerminal.generated.h"

class UStaticMeshComponent;
class AProjectOrganoidCharacter;
class UProjectOrganoidUpgradeWidget;
class UProjectOrganoidWeaponModData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidUpgradePurchased, AProjectOrganoidCharacter*, Character, EProjectOrganoidUpgradeType, UpgradeType, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidWeaponModInstalled, AProjectOrganoidCharacter*, Character, UProjectOrganoidWeaponModData*, ModData);

/**
 *  Dr. Sterling upgrade terminal — spend Synthetic Organoid Tissue (SOT)
 *  to improve Avery's suit / weapon stats, install attachments, and save.
 */
UCLASS(Blueprintable)
class AProjectOrganoidUpgradeTerminal : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidUpgradeTerminal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TerminalMesh;

	/** UMG class spawned when Avery opens the terminal (defaults to C++ base) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|UI")
	TSubclassOf<UProjectOrganoidUpgradeWidget> UpgradeWidgetClass;

	/** Mods Avery can purchase/install at this terminal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Mods")
	TArray<TObjectPtr<UProjectOrganoidWeaponModData>> AvailableWeaponMods;

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

	/** If true, successful interact / purchase also writes a save slot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Save")
	bool bAutoSaveOnInteract = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Save")
	bool bAutoSaveOnUpgrade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Save")
	FString SaveSlotName = TEXT("OrganoidSave0");

	UPROPERTY(BlueprintAssignable, Category = "Upgrade")
	FOnProjectOrganoidUpgradePurchased OnUpgradePurchased;

	UPROPERTY(BlueprintAssignable, Category = "Upgrade|Mods")
	FOnProjectOrganoidWeaponModInstalled OnWeaponModInstalled;

	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetUpgradeCost(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	bool CanAffordUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const;

	/** Spend SOT and apply the selected upgrade (including weapon damage) */
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool TryPurchaseUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType);

	/** Spend SOT specifically to raise equipped weapon damage */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Weapon")
	bool TryPurchaseWeaponDamageUpgrade(AProjectOrganoidCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Upgrade|Mods")
	int32 GetWeaponModInstallCost(UProjectOrganoidWeaponModData* ModData) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade|Mods")
	bool CanAffordWeaponMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData) const;

	/** Spend SOT and install an attachment (e.g. suppressed barrel) on the equipped weapon */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Mods")
	bool TryInstallWeaponMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData);

	UFUNCTION(BlueprintPure, Category = "Upgrade|Mods")
	TArray<UProjectOrganoidWeaponModData*> GetAvailableWeaponMods() const;

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	UProjectOrganoidUpgradeWidget* OpenUpgradeUI(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Upgrade|Save")
	bool SaveProgress(AProjectOrganoidCharacter* Character) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnTerminalOpened(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnUpgradeSucceeded(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType, int32 NewLevel);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade")
	void BP_OnUpgradeFailed(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade|Mods")
	void BP_OnWeaponModInstalled(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData);

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade|Mods")
	void BP_OnWeaponModFailed(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData);

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidUpgradeWidget> ActiveUpgradeWidget;
};
