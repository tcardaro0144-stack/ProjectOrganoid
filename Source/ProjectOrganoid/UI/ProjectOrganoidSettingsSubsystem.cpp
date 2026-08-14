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
	ApplyDisplaySettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::SetWindowMode(EProjectOrganoidWindowMode NewMode)
{
	WindowMode = NewMode;
	ApplyDisplaySettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::SetResolutionScalePercent(float NewPercent)
{
	ResolutionScalePercent = ClampResolutionScale(NewPercent);
	ApplyDisplaySettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::ApplyAllSettings()
{
	ApplyAudioSettings();
	ApplyDisplaySettings();
	SaveSettingsToConfig();
	OnSettingsChanged.Broadcast();
}

void UProjectOrganoidSettingsSubsystem::LoadSettingsFromConfig()
{
	if (UGameUserSettings* UserSettings = GetUserSettings())
	{
		UserSettings->LoadSettings(true);

		GraphicsQuality = static_cast<EProjectOrganoidGraphicsQuality>(
			FMath::Clamp(UserSettings->GetOverallScalabilityLevel(), 0, 4));
		WindowMode = FromEngineWindowMode(UserSettings->GetFullscreenMode());

		float CurrentNormalized = 1.0f;
		float CurrentScaleValue = 100.0f;
		float MinScaleValue = MinResolutionScalePercent;
		float MaxScaleValue = MaxResolutionScalePercent;
		UserSettings->GetResolutionScaleInformationEx(CurrentNormalized, CurrentScaleValue, MinScaleValue, MaxScaleValue);
		ResolutionScalePercent = ClampResolutionScale(CurrentScaleValue);
	}

	if (!GConfig)
	{
		return;
	}

	float LoadedMaster = MasterVolume;
	float LoadedSFX = SFXVolume;
	float LoadedMusic = MusicVolume;
	int32 LoadedQuality = static_cast<int32>(GraphicsQuality);
	int32 LoadedWindow = static_cast<int32>(WindowMode);
	float LoadedResScale = ResolutionScalePercent;

	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("MasterVolume"), LoadedMaster, GGameUserSettingsIni);
	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("SFXVolume"), LoadedSFX, GGameUserSettingsIni);
	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("MusicVolume"), LoadedMusic, GGameUserSettingsIni);
	GConfig->GetInt(ProjectOrganoidSettings::Section, TEXT("GraphicsQuality"), LoadedQuality, GGameUserSettingsIni);
	GConfig->GetInt(ProjectOrganoidSettings::Section, TEXT("WindowMode"), LoadedWindow, GGameUserSettingsIni);
	GConfig->GetFloat(ProjectOrganoidSettings::Section, TEXT("ResolutionScalePercent"), LoadedResScale, GGameUserSettingsIni);

	MasterVolume = FMath::Clamp(LoadedMaster, 0.0f, 1.0f);
	SFXVolume = FMath::Clamp(LoadedSFX, 0.0f, 1.0f);
	MusicVolume = FMath::Clamp(LoadedMusic, 0.0f, 1.0f);
	GraphicsQuality = static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(LoadedQuality, 0, 4));
	WindowMode = static_cast<EProjectOrganoidWindowMode>(FMath::Clamp(LoadedWindow, 0, 2));
	ResolutionScalePercent = ClampResolutionScale(LoadedResScale);
}

void UProjectOrganoidSettingsSubsystem::SaveSettingsToConfig() const
{
	if (GConfig)
	{
		GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("MasterVolume"), MasterVolume, GGameUserSettingsIni);
		GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("SFXVolume"), SFXVolume, GGameUserSettingsIni);
		GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("MusicVolume"), MusicVolume, GGameUserSettingsIni);
		GConfig->SetInt(ProjectOrganoidSettings::Section, TEXT("GraphicsQuality"), static_cast<int32>(GraphicsQuality), GGameUserSettingsIni);
		GConfig->SetInt(ProjectOrganoidSettings::Section, TEXT("WindowMode"), static_cast<int32>(WindowMode), GGameUserSettingsIni);
		GConfig->SetFloat(ProjectOrganoidSettings::Section, TEXT("ResolutionScalePercent"), ResolutionScalePercent, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}

	if (UGameUserSettings* UserSettings = GetUserSettings())
	{
		UserSettings->SaveSettings();
	}
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

void UProjectOrganoidSettingsSubsystem::ApplyDisplaySettings()
{
	UGameUserSettings* UserSettings = GetUserSettings();
	if (!UserSettings)
	{
		return;
	}

	UserSettings->SetOverallScalabilityLevel(static_cast<int32>(GraphicsQuality));
	UserSettings->SetFullscreenMode(ToEngineWindowMode(WindowMode));
	UserSettings->SetResolutionScaleValueEx(ResolutionScalePercent);
	UserSettings->ApplySettings(false);
	UserSettings->SaveSettings();
}

UGameUserSettings* UProjectOrganoidSettingsSubsystem::GetUserSettings() const
{
	return GEngine ? GEngine->GetGameUserSettings() : nullptr;
}

float UProjectOrganoidSettingsSubsystem::ClampResolutionScale(float Percent) const
{
	const float MinScale = FMath::Min(MinResolutionScalePercent, MaxResolutionScalePercent);
	const float MaxScale = FMath::Max(MinResolutionScalePercent, MaxResolutionScalePercent);
	return FMath::Clamp(Percent, MinScale, MaxScale);
}

EWindowMode::Type UProjectOrganoidSettingsSubsystem::ToEngineWindowMode(EProjectOrganoidWindowMode Mode) const
{
	switch (Mode)
	{
	case EProjectOrganoidWindowMode::WindowedFullscreen:
		return EWindowMode::WindowedFullscreen;
	case EProjectOrganoidWindowMode::Windowed:
		return EWindowMode::Windowed;
	case EProjectOrganoidWindowMode::Fullscreen:
	default:
		return EWindowMode::Fullscreen;
	}
}

EProjectOrganoidWindowMode UProjectOrganoidSettingsSubsystem::FromEngineWindowMode(EWindowMode::Type Mode) const
{
	switch (Mode)
	{
	case EWindowMode::WindowedFullscreen:
		return EProjectOrganoidWindowMode::WindowedFullscreen;
	case EWindowMode::Windowed:
		return EProjectOrganoidWindowMode::Windowed;
	case EWindowMode::Fullscreen:
	default:
		return EProjectOrganoidWindowMode::Fullscreen;
	}
}

FString UProjectOrganoidSettingsSubsystem::WindowModeToLabel(EProjectOrganoidWindowMode Mode)
{
	switch (Mode)
	{
	case EProjectOrganoidWindowMode::WindowedFullscreen:
		return TEXT("Borderless Window");
	case EProjectOrganoidWindowMode::Windowed:
		return TEXT("Windowed");
	case EProjectOrganoidWindowMode::Fullscreen:
	default:
		return TEXT("Fullscreen");
	}
}

EProjectOrganoidWindowMode UProjectOrganoidSettingsSubsystem::WindowModeFromLabel(const FString& Label)
{
	if (Label.Equals(TEXT("Borderless Window"), ESearchCase::IgnoreCase)
		|| Label.Equals(TEXT("WindowedFullscreen"), ESearchCase::IgnoreCase))
	{
		return EProjectOrganoidWindowMode::WindowedFullscreen;
	}
	if (Label.Equals(TEXT("Windowed"), ESearchCase::IgnoreCase))
	{
		return EProjectOrganoidWindowMode::Windowed;
	}
	return EProjectOrganoidWindowMode::Fullscreen;
}
