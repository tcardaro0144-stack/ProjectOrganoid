// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidUpgradeWidget.h"
#include "ProjectOrganoidUpgradeTerminal.h"
#include "ProjectOrganoidCharacter.h"

void UProjectOrganoidUpgradeWidget::NativeDestruct()
{
	UnbindFromTerminal();
	Super::NativeDestruct();
}

void UProjectOrganoidUpgradeWidget::BindToTerminal(AProjectOrganoidUpgradeTerminal* InTerminal, AProjectOrganoidCharacter* InCharacter)
{
	BoundTerminal = InTerminal;
	BoundCharacter = InCharacter;
	OnTerminalUIOpened();
	OnUpgradeListNeedsRefresh();
}

void UProjectOrganoidUpgradeWidget::UnbindFromTerminal()
{
	BoundTerminal = nullptr;
	BoundCharacter = nullptr;
}

bool UProjectOrganoidUpgradeWidget::PurchaseUpgrade(EProjectOrganoidUpgradeType UpgradeType)
{
	if (!BoundTerminal || !BoundCharacter)
	{
		return false;
	}

	const bool bOk = BoundTerminal->TryPurchaseUpgrade(BoundCharacter, UpgradeType);
	if (bOk)
	{
		OnUpgradeListNeedsRefresh();
	}
	return bOk;
}

bool UProjectOrganoidUpgradeWidget::PurchaseWeaponDamageUpgrade()
{
	return PurchaseUpgrade(EProjectOrganoidUpgradeType::WeaponDamage);
}

bool UProjectOrganoidUpgradeWidget::InstallWeaponMod(UProjectOrganoidWeaponModData* ModData)
{
	if (!BoundTerminal || !BoundCharacter)
	{
		return false;
	}

	const bool bOk = BoundTerminal->TryInstallWeaponMod(BoundCharacter, ModData);
	if (bOk)
	{
		OnUpgradeListNeedsRefresh();
	}
	return bOk;
}

void UProjectOrganoidUpgradeWidget::CloseTerminalUI()
{
	OnTerminalUIClosed();

	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetPause(false);
	}

	RemoveFromParent();
	UnbindFromTerminal();
}

int32 UProjectOrganoidUpgradeWidget::GetUpgradeCost(EProjectOrganoidUpgradeType UpgradeType) const
{
	return BoundTerminal ? BoundTerminal->GetUpgradeCost(BoundCharacter, UpgradeType) : 0;
}

bool UProjectOrganoidUpgradeWidget::CanAffordUpgrade(EProjectOrganoidUpgradeType UpgradeType) const
{
	return BoundTerminal && BoundTerminal->CanAffordUpgrade(BoundCharacter, UpgradeType);
}

TArray<UProjectOrganoidWeaponModData*> UProjectOrganoidUpgradeWidget::GetAvailableWeaponMods() const
{
	if (BoundTerminal)
	{
		return BoundTerminal->GetAvailableWeaponMods();
	}
	return TArray<UProjectOrganoidWeaponModData*>();
}
