// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"

UProjectOrganoidInventoryComponent::UProjectOrganoidInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitializeGrid();
}

void UProjectOrganoidInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeGrid();
}

void UProjectOrganoidInventoryComponent::InitializeGrid()
{
	GridWidth = FMath::Max(1, GridWidth);
	GridHeight = FMath::Max(1, GridHeight);

	Occupancy.SetNum(GridWidth * GridHeight);
	for (int32& Slot : Occupancy)
	{
		Slot = INDEX_NONE;
	}

	RebuildOccupancy();
}

void UProjectOrganoidInventoryComponent::RebuildOccupancy()
{
	Occupancy.SetNum(GridWidth * GridHeight);
	for (int32& Slot : Occupancy)
	{
		Slot = INDEX_NONE;
	}

	for (int32 ItemIndex = 0; ItemIndex < PlacedItems.Num(); ++ItemIndex)
	{
		const FProjectOrganoidPlacedItem& Placed = PlacedItems[ItemIndex];
		if (!Placed.IsValid())
		{
			continue;
		}

		const int32 Width = FMath::Max(1, Placed.ItemData->GridWidth);
		const int32 Height = FMath::Max(1, Placed.ItemData->GridHeight);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				const int32 SlotX = Placed.OriginX + X;
				const int32 SlotY = Placed.OriginY + Y;
				if (IsValidSlot(SlotX, SlotY))
				{
					Occupancy[SlotIndex(SlotX, SlotY)] = ItemIndex;
				}
			}
		}
	}
}

int32 UProjectOrganoidInventoryComponent::SlotIndex(int32 SlotX, int32 SlotY) const
{
	return SlotY * GridWidth + SlotX;
}

int32 UProjectOrganoidInventoryComponent::FindPlacedItemIndex(FGuid InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < PlacedItems.Num(); ++Index)
	{
		if (PlacedItems[Index].InstanceId == InstanceId)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

void UProjectOrganoidInventoryComponent::NotifyInventoryChanged()
{
	OnInventoryChanged.Broadcast();
	NotifyWeightChanged();
}

void UProjectOrganoidInventoryComponent::NotifyWeightChanged()
{
	OnInventoryWeightChanged.Broadcast(GetCurrentCarryWeight(), MaxCarryWeight);
}

void UProjectOrganoidInventoryComponent::BroadcastItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity)
{
	if (ItemData && Quantity > 0)
	{
		OnItemPickedUp.Broadcast(ItemData, Quantity);
	}
}

int32 UProjectOrganoidInventoryComponent::GetMaxStackForItem(const UProjectOrganoidItemData* ItemData) const
{
	if (!ItemData)
	{
		return 1;
	}

	if (!ItemData->bCanStack)
	{
		return 1;
	}

	return FMath::Max(1, ItemData->MaxStackCount);
}

float UProjectOrganoidInventoryComponent::GetCurrentCarryWeight() const
{
	float Total = 0.0f;
	for (const FProjectOrganoidPlacedItem& Placed : PlacedItems)
	{
		Total += Placed.GetTotalWeight();
	}
	return Total;
}

float UProjectOrganoidInventoryComponent::GetRemainingCarryWeight() const
{
	if (MaxCarryWeight <= KINDA_SMALL_NUMBER)
	{
		return TNumericLimits<float>::Max();
	}
	return FMath::Max(0.0f, MaxCarryWeight - GetCurrentCarryWeight());
}

