// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPauseWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace ProjectOrganoidPauseUI
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

void UProjectOrganoidPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindWidgetCallbacks();
	SyncSettingsWidgets();
	OnPauseOpened();
}

void UProjectOrganoidPauseWidget::BindWidgetCallbacks()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleResumeClicked);
	}
	if (ReturnToMainMenuButton)
	{
		ReturnToMainMenuButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleReturnToMainMenuClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleQuitClicked);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleMasterVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleSFXVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleMusicVolumeChanged);
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleGraphicsQualityChanged);
	}
}

void UProjectOrganoidPauseWidget::SyncSettingsWidgets()
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
		for (const FString& Label : ProjectOrganoidPauseUI::GraphicsQualityLabels)
		{
			GraphicsQualityCombo->AddOption(Label);
		}
		const int32 QualityIndex = static_cast<int32>(GetGraphicsQuality());
		if (ProjectOrganoidPauseUI::GraphicsQualityLabels.IsValidIndex(QualityIndex))
		{
			GraphicsQualityCombo->SetSelectedOption(ProjectOrganoidPauseUI::GraphicsQualityLabels[QualityIndex]);
		}
	}
}

void UProjectOrganoidPauseWidget::HandleResumeClicked()
{
	ResumeGame();
}

void UProjectOrganoidPauseWidget::HandleReturnToMainMenuClicked()
{
	ReturnToMainMenu();
}

void UProjectOrganoidPauseWidget::HandleQuitClicked()
{
	QuitGame();
}

void UProjectOrganoidPauseWidget::HandleMasterVolumeChanged(float Value)
{
	SetMasterVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleSFXVolumeChanged(float Value)
{
	SetSFXVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleMusicVolumeChanged(float Value)
{
	SetMusicVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	SetGraphicsQuality(ProjectOrganoidPauseUI::QualityFromLabel(SelectedItem));
}

void UProjectOrganoidPauseWidget::ResumeGame()
{
	if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
	else if (APlayerController* FallbackPC = GetOwningPlayer())
	{
		FallbackPC->SetPause(false);
		RemoveFromParent();
	}
}

void UProjectOrganoidPauseWidget::ReturnToMainMenu()
{
	if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
	else if (APlayerController* FallbackPC = GetOwningPlayer())
	{
		FallbackPC->SetPause(false);
	}

	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UProjectOrganoidPauseWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PC))
	{
		OrganoidPC->ClosePauseMenu();
	}
	else if (PC)
	{
		PC->SetPause(false);
	}

	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}

void UProjectOrganoidPauseWidget::SetMasterVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMasterVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetSFXVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetSFXVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetMusicVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMusicVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetGraphicsQuality(NewQuality);
	}
}

float UProjectOrganoidPauseWidget::GetMasterVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMasterVolume();
	}
	return 1.0f;
}

float UProjectOrganoidPauseWidget::GetSFXVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetSFXVolume();
	}
	return 1.0f;
}

float UProjectOrganoidPauseWidget::GetMusicVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMusicVolume();
	}
	return 1.0f;
}

EProjectOrganoidGraphicsQuality UProjectOrganoidPauseWidget::GetGraphicsQuality() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetGraphicsQuality();
	}
	return EProjectOrganoidGraphicsQuality::High;
}

void UProjectOrganoidPauseWidget::ApplySettings()
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->ApplyAllSettings();
	}
}

UProjectOrganoidSettingsSubsystem* UProjectOrganoidPauseWidget::GetSettingsSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>();
	}
	return nullptr;
}
