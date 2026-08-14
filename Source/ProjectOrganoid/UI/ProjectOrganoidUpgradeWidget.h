// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidUpgradeWidget.generated.h"

class AProjectOrganoidUpgradeTerminal;
class AProjectOrganoidCharacter;
class UProjectOrganoidWeaponModData;

/**
 *  C++ base for Sterling terminal UMG — suit/weapon upgrades and attachment install.
 */
UCLASS()
class UProjectOrganoidUpgradeWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void BindToTerminal(AProjectOrganoidUpgradeTerminal* InTerminal, AProjectOrganoidCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void UnbindFromTerminal();

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	AProjectOrganoidUpgradeTerminal* GetBoundTerminal() const { return BoundTerminal; }

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	AProjectOrganoidCharacter* GetBoundCharacter() const { return BoundCharacter; }

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	bool PurchaseUpgrade(EProjectOrganoidUpgradeType UpgradeType);

	/** Convenience — spend SOT to raise weapon damage at the bound terminal */
	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	bool PurchaseWeaponDamageUpgrade();

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	bool InstallWeaponMod(UProjectOrganoidWeaponModData* ModData);

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void CloseTerminalUI();

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	int32 GetUpgradeCost(EProjectOrganoidUpgradeType UpgradeType) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	bool CanAffordUpgrade(EProjectOrganoidUpgradeType UpgradeType) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	TArray<UProjectOrganoidWeaponModData*> GetAvailableWeaponMods() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade|UI")
	void OnTerminalUIOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade|UI")
	void OnTerminalUIClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade|UI")
	void OnUpgradeListNeedsRefresh();

protected:

	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade|UI")
	TObjectPtr<AProjectOrganoidUpgradeTerminal> BoundTerminal;

	UPROPERTY(BlueprintReadOnly, Category = "Upgrade|UI")
	TObjectPtr<AProjectOrganoidCharacter> BoundCharacter;
};
