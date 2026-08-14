// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidPauseWidget.generated.h"

class UProjectOrganoidSettingsSubsystem;

/**
 *  In-game pause overlay: resume, settings, return to main menu, quit.
 */
UCLASS()
class UProjectOrganoidPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Main menu map opened by Return To Main Menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pause")
	FName MainMenuLevelName = FName(TEXT("Lvl_MainMenu"));

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void ResumeGame();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void ReturnToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Pause")
	void QuitGame();

	// --- Settings passthrough ---

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality);

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	EProjectOrganoidGraphicsQuality GetGraphicsQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void ApplySettings();

	/** Fired when the pause menu opens (show settings panel state, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnPauseOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnPauseClosed();

protected:

	virtual void NativeConstruct() override;

	UProjectOrganoidSettingsSubsystem* GetSettingsSubsystem() const;
};
