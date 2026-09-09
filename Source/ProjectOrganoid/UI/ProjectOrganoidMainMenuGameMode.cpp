// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidMainMenuGameMode::AProjectOrganoidMainMenuGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = AProjectOrganoidPlayerController::StaticClass();
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

void AProjectOrganoidMainMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	SpawnMainMenuForPlayer(NewPlayer);
}

void AProjectOrganoidMainMenuGameMode::SpawnMainMenuForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return;
	}

	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
	{
		if (!OrganoidPC->ShouldAutoSpawnTitleMainMenu())
		{
			return;
		}

		OrganoidPC->SetPauseMenuAllowed(false);
		if (UProjectOrganoidMainMenuWidget* MenuWidget = OrganoidPC->EnsureTitleMainMenu())
		{
			PlayerMenuWidgets.Add(PlayerController, MenuWidget);
		}
	}
}
