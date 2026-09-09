// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/PlayerController.h"

void UProjectOrganoidResearchStationWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureFunctionalLayout();
	RefreshPresentation();
}

void UProjectOrganoidResearchStationWidget::NativeDestruct()
{
	UnbindFromStation();
	Super::NativeDestruct();
}

TSharedRef<SWidget> UProjectOrganoidResearchStationWidget::RebuildWidget()
{
	EnsureFunctionalLayout();
	return Super::RebuildWidget();
}

void UProjectOrganoidResearchStationWidget::EnsureFunctionalLayout()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	if (WidgetTree->RootWidget)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResearchStationRoot"));
	WidgetTree->RootWidget = Root;

	StatusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusLabel"));
	StatusLabel->SetText(FText::FromString(TEXT("Research Station")));
	Root->AddChildToVerticalBox(StatusLabel);

	InstallButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InstallButton"));
	UTextBlock* InstallLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InstallLabel"));
	InstallLabel->SetText(FText::FromString(TEXT("Install Stabilized Barrel (0 SOT)")));
	InstallButton->AddChild(InstallLabel);
	InstallButton->OnClicked.AddDynamic(this, &UProjectOrganoidResearchStationWidget::HandleInstallClicked);
	Root->AddChildToVerticalBox(InstallButton);

	RemoveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RemoveButton"));
	UTextBlock* RemoveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RemoveLabel"));
	RemoveLabel->SetText(FText::FromString(TEXT("Remove Barrel (0 SOT)")));
	RemoveButton->AddChild(RemoveLabel);
	RemoveButton->OnClicked.AddDynamic(this, &UProjectOrganoidResearchStationWidget::HandleRemoveClicked);
	Root->AddChildToVerticalBox(RemoveButton);

	EquipAdaptationButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("EquipAdaptationButton"));
	UTextBlock* EquipAdaptLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EquipAdaptLabel"));
	EquipAdaptLabel->SetText(FText::FromString(TEXT("Equip Neural Slow (0 SOT)")));
	EquipAdaptationButton->AddChild(EquipAdaptLabel);
	EquipAdaptationButton->OnClicked.AddDynamic(this, &UProjectOrganoidResearchStationWidget::HandleEquipAdaptationClicked);
	Root->AddChildToVerticalBox(EquipAdaptationButton);

	UnequipAdaptationButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UnequipAdaptationButton"));
	UTextBlock* UnequipAdaptLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnequipAdaptLabel"));
	UnequipAdaptLabel->SetText(FText::FromString(TEXT("Unequip Adaptation (0 SOT)")));
	UnequipAdaptationButton->AddChild(UnequipAdaptLabel);
	UnequipAdaptationButton->OnClicked.AddDynamic(this, &UProjectOrganoidResearchStationWidget::HandleUnequipAdaptationClicked);
	Root->AddChildToVerticalBox(UnequipAdaptationButton);

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
	CloseLabel->SetText(FText::FromString(TEXT("Close")));
	CloseButton->AddChild(CloseLabel);
	CloseButton->OnClicked.AddDynamic(this, &UProjectOrganoidResearchStationWidget::HandleCloseClicked);
	Root->AddChildToVerticalBox(CloseButton);
}

void UProjectOrganoidResearchStationWidget::BindToStation(AProjectOrganoidResearchStation* InStation, AProjectOrganoidCharacter* InCharacter)
{
	BoundStation = InStation;
	BoundCharacter = InCharacter;
	RefreshPresentation();
}

void UProjectOrganoidResearchStationWidget::UnbindFromStation()
{
	BoundStation = nullptr;
	BoundCharacter = nullptr;
}

bool UProjectOrganoidResearchStationWidget::InstallUnlockedMod(UProjectOrganoidWeaponModData* ModData)
{
	if (!BoundStation)
	{
		return false;
	}

	const bool bOk = BoundStation->TryInstallUnlockedMod(BoundCharacter, ModData);
	if (bOk)
	{
		RefreshPresentation();
	}
	return bOk;
}

bool UProjectOrganoidResearchStationWidget::RemoveInstalledMod()
{
	if (!BoundStation)
	{
		return false;
	}

	const bool bOk = BoundStation->TryRemoveInstalledMod(BoundCharacter, EProjectOrganoidWeaponModSlot::Barrel);
	if (bOk)
	{
		RefreshPresentation();
	}
	return bOk;
}

