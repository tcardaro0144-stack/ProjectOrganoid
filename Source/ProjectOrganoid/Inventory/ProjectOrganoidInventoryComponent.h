// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidInventoryComponent.generated.h"

class UProjectOrganoidItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidInventoryChanged);

/**
 *  Grid inventory for Avery Vance — Resident Evil-style slot packing.
 *  Handles can-place checks, add, move, and remove within Width x Height.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidInventoryComponent();

	virtual void BeginPlay() override;

	/** Inventory grid columns */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 8;

	/** Inventory grid rows */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 6;

	/** Fired after any successful add / move / remove (UMG refresh) */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnProjectOrganoidInventoryChanged OnInventoryChanged;

	/** True if ItemData footprint fits at (OriginX, OriginY) without overlap */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceItem(const UProjectOrganoidItemData* ItemData, int32 OriginX, int32 OriginY, FGuid IgnoreInstanceId) const;

	/** Find the first top-left slot that fits ItemData */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindFirstFit(const UProjectOrganoidItemData* ItemData, int32& OutOriginX, int32& OutOriginY) const;

	/** Place ItemData at the first available fit. Returns false if full. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(UProjectOrganoidItemData* ItemData, FGuid& OutInstanceId);

	/** Place ItemData at an explicit origin. Returns false if blocked. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItemAt(UProjectOrganoidItemData* ItemData, int32 OriginX, int32 OriginY, FGuid& OutInstanceId);

	/** Move an existing instance to a new origin */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryMoveItem(FGuid InstanceId, int32 NewOriginX, int32 NewOriginY);

	/** Remove by instance id */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid InstanceId);

	/** Remove whatever occupies slot (X, Y) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(int32 SlotX, int32 SlotY);

	/** Placed item covering slot (X, Y), if any */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItemAt(int32 SlotX, int32 SlotY, FProjectOrganoidPlacedItem& OutItem) const;

	/** Lookup by instance id */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool GetItemById(FGuid InstanceId, FProjectOrganoidPlacedItem& OutItem) const;

	/** All placed items (UMG grid rebuild) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<FProjectOrganoidPlacedItem> GetAllItems() const { return PlacedItems; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsSlotOccupied(int32 SlotX, int32 SlotY) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsValidSlot(int32 SlotX, int32 SlotY) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridWidth() const { return GridWidth; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetGridHeight() const { return GridHeight; }

	/** Flattened occupancy: INDEX_NONE = empty, else index into PlacedItems */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	TArray<int32> GetOccupancyMap() const { return Occupancy; }

	/** True if inventory holds a KeyItem whose SecurityTier >= RequiredTier */
	UFUNCTION(BlueprintPure, Category = "Inventory|Security")
	bool HasKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier) const;

	/** Remove one matching keycard (highest qualifying tier preferred). Returns false if none. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Security")
	bool ConsumeKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier);

protected:

	/** Placed item instances currently in the grid */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FProjectOrganoidPlacedItem> PlacedItems;

	/** Size GridWidth * GridHeight; values are PlacedItems indices or INDEX_NONE */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<int32> Occupancy;

	void InitializeGrid();
	void RebuildOccupancy();
	int32 SlotIndex(int32 SlotX, int32 SlotY) const;
	int32 FindPlacedItemIndex(FGuid InstanceId) const;
	void NotifyInventoryChanged();
};
