// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidCraftingSubsystem.h"
#include "ProjectOrganoidUpgradeTerminal.h"
#include "Engine/GameInstance.h"

void UProjectOrganoidGameplayHUDController::Initialize(AProjectOrganoidCharacter* InCharacter, UProjectOrganoidHUDWidget* InHUD)
{
	Shutdown();

	BoundCharacter = InCharacter;
	BoundHUD = InHUD;
	if (!InCharacter || !InHUD)
	{
		return;
	}

	if (UGameInstance* GI = InCharacter->GetGameInstance())
	{
		ObjectiveSubsystem = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
		CraftingSubsystem = GI->GetSubsystem<UProjectOrganoidCraftingSubsystem>();
	}

	if (UProjectOrganoidInventoryComponent* Inventory = InCharacter->GetInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleInventoryChanged);
		Inventory->OnInventoryWeightChanged.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleInventoryWeightChanged);
		Inventory->OnItemPickedUp.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleItemPickedUp);
	}

	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnJournalUpdated.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleJournalUpdated);
	}

	if (CraftingSubsystem)
	{
		CraftingSubsystem->OnCraftSucceeded.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleCraftSucceeded);
		CraftingSubsystem->OnCraftFailed.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleCraftFailed);
	}

	InHUD->BindGameplayLoopController(this);
	SyncAllPanels();
	SetActivePanel(EProjectOrganoidHUDPanel::Vitals);
}

void UProjectOrganoidGameplayHUDController::Shutdown()
{
	if (AProjectOrganoidCharacter* Character = BoundCharacter.Get())
	{
		if (UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent())
		{
			Inventory->OnInventoryChanged.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleInventoryChanged);
			Inventory->OnInventoryWeightChanged.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleInventoryWeightChanged);
			Inventory->OnItemPickedUp.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleItemPickedUp);
		}
	}

	if (ObjectiveSubsystem)
	{
		ObjectiveSubsystem->OnJournalUpdated.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleJournalUpdated);
	}

	if (CraftingSubsystem)
	{
		CraftingSubsystem->OnCraftSucceeded.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleCraftSucceeded);
		CraftingSubsystem->OnCraftFailed.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleCraftFailed);
	}

	ClearUpgradeTerminal();

	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->BindGameplayLoopController(nullptr);
	}

	BoundCharacter.Reset();
	BoundHUD.Reset();
	ObjectiveSubsystem = nullptr;
	CraftingSubsystem = nullptr;
	ActivePanel = EProjectOrganoidHUDPanel::None;
}

void UProjectOrganoidGameplayHUDController::SetActivePanel(EProjectOrganoidHUDPanel Panel)
{
	ActivePanel = Panel;
	OnHUDPanelChanged.Broadcast(ActivePanel);

	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnGameplayPanelChanged(ActivePanel);
	}

	SyncAllPanels();
}

void UProjectOrganoidGameplayHUDController::TogglePanel(EProjectOrganoidHUDPanel Panel)
{
	if (ActivePanel == Panel)
	{
		SetActivePanel(EProjectOrganoidHUDPanel::Vitals);
	}
	else
	{
		SetActivePanel(Panel);
	}
}

void UProjectOrganoidGameplayHUDController::SyncAllPanels()
{
	PushInventoryToHUD();
	PushJournalToHUD();
	PushCraftingToHUD();
	PushUpgradeToHUD();
	OnGameplayLoopSynced.Broadcast();
}

void UProjectOrganoidGameplayHUDController::PushInventoryToHUD()
{
	UProjectOrganoidHUDWidget* HUD = BoundHUD.Get();
	AProjectOrganoidCharacter* Character = BoundCharacter.Get();
	if (!HUD || !Character)
	{
		return;
	}

	if (UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent())
	{
		HUD->RefreshInventoryGrid(Inventory->GetAllItems(), Inventory->GetCurrentCarryWeight(), Inventory->GetMaxCarryWeight());
	}
}

void UProjectOrganoidGameplayHUDController::PushJournalToHUD()
{
	UProjectOrganoidHUDWidget* HUD = BoundHUD.Get();
	if (!HUD || !ObjectiveSubsystem)
	{
		return;
	}

	HUD->RefreshJournal(ObjectiveSubsystem->GetJournalEntries());
	HUD->RefreshObjectiveList(ObjectiveSubsystem->GetActiveObjectives());
}

