// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidSettingsTypes.h"
#include "ProjectOrganoidMainMenuWidget.generated.h"

class UProjectOrganoidSaveSubsystem;
class UProjectOrganoidSettingsSubsystem;

/**
 *  Title-screen menu: new game, load slots, audio/graphics settings, quit.
 */
UCLASS()
class UProjectOrganoidMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Gameplay map opened by New Game / Load Game (soft path or short name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Menu")
	FName GameplayLevelName = FName(TEXT("Lvl_ThirdPerson"));

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

	UProjectOrganoidSaveSubsystem* GetSaveSubsystem() const;
	UProjectOrganoidSettingsSubsystem* GetSettingsSubsystem() const;

	void RefreshSaveSlots();
};
