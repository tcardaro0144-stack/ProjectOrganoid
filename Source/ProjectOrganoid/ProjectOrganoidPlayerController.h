// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectOrganoidPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UProjectOrganoidPauseWidget;
class UProjectOrganoidMainMenuWidget;

/**
 *  Organoid player controller — input mappings, Escape pause menu, touch controls,
 *  and automatic title-screen main menu spawn (no Level Blueprint required).
 */
UCLASS()
class AProjectOrganoidPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	AProjectOrganoidPlayerController();

	/** Pause widget class spawned on Escape (defaults to UProjectOrganoidPauseWidget). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Pause")
	TSubclassOf<UProjectOrganoidPauseWidget> PauseWidgetClass;

	/** Map name token that triggers automatic main-menu spawn (PIE-safe Contains match). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|MainMenu")
	FName TitleMapNameToken = FName(TEXT("Lvl_MainMenu"));

	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void TogglePauseMenu();

	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void OpenPauseMenu();

	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void ClosePauseMenu();

	UFUNCTION(BlueprintPure, Category = "UI|Pause")
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	/** Main-menu GameMode disables pause; gameplay enables it. */
	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void SetPauseMenuAllowed(bool bAllowed);

	UFUNCTION(BlueprintPure, Category = "UI|Pause")
	bool IsPauseMenuAllowed() const { return bPauseMenuAllowed; }

	/**
	 *  Creates the C++ title menu, adds it to the viewport, and switches to UI-only input.
	 *  Safe to call multiple times — no-ops if the title menu is already up.
	 */
	UFUNCTION(BlueprintCallable, Category = "UI|MainMenu")
	UProjectOrganoidMainMenuWidget* EnsureTitleMainMenu();

	UFUNCTION(BlueprintPure, Category = "UI|MainMenu")
	bool IsTitleMainMenuVisible() const;

	/** True when the loaded map is Lvl_MainMenu or the auth GameMode is the title GameMode. */
	bool ShouldAutoSpawnTitleMainMenu() const;

	/** Remove the title overlay and release UI capture. Safe if no menu is up. */
	UFUNCTION(BlueprintCallable, Category = "UI|MainMenu")
	void DismissTitleMainMenu();

	/** GameOnly input, hidden cursor, unpaused — required after New Game travel. */
	UFUNCTION(BlueprintCallable, Category = "UI|MainMenu")
	void EnterGameplayControl();

protected:

	/** Input Mapping Contexts — optional content assets. Runtime fallback is created on Avery. */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidPauseWidget> PauseWidget;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidMainMenuWidget> TitleMainMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Pause")
	bool bPauseMenuOpen = false;

	/** Gameplay GameMode enables this. Title / default stays off so WBP_PauseMenu cannot cover the main menu. */
	UPROPERTY(BlueprintReadOnly, Category = "UI|Pause")
	bool bPauseMenuAllowed = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Local player is assigned here — safest moment to CreateWidget. */
	virtual void ReceivedPlayer() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	virtual void OnPossess(APawn* InPawn) override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	TSubclassOf<UProjectOrganoidPauseWidget> ResolvePauseWidgetClass() const;
};
