// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidPauseWidget.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoid.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "InputCoreTypes.h"

AProjectOrganoidPlayerController::AProjectOrganoidPlayerController()
{
	// Keep input ticking so Escape can close the pause menu while the world is paused.
	bShouldPerformFullTickWhenPaused = true;
}

void AProjectOrganoidPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!PauseWidgetClass || PauseWidgetClass == UProjectOrganoidPauseWidget::StaticClass())
	{
		if (UClass* WBPClass = LoadClass<UProjectOrganoidPauseWidget>(
			nullptr, TEXT("/Game/UI/Menus/WBP_PauseMenu.WBP_PauseMenu_C")))
		{
			PauseWidgetClass = WBPClass;
		}
		else if (!PauseWidgetClass)
		{
			PauseWidgetClass = UProjectOrganoidPauseWidget::StaticClass();
		}
	}

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		}
		else
		{
			UE_LOG(LogProjectOrganoid, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void AProjectOrganoidPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AProjectOrganoidPlayerController::TogglePauseMenu);
		}
	}
}

void AProjectOrganoidPlayerController::TogglePauseMenu()
{
	if (!bPauseMenuAllowed || !IsLocalPlayerController())
	{
		return;
	}

	if (bPauseMenuOpen)
	{
		ClosePauseMenu();
	}
	else
	{
		OpenPauseMenu();
	}
}

void AProjectOrganoidPlayerController::OpenPauseMenu()
{
	if (!bPauseMenuAllowed || bPauseMenuOpen || !IsLocalPlayerController())
	{
		return;
	}

	TSubclassOf<UProjectOrganoidPauseWidget> ClassToSpawn = PauseWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = LoadClass<UProjectOrganoidPauseWidget>(
			nullptr, TEXT("/Game/UI/Menus/WBP_PauseMenu.WBP_PauseMenu_C"));
	}
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidPauseWidget::StaticClass();
	}

	PauseWidget = CreateWidget<UProjectOrganoidPauseWidget>(this, ClassToSpawn);
	if (!PauseWidget)
	{
		return;
	}

	PauseWidget->AddToViewport(100);
	bPauseMenuOpen = true;

	SetPause(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(PauseWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AProjectOrganoidPlayerController::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return;
	}

	if (PauseWidget)
	{
		PauseWidget->OnPauseClosed();
		PauseWidget->RemoveFromParent();
		PauseWidget = nullptr;
	}

	bPauseMenuOpen = false;
	SetPause(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void AProjectOrganoidPlayerController::SetPauseMenuAllowed(bool bAllowed)
{
	bPauseMenuAllowed = bAllowed;
	if (!bAllowed && bPauseMenuOpen)
	{
		ClosePauseMenu();
	}
}

bool AProjectOrganoidPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
