// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidSettingsSubsystem.generated.h"

class USoundClass;
class USoundMix;
class UGameUserSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidSettingsChanged);

/**
 *  Game options director:
 *  - Master / SFX / Music volumes via SoundMix class overrides (+ device master)
 *  - Graphics quality scalability
 *  - Dynamic resolution scale + window mode through UGameUserSettings
 *  - Persistent write to GameUserSettings.ini
 */
UCLASS()
class UProjectOrganoidSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnProjectOrganoidSettingsChanged OnSettingsChanged;

	// -------------------------------------------------------------------------
	// Audio
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float NewVolume);

	// -------------------------------------------------------------------------
	// Graphics / display
	// -------------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	EProjectOrganoidGraphicsQuality GetGraphicsQuality() const { return GraphicsQuality; }

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	EProjectOrganoidWindowMode GetWindowMode() const { return WindowMode; }

	/** 50–100 screen-percentage resolution scale */
	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	float GetResolutionScalePercent() const { return ResolutionScalePercent; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void SetWindowMode(EProjectOrganoidWindowMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void SetResolutionScalePercent(float NewPercent);

	/** Apply audio + display to the engine and persist to GameUserSettings. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAllSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettingsFromConfig();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettingsToConfig() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	static FString WindowModeToLabel(EProjectOrganoidWindowMode Mode);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	static EProjectOrganoidWindowMode WindowModeFromLabel(const FString& Label);

	/** Optional SoundMix used for class volume overrides (assign in BP defaults / soft load). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display", meta = (ClampMin = "50.0", ClampMax = "100.0"))
	float MinResolutionScalePercent = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Display", meta = (ClampMin = "50.0", ClampMax = "100.0"))
	float MaxResolutionScalePercent = 100.0f;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Graphics")
	EProjectOrganoidGraphicsQuality GraphicsQuality = EProjectOrganoidGraphicsQuality::High;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Display")
	EProjectOrganoidWindowMode WindowMode = EProjectOrganoidWindowMode::Fullscreen;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Display")
	float ResolutionScalePercent = 100.0f;

	void ApplyAudioSettings();
	void ApplyDisplaySettings();
	void ApplySoundClassVolume(USoundMix* Mix, USoundClass* SoundClass, float Volume) const;
	UGameUserSettings* GetUserSettings() const;
	float ClampResolutionScale(float Percent) const;
	EWindowMode::Type ToEngineWindowMode(EProjectOrganoidWindowMode Mode) const;
	EProjectOrganoidWindowMode FromEngineWindowMode(EWindowMode::Type Mode) const;
};