bool UProjectOrganoidResearchStationWidget::EquipUnlockedAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData)
{
	if (!BoundStation)
	{
		return false;
	}

	const bool bOk = BoundStation->TryEquipUnlockedAdaptation(BoundCharacter, AdaptationData);
	if (bOk)
	{
		RefreshPresentation();
	}
	return bOk;
}

bool UProjectOrganoidResearchStationWidget::UnequipAdaptation()
{
	if (!BoundStation)
	{
		return false;
	}

	const bool bOk = BoundStation->TryUnequipAdaptation(BoundCharacter);
	if (bOk)
	{
		RefreshPresentation();
	}
	return bOk;
}

void UProjectOrganoidResearchStationWidget::CloseStationUI()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		PC->SetPause(false);
	}

	if (BoundStation)
	{
		BoundStation->NotifyStationUIClosed();
	}

	RemoveFromParent();
	UnbindFromStation();
}

void UProjectOrganoidResearchStationWidget::RefreshPresentation()
{
	if (StatusLabel)
	{
		StatusLabel->SetText(GetStatusText());
	}
}

FText UProjectOrganoidResearchStationWidget::GetStatusText() const
{
	FString UnlockedNames = TEXT("(none)");
	FString InstalledName = TEXT("(none)");
	FString UnlockedAdaptNames = TEXT("(none)");
	FString EquippedAdaptName = TEXT("(none)");
	if (BoundCharacter)
	{
		const TArray<UProjectOrganoidWeaponModData*> Unlocked = BoundCharacter->GetUnlockedWeaponMods();
		if (Unlocked.Num() > 0)
		{
			UnlockedNames.Reset();
			for (int32 i = 0; i < Unlocked.Num(); ++i)
			{
				if (Unlocked[i])
				{
					if (i > 0)
					{
						UnlockedNames += TEXT(", ");
					}
					UnlockedNames += Unlocked[i]->DisplayName.ToString();
				}
			}
		}

		if (UProjectOrganoidWeaponComponent* WeaponComp = BoundCharacter->GetWeaponComponent())
		{
			if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
			{
				if (UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent())
				{
					if (UProjectOrganoidWeaponModData* Installed = ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel))
					{
						InstalledName = Installed->DisplayName.ToString();
					}
				}
			}
		}

		if (UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = BoundCharacter->GetBiologicalAdaptationComponent())
		{
			const TArray<UProjectOrganoidBiologicalAdaptationData*> UnlockedAdapt = AdaptComp->GetUnlockedAdaptations();
			if (UnlockedAdapt.Num() > 0)
			{
				UnlockedAdaptNames.Reset();
				for (int32 i = 0; i < UnlockedAdapt.Num(); ++i)
				{
					if (UnlockedAdapt[i])
					{
						if (i > 0)
						{
							UnlockedAdaptNames += TEXT(", ");
						}
						UnlockedAdaptNames += UnlockedAdapt[i]->DisplayName.ToString();
					}
				}
			}
			if (UProjectOrganoidBiologicalAdaptationData* Equipped = AdaptComp->GetEquippedAdaptation())
			{
				EquippedAdaptName = Equipped->DisplayName.ToString();
			}
		}
	}

	return FText::FromString(FString::Printf(
		TEXT("RESEARCH STATION\nUnlocked: %s\nInstalled Barrel: %s\nUnlocked Adaptations: %s\nEquipped Adaptation: %s\nConfiguration cost: 0 SOT"),
		*UnlockedNames,
		*InstalledName,
		*UnlockedAdaptNames,
		*EquippedAdaptName));
}

bool UProjectOrganoidResearchStationWidget::IsStationUIOpen() const
{
	return IsInViewport();
}

void UProjectOrganoidResearchStationWidget::HandleInstallClicked()
{
	InstallUnlockedMod(UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve());
}

void UProjectOrganoidResearchStationWidget::HandleRemoveClicked()
{
	RemoveInstalledMod();
}

void UProjectOrganoidResearchStationWidget::HandleEquipAdaptationClicked()
{
	EquipUnlockedAdaptation(UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve());
}

void UProjectOrganoidResearchStationWidget::HandleUnequipAdaptationClicked()
{
	UnequipAdaptation();
}

void UProjectOrganoidResearchStationWidget::HandleCloseClicked()
{
	CloseStationUI();
}
