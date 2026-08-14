// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidMainMenuWidget.generated.h"

class UProjectOrganoidSaveSubsystem;
class UProjectOrganoidSettingsSubsystem;
class UButton;
class USlider;
class UTextBlock;

/**
 *  Title-screen menu: new game, load slots, audio/graphics settings, quit.
 *  Optional BindWidget names match WBP_MainMenu (created by setup_main_menu.py).
 */
UCLASS()
class UProjectOrganoidMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Gameplay map opened by New Game / Load Game (soft path or short name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName GameplayLevelName = FName(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));

	/** Number of save slots shown in the load list (OrganoidSave0..N-1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu|Save", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxSaveSlots = 3;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartNewGame();

	UFUNCTION(BlueprintCallable, Category = "Menu|Save")
	bool LoadGameFromSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Menu|Save")
	bool LoadGameFromSlotName(const FString& SlotName);

	UFUNCTION(BlueprintPure, Category = "Menu|Save")
	TArray<FProjectOrganoidSaveSlotInfo> GetSaveSlotInfos() const;

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

	// --- Settings passthrough ---

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void SetSFXVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality);

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	float GetSFXVolume() const;

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "Menu|Settings")
	EProjectOrganoidGraphicsQuality GetGraphicsQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Menu|Settings")
	void ApplySettings();

	/** Blueprint hook when save-slot list should refresh. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu|Save")
	void OnSaveSlotsRefreshed(const TArray<FProjectOrganoidSaveSlotInfo>& Slots);

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> LoadGameButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> LoadSlot0Button;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> LoadSlot1Button;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> LoadSlot2Button;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UComboBoxString> GraphicsQualityCombo;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Menu|Widgets")
	TObjectPtr<UTextBlock> TitleText;

	UProjectOrganoidSaveSubsystem* GetSaveSubsystem() const;
	UProjectOrganoidSettingsSubsystem* GetSettingsSubsystem() const;

	void RefreshSaveSlots();
	void BindWidgetCallbacks();
	void SyncSettingsWidgets();

	UFUNCTION()
	void HandleNewGameClicked();

	UFUNCTION()
	void HandleLoadSlot0Clicked();

	UFUNCTION()
	void HandleLoadSlot1Clicked();

	UFUNCTION()
	void HandleLoadSlot2Clicked();

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
};
