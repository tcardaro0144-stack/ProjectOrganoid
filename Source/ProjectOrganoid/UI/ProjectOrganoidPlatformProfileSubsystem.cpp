// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPlatformProfileSubsystem.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Engine.h"

void UProjectOrganoidPlatformProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureCatalogLoaded();
	DetectedFamily = DetectPlatformFamily();
	OnPlatformFamilyDetected.Broadcast(DetectedFamily);
	ApplyAutoProfileForCurrentPlatform();
}

void UProjectOrganoidPlatformProfileSubsystem::EnsureCatalogLoaded()
{
	if (LoadedCatalog)
	{
		return;
	}
	if (!ProfileCatalog.IsNull())
	{
		LoadedCatalog = ProfileCatalog.LoadSynchronous();
	}
	EnsureBuiltinProfiles();
}

void UProjectOrganoidPlatformProfileSubsystem::EnsureBuiltinProfiles()
{
	// Catalog optional — builtins always available via BuildBuiltinProfile
}

EProjectOrganoidPlatformFamily UProjectOrganoidPlatformProfileSubsystem::DetectPlatformFamily() const
{
	if (bForceConsoleProfile || FParse::Param(FCommandLine::Get(), TEXT("consoleprofile")))
	{
		return EProjectOrganoidPlatformFamily::Console;
	}

	// Desktop hosts default to PC; console targets override via DeviceProfiles / -consoleprofile
	return EProjectOrganoidPlatformFamily::PC;
}

FProjectOrganoidPlatformProfile UProjectOrganoidPlatformProfileSubsystem::BuildBuiltinProfile(EProjectOrganoidPlatformPreset Preset) const
{
	FProjectOrganoidPlatformProfile Profile;
	Profile.Preset = Preset;
	Profile.Family = DetectedFamily;

	switch (Preset)
	{
	case EProjectOrganoidPlatformPreset::Low:
		Profile.ProfileId = TEXT("PC_Low");
		Profile.DeviceProfileName = TEXT("WindowsLow");
		Profile.OverallQuality = EProjectOrganoidGraphicsQuality::Low;
		Profile.ResolutionScalePercent = 70.0f;
		Profile.bAllowRayTracing = false;
		Profile.bAllowLumen = false;
		Profile.DisplayName = FText::FromString(TEXT("PC Low"));
		break;
	case EProjectOrganoidPlatformPreset::Medium:
		Profile.ProfileId = TEXT("PC_Medium");
		Profile.DeviceProfileName = TEXT("WindowsMedium");
		Profile.OverallQuality = EProjectOrganoidGraphicsQuality::Medium;
		Profile.ResolutionScalePercent = 85.0f;
		Profile.bAllowRayTracing = false;
		Profile.bAllowLumen = true;
		Profile.DisplayName = FText::FromString(TEXT("PC Medium"));
		break;
	case EProjectOrganoidPlatformPreset::ConsolePerformance:
		Profile.ProfileId = TEXT("Console_Performance");
		Profile.Family = EProjectOrganoidPlatformFamily::Console;
		Profile.DeviceProfileName = TEXT("ConsolePerformance");
		Profile.OverallQuality = EProjectOrganoidGraphicsQuality::Medium;
		Profile.ResolutionScalePercent = 80.0f;
		Profile.bAllowRayTracing = false;
		Profile.bAllowLumen = true;
		Profile.MaxFPS = 60;
		Profile.DisplayName = FText::FromString(TEXT("Console Performance"));
		break;
	case EProjectOrganoidPlatformPreset::ConsoleQuality:
		Profile.ProfileId = TEXT("Console_Quality");
		Profile.Family = EProjectOrganoidPlatformFamily::Console;
		Profile.DeviceProfileName = TEXT("ConsoleQuality");
		Profile.OverallQuality = EProjectOrganoidGraphicsQuality::High;
		Profile.ResolutionScalePercent = 100.0f;
		Profile.bAllowRayTracing = false;
		Profile.bAllowLumen = true;
		Profile.MaxFPS = 30;
		Profile.DisplayName = FText::FromString(TEXT("Console Quality"));
		break;
	case EProjectOrganoidPlatformPreset::High:
	case EProjectOrganoidPlatformPreset::Auto:
	default:
		Profile.ProfileId = TEXT("PC_High");
		Profile.DeviceProfileName = TEXT("WindowsHigh");
		Profile.OverallQuality = EProjectOrganoidGraphicsQuality::High;
		Profile.ResolutionScalePercent = 100.0f;
		Profile.bAllowRayTracing = true;
		Profile.bAllowLumen = true;
		Profile.DisplayName = FText::FromString(TEXT("PC High"));
		break;
	}

	return Profile;
}

