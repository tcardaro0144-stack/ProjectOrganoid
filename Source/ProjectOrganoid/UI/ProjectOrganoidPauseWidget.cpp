// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPauseWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UProjectOrganoidPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OnPauseOpened();
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
