// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidInventoryComponent.generated.h"

class UProjectOrganoidItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidItemPickedUp, UProjectOrganoidItemData*, ItemData, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidInventoryWeightChanged, float, CurrentWeight, float, MaxWeight);

/**
 *  Grid inventory for Avery Vance — RE-style slot packing with weight limits,
 *  unique-slot caps, and stackable ammo / SOT / consumables.
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

	/** Hard cap on distinct placed stacks (0 = unlimited beyond grid) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Limits", meta = (ClampMin = "0"))
	int32 MaxUniqueItemSlots = 0;

	/** Max carry weight; 0 = unlimited */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Weight", meta = (ClampMin = "0.0"))
	float MaxCarryWeight = 40.0f;

	/** Fired after any successful add / move / remove (UMG refresh) */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnProjectOrganoidInventoryChanged OnInventoryChanged;

	/** Fired when items are newly acquired (stats / achievements / UI toasts) */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnProjectOrganoidItemPickedUp OnItemPickedUp;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Weight")
	FOnProjectOrganoidInventoryWeightChanged OnInventoryWeightChanged;

	/** True if ItemData footprint fits at (OriginX, OriginY) without overlap */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceItem(const UProjectOrganoidItemData* ItemData, int32 OriginX, int32 OriginY, FGuid IgnoreInstanceId) const;

	/** Find the first top-left slot that fits ItemData */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindFirstFit(const UProjectOrganoidItemData* ItemData, int32& OutOriginX, int32& OutOriginY) const;

	/**
	 *  Add Quantity units — stacks into existing cells when possible, then places new cells.
	 *  Returns false if weight / slots / grid cannot accept the full quantity.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(UProjectOrganoidItemData* ItemData, FGuid& OutInstanceId, int32 Quantity = 1);

	/** Place ItemData at an explicit origin. Returns false if blocked. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItemAt(UProjectOrganoidItemData* ItemData, int32 OriginX, int32 OriginY, FGuid& OutInstanceId, FGuid PreferredInstanceId = FGuid(), int32 Quantity = 1);

	/** Move an existing instance to a new origin */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryMoveItem(FGuid InstanceId, int32 NewOriginX, int32 NewOriginY);

	/** Remove by instance id */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid InstanceId);

	/** Remove whatever occupies slot (X, Y) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItemAt(int32 SlotX, int32 SlotY);

	/** Reduce a stack; removes the cell when StackCount hits 0 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stack")
	bool ConsumeStack(FGuid InstanceId, int32 Quantity = 1);

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

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetCurrentCarryWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetMaxCarryWeight() const { return MaxCarryWeight; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	float GetRemainingCarryWeight() const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Weight")
	bool CanCarryAdditionalWeight(float AdditionalWeight) const;

	UFUNCTION(BlueprintPure, Category = "Inventory|Limits")
	int32 GetUsedUniqueItemSlots() const { return PlacedItems.Num(); }

	UFUNCTION(BlueprintPure, Category = "Inventory|Limits")
	bool HasUniqueSlotCapacity(int32 AdditionalSlots = 1) const;

	/** True if inventory holds a KeyItem whose SecurityTier >= RequiredTier */
	UFUNCTION(BlueprintPure, Category = "Inventory|Security")
	bool HasKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier) const;

	/** Remove one matching keycard (highest qualifying tier preferred). Returns false if none. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Security")
	bool ConsumeKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier);

	/** True if inventory holds a hacking tool that can clear RequiredTier gates */
	UFUNCTION(BlueprintPure, Category = "Inventory|Security")
	bool HasSecurityOverrideTool(EProjectOrganoidSecurityTier RequiredTier) const;

	/** Consume one qualifying hacking tool (lowest sufficient OverrideClearsUpTo preferred). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Security")
	bool ConsumeSecurityOverrideTool(EProjectOrganoidSecurityTier RequiredTier);

	/** Count stacked items of a given type (sums StackCount) */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 CountItemsOfType(EProjectOrganoidItemType ItemType) const;

	/** Remove up to Count stacked units of the given type. Returns false if not enough. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeItemsOfType(EProjectOrganoidItemType ItemType, int32 Count);

	/** Wipe the grid (used by save restore) */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearAllItems();

	/** Resize grid and rebuild occupancy */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetGridDimensions(int32 NewWidth, int32 NewHeight);

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
	void NotifyWeightChanged();
	void BroadcastItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity);
	int32 GetMaxStackForItem(const UProjectOrganoidItemData* ItemData) const;
	int32 TryFillExistingStacks(UProjectOrganoidItemData* ItemData, int32 Quantity);
};