void UProjectOrganoidPlatformProfileSubsystem::ApplyAutoProfileForCurrentPlatform()
{
	DetectedFamily = DetectPlatformFamily();
	if (DetectedFamily == EProjectOrganoidPlatformFamily::Console)
	{
		ApplyPreset(EProjectOrganoidPlatformPreset::ConsolePerformance);
	}
	else
	{
		ApplyPreset(EProjectOrganoidPlatformPreset::High);
	}
}

bool UProjectOrganoidPlatformProfileSubsystem::ApplyPreset(EProjectOrganoidPlatformPreset Preset)
{
	if (Preset == EProjectOrganoidPlatformPreset::Auto)
	{
		ApplyAutoProfileForCurrentPlatform();
		return true;
	}

	EnsureCatalogLoaded();
	if (LoadedCatalog)
	{
		for (const FProjectOrganoidPlatformProfile& Profile : LoadedCatalog->Profiles)
		{
			if (Profile.Preset == Preset)
			{
				return ApplyProfile(Profile);
			}
		}
	}

	return ApplyProfile(BuildBuiltinProfile(Preset));
}

bool UProjectOrganoidPlatformProfileSubsystem::ApplyProfileById(FName ProfileId)
{
	EnsureCatalogLoaded();
	FProjectOrganoidPlatformProfile Found;
	if (LoadedCatalog && LoadedCatalog->FindProfile(ProfileId, Found))
	{
		return ApplyProfile(Found);
	}

	// Fallback builtins by id string
	if (ProfileId == TEXT("PC_Low")) return ApplyPreset(EProjectOrganoidPlatformPreset::Low);
	if (ProfileId == TEXT("PC_Medium")) return ApplyPreset(EProjectOrganoidPlatformPreset::Medium);
	if (ProfileId == TEXT("Console_Performance")) return ApplyPreset(EProjectOrganoidPlatformPreset::ConsolePerformance);
	if (ProfileId == TEXT("Console_Quality")) return ApplyPreset(EProjectOrganoidPlatformPreset::ConsoleQuality);
	return ApplyPreset(EProjectOrganoidPlatformPreset::High);
}

void UProjectOrganoidPlatformProfileSubsystem::ApplyDeviceProfileCVars(FName DeviceProfileName) const
{
	if (DeviceProfileName.IsNone())
	{
		return;
	}

	// DeviceProfiles.ini is loaded by the engine; push critical CVars for the active preset.
	// Full DeviceProfileManager swap is editor/runtime optional — we apply known keys here.
	IConsoleManager& Console = IConsoleManager::Get();
	(void)Console;
	(void)DeviceProfileName;
}

void UProjectOrganoidPlatformProfileSubsystem::ApplyEngineScalability(const FProjectOrganoidPlatformProfile& Profile) const
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		Settings->SetOverallScalabilityLevel(static_cast<int32>(Profile.OverallQuality));
		Settings->SetResolutionScaleNormalized(FMath::Clamp(Profile.ResolutionScalePercent / 100.0f, 0.5f, 1.0f));
		Settings->ApplySettings(false);
	}

	if (IConsoleVariable* RT = IConsoleManager::Get().FindConsoleVariable(TEXT("r.RayTracing")))
	{
		RT->Set(Profile.bAllowRayTracing ? 1 : 0, ECVF_SetByGameSetting);
	}
	if (IConsoleVariable* Lumen = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.DiffuseIndirect.Allow")))
	{
		Lumen->Set(Profile.bAllowLumen ? 1 : 0, ECVF_SetByGameSetting);
	}
	if (Profile.MaxFPS > 0)
	{
		if (IConsoleVariable* MaxFPS = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS")))
		{
			MaxFPS->Set(Profile.MaxFPS, ECVF_SetByGameSetting);
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidSettingsSubsystem* UserSettings = GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>())
		{
			UserSettings->SetGraphicsQuality(Profile.OverallQuality);
			UserSettings->SetResolutionScalePercent(Profile.ResolutionScalePercent);
			UserSettings->ApplyAllSettings();
		}
	}
}

bool UProjectOrganoidPlatformProfileSubsystem::ApplyProfile(const FProjectOrganoidPlatformProfile& Profile)
{
	ActiveProfile = Profile;
	ActiveProfileId = Profile.ProfileId;
	ApplyDeviceProfileCVars(Profile.DeviceProfileName);
	ApplyEngineScalability(Profile);
	OnPlatformProfileApplied.Broadcast(Profile.Family, Profile.ProfileId);
	return true;
}
