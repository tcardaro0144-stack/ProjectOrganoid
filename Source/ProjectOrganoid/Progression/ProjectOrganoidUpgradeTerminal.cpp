// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidUpgradeTerminal.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidUpgradeWidget.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

AProjectOrganoidUpgradeTerminal::AProjectOrganoidUpgradeTerminal()
{
	InteractionPrompt = FText::FromString(TEXT("Use Sterling Terminal"));

	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	TerminalMesh->SetupAttachment(InteractionSphere);
	TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	UpgradeWidgetClass = UProjectOrganoidUpgradeWidget::StaticClass();
}

bool AProjectOrganoidUpgradeTerminal::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	BP_OnTerminalOpened(Interactor);
	OpenUpgradeUI(Interactor);

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(TEXT("Event_SterlingTerminalUsed"));
		}

		if (bAutoSaveOnInteract && Interactor)
		{
			SaveProgress(Interactor);
		}
	}

	return true;
}

UProjectOrganoidUpgradeWidget* AProjectOrganoidUpgradeTerminal::OpenUpgradeUI(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor)
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return nullptr;
	}

	if (ActiveUpgradeWidget)
	{
		ActiveUpgradeWidget->RemoveFromParent();
		ActiveUpgradeWidget = nullptr;
	}

	TSubclassOf<UProjectOrganoidUpgradeWidget> ClassToSpawn = UpgradeWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidUpgradeWidget::StaticClass();
	}

	ActiveUpgradeWidget = CreateWidget<UProjectOrganoidUpgradeWidget>(PC, ClassToSpawn);
	if (!ActiveUpgradeWidget)
	{
		return nullptr;
	}

	ActiveUpgradeWidget->BindToTerminal(this, Interactor);
	ActiveUpgradeWidget->AddToViewport(50);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActiveUpgradeWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	return ActiveUpgradeWidget;
}

int32 AProjectOrganoidUpgradeTerminal::GetUpgradeCost(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const
{
	if (!Character)
	{
		return BaseSOTCost;
	}

	const int32 Level = Character->GetUpgradeLevel(UpgradeType);
	return BaseSOTCost + (Level * SOTCostPerLevel);
}

bool AProjectOrganoidUpgradeTerminal::CanAffordUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const
{
	if (!Character || !Character->GetInventoryComponent())
	{
		return false;
	}

	const int32 Cost = GetUpgradeCost(Character, UpgradeType);
	return Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT) >= Cost;
}

bool AProjectOrganoidUpgradeTerminal::TryPurchaseUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType)
{
	if (!Character || !CanAffordUpgrade(Character, UpgradeType))
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
	const int32 Cost = GetUpgradeCost(Character, UpgradeType);
	if (!Inventory || !Inventory->ConsumeItemsOfType(EProjectOrganoidItemType::SOT, Cost))
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	const bool bApplied = Character->ApplyUpgrade(UpgradeType, MaxHealthPerLevel, ToxicityThresholdPerLevel, PEEnergyMaxPerLevel,
		WeaponDamagePerLevel, WeaponFireRatePerLevel, WeaponPenetrationPerLevel);

	if (!bApplied)
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	const int32 NewLevel = Character->GetUpgradeLevel(UpgradeType);
	OnUpgradePurchased.Broadcast(Character, UpgradeType, NewLevel);
	BP_OnUpgradeSucceeded(Character, UpgradeType, NewLevel);

	if (bAutoSaveOnUpgrade)
	{
		SaveProgress(Character);
	}

	return true;
}

bool AProjectOrganoidUpgradeTerminal::TryPurchaseWeaponDamageUpgrade(AProjectOrganoidCharacter* Character)
{
	return TryPurchaseUpgrade(Character, EProjectOrganoidUpgradeType::WeaponDamage);
}

int32 AProjectOrganoidUpgradeTerminal::GetWeaponModInstallCost(UProjectOrganoidWeaponModData* ModData) const
{
	return ModData ? FMath::Max(0, ModData->SOTInstallCost) : 0;
}

bool AProjectOrganoidUpgradeTerminal::CanAffordWeaponMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData) const
{
	if (!Character || !ModData || !Character->GetInventoryComponent())
	{
		return false;
	}

	if (!AvailableWeaponMods.Contains(ModData))
	{
		return false;
	}

	const int32 Cost = GetWeaponModInstallCost(ModData);
	return Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT) >= Cost;
}

bool AProjectOrganoidUpgradeTerminal::TryInstallWeaponMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData)
{
	if (!Character || !ModData || !CanAffordWeaponMod(Character, ModData))
	{
		BP_OnWeaponModFailed(Character, ModData);
		return false;
	}

	UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent();
	AProjectOrganoidWeapon* Weapon = WeaponComp ? WeaponComp->GetEquippedWeapon() : nullptr;
	UProjectOrganoidWeaponModComponent* ModComp = Weapon ? Weapon->GetWeaponModComponent() : nullptr;
	if (!ModComp)
	{
		BP_OnWeaponModFailed(Character, ModData);
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
	const int32 Cost = GetWeaponModInstallCost(ModData);
	if (Cost > 0)
	{
		if (!Inventory || !Inventory->ConsumeItemsOfType(EProjectOrganoidItemType::SOT, Cost))
		{
			BP_OnWeaponModFailed(Character, ModData);
			return false;
		}
	}

	if (!ModComp->InstallMod(ModData, true))
	{
		BP_OnWeaponModFailed(Character, ModData);
		return false;
	}

	OnWeaponModInstalled.Broadcast(Character, ModData);
	BP_OnWeaponModInstalled(Character, ModData);

	if (bAutoSaveOnUpgrade)
	{
		SaveProgress(Character);
	}

	return true;
}

TArray<UProjectOrganoidWeaponModData*> AProjectOrganoidUpgradeTerminal::GetAvailableWeaponMods() const
{
	TArray<UProjectOrganoidWeaponModData*> Result;
	for (const TObjectPtr<UProjectOrganoidWeaponModData>& Mod : AvailableWeaponMods)
	{
		if (Mod)
		{
			Result.Add(Mod.Get());
		}
	}
	return Result;
}

bool AProjectOrganoidUpgradeTerminal::SaveProgress(AProjectOrganoidCharacter* Character) const
{
	if (!Character)
	{
		return false;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
		{
			return SaveSubsystem->SavePlayerProgress(Character, SaveSlotName);
		}
	}
	return false;
}
