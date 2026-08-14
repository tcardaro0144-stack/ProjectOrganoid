// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace ProjectOrganoidMenuUI
{
	static const TArray<FString> GraphicsQualityLabels = {
		TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic")
	};

	static EProjectOrganoidGraphicsQuality QualityFromLabel(const FString& Label)
	{
		const int32 Index = GraphicsQualityLabels.IndexOfByKey(Label);
		return static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(Index, 0, 4));
	}
}

void UProjectOrganoidMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindWidgetCallbacks();
	SyncSettingsWidgets();
	RefreshSaveSlots();
}

void UProjectOrganoidMainMenuWidget::BindWidgetCallbacks()
{
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleNewGameClicked);
	}
	if (LoadGameButton)
	{
		LoadGameButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked);
	}
	if (LoadSlot0Button)
	{
		LoadSlot0Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked);
	}
	if (LoadSlot1Button)
	{
		LoadSlot1Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot1Clicked);
	}
	if (LoadSlot2Button)
	{
		LoadSlot2Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot2Clicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleQuitClicked);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleMasterVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleSFXVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleMusicVolumeChanged);
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleGraphicsQualityChanged);
	}
}

void UProjectOrganoidMainMenuWidget::SyncSettingsWidgets()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(GetMasterVolume());
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(GetSFXVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(GetMusicVolume());
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->ClearOptions();
		for (const FString& Label : ProjectOrganoidMenuUI::GraphicsQualityLabels)
		{
			GraphicsQualityCombo->AddOption(Label);
		}
		const int32 QualityIndex = static_cast<int32>(GetGraphicsQuality());
		if (ProjectOrganoidMenuUI::GraphicsQualityLabels.IsValidIndex(QualityIndex))
		{
			GraphicsQualityCombo->SetSelectedOption(ProjectOrganoidMenuUI::GraphicsQualityLabels[QualityIndex]);
		}
	}
}

void UProjectOrganoidMainMenuWidget::HandleNewGameClicked()
{
	StartNewGame();
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked()
{
	LoadGameFromSlot(0);
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot1Clicked()
{
	LoadGameFromSlot(1);
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot2Clicked()
{
	LoadGameFromSlot(2);
}

void UProjectOrganoidMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}

void UProjectOrganoidMainMenuWidget::HandleMasterVolumeChanged(float Value)
{
	SetMasterVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleSFXVolumeChanged(float Value)
{
	SetSFXVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleMusicVolumeChanged(float Value)
{
	SetMusicVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	SetGraphicsQuality(ProjectOrganoidMenuUI::QualityFromLabel(SelectedItem));
}

void UProjectOrganoidMainMenuWidget::StartNewGame()
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->ClearPendingLoad();
	}

	UGameplayStatics::OpenLevel(this, GameplayLevelName);
}

bool UProjectOrganoidMainMenuWidget::LoadGameFromSlot(int32 SlotIndex)
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		return LoadGameFromSlotName(SaveSubsystem->GetSlotNameForIndex(SlotIndex));
	}
	return false;
}

bool UProjectOrganoidMainMenuWidget::LoadGameFromSlotName(const FString& SlotName)
{
	UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !SaveSubsystem->DoesSaveExist(SlotName))
	{
		return false;
	}

	SaveSubsystem->RequestLoadOnNextTravel(SlotName);
	UGameplayStatics::OpenLevel(this, GameplayLevelName);
	return true;
}

TArray<FProjectOrganoidSaveSlotInfo> UProjectOrganoidMainMenuWidget::GetSaveSlotInfos() const
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		return SaveSubsystem->GetSaveSlotInfos(MaxSaveSlots);
	}
	return TArray<FProjectOrganoidSaveSlotInfo>();
}

void UProjectOrganoidMainMenuWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}

void UProjectOrganoidMainMenuWidget::SetMasterVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMasterVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetSFXVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetSFXVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetMusicVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMusicVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetGraphicsQuality(NewQuality);
	}
}

float UProjectOrganoidMainMenuWidget::GetMasterVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMasterVolume();
	}
	return 1.0f;
}

float UProjectOrganoidMainMenuWidget::GetSFXVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetSFXVolume();
	}
	return 1.0f;
}

float UProjectOrganoidMainMenuWidget::GetMusicVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMusicVolume();
	}
	return 1.0f;
}

EProjectOrganoidGraphicsQuality UProjectOrganoidMainMenuWidget::GetGraphicsQuality() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetGraphicsQuality();
	}
	return EProjectOrganoidGraphicsQuality::High;
}

void UProjectOrganoidMainMenuWidget::ApplySettings()
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->ApplyAllSettings();
	}
}

UProjectOrganoidSaveSubsystem* UProjectOrganoidMainMenuWidget::GetSaveSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSaveSubsystem>();
	}
	return nullptr;
}

UProjectOrganoidSettingsSubsystem* UProjectOrganoidMainMenuWidget::GetSettingsSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>();
	}
	return nullptr;
}

void UProjectOrganoidMainMenuWidget::RefreshSaveSlots()
{
	OnSaveSlotsRefreshed(GetSaveSlotInfos());
}
