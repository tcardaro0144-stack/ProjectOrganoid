// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectOrganoidPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UProjectOrganoidPauseWidget;

/**
 *  Organoid player controller — input mappings, Escape pause menu, touch controls.
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

protected:

	/** Input Mapping Contexts */
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

	UPROPERTY(BlueprintReadOnly, Category = "UI|Pause")
	bool bPauseMenuOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Pause")
	bool bPauseMenuAllowed = true;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
