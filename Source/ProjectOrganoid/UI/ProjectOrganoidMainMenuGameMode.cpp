// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "GameFramework/SpectatorPawn.h"

AProjectOrganoidMainMenuGameMode::AProjectOrganoidMainMenuGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = AProjectOrganoidPlayerController::StaticClass();
	MainMenuWidgetClass = UProjectOrganoidMainMenuWidget::StaticClass();
	bStartPlayersAsSpectators = true;
}

namespace ProjectOrganoidMainMenuGM
{
	static TSubclassOf<UProjectOrganoidMainMenuWidget> ResolveMainMenuWidgetClass(
		TSubclassOf<UProjectOrganoidMainMenuWidget> ConfiguredClass)
	{
		if (ConfiguredClass && ConfiguredClass != UProjectOrganoidMainMenuWidget::StaticClass())
		{
			return ConfiguredClass;
		}

		if (UClass* WBPClass = LoadClass<UProjectOrganoidMainMenuWidget>(
			nullptr, TEXT("/Game/UI/Menus/WBP_MainMenu.WBP_MainMenu_C")))
		{
			return WBPClass;
		}

		return ConfiguredClass ? ConfiguredClass : UProjectOrganoidMainMenuWidget::StaticClass();
	}
}

void AProjectOrganoidMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			SpawnMainMenuForPlayer(It->Get());
		}
	}
}

void AProjectOrganoidMainMenuGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	SpawnMainMenuForPlayer(NewPlayer);
}

void AProjectOrganoidMainMenuGameMode::SpawnMainMenuForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return;
	}

	if (PlayerMenuWidgets.Contains(PlayerController) && PlayerMenuWidgets[PlayerController])
	{
		return;
	}

	TSubclassOf<UProjectOrganoidMainMenuWidget> ClassToSpawn =
		ProjectOrganoidMainMenuGM::ResolveMainMenuWidgetClass(MainMenuWidgetClass);

	UProjectOrganoidMainMenuWidget* MenuWidget = CreateWidget<UProjectOrganoidMainMenuWidget>(PlayerController, ClassToSpawn);
	if (!MenuWidget)
	{
		return;
	}

	MenuWidget->AddToViewport(10);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
	PlayerController->bShowMouseCursor = true;

	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
	{
		OrganoidPC->SetPauseMenuAllowed(false);
	}

	PlayerMenuWidgets.Add(PlayerController, MenuWidget);
}
