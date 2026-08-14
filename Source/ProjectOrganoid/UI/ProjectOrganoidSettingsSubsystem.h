// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidSettingsSubsystem.generated.h"

class USoundClass;
class USoundMix;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectOrganoidSettingsChanged);

/**
 *  Master / SFX / Music volumes and graphics scalability for menus.
 *  Volumes apply via optional SoundMix overrides; master also uses the audio device.
 */
UCLASS()
class UProjectOrganoidSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintAssignable, Category = "Settings")
	FOnProjectOrganoidSettingsChanged OnSettingsChanged;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const { return MasterVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const { return SFXVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const { return MusicVolume; }

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	EProjectOrganoidGraphicsQuality GetGraphicsQuality() const { return GraphicsQuality; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality);

	/** Apply current audio + graphics to the engine and persist to config. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAllSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettingsFromConfig();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettingsToConfig() const;

	/** Optional SoundMix used for class volume overrides (assign in BP defaults / soft load). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundMix> SettingsSoundMix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float SFXVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Settings|Graphics")
	EProjectOrganoidGraphicsQuality GraphicsQuality = EProjectOrganoidGraphicsQuality::High;

	void ApplyAudioSettings();
	void ApplyGraphicsSettings();
	void ApplySoundClassVolume(USoundMix* Mix, USoundClass* SoundClass, float Volume) const;
};
