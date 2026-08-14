// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "Kismet/GameplayStatics.h"

bool UProjectOrganoidSaveSubsystem::SavePlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName)
{
	UProjectOrganoidSaveGame* SaveGame = CaptureSaveFromCharacter(Character);
	if (!SaveGame)
	{
		return false;
	}

	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	SaveGame->SaveSlotName = Slot;
	return UGameplayStatics::SaveGameToSlot(SaveGame, Slot, SaveGame->UserIndex);
}

bool UProjectOrganoidSaveSubsystem::LoadPlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName)
{
	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	if (!DoesSaveExist(Slot))
	{
		return false;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, 0);
	UProjectOrganoidSaveGame* SaveGame = Cast<UProjectOrganoidSaveGame>(Loaded);
	if (!SaveGame)
	{
		return false;
	}

	return ApplySaveToCharacter(SaveGame, Character);
}

bool UProjectOrganoidSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	return UGameplayStatics::DoesSaveGameExist(Slot, 0);
}

bool UProjectOrganoidSaveSubsystem::DeleteSave(const FString& SlotName)
{
	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	return UGameplayStatics::DeleteGameInSlot(Slot, 0);
}

UProjectOrganoidSaveGame* UProjectOrganoidSaveSubsystem::CaptureSaveFromCharacter(AProjectOrganoidCharacter* Character) const
{
	if (!Character)
	{
		return nullptr;
	}

	UProjectOrganoidSaveGame* SaveGame = Cast<UProjectOrganoidSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UProjectOrganoidSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return nullptr;
	}

	SaveGame->Health = Character->GetHealth();
	SaveGame->MaxHealth = Character->GetMaxHealth();
	SaveGame->Toxicity = Character->GetToxicity();
	SaveGame->MaxToxicity = Character->GetMaxToxicity();
	SaveGame->HeartRate = Character->GetHeartRate();
	SaveGame->PEEnergy = Character->GetPEEnergy();
	SaveGame->MaxPEEnergy = Character->GetMaxPEEnergy();

	SaveGame->SuitHealthUpgradeLevel = Character->GetSuitHealthUpgradeLevel();
	SaveGame->SuitToxicityUpgradeLevel = Character->GetSuitToxicityUpgradeLevel();
	SaveGame->SuitPEUpgradeLevel = Character->GetSuitPEUpgradeLevel();
	SaveGame->WeaponDamageUpgradeLevel = Character->GetWeaponDamageUpgradeLevel();
	SaveGame->WeaponFireRateUpgradeLevel = Character->GetWeaponFireRateUpgradeLevel();
	SaveGame->WeaponPenetrationUpgradeLevel = Character->GetWeaponPenetrationUpgradeLevel();

	if (UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent())
	{
		if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
		{
			SaveGame->WeaponDamage = Weapon->Damage;
			SaveGame->WeaponFireRate = Weapon->FireRate;
			SaveGame->WeaponPenetration = Weapon->Penetration;
		}
	}

	if (UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent())
	{
		SaveGame->InventoryGridWidth = Inventory->GetGridWidth();
		SaveGame->InventoryGridHeight = Inventory->GetGridHeight();
		SaveGame->InventoryItems.Reset();
		SaveGame->OwnedKeycardTiers.Reset();

		TSet<EProjectOrganoidSecurityTier> UniqueTiers;

		for (const FProjectOrganoidPlacedItem& Placed : Inventory->GetAllItems())
		{
			if (!Placed.IsValid())
			{
				continue;
			}

			FProjectOrganoidSavedInventoryItem Saved;
			Saved.ItemDataPath = FSoftObjectPath(Placed.ItemData);
			Saved.InstanceId = Placed.InstanceId;
			Saved.OriginX = Placed.OriginX;
			Saved.OriginY = Placed.OriginY;
			Saved.SecurityTier = Placed.ItemData->SecurityTier;
			SaveGame->InventoryItems.Add(Saved);

			if (Placed.ItemData->ItemType == EProjectOrganoidItemType::KeyItem
				&& Placed.ItemData->SecurityTier != EProjectOrganoidSecurityTier::None)
			{
				UniqueTiers.Add(Placed.ItemData->SecurityTier);
			}
		}

		SaveGame->OwnedKeycardTiers = UniqueTiers.Array();
		SaveGame->OwnedKeycardTiers.Sort([](const EProjectOrganoidSecurityTier& A, const EProjectOrganoidSecurityTier& B)
		{
			return static_cast<uint8>(A) < static_cast<uint8>(B);
		});
	}

	return SaveGame;
}

bool UProjectOrganoidSaveSubsystem::ApplySaveToCharacter(UProjectOrganoidSaveGame* SaveGame, AProjectOrganoidCharacter* Character) const
{
	if (!SaveGame || !Character)
	{
		return false;
	}

	Character->ApplySavedVitals(
		SaveGame->Health,
		SaveGame->MaxHealth,
		SaveGame->Toxicity,
		SaveGame->MaxToxicity,
		SaveGame->HeartRate,
		SaveGame->PEEnergy,
		SaveGame->MaxPEEnergy);

	Character->ApplySavedUpgradeLevels(
		SaveGame->SuitHealthUpgradeLevel,
		SaveGame->SuitToxicityUpgradeLevel,
		SaveGame->SuitPEUpgradeLevel,
		SaveGame->WeaponDamageUpgradeLevel,
		SaveGame->WeaponFireRateUpgradeLevel,
		SaveGame->WeaponPenetrationUpgradeLevel);

	Character->ApplySavedWeaponStats(
		SaveGame->WeaponDamage,
		SaveGame->WeaponFireRate,
		SaveGame->WeaponPenetration);

	if (UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent())
	{
		Inventory->ClearAllItems();
		Inventory->SetGridDimensions(SaveGame->InventoryGridWidth, SaveGame->InventoryGridHeight);

		for (const FProjectOrganoidSavedInventoryItem& Saved : SaveGame->InventoryItems)
		{
			UObject* Loaded = Saved.ItemDataPath.TryLoad();
			UProjectOrganoidItemData* ItemData = Cast<UProjectOrganoidItemData>(Loaded);
			if (!ItemData)
			{
				continue;
			}

			FGuid NewId;
			Inventory->TryAddItemAt(ItemData, Saved.OriginX, Saved.OriginY, NewId);
		}
	}

	return true;
}
