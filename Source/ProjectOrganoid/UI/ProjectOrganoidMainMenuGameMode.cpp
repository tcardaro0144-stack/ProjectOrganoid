// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
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

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidFlowManagerSubsystem* Flow = GI->GetSubsystem<UProjectOrganoidFlowManagerSubsystem>())
		{
			Flow->EnterTitleState();
		}
	}

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

	// Prefer the PlayerController path (also covers map-name auto-detect / PIE timing).
	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
	{
		if (MainMenuWidgetClass && MainMenuWidgetClass != UProjectOrganoidMainMenuWidget::StaticClass())
		{
			OrganoidPC->MainMenuWidgetClass = MainMenuWidgetClass;
		}

		if (UProjectOrganoidMainMenuWidget* MenuWidget = OrganoidPC->EnsureTitleMainMenu())
		{
			PlayerMenuWidgets.Add(PlayerController, MenuWidget);
		}
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("MainMenu: PlayerController is not AProjectOrganoidPlayerController — cannot auto-spawn WBP_MainMenu."));
}
