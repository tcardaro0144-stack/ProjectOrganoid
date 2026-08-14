// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidStatsSubsystem.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UProjectOrganoidSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency(UProjectOrganoidObjectiveSubsystem::StaticClass());

	// Key lockdown mission tasks always qualify for autosave even if Main-type filter is off.
	AutosaveObjectiveIds.AddUnique(TEXT("Main_OverrideSecurityGate"));
	AutosaveObjectiveIds.AddUnique(TEXT("Main_ReadFacilityDataPads"));
	AutosaveObjectiveIds.AddUnique(TEXT("Main_ReachSterlingTerminal"));

	BindObjectiveAutosave();
}

void UProjectOrganoidSaveSubsystem::Deinitialize()
{
	UnbindObjectiveAutosave();
	Super::Deinitialize();
}

void UProjectOrganoidSaveSubsystem::BindObjectiveAutosave()
{
	if (bBoundToObjectives)
	{
		return;
	}

	if (UProjectOrganoidObjectiveSubsystem* Objectives = GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
	{
		Objectives->OnObjectiveCompleted.AddDynamic(this, &UProjectOrganoidSaveSubsystem::HandleObjectiveCompleted);
		Objectives->OnMissionCompleted.AddDynamic(this, &UProjectOrganoidSaveSubsystem::HandleMissionCompleted);
		bBoundToObjectives = true;
	}
}

void UProjectOrganoidSaveSubsystem::UnbindObjectiveAutosave()
{
	if (!bBoundToObjectives || !GetGameInstance())
	{
		return;
	}

	if (UProjectOrganoidObjectiveSubsystem* Objectives = GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
	{
		Objectives->OnObjectiveCompleted.RemoveDynamic(this, &UProjectOrganoidSaveSubsystem::HandleObjectiveCompleted);
		Objectives->OnMissionCompleted.RemoveDynamic(this, &UProjectOrganoidSaveSubsystem::HandleMissionCompleted);
	}

	bBoundToObjectives = false;
}

void UProjectOrganoidSaveSubsystem::HandleObjectiveCompleted(const FProjectOrganoidObjective& Objective)
{
	const bool bKeyId = AutosaveObjectiveIds.Contains(Objective.ObjectiveId);
	const bool bMain = bAutosaveOnMainObjectiveComplete
		&& Objective.Type == EProjectOrganoidObjectiveType::Main;

	if (!bKeyId && !bMain)
	{
		return;
	}

	if (AProjectOrganoidCharacter* Character = ResolveLocalCharacter())
	{
		Autosave(Character, EProjectOrganoidSaveReason::ObjectiveAutosave);
	}
}

void UProjectOrganoidSaveSubsystem::HandleMissionCompleted(FName MissionId)
{
	if (!bAutosaveOnMissionComplete || MissionId.IsNone())
	{
		return;
	}

	if (AProjectOrganoidCharacter* Character = ResolveLocalCharacter())
	{
		Autosave(Character, EProjectOrganoidSaveReason::MissionComplete);
	}
}

AProjectOrganoidCharacter* UProjectOrganoidSaveSubsystem::ResolveLocalCharacter() const
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}

	return Cast<AProjectOrganoidCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
}

FString UProjectOrganoidSaveSubsystem::GetSlotNameForIndex(int32 SlotIndex) const
{
	return FString::Printf(TEXT("%s%d"), *SlotNamePrefix, FMath::Max(0, SlotIndex));
}

TArray<FProjectOrganoidSaveSlotInfo> UProjectOrganoidSaveSubsystem::GetSaveSlotInfos(int32 NumSlots) const
{
	TArray<FProjectOrganoidSaveSlotInfo> Results;
	const int32 Count = FMath::Clamp(NumSlots, 1, 10);
	Results.Reserve(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		FProjectOrganoidSaveSlotInfo Info;
		Info.SlotIndex = Index;
		Info.SlotName = GetSlotNameForIndex(Index);
		Info.bExists = DoesSaveExist(Info.SlotName);

		if (Info.bExists)
		{
			if (UProjectOrganoidSaveGame* SaveGame = LoadSaveGameObject(Info.SlotName))
			{
				Info.Health = SaveGame->Health;
				Info.PEEnergy = SaveGame->PEEnergy;
				Info.SuitHealthUpgradeLevel = SaveGame->SuitHealthUpgradeLevel;
			}
		}

		Results.Add(Info);
	}

	return Results;
}

UProjectOrganoidSaveGame* UProjectOrganoidSaveSubsystem::LoadSaveGameObject(const FString& SlotName) const
{
	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	if (!DoesSaveExist(Slot))
	{
		return nullptr;
	}

	return Cast<UProjectOrganoidSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
}

void UProjectOrganoidSaveSubsystem::RequestLoadOnNextTravel(const FString& SlotName)
{
	PendingLoadSlotName = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	bHasPendingLoad = true;
}

void UProjectOrganoidSaveSubsystem::ClearPendingLoad()
{
	PendingLoadSlotName.Reset();
	bHasPendingLoad = false;
}

