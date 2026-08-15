// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCraftingSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"

void UProjectOrganoidCraftingSubsystem::EnsureCatalogLoaded()
{
	if (LoadedCatalog)
	{
		return;
	}
	if (!DefaultCatalog.IsNull())
	{
		LoadedCatalog = DefaultCatalog.LoadSynchronous();
	}
}

void UProjectOrganoidCraftingSubsystem::SetCatalog(UProjectOrganoidCraftingCatalog* Catalog)
{
	LoadedCatalog = Catalog;
	if (Catalog)
	{
		DefaultCatalog = Catalog;
	}
}

TArray<FProjectOrganoidCraftRecipe> UProjectOrganoidCraftingSubsystem::GetAllRecipes() const
{
	if (LoadedCatalog)
	{
		return LoadedCatalog->Recipes;
	}
	if (const UProjectOrganoidCraftingCatalog* Catalog = DefaultCatalog.Get())
	{
		return Catalog->Recipes;
	}
	return TArray<FProjectOrganoidCraftRecipe>();
}

bool UProjectOrganoidCraftingSubsystem::HasIngredients(
	const UProjectOrganoidInventoryComponent* Inventory,
	const FProjectOrganoidCraftRecipe& Recipe) const
{
	if (!Inventory)
	{
		return false;
	}

	for (const FProjectOrganoidCraftIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Inventory->CountItemsOfType(Ingredient.ItemType) < Ingredient.Quantity)
		{
			return false;
		}
	}
	return true;
}

bool UProjectOrganoidCraftingSubsystem::ConsumeIngredients(
	UProjectOrganoidInventoryComponent* Inventory,
	const FProjectOrganoidCraftRecipe& Recipe) const
{
	if (!HasIngredients(Inventory, Recipe))
	{
		return false;
	}

	for (const FProjectOrganoidCraftIngredient& Ingredient : Recipe.Ingredients)
	{
		if (!Inventory->ConsumeItemsOfType(Ingredient.ItemType, Ingredient.Quantity))
		{
			return false;
		}
	}
	return true;
}

bool UProjectOrganoidCraftingSubsystem::CanCraft(AProjectOrganoidCharacter* Character, FName RecipeId) const
{
	if (!Character || RecipeId.IsNone())
	{
		return false;
	}

	const_cast<UProjectOrganoidCraftingSubsystem*>(this)->EnsureCatalogLoaded();
	FProjectOrganoidCraftRecipe Recipe;
	if (!LoadedCatalog || !LoadedCatalog->FindRecipe(RecipeId, Recipe))
	{
		return false;
	}

	return HasIngredients(Character->GetInventoryComponent(), Recipe);
}

bool UProjectOrganoidCraftingSubsystem::TryCraft(AProjectOrganoidCharacter* Character, FName RecipeId)
{
	EnsureCatalogLoaded();
	if (!Character || !LoadedCatalog)
	{
		OnCraftFailed.Broadcast(RecipeId, TEXT("NoCatalog"));
		return false;
	}

	FProjectOrganoidCraftRecipe Recipe;
	if (!LoadedCatalog->FindRecipe(RecipeId, Recipe))
	{
		OnCraftFailed.Broadcast(RecipeId, TEXT("UnknownRecipe"));
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
	if (!ConsumeIngredients(Inventory, Recipe))
	{
		OnCraftFailed.Broadcast(RecipeId, TEXT("MissingIngredients"));
		return false;
	}

	if (!Recipe.OutputItem.IsNull())
	{
		if (UProjectOrganoidItemData* Output = Recipe.OutputItem.LoadSynchronous())
		{
			FGuid NewId;
			if (!Inventory->TryAddItem(Output, NewId, Recipe.OutputQuantity))
			{
				OnCraftFailed.Broadcast(RecipeId, TEXT("InventoryFull"));
				return false;
			}
		}
	}

	if (Recipe.bGrantsUpgrade)
	{
		Character->ApplyUpgrade(Recipe.UpgradeReward, 10.0f, 5.0f, 10.0f, 5.0f, 0.05f, 0.05f);
	}

	OnCraftSucceeded.Broadcast(RecipeId, Character);
	return true;
}
