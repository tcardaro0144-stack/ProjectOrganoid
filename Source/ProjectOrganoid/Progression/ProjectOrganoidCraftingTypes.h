// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidCraftingTypes.generated.h"

USTRUCT(BlueprintType)
struct FProjectOrganoidCraftIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	EProjectOrganoidItemType ItemType = EProjectOrganoidItemType::Consumable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft", meta = (ClampMin = "1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FProjectOrganoidCraftRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	TArray<FProjectOrganoidCraftIngredient> Ingredients;

	/** Optional direct item grant (soft path resolved at craft time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	TSoftObjectPtr<UProjectOrganoidItemData> OutputItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft", meta = (ClampMin = "1"))
	int32 OutputQuantity = 1;

	/** Optional Sterling-style upgrade applied on craft success */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft")
	bool bGrantsUpgrade = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craft", meta = (EditCondition = "bGrantsUpgrade"))
	EProjectOrganoidUpgradeType UpgradeReward = EProjectOrganoidUpgradeType::SuitMaxHealth;
};

UCLASS(BlueprintType)
class UProjectOrganoidCraftingCatalog : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Craft")
	TArray<FProjectOrganoidCraftRecipe> Recipes;

	UFUNCTION(BlueprintPure, Category = "Craft")
	bool FindRecipe(FName RecipeId, FProjectOrganoidCraftRecipe& OutRecipe) const
	{
		for (const FProjectOrganoidCraftRecipe& Recipe : Recipes)
		{
			if (Recipe.RecipeId == RecipeId)
			{
				OutRecipe = Recipe;
				return true;
			}
		}
		return false;
	}
};
