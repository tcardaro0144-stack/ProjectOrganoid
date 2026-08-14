// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AProjectOrganoidGameMode::AProjectOrganoidGameMode()
{
	DefaultPawnClass = AProjectOrganoidCharacter::StaticClass();
	PlayerControllerClass = AProjectOrganoidPlayerController::StaticClass();
	HUDWidgetClass = UProjectOrganoidHUDWidget::StaticClass();
}

void AProjectOrganoidGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			SpawnHUDForPlayer(It->Get());
		}
	}
}

void AProjectOrganoidGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	SpawnHUDForPlayer(NewPlayer);
}

void AProjectOrganoidGameMode::SpawnHUDForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return;
	}

	if (PlayerHUDWidgets.Contains(PlayerController) && PlayerHUDWidgets[PlayerController])
	{
		TryBindHUDToPawn(PlayerController);
		return;
	}

	TSubclassOf<UProjectOrganoidHUDWidget> ClassToSpawn = HUDWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidHUDWidget::StaticClass();
	}

	UProjectOrganoidHUDWidget* HUDWidget = CreateWidget<UProjectOrganoidHUDWidget>(PlayerController, ClassToSpawn);
	if (!HUDWidget)
	{
		return;
	}

	HUDWidget->AddToViewport();
	PlayerHUDWidgets.Add(PlayerController, HUDWidget);
	TryBindHUDToPawn(PlayerController);

	// Pawn may not be possessed yet — retry next tick
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<AProjectOrganoidGameMode> WeakThis(this);
		TWeakObjectPtr<APlayerController> WeakPC(PlayerController);
		World->GetTimerManager().SetTimerForNextTick([WeakThis, WeakPC]()
		{
			if (WeakThis.IsValid() && WeakPC.IsValid())
			{
				WeakThis->TryBindHUDToPawn(WeakPC.Get());
			}
		});
	}
}

void AProjectOrganoidGameMode::TryBindHUDToPawn(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerHUDWidgets.Contains(PlayerController))
	{
		return;
	}

	UProjectOrganoidHUDWidget* HUDWidget = PlayerHUDWidgets[PlayerController];
	AProjectOrganoidCharacter* Avery = Cast<AProjectOrganoidCharacter>(PlayerController->GetPawn());
	if (HUDWidget && Avery)
	{
		HUDWidget->BindToCharacter(Avery);

		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
			{
				SaveSubsystem->TryApplyPendingLoad(Avery);
			}
		}
	}

	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
	{
		OrganoidPC->SetPauseMenuAllowed(true);
	}
}