bool UProjectOrganoidInventoryComponent::CanCarryAdditionalWeight(float AdditionalWeight) const
{
	if (AdditionalWeight <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	if (MaxCarryWeight <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	return GetCurrentCarryWeight() + AdditionalWeight <= MaxCarryWeight + KINDA_SMALL_NUMBER;
}

bool UProjectOrganoidInventoryComponent::HasUniqueSlotCapacity(int32 AdditionalSlots) const
{
	if (MaxUniqueItemSlots <= 0)
	{
		return true;
	}
	return PlacedItems.Num() + AdditionalSlots <= MaxUniqueItemSlots;
}

int32 UProjectOrganoidInventoryComponent::TryFillExistingStacks(UProjectOrganoidItemData* ItemData, int32 Quantity)
{
	if (!ItemData || Quantity <= 0 || !ItemData->bCanStack)
	{
		return Quantity;
	}

	const int32 MaxStack = GetMaxStackForItem(ItemData);
	int32 Remaining = Quantity;

	for (FProjectOrganoidPlacedItem& Placed : PlacedItems)
	{
		if (!Placed.IsValid() || Placed.ItemData != ItemData)
		{
			continue;
		}

		const int32 FreeSpace = MaxStack - Placed.StackCount;
		if (FreeSpace <= 0)
		{
			continue;
		}

		const int32 ToAdd = FMath::Min(FreeSpace, Remaining);
		const float AddedWeight = ItemData->ItemWeight * static_cast<float>(ToAdd);
		if (!CanCarryAdditionalWeight(AddedWeight))
		{
			const int32 WeightLimited = FMath::FloorToInt(GetRemainingCarryWeight() / FMath::Max(ItemData->ItemWeight, KINDA_SMALL_NUMBER));
			const int32 Clamped = FMath::Clamp(WeightLimited, 0, ToAdd);
			if (Clamped <= 0)
			{
				break;
			}
			Placed.StackCount += Clamped;
			Remaining -= Clamped;
			break;
		}

		Placed.StackCount += ToAdd;
		Remaining -= ToAdd;
		if (Remaining <= 0)
		{
			break;
		}
	}

	return Remaining;
}

bool UProjectOrganoidInventoryComponent::IsValidSlot(int32 SlotX, int32 SlotY) const
{
	return SlotX >= 0 && SlotY >= 0 && SlotX < GridWidth && SlotY < GridHeight;
}

bool UProjectOrganoidInventoryComponent::IsSlotOccupied(int32 SlotX, int32 SlotY) const
{
	if (!IsValidSlot(SlotX, SlotY) || Occupancy.Num() != GridWidth * GridHeight)
	{
		return false;
	}

	return Occupancy[SlotIndex(SlotX, SlotY)] != INDEX_NONE;
}

bool UProjectOrganoidInventoryComponent::CanPlaceItem(
	const UProjectOrganoidItemData* ItemData,
	int32 OriginX,
	int32 OriginY,
	FGuid IgnoreInstanceId) const
{
	if (!ItemData || Occupancy.Num() != GridWidth * GridHeight)
	{
		return false;
	}

	const int32 Width = FMath::Max(1, ItemData->GridWidth);
	const int32 Height = FMath::Max(1, ItemData->GridHeight);

	if (OriginX < 0 || OriginY < 0 || OriginX + Width > GridWidth || OriginY + Height > GridHeight)
	{
		return false;
	}

	const int32 IgnoredIndex = FindPlacedItemIndex(IgnoreInstanceId);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const int32 Occupant = Occupancy[SlotIndex(OriginX + X, OriginY + Y)];
			if (Occupant != INDEX_NONE && Occupant != IgnoredIndex)
			{
				return false;
			}
		}
	}

	return true;
}

bool UProjectOrganoidInventoryComponent::FindFirstFit(
	const UProjectOrganoidItemData* ItemData,
	int32& OutOriginX,
	int32& OutOriginY) const
{
	OutOriginX = INDEX_NONE;
	OutOriginY = INDEX_NONE;

	if (!ItemData)
	{
		return false;
	}

	const int32 Width = FMath::Max(1, ItemData->GridWidth);
	const int32 Height = FMath::Max(1, ItemData->GridHeight);

	for (int32 Y = 0; Y <= GridHeight - Height; ++Y)
	{
		for (int32 X = 0; X <= GridWidth - Width; ++X)
		{
			if (CanPlaceItem(ItemData, X, Y, FGuid()))
			{
				OutOriginX = X;
				OutOriginY = Y;
				return true;
			}
		}
	}

	return false;
}

bool UProjectOrganoidInventoryComponent::TryAddItem(UProjectOrganoidItemData* ItemData, FGuid& OutInstanceId, int32 Quantity)
{
	OutInstanceId.Invalidate();
	Quantity = FMath::Max(1, Quantity);

	if (!ItemData)
	{
		return false;
	}

	const float TotalWeight = ItemData->ItemWeight * static_cast<float>(Quantity);
	if (!CanCarryAdditionalWeight(TotalWeight))
	{
		return false;
	}

	int32 Remaining = TryFillExistingStacks(ItemData, Quantity);

	while (Remaining > 0)
	{
		if (!HasUniqueSlotCapacity(1))
		{
			break;
		}

		int32 OriginX = 0;
		int32 OriginY = 0;
		if (!FindFirstFit(ItemData, OriginX, OriginY))
		{
			break;
		}

		const int32 MaxStack = GetMaxStackForItem(ItemData);
		const int32 PlaceQty = FMath::Min(Remaining, MaxStack);
		FGuid NewId;
		if (!TryAddItemAt(ItemData, OriginX, OriginY, NewId, FGuid(), PlaceQty))
		{
			break;
		}

		OutInstanceId = NewId;
		Remaining -= PlaceQty;
	}

	const int32 Accepted = Quantity - Remaining;
	if (Accepted <= 0)
	{
		return false;
	}

	BroadcastItemPickedUp(ItemData, Accepted);
	NotifyInventoryChanged();
	return Remaining <= 0;
}

