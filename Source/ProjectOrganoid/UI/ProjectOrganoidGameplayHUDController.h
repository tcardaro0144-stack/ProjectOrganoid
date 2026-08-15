// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidCraftingTypes.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidGameplayHUDController.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidHUDWidget;
class UProjectOrganoidInventoryComponent;
class UProjectOrganoidObjectiveSubsystem;
class UProjectOrganoidCraftingSubsystem;
class AProjectOrganoidUpgradeTerminal;

UENUM(BlueprintType)
enum class EProjectOrganoidHUDPanel : uint8
{
	None UMETA(DisplayName = "None"),
	Vitals UMETA(DisplayName = "Vitals"),
	Inventory UMETA(DisplayName = "Inventory"),
	Journal UMETA(DisplayName = "Journal"),
	Crafting UMETA(DisplayName = "Crafting"),
	Upgrade UMETA(DisplayName = "Upgrade")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHUDPanelChanged, EProjectOrganoidHUDPanel, ActivePanel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidGameplayLoopSynced);

/**
 *  Unified gameplay-loop HUD controller — binds inventory, quest journal,
 *  crafting, and Sterling upgrade events into one overlay driver.
 */
UCLASS(BlueprintType)
class UProjectOrganoidGameplayHUDController : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "HUD|Loop")
	FOnProjectOrganoidHUDPanelChanged OnHUDPanelChanged;

	UPROPERTY(BlueprintAssignable, Category = "HUD|Loop")
	FOnProjectOrganoidGameplayLoopSynced OnGameplayLoopSynced;

	UFUNCTION(BlueprintCallable, Category = "HUD|Loop")
	void Initialize(AProjectOrganoidCharacter* InCharacter, UProjectOrganoidHUDWidget* InHUD);

	UFUNCTION(BlueprintCallable, Category = "HUD|Loop")
	void Shutdown();

	UFUNCTION(BlueprintCallable, Category = "HUD|Loop")
	void SetActivePanel(EProjectOrganoidHUDPanel Panel);

	UFUNCTION(BlueprintCallable, Category = "HUD|Loop")
	void TogglePanel(EProjectOrganoidHUDPanel Panel);

	UFUNCTION(BlueprintPure, Category = "HUD|Loop")
	EProjectOrganoidHUDPanel GetActivePanel() const { return ActivePanel; }

	UFUNCTION(BlueprintCallable, Category = "HUD|Loop")
	void SyncAllPanels();

	UFUNCTION(BlueprintCallable, Category = "HUD|Craft")
	bool RequestCraft(FName RecipeId);

	UFUNCTION(BlueprintCallable, Category = "HUD|Upgrade")
	void BindUpgradeTerminal(AProjectOrganoidUpgradeTerminal* Terminal);

	UFUNCTION(BlueprintCallable, Category = "HUD|Upgrade")
	void ClearUpgradeTerminal();

	UFUNCTION(BlueprintCallable, Category = "HUD|Upgrade")
	bool RequestUpgrade(EProjectOrganoidUpgradeType UpgradeType);

protected:

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY()
	TWeakObjectPtr<UProjectOrganoidHUDWidget> BoundHUD;

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidUpgradeTerminal> BoundUpgradeTerminal;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidObjectiveSubsystem> ObjectiveSubsystem;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidCraftingSubsystem> CraftingSubsystem;

	EProjectOrganoidHUDPanel ActivePanel = EProjectOrganoidHUDPanel::Vitals;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleInventoryWeightChanged(float CurrentWeight, float MaxWeight);

	UFUNCTION()
	void HandleItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity);

	UFUNCTION()
	void HandleJournalUpdated();

	UFUNCTION()
	void HandleCraftSucceeded(FName RecipeId, AProjectOrganoidCharacter* Character);

	UFUNCTION()
	void HandleCraftFailed(FName RecipeId, FName FailureReason);

	UFUNCTION()
	void HandleUpgradePurchased(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType, int32 NewLevel);

	void PushInventoryToHUD();
	void PushJournalToHUD();
	void PushCraftingToHUD();
	void PushUpgradeToHUD();
};
