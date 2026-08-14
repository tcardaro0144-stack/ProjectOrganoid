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

	TSubclassOf<UProjectOrganoidMainMenuWidget> ClassToSpawn = MainMenuWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidMainMenuWidget::StaticClass();
	}

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