bool UProjectOrganoidInventoryComponent::TryAddItemAt(
	UProjectOrganoidItemData* ItemData,
	int32 OriginX,
	int32 OriginY,
	FGuid& OutInstanceId,
	FGuid PreferredInstanceId,
	int32 Quantity)
{
	OutInstanceId.Invalidate();
	Quantity = FMath::Max(1, Quantity);

	if (!ItemData)
	{
		return false;
	}

	const int32 MaxStack = GetMaxStackForItem(ItemData);
	Quantity = FMath::Min(Quantity, MaxStack);

	const float AddedWeight = ItemData->ItemWeight * static_cast<float>(Quantity);
	if (!CanCarryAdditionalWeight(AddedWeight))
	{
		return false;
	}

	if (!HasUniqueSlotCapacity(1))
	{
		return false;
	}

	if (!CanPlaceItem(ItemData, OriginX, OriginY, FGuid()))
	{
		return false;
	}

	FProjectOrganoidPlacedItem NewItem;
	NewItem.InstanceId = PreferredInstanceId.IsValid() ? PreferredInstanceId : FGuid::NewGuid();
	NewItem.ItemData = ItemData;
	NewItem.OriginX = OriginX;
	NewItem.OriginY = OriginY;
	NewItem.StackCount = Quantity;

	PlacedItems.Add(NewItem);
	RebuildOccupancy();

	OutInstanceId = NewItem.InstanceId;
	NotifyInventoryChanged();
	return true;
}

bool UProjectOrganoidInventoryComponent::ConsumeStack(FGuid InstanceId, int32 Quantity)
{
	Quantity = FMath::Max(1, Quantity);
	const int32 ItemIndex = FindPlacedItemIndex(InstanceId);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	FProjectOrganoidPlacedItem& Placed = PlacedItems[ItemIndex];
	if (Placed.StackCount < Quantity)
	{
		return false;
	}

	Placed.StackCount -= Quantity;
	if (Placed.StackCount <= 0)
	{
		PlacedItems.RemoveAt(ItemIndex);
		RebuildOccupancy();
	}

	NotifyInventoryChanged();
	return true;
}

bool UProjectOrganoidInventoryComponent::TryMoveItem(FGuid InstanceId, int32 NewOriginX, int32 NewOriginY)
{
	const int32 ItemIndex = FindPlacedItemIndex(InstanceId);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	FProjectOrganoidPlacedItem& Placed = PlacedItems[ItemIndex];
	if (!Placed.IsValid())
	{
		return false;
	}

	if (!CanPlaceItem(Placed.ItemData, NewOriginX, NewOriginY, InstanceId))
	{
		return false;
	}

	Placed.OriginX = NewOriginX;
	Placed.OriginY = NewOriginY;
	RebuildOccupancy();
	NotifyInventoryChanged();
	return true;
}

bool UProjectOrganoidInventoryComponent::RemoveItem(FGuid InstanceId)
{
	const int32 ItemIndex = FindPlacedItemIndex(InstanceId);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	PlacedItems.RemoveAt(ItemIndex);
	RebuildOccupancy();
	NotifyInventoryChanged();
	return true;
}

bool UProjectOrganoidInventoryComponent::RemoveItemAt(int32 SlotX, int32 SlotY)
{
	FProjectOrganoidPlacedItem Placed;
	if (!GetItemAt(SlotX, SlotY, Placed))
	{
		return false;
	}

	return RemoveItem(Placed.InstanceId);
}

bool UProjectOrganoidInventoryComponent::GetItemAt(int32 SlotX, int32 SlotY, FProjectOrganoidPlacedItem& OutItem) const
{
	OutItem = FProjectOrganoidPlacedItem();

	if (!IsValidSlot(SlotX, SlotY) || Occupancy.Num() != GridWidth * GridHeight)
	{
		return false;
	}

	const int32 ItemIndex = Occupancy[SlotIndex(SlotX, SlotY)];
	if (ItemIndex == INDEX_NONE || !PlacedItems.IsValidIndex(ItemIndex))
	{
		return false;
	}

	OutItem = PlacedItems[ItemIndex];
	return OutItem.IsValid();
}

bool UProjectOrganoidInventoryComponent::GetItemById(FGuid InstanceId, FProjectOrganoidPlacedItem& OutItem) const
{
	OutItem = FProjectOrganoidPlacedItem();

	const int32 ItemIndex = FindPlacedItemIndex(InstanceId);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	OutItem = PlacedItems[ItemIndex];
	return OutItem.IsValid();
}

