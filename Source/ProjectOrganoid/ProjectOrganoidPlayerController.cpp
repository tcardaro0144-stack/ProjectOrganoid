// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidPauseWidget.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "ProjectOrganoidUIAssetPaths.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "ProjectOrganoid.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "InputCoreTypes.h"

AProjectOrganoidPlayerController::AProjectOrganoidPlayerController()
{
	// Keep input ticking so Escape can close the pause menu while the world is paused.
	bShouldPerformFullTickWhenPaused = true;

	// Content IMC is optional. Never probe the template ThirdPerson path — that asset
	// is not in this project and LoadObject spam-fails Enhanced Input on every PIE.
	if (DefaultMappingContexts.IsEmpty())
	{
		if (UInputMappingContext* IMC = LoadObject<UInputMappingContext>(
			nullptr,
			TEXT("/Game/Input/IMC_Default.IMC_Default"),
			nullptr,
			LOAD_NoWarn | LOAD_Quiet))
		{
			DefaultMappingContexts.AddUnique(IMC);
		}
	}
}

void AProjectOrganoidPlayerController::BeginPlay()
{
	Super::BeginPlay();

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

	if (!IsLocalPlayerController())
	{
		return;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		EnterGameplayControl();
		return;
	}

	SetPauseMenuAllowed(false);

	TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(
		this, UProjectOrganoidMainMenuWidget::StaticClass());
	if (!TitleMainMenuWidget)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(
				GI, UProjectOrganoidMainMenuWidget::StaticClass());
		}
	}
	if (!TitleMainMenuWidget)
	{
		UE_LOG(LogProjectOrganoid, Error, TEXT("BeginPlay: CreateWidget failed for C++ title menu."));
		return;
	}

	TitleMainMenuWidget->AddToViewport(10);
	TitleMainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	TitleMainMenuWidget->TakeWidget();

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TitleMainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	bShowMouseCursor = true;

	UE_LOG(LogProjectOrganoid, Log, TEXT("TITLE_CPP_ONLY: BeginPlay spawned UProjectOrganoidMainMenuWidget on '%s'."), *MapName);
}

void AProjectOrganoidPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalPlayerController() && ShouldAutoSpawnTitleMainMenu())
	{
		SetPauseMenuAllowed(false);
		EnsureTitleMainMenu();
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

bool AProjectOrganoidPlayerController::ShouldAutoSpawnTitleMainMenu() const
{
	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	const FString Token = TitleMapNameToken.ToString();
	if (!Token.IsEmpty() && MapName.Contains(Token, ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (const UWorld* World = GetWorld())
	{
		if (Cast<AProjectOrganoidMainMenuGameMode>(World->GetAuthGameMode()))
		{
			return true;
		}
	}

	return false;
}

TSubclassOf<UProjectOrganoidPauseWidget> AProjectOrganoidPlayerController::ResolvePauseWidgetClass() const
{
	auto IsUsablePauseClass = [](const UClass* Class) -> bool
	{
		return Class
			&& !Class->HasAnyClassFlags(CLASS_Abstract)
			&& Class->IsChildOf(UProjectOrganoidPauseWidget::StaticClass());
	};

	if (IsUsablePauseClass(PauseWidgetClass) && PauseWidgetClass != UProjectOrganoidPauseWidget::StaticClass())
	{
		return PauseWidgetClass;
	}

	const FSoftClassPath SoftClassPath(ProjectOrganoidUIAssetPaths::PauseMenuWidgetClass);
	if (UClass* LoadedClass = SoftClassPath.TryLoadClass<UProjectOrganoidPauseWidget>())
	{
		if (IsUsablePauseClass(LoadedClass))
		{
			return LoadedClass;
		}
	}

	if (UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, ProjectOrganoidUIAssetPaths::PauseMenuWidgetAsset))
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
		{
			if (UClass* Generated = Blueprint->GeneratedClass.Get())
			{
				if (IsUsablePauseClass(Generated))
				{
					return Generated;
				}
			}
		}
	}

	UE_LOG(LogProjectOrganoid, Warning,
		TEXT("ResolvePauseWidgetClass: WBP_PauseMenu unavailable, using C++ parent."));
	return UProjectOrganoidPauseWidget::StaticClass();
}

bool AProjectOrganoidPlayerController::IsTitleMainMenuVisible() const
{
	return TitleMainMenuWidget && TitleMainMenuWidget->IsInViewport();
}

UProjectOrganoidMainMenuWidget* AProjectOrganoidPlayerController::EnsureTitleMainMenu()
{
	if (!IsLocalPlayerController())
	{
		return nullptr;
	}

	if (IsTitleMainMenuVisible())
	{
		return TitleMainMenuWidget;
	}

	const TSubclassOf<UProjectOrganoidMainMenuWidget> ClassToSpawn = UProjectOrganoidMainMenuWidget::StaticClass();
	TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(this, ClassToSpawn);
	if (!TitleMainMenuWidget)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(GI, ClassToSpawn);
		}
	}
	if (!TitleMainMenuWidget)
	{
		UE_LOG(LogProjectOrganoid, Error, TEXT("EnsureTitleMainMenu: CreateWidget failed for C++ title menu."));
		return nullptr;
	}

	TitleMainMenuWidget->AddToViewport(10);
	TitleMainMenuWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly InputMode;
	TSharedPtr<SWidget> FocusTarget;
	if (UWidget* DefaultFocus = TitleMainMenuWidget->GetDefaultFocusWidget())
	{
		FocusTarget = DefaultFocus->GetCachedWidget();
	}
	InputMode.SetWidgetToFocus(FocusTarget.IsValid() ? FocusTarget : TitleMainMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	bShowMouseCursor = true;
	SetPauseMenuAllowed(false);

	UE_LOG(LogProjectOrganoid, Log, TEXT("TITLE_CPP_ONLY: EnsureTitleMainMenu spawned UProjectOrganoidMainMenuWidget."));
	return TitleMainMenuWidget;
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

	TSubclassOf<UProjectOrganoidPauseWidget> ClassToSpawn = ResolvePauseWidgetClass();

	PauseWidget = CreateWidget<UProjectOrganoidPauseWidget>(this, ClassToSpawn);
	if (!PauseWidget && ClassToSpawn != UProjectOrganoidPauseWidget::StaticClass())
	{
		UE_LOG(LogProjectOrganoid, Warning, TEXT("OpenPauseMenu: WBP_PauseMenu failed to spawn, using C++ layout."));
		PauseWidget = CreateWidget<UProjectOrganoidPauseWidget>(this, UProjectOrganoidPauseWidget::StaticClass());
	}
	if (!PauseWidget)
	{
		return;
	}

	PauseWidget->AddToViewport(100);
	PauseWidget->OnPauseOpened();
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

void AProjectOrganoidPlayerController::DismissTitleMainMenu()
{
	if (TitleMainMenuWidget)
	{
		TitleMainMenuWidget->DismissFromViewport();
		TitleMainMenuWidget = nullptr;
	}

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ReleaseAllPointerCapture();
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::SetDirectly);
	}

	FlushPressedKeys();
}

void AProjectOrganoidPlayerController::EnterGameplayControl()
{
	DismissTitleMainMenu();

	if (bPauseMenuOpen)
	{
		ClosePauseMenu();
	}

	SetPause(false);
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true);
	SetInputMode(InputMode);
	SetShowMouseCursor(false);
	bShowMouseCursor = false;

	if (UWorld* World = GetWorld())
	{
		if (APlayerCameraManager* CameraManager = PlayerCameraManager)
		{
			CameraManager->SetManualCameraFade(0.0f, FLinearColor::Black, false);
		}

		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->SetMouseLockMode(EMouseLockMode::LockOnCapture);
			Viewport->SetHideCursorDuringCapture(true);
		}
	}

	UE_LOG(LogProjectOrganoid, Log, TEXT("EnterGameplayControl: GameOnly input, cursor hidden, title dismissed."));
}

void AProjectOrganoidPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (InPawn && MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		EnterGameplayControl();
	}
}

bool AProjectOrganoidPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
