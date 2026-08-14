// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidUpgradeTerminal.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidItemData.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidUpgradeTerminal::AProjectOrganoidUpgradeTerminal()
{
	InteractionPrompt = FText::FromString(TEXT("Use Sterling Terminal"));

	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	TerminalMesh->SetupAttachment(InteractionSphere);
	TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

bool AProjectOrganoidUpgradeTerminal::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	BP_OnTerminalOpened(Interactor);

	if (bAutoSaveOnInteract && Interactor)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
			{
				SaveSubsystem->SavePlayerProgress(Interactor, SaveSlotName);
			}
		}
	}

	return true;
}

int32 AProjectOrganoidUpgradeTerminal::GetUpgradeCost(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const
{
	if (!Character)
	{
		return BaseSOTCost;
	}

	const int32 Level = Character->GetUpgradeLevel(UpgradeType);
	return BaseSOTCost + (Level * SOTCostPerLevel);
}

bool AProjectOrganoidUpgradeTerminal::CanAffordUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType) const
{
	if (!Character || !Character->GetInventoryComponent())
	{
		return false;
	}

	const int32 Cost = GetUpgradeCost(Character, UpgradeType);
	return Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT) >= Cost;
}

bool AProjectOrganoidUpgradeTerminal::TryPurchaseUpgrade(AProjectOrganoidCharacter* Character, EProjectOrganoidUpgradeType UpgradeType)
{
	if (!Character || !CanAffordUpgrade(Character, UpgradeType))
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
	const int32 Cost = GetUpgradeCost(Character, UpgradeType);
	if (!Inventory || !Inventory->ConsumeItemsOfType(EProjectOrganoidItemType::SOT, Cost))
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	const bool bApplied = Character->ApplyUpgrade(UpgradeType, MaxHealthPerLevel, ToxicityThresholdPerLevel, PEEnergyMaxPerLevel,
		WeaponDamagePerLevel, WeaponFireRatePerLevel, WeaponPenetrationPerLevel);

	if (!bApplied)
	{
		BP_OnUpgradeFailed(Character, UpgradeType);
		return false;
	}

	const int32 NewLevel = Character->GetUpgradeLevel(UpgradeType);
	OnUpgradePurchased.Broadcast(Character, UpgradeType, NewLevel);
	BP_OnUpgradeSucceeded(Character, UpgradeType, NewLevel);

	if (bAutoSaveOnInteract)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
			{
				SaveSubsystem->SavePlayerProgress(Character, SaveSlotName);
			}
		}
	}

	return true;
}
