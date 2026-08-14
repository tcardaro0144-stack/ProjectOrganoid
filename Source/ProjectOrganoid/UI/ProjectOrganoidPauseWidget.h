// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidPauseWidget.generated.h"

class UProjectOrganoidSettingsSubsystem;
class UButton;
class USlider;
class UTextBlock;

/**
 *  In-game pause overlay: resume, settings, return to main menu, quit.
 *  Optional BindWidget names match WBP_PauseMenu (created by setup_main_menu.py).
 */
UCLASS()
class UProjectOrganoidPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Main menu map opened by Return To Main Menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pause")
	FName MainMenuLevelName = FName(TEXT("/Game/Maps/Lvl_MainMenu"));

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

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetWindowMode(EProjectOrganoidWindowMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void SetResolutionScalePercent(float NewPercent);

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	EProjectOrganoidGraphicsQuality GetGraphicsQuality() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	EProjectOrganoidWindowMode GetWindowMode() const;

	UFUNCTION(BlueprintPure, Category = "Pause|Settings")
	float GetResolutionScalePercent() const;

	UFUNCTION(BlueprintCallable, Category = "Pause|Settings")
	void ApplySettings();

	/** Fired when the pause menu opens (show settings panel state, etc.). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnPauseOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Pause")
	void OnPauseClosed();

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UButton> ReturnToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UComboBoxString> GraphicsQualityCombo;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UComboBoxString> WindowModeCombo;

	/** Expects 0–1 normalized UI value; maps to 50–100% resolution scale. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Pause|Widgets")
	TObjectPtr<UTextBlock> TitleText;

	UProjectOrganoidSettingsSubsystem* GetSettingsSubsystem() const;

	void BindWidgetCallbacks();
	void SyncSettingsWidgets();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleReturnToMainMenuClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleResolutionScaleChanged(float Value);
};
