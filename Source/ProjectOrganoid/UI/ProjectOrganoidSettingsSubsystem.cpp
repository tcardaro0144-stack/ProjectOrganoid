// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSettingsSubsystem.h"
#include "AudioDevice.h"
#include "AudioDeviceManager.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace ProjectOrganoidSettings
{
	static const TCHAR* Section = TEXT("/Script/ProjectOrganoid.ProjectOrganoidSettings");
}

void UProjectOrganoidSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettingsFromConfig();
	ApplyAllSettings();
}

void UProjectOrganoidSettingsSubsystem::SetMasterVolume(float NewVolume)
{
	MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplyAudioSettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::SetSFXVolume(float NewVolume)
{
	SFXVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplyAudioSettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::SetMusicVolume(float NewVolume)
{
	MusicVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	ApplyAudioSettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality)
{
	GraphicsQuality = NewQuality;
	ApplyGraphicsSettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::ApplyAllSettings()
{
	ApplyAudioSettings();
	ApplyGraphicsSettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::LoadSettingsFromConfig()
{
	if (!GConfig)
	{
		return;
	}

	float LoadedMaster = MasterVolume;
	float LoadedSFX = SFXVolume;
	float LoadedMusic = MusicVolume;
	int32 LoadedQuality = static_cast<int32>(GraphicsQuality);

	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("MasterVolume"), LoadedMaster, GGameUserSettingsIni);
	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("SFXVolume"), LoadedSFX, GGameUserSettingsIni);
	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("MusicVolume"), LoadedMusic, GGameUserSettingsIni);
	GConfig->GetInt(ProjectOrganoidSettings::Section, TEXT("GraphicsQuality"), LoadedQuality, GGameUserSettingsIni);

	MasterVolume = FMath::Clamp(LoadedMaster, 0.0f, 1.0f);
	SFXVolume = FMath::Clamp(LoadedSFX, 0.0f, 1.0f);
	MusicVolume = FMath::Clamp(LoadedMusic, 0.0f, 1.0f);
	GraphicsQuality = static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(LoadedQuality, 0, 4));
}

void UProjectOrganoidSettingsSubsystem::SaveSettingsToConfig() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("SFXVolume"), SFXVolume, GGameUserSettingsIni);
	GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
	GConfig->SetInt(ProjectOrganoidSettings::Section, TEXT("GraphicsQuality"), static_cast<int32>(GraphicsQuality), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UProjectOrganoidSettingsSubsystem::ApplyAudioSettings()
{
	if (GEngine)
	{
		if (FAudioDeviceHandle AudioDevice = GEngine->GetMainAudioDeviceHandled())
		{
			AudioDevice->SetTransientMasterVolume(MasterVolume);
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			World = GI->GetWorld();
		}
	}

	USoundMix* Mix = SettingsSoundMix.LoadSynchronous();
	if (!World || !Mix)
	{
		return;
	}

	UGameplayStatics::SetBaseSoundMix(World, Mix);
	ApplySoundClassVolume(Mix, MasterSoundClass.LoadSynchronous(), MasterVolume);
	ApplySoundClassVolume(Mix, SFXSoundClass.LoadSynchronous(), SFXVolume);
	ApplySoundClassVolume(Mix, MusicSoundClass.LoadSynchronous(), MusicVolume);
}

void UProjectOrganoidSettingsSubsystem::ApplySoundClassVolume(USoundMix* Mix, USoundClass* SoundClass, float Volume) const
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || !Mix || !SoundClass)
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(World, Mix, SoundClass, Volume, 1.0f, 0.0f, true);
}

void UProjectOrganoidSettingsSubsystem::ApplyGraphicsSettings()
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		UserSettings->SetOverallScalabilityLevel(static_cast<int32>(GraphicsQuality));
		UserSettings->ApplySettings(false);
	}
}
