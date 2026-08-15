// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidCraftingTypes.h"
#include "ProjectOrganoidCraftingSubsystem.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidCraftSucceeded, FName, RecipeId, AProjectOrganoidCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidCraftFailed, FName, RecipeId, FName, FailureReason);

/**
 *  Lightweight crafting / field-upgrade loop — consumes inventory stacks and
 *  optionally grants items or suit/weapon upgrade levels.
 */
UCLASS()
class UProjectOrganoidCraftingSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	TSoftObjectPtr<UProjectOrganoidCraftingCatalog> DefaultCatalog;

	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnProjectOrganoidCraftSucceeded OnCraftSucceeded;

	UPROPERTY(BlueprintAssignable, Category = "Craft")
	FOnProjectOrganoidCraftFailed OnCraftFailed;

	UFUNCTION(BlueprintCallable, Category = "Craft")
	void SetCatalog(UProjectOrganoidCraftingCatalog* Catalog);

	UFUNCTION(BlueprintPure, Category = "Craft")
	TArray<FProjectOrganoidCraftRecipe> GetAllRecipes() const;

	UFUNCTION(BlueprintPure, Category = "Craft")
	bool CanCraft(AProjectOrganoidCharacter* Character, FName RecipeId) const;

	UFUNCTION(BlueprintCallable, Category = "Craft")
	bool TryCraft(AProjectOrganoidCharacter* Character, FName RecipeId);

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidCraftingCatalog> LoadedCatalog;

	void EnsureCatalogLoaded();
	bool HasIngredients(const UProjectOrganoidInventoryComponent* Inventory, const FProjectOrganoidCraftRecipe& Recipe) const;
	bool ConsumeIngredients(UProjectOrganoidInventoryComponent* Inventory, const FProjectOrganoidCraftRecipe& Recipe) const;
};
