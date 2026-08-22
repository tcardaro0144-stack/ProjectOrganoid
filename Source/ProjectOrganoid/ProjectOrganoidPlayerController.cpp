// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidPauseWidget.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidUIAssetPaths.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Blueprint.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/ConstructorHelpers.h"
#include "ProjectOrganoid.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "InputCoreTypes.h"

AProjectOrganoidPlayerController::AProjectOrganoidPlayerController()
{
	// Keep input ticking so Escape can close the pause menu while the world is paused.
	bShouldPerformFullTickWhenPaused = true;

	// Bind CDO default to the Content Browser asset:
	// /Game/UI/Menus/WBP_MainMenu  (Content/UI/Menus/WBP_MainMenu.uasset)
	static ConstructorHelpers::FClassFinder<UProjectOrganoidMainMenuWidget> MainMenuBP(
		ProjectOrganoidUIAssetPaths::MainMenuWidgetFinder);
	if (MainMenuBP.Succeeded())
	{
		MainMenuWidgetClass = MainMenuBP.Class;
	}
	// WBP_PauseMenu is optional — resolved at runtime if/when the asset exists.
}

void AProjectOrganoidPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!PauseWidgetClass || PauseWidgetClass == UProjectOrganoidPauseWidget::StaticClass())
	{
		if (UClass* WBPClass = LoadClass<UProjectOrganoidPauseWidget>(
			nullptr, ProjectOrganoidUIAssetPaths::PauseMenuWidgetClass))
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

	// Auto-create WBP_MainMenu on title maps / MainMenu GameMode (no Level Blueprint).
	if (IsLocalPlayerController() && ShouldAutoSpawnTitleMainMenu())
	{
		if (UWorld* World = GetWorld())
		{
			// Next tick so local player / viewport are fully ready in PIE.
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				EnsureTitleMainMenu();
			}));
		}
		else
		{
			EnsureTitleMainMenu();
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

bool AProjectOrganoidPlayerController::ShouldAutoSpawnTitleMainMenu() const
{
	if (const UWorld* World = GetWorld())
	{
		if (Cast<AProjectOrganoidMainMenuGameMode>(World->GetAuthGameMode()))
		{
			return true;
		}
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	const FString Token = TitleMapNameToken.ToString();
	return !Token.IsEmpty() && MapName.Contains(Token);
}

TSubclassOf<UProjectOrganoidMainMenuWidget> AProjectOrganoidPlayerController::ResolveMainMenuWidgetClass() const
{
	auto IsUsableMenuClass = [](const UClass* Class) -> bool
	{
		return Class
			&& !Class->HasAnyClassFlags(CLASS_Abstract)
			&& Class->IsChildOf(UProjectOrganoidMainMenuWidget::StaticClass());
	};

	if (IsUsableMenuClass(MainMenuWidgetClass))
	{
		return MainMenuWidgetClass;
	}

	// Exact Content Browser generated-class path:
	// /Game/UI/Menus/WBP_MainMenu.WBP_MainMenu_C
	{
		const FSoftClassPath SoftClassPath(ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
		if (UClass* LoadedClass = SoftClassPath.TryLoadClass<UProjectOrganoidMainMenuWidget>())
		{
			if (IsUsableMenuClass(LoadedClass))
			{
				UE_LOG(LogProjectOrganoid, Log, TEXT("ResolveMainMenuWidgetClass: SoftClassPath OK (%s)"),
					ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
				return LoadedClass;
			}
		}
	}

	if (UClass* LoadedClass = LoadClass<UProjectOrganoidMainMenuWidget>(
		nullptr, ProjectOrganoidUIAssetPaths::MainMenuWidgetClass))
	{
		if (IsUsableMenuClass(LoadedClass))
		{
			UE_LOG(LogProjectOrganoid, Log, TEXT("ResolveMainMenuWidgetClass: LoadClass OK (%s)"),
				ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
			return LoadedClass;
		}
	}

	// Load the WidgetBlueprint asset, then use GeneratedClass.
	// /Game/UI/Menus/WBP_MainMenu.WBP_MainMenu
	if (UObject* Asset = StaticLoadObject(
		UObject::StaticClass(),
		nullptr,
		ProjectOrganoidUIAssetPaths::MainMenuWidgetAsset))
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(Asset))
		{
			if (UClass* Generated = Blueprint->GeneratedClass.Get())
			{
				if (IsUsableMenuClass(Generated))
				{
					UE_LOG(LogProjectOrganoid, Log, TEXT("ResolveMainMenuWidgetClass: Blueprint GeneratedClass OK (%s)"),
						ProjectOrganoidUIAssetPaths::MainMenuWidgetAsset);
					return Generated;
				}
			}
		}

		if (UClass* AsClass = Cast<UClass>(Asset))
		{
			if (IsUsableMenuClass(AsClass))
			{
				return AsClass;
			}
		}
	}

	UE_LOG(LogProjectOrganoid, Error,
		TEXT("ResolveMainMenuWidgetClass: failed to load %s (disk: Content/UI/Menus/WBP_MainMenu.uasset)"),
		ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);

	return UProjectOrganoidMainMenuWidget::StaticClass();
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

	// CreateWidget requires a valid owning Player (or World/GI). During Live Coding
	// re-instancing / early BeginPlay the PC can exist without Player yet.
	if (!Player)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (Player)
				{
					EnsureTitleMainMenu();
				}
			}));
		}
		UE_LOG(LogProjectOrganoid, Verbose, TEXT("EnsureTitleMainMenu: deferring until Player is attached."));
		return nullptr;
	}

	const TSubclassOf<UProjectOrganoidMainMenuWidget> ClassToSpawn = ResolveMainMenuWidgetClass();
	if (!ClassToSpawn)
	{
		UE_LOG(LogProjectOrganoid, Error,
			TEXT("EnsureTitleMainMenu: no widget class (expected %s)."),
			ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
		return nullptr;
	}

	if (ClassToSpawn == UProjectOrganoidMainMenuWidget::StaticClass())
	{
		UE_LOG(LogProjectOrganoid, Warning,
			TEXT("EnsureTitleMainMenu: using C++ parent only — failed to load %s"),
			ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
	}

	TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(this, ClassToSpawn);
	if (!TitleMainMenuWidget)
	{
		// Fallback owner if PC still rejects CreateWidget.
		if (UGameInstance* GI = GetGameInstance())
		{
			TitleMainMenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(GI, ClassToSpawn);
		}
	}
	if (!TitleMainMenuWidget)
	{
		UE_LOG(LogProjectOrganoid, Error, TEXT("EnsureTitleMainMenu: CreateWidget failed for %s"), *GetNameSafe(ClassToSpawn));
		return nullptr;
	}

	TitleMainMenuWidget->AddToViewport(10);
	TitleMainMenuWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly InputMode;
	// Focus the New Game button so keyboard/gamepad navigation starts on it.
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

	UE_LOG(LogProjectOrganoid, Log, TEXT("EnsureTitleMainMenu: spawned %s from %s (UI-only + cursor)"),
		*GetNameSafe(ClassToSpawn),
		ProjectOrganoidUIAssetPaths::MainMenuWidgetClass);
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

	TSubclassOf<UProjectOrganoidPauseWidget> ClassToSpawn = PauseWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = LoadClass<UProjectOrganoidPauseWidget>(
			nullptr, ProjectOrganoidUIAssetPaths::PauseMenuWidgetClass);
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