void UProjectOrganoidGameplayHUDController::PushCraftingToHUD()
{
	UProjectOrganoidHUDWidget* HUD = BoundHUD.Get();
	if (!HUD || !CraftingSubsystem)
	{
		return;
	}

	HUD->RefreshCraftingRecipes(CraftingSubsystem->GetAllRecipes());
}

void UProjectOrganoidGameplayHUDController::PushUpgradeToHUD()
{
	UProjectOrganoidHUDWidget* HUD = BoundHUD.Get();
	AProjectOrganoidCharacter* Character = BoundCharacter.Get();
	if (!HUD || !Character)
	{
		return;
	}

	HUD->RefreshUpgradeSummary(
		Character->GetUpgradeLevel(EProjectOrganoidUpgradeType::SuitMaxHealth),
		Character->GetUpgradeLevel(EProjectOrganoidUpgradeType::SuitToxicityThreshold),
		Character->GetUpgradeLevel(EProjectOrganoidUpgradeType::SuitPEEnergyMax),
		Character->GetUpgradeLevel(EProjectOrganoidUpgradeType::WeaponDamage));
}

bool UProjectOrganoidGameplayHUDController::RequestCraft(FName RecipeId)
{
	AProjectOrganoidCharacter* Character = BoundCharacter.Get();
	if (!Character || !CraftingSubsystem)
	{
		return false;
	}
	return CraftingSubsystem->TryCraft(Character, RecipeId);
}

void UProjectOrganoidGameplayHUDController::BindUpgradeTerminal(AProjectOrganoidUpgradeTerminal* Terminal)
{
	ClearUpgradeTerminal();
	BoundUpgradeTerminal = Terminal;
	if (Terminal)
	{
		Terminal->OnUpgradePurchased.AddDynamic(this, &UProjectOrganoidGameplayHUDController::HandleUpgradePurchased);
		SetActivePanel(EProjectOrganoidHUDPanel::Upgrade);
	}
}

void UProjectOrganoidGameplayHUDController::ClearUpgradeTerminal()
{
	if (AProjectOrganoidUpgradeTerminal* Terminal = BoundUpgradeTerminal.Get())
	{
		Terminal->OnUpgradePurchased.RemoveDynamic(this, &UProjectOrganoidGameplayHUDController::HandleUpgradePurchased);
	}
	BoundUpgradeTerminal.Reset();
}

bool UProjectOrganoidGameplayHUDController::RequestUpgrade(EProjectOrganoidUpgradeType UpgradeType)
{
	AProjectOrganoidCharacter* Character = BoundCharacter.Get();
	AProjectOrganoidUpgradeTerminal* Terminal = BoundUpgradeTerminal.Get();
	if (!Character || !Terminal)
	{
		return false;
	}
	return Terminal->TryPurchaseUpgrade(Character, UpgradeType);
}

void UProjectOrganoidGameplayHUDController::HandleInventoryChanged()
{
	PushInventoryToHUD();
}

void UProjectOrganoidGameplayHUDController::HandleInventoryWeightChanged(float CurrentWeight, float MaxWeight)
{
	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnInventoryWeightUpdated(CurrentWeight, MaxWeight);
	}
}

void UProjectOrganoidGameplayHUDController::HandleItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity)
{
	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnInventoryItemAcquired(ItemData, Quantity);
	}
	PushInventoryToHUD();
}

void UProjectOrganoidGameplayHUDController::HandleJournalUpdated()
{
	PushJournalToHUD();
}

void UProjectOrganoidGameplayHUDController::HandleCraftSucceeded(FName RecipeId, AProjectOrganoidCharacter* /*Character*/)
{
	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnCraftResult(RecipeId, true, NAME_None);
	}
	PushInventoryToHUD();
	PushCraftingToHUD();
	PushUpgradeToHUD();
}

void UProjectOrganoidGameplayHUDController::HandleCraftFailed(FName RecipeId, FName FailureReason)
{
	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnCraftResult(RecipeId, false, FailureReason);
	}
}

void UProjectOrganoidGameplayHUDController::HandleUpgradePurchased(
	AProjectOrganoidCharacter* /*Character*/,
	EProjectOrganoidUpgradeType UpgradeType,
	int32 NewLevel)
{
	if (UProjectOrganoidHUDWidget* HUD = BoundHUD.Get())
	{
		HUD->OnUpgradeApplied(UpgradeType, NewLevel);
	}
	PushUpgradeToHUD();
	PushInventoryToHUD();
}