bool UProjectOrganoidInventoryComponent::HasKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier) const
{
	if (RequiredTier == EProjectOrganoidSecurityTier::None)
	{
		return true;
	}

	const uint8 Required = static_cast<uint8>(RequiredTier);
	for (const FProjectOrganoidPlacedItem& Placed : PlacedItems)
	{
		if (!Placed.IsValid() || Placed.ItemData->ItemType != EProjectOrganoidItemType::KeyItem)
		{
			continue;
		}

		if (static_cast<uint8>(Placed.ItemData->SecurityTier) >= Required)
		{
			return true;
		}
	}

	return false;
}

bool UProjectOrganoidInventoryComponent::ConsumeKeycardOfTier(EProjectOrganoidSecurityTier RequiredTier)
{
	const uint8 Required = static_cast<uint8>(RequiredTier);
	int32 BestIndex = INDEX_NONE;
	uint8 BestTier = 255;

	for (int32 Index = 0; Index < PlacedItems.Num(); ++Index)
	{
		const FProjectOrganoidPlacedItem& Placed = PlacedItems[Index];
		if (!Placed.IsValid() || Placed.ItemData->ItemType != EProjectOrganoidItemType::KeyItem)
		{
			continue;
		}

		const uint8 Tier = static_cast<uint8>(Placed.ItemData->SecurityTier);
		if (Tier >= Required && Tier <= BestTier)
		{
			BestTier = Tier;
			BestIndex = Index;
		}
	}

	if (BestIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveItem(PlacedItems[BestIndex].InstanceId);
}

bool UProjectOrganoidInventoryComponent::HasSecurityOverrideTool(EProjectOrganoidSecurityTier RequiredTier) const
{
	if (RequiredTier == EProjectOrganoidSecurityTier::None)
	{
		return true;
	}

	const uint8 Required = static_cast<uint8>(RequiredTier);
	for (const FProjectOrganoidPlacedItem& Placed : PlacedItems)
	{
		if (!Placed.IsValid() || !Placed.ItemData->bIsSecurityOverrideTool)
		{
			continue;
		}

		if (static_cast<uint8>(Placed.ItemData->OverrideClearsUpTo) >= Required)
		{
			return true;
		}
	}

	return false;
}

bool UProjectOrganoidInventoryComponent::ConsumeSecurityOverrideTool(EProjectOrganoidSecurityTier RequiredTier)
{
	const uint8 Required = static_cast<uint8>(RequiredTier);
	int32 BestIndex = INDEX_NONE;
	uint8 BestClearance = 255;

	for (int32 Index = 0; Index < PlacedItems.Num(); ++Index)
	{
		const FProjectOrganoidPlacedItem& Placed = PlacedItems[Index];
		if (!Placed.IsValid() || !Placed.ItemData->bIsSecurityOverrideTool)
		{
			continue;
		}

		const uint8 Clearance = static_cast<uint8>(Placed.ItemData->OverrideClearsUpTo);
		if (Clearance >= Required && Clearance <= BestClearance)
		{
			BestClearance = Clearance;
			BestIndex = Index;
		}
	}

	if (BestIndex == INDEX_NONE)
	{
		return false;
	}

	return RemoveItem(PlacedItems[BestIndex].InstanceId);
}

int32 UProjectOrganoidInventoryComponent::CountItemsOfType(EProjectOrganoidItemType ItemType) const
{
	int32 Count = 0;
	for (const FProjectOrganoidPlacedItem& Placed : PlacedItems)
	{
		if (Placed.IsValid() && Placed.ItemData->ItemType == ItemType)
		{
			Count += Placed.StackCount;
		}
	}
	return Count;
}

bool UProjectOrganoidInventoryComponent::ConsumeItemsOfType(EProjectOrganoidItemType ItemType, int32 Count)
{
	if (Count <= 0)
	{
		return true;
	}

	if (CountItemsOfType(ItemType) < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	for (int32 Index = PlacedItems.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FProjectOrganoidPlacedItem& Placed = PlacedItems[Index];
		if (!Placed.IsValid() || Placed.ItemData->ItemType != ItemType)
		{
			continue;
		}

		const int32 Take = FMath::Min(Placed.StackCount, Remaining);
		Placed.StackCount -= Take;
		Remaining -= Take;
		if (Placed.StackCount <= 0)
		{
			PlacedItems.RemoveAt(Index);
		}
	}

	RebuildOccupancy();
	NotifyInventoryChanged();
	return Remaining <= 0;
}

void UProjectOrganoidInventoryComponent::ClearAllItems()
{
	PlacedItems.Reset();
	RebuildOccupancy();
	NotifyInventoryChanged();
}

void UProjectOrganoidInventoryComponent::SetGridDimensions(int32 NewWidth, int32 NewHeight)
{
	GridWidth = FMath::Max(1, NewWidth);
	GridHeight = FMath::Max(1, NewHeight);
	InitializeGrid();
	NotifyInventoryChanged();
}
