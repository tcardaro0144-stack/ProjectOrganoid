// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UProjectOrganoidMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshSaveSlots();
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
