// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidPlatformProfileTypes.h"
#include "ProjectOrganoidPlatformProfileSubsystem.generated.h"

class UProjectOrganoidSettingsSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidPlatformProfileApplied, EProjectOrganoidPlatformFamily, Family, FName, ProfileId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidPlatformFamilyDetected, EProjectOrganoidPlatformFamily, Family);

/**
 *  Standalone platform profile manager — PC vs console scalability presets,
 *  device-profile CVars, and locked console buckets.
 */
UCLASS(Config = Game)
class UProjectOrganoidPlatformProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	TSoftObjectPtr<UProjectOrganoidPlatformProfileCatalog> ProfileCatalog;

	/** Force console presets even on PC (CI / TV-mode testing) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Platform")
	bool bForceConsoleProfile = false;

	UPROPERTY(BlueprintAssignable, Category = "Platform")
	FOnProjectOrganoidPlatformProfileApplied OnPlatformProfileApplied;

	UPROPERTY(BlueprintAssignable, Category = "Platform")
	FOnProjectOrganoidPlatformFamilyDetected OnPlatformFamilyDetected;

	UFUNCTION(BlueprintPure, Category = "Platform")
	EProjectOrganoidPlatformFamily GetDetectedPlatformFamily() const { return DetectedFamily; }

	UFUNCTION(BlueprintPure, Category = "Platform")
	FName GetActiveProfileId() const { return ActiveProfileId; }

	UFUNCTION(BlueprintPure, Category = "Platform")
	FProjectOrganoidPlatformProfile GetActiveProfile() const { return ActiveProfile; }

	UFUNCTION(BlueprintCallable, Category = "Platform")
	EProjectOrganoidPlatformFamily DetectPlatformFamily() const;

	UFUNCTION(BlueprintCallable, Category = "Platform")
	bool ApplyPreset(EProjectOrganoidPlatformPreset Preset);

	UFUNCTION(BlueprintCallable, Category = "Platform")
	bool ApplyProfileById(FName ProfileId);

	UFUNCTION(BlueprintCallable, Category = "Platform")
	bool ApplyProfile(const FProjectOrganoidPlatformProfile& Profile);

	UFUNCTION(BlueprintCallable, Category = "Platform")
	void ApplyAutoProfileForCurrentPlatform();

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidPlatformProfileCatalog> LoadedCatalog;

	EProjectOrganoidPlatformFamily DetectedFamily = EProjectOrganoidPlatformFamily::Unknown;
	FName ActiveProfileId = NAME_None;
	FProjectOrganoidPlatformProfile ActiveProfile;

	void EnsureBuiltinProfiles();
	void EnsureCatalogLoaded();
	FProjectOrganoidPlatformProfile BuildBuiltinProfile(EProjectOrganoidPlatformPreset Preset) const;
	void ApplyDeviceProfileCVars(FName DeviceProfileName) const;
	void ApplyEngineScalability(const FProjectOrganoidPlatformProfile& Profile) const;
};
