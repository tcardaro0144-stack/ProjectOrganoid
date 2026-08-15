// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidPlatformProfileTypes.generated.h"

/** High-level deployment target family */
UENUM(BlueprintType)
enum class EProjectOrganoidPlatformFamily : uint8
{
	PC UMETA(DisplayName = "PC"),
	Console UMETA(DisplayName = "Console"),
	Unknown UMETA(DisplayName = "Unknown")
};

/** Named scalability preset within a platform family */
UENUM(BlueprintType)
enum class EProjectOrganoidPlatformPreset : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	Low UMETA(DisplayName = "Low"),
	Medium UMETA(DisplayName = "Medium"),
	High UMETA(DisplayName = "High"),
	ConsolePerformance UMETA(DisplayName = "Console — Performance"),
	ConsoleQuality UMETA(DisplayName = "Console — Quality")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidPlatformProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FName ProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	EProjectOrganoidPlatformFamily Family = EProjectOrganoidPlatformFamily::PC;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	EProjectOrganoidPlatformPreset Preset = EProjectOrganoidPlatformPreset::High;

	/** Device profile name from DefaultDeviceProfiles.ini */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FName DeviceProfileName = TEXT("WindowsHigh");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	EProjectOrganoidGraphicsQuality OverallQuality = EProjectOrganoidGraphicsQuality::High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "50.0", ClampMax = "100.0"))
	float ResolutionScalePercent = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bAllowRayTracing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bAllowLumen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "0"))
	int32 MaxFPS = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	FText DisplayName;
};

UCLASS(BlueprintType)
class UProjectOrganoidPlatformProfileCatalog : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Platform")
	TArray<FProjectOrganoidPlatformProfile> Profiles;

	UFUNCTION(BlueprintPure, Category = "Platform")
	bool FindProfile(FName ProfileId, FProjectOrganoidPlatformProfile& OutProfile) const
	{
		for (const FProjectOrganoidPlatformProfile& Profile : Profiles)
		{
			if (Profile.ProfileId == ProfileId)
			{
				OutProfile = Profile;
				return true;
			}
		}
		return false;
	}
};