bool UProjectOrganoidSaveSubsystem::TryApplyPendingLoad(AProjectOrganoidCharacter* Character)
{
	if (!bHasPendingLoad || !Character)
	{
		return false;
	}

	const FString Slot = PendingLoadSlotName;
	ClearPendingLoad();
	return LoadPlayerProgress(Character, Slot);
}

bool UProjectOrganoidSaveSubsystem::SavePlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName)
{
	return SerializeGameState(Character, SlotName, EProjectOrganoidSaveReason::Manual, nullptr);
}

bool UProjectOrganoidSaveSubsystem::SaveAtCheckpoint(
	AProjectOrganoidCharacter* Character,
	AProjectOrganoidCheckpoint* Checkpoint,
	const FString& SlotName)
{
	const FString Slot = SlotName.IsEmpty() ? AutosaveSlotName : SlotName;
	return SerializeGameState(Character, Slot, EProjectOrganoidSaveReason::Checkpoint, Checkpoint);
}

bool UProjectOrganoidSaveSubsystem::Autosave(AProjectOrganoidCharacter* Character, EProjectOrganoidSaveReason Reason)
{
	return SerializeGameState(Character, AutosaveSlotName, Reason, nullptr);
}

bool UProjectOrganoidSaveSubsystem::SerializeGameState(
	AProjectOrganoidCharacter* Character,
	const FString& SlotName,
	EProjectOrganoidSaveReason Reason,
	AProjectOrganoidCheckpoint* Checkpoint)
{
	UProjectOrganoidSaveGame* SaveGame = CaptureSaveFromCharacter(Character);
	if (!SaveGame)
	{
		OnGameSaved.Broadcast(SlotName, Reason, false);
		return false;
	}

	const FString Slot = SlotName.IsEmpty()
		? (Reason == EProjectOrganoidSaveReason::Manual ? DefaultSaveSlot : AutosaveSlotName)
		: SlotName;

	FillSaveMeta(SaveGame, Character, Reason, Checkpoint);
	SaveGame->SaveSlotName = Slot;

	const bool bSucceeded = UGameplayStatics::SaveGameToSlot(SaveGame, Slot, SaveGame->UserIndex);
	OnGameSaved.Broadcast(Slot, Reason, bSucceeded);
	return bSucceeded;
}

void UProjectOrganoidSaveSubsystem::FillSaveMeta(
	UProjectOrganoidSaveGame* SaveGame,
	AProjectOrganoidCharacter* Character,
	EProjectOrganoidSaveReason Reason,
	AProjectOrganoidCheckpoint* Checkpoint) const
{
	if (!SaveGame)
	{
		return;
	}

	SaveGame->SaveReason = Reason;
	SaveGame->SaveTimestamp = FDateTime::UtcNow();

	if (Character)
	{
		SaveGame->PlayerTransform = Character->GetActorTransform();
		SaveGame->bHasPlayerTransform = true;

		if (UWorld* World = Character->GetWorld())
		{
			SaveGame->LevelName = World->GetMapName();
			SaveGame->LevelName.RemoveFromStart(World->StreamingLevelsPrefix);
		}
	}

	if (Checkpoint)
	{
		SaveGame->LastCheckpointId = Checkpoint->CheckpointId;
		SaveGame->LastCheckpointDisplayName = Checkpoint->CheckpointDisplayName;
		SaveGame->PlayerTransform = Checkpoint->GetActorTransform();
		SaveGame->bHasPlayerTransform = true;
	}
}

bool UProjectOrganoidSaveSubsystem::LoadPlayerProgress(AProjectOrganoidCharacter* Character, const FString& SlotName)
{
	const FString Slot = SlotName.IsEmpty() ? DefaultSaveSlot : SlotName;
	if (!DoesSaveExist(Slot))
	{
		OnGameLoaded.Broadcast(Slot, false);
		return false;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(Slot, 0);
	UProjectOrganoidSaveGame* SaveGame = Cast<UProjectOrganoidSaveGame>(Loaded);
	if (!SaveGame)
	{
		OnGameLoaded.Broadcast(Slot, false);
		return false;
	}

	const bool bSucceeded = ApplySaveToCharacter(SaveGame, Character);
	OnGameLoaded.Broadcast(Slot, bSucceeded);
	return bSucceeded;
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

			if (UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent())
			{
				SaveGame->EquippedWeaponMods = ModComp->CaptureModState();
			}
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

	if (UGameInstance* GI = Character->GetGameInstance())
	{
		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->CaptureStatsToSaveGame(SaveGame);
		}

		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
		}
	}

	FillSaveMeta(SaveGame, Character, EProjectOrganoidSaveReason::Manual, nullptr);
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

	if (UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent())
	{
		if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
		{
			if (UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent())
			{
				ModComp->ApplyModState(SaveGame->EquippedWeaponMods);
			}
		}
	}

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
			Inventory->TryAddItemAt(ItemData, Saved.OriginX, Saved.OriginY, NewId, Saved.InstanceId);
		}
	}

	if (UGameInstance* GI = Character->GetGameInstance())
	{
		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->ApplyStatsFromSaveGame(SaveGame);
		}

		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
		}
	}

	if (bRestorePlayerTransformOnLoad && SaveGame->bHasPlayerTransform)
	{
		Character->SetActorTransform(SaveGame->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}

	return true;
}
