// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
AProjectOrganoidGameMode::AProjectOrganoidGameMode()
{
	DefaultPawnClass = AProjectOrganoidCharacter::StaticClass();
	PlayerControllerClass = AProjectOrganoidPlayerController::StaticClass();
	HUDWidgetClass = UProjectOrganoidHUDWidget::StaticClass();
}

void AProjectOrganoidGameMode::BeginPlay()
{
	Super::BeginPlay();

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_MainMenu"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Warning, TEXT("Gameplay GameMode on %s — spawning title menu instead of Avery."), *MapName);
		if (UWorld* World = GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(It->Get()))
				{
					OrganoidPC->SetPauseMenuAllowed(false);
					OrganoidPC->EnsureTitleMainMenu();
				}
			}
		}
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidFlowManagerSubsystem* Flow = GI->GetSubsystem<UProjectOrganoidFlowManagerSubsystem>())
		{
			Flow->NotifyGameplayMapReady(this);
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
		{
			if (AWorldSettings* WorldSettings = World->GetWorldSettings())
			{
				WorldSettings->bEnableWorldBoundsChecks = true;
				WorldSettings->KillZ = -7200.0f;
			}

			TryStartEpitopePlay();
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				SpawnHUDForPlayer(It->Get());
			}
			return;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			EnsurePossessedGameplayPawn(It->Get());
			SpawnHUDForPlayer(It->Get());
		}
	}
}

void AProjectOrganoidGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (IsEpitopeMap())
	{
		TryStartEpitopePlay();
		SpawnHUDForPlayer(NewPlayer);
		return;
	}

	EnsurePossessedGameplayPawn(NewPlayer);
	SpawnHUDForPlayer(NewPlayer);
}

void AProjectOrganoidGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (IsEpitopeMap())
	{
		TryStartEpitopePlay();
		return;
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
	EnsurePossessedGameplayPawn(NewPlayer);
}

bool AProjectOrganoidGameMode::IsEpitopeMap() const
{
	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	return MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase);
}

void AProjectOrganoidGameMode::TryStartEpitopePlay()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>();
	const FName AdminName = Levels ? Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel1_Admin) : NAME_None;
	const bool bAdminReady = Levels && Levels->IsPartitionReady(AdminName);
	const bool bTimedOut = EpitopeReadyAttempts >= 80;

	if (bAdminReady || bTimedOut)
	{
		World->GetTimerManager().ClearTimer(EpitopeReadyTimerHandle);
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			EnsurePossessedGameplayPawn(It->Get());
			if (bTimedOut && Levels)
			{
				if (AProjectOrganoidCharacter* Nathan = Cast<AProjectOrganoidCharacter>(It->Get() ? It->Get()->GetPawn() : nullptr))
				{
					Nathan->ApplyCampaignOpeningStart();
				}
			}
		}
		return;
	}

	++EpitopeReadyAttempts;
	if (!World->GetTimerManager().IsTimerActive(EpitopeReadyTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			EpitopeReadyTimerHandle,
			FTimerDelegate::CreateUObject(this, &AProjectOrganoidGameMode::TryStartEpitopePlay),
			0.1f,
			true);
	}
}

void AProjectOrganoidGameMode::EnsurePossessedGameplayPawn(APlayerController* PlayerController)
{
	if (!PlayerController || !GetWorld())
	{
		return;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_MainMenu"), ESearchCase::IgnoreCase))
	{
		if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
		{
			OrganoidPC->SetPauseMenuAllowed(false);
			OrganoidPC->EnsureTitleMainMenu();
		}
		return;
	}

	APawn* ExistingPawn = PlayerController->GetPawn();
	const bool bNeedsAvery = !ExistingPawn || ExistingPawn->IsA(ASpectatorPawn::StaticClass());
	if (bNeedsAvery)
	{
		if (ExistingPawn)
		{
			PlayerController->UnPossess();
			ExistingPawn->Destroy();
		}

		RestartPlayer(PlayerController);

		if (!PlayerController->GetPawn() || PlayerController->GetPawn()->IsA(ASpectatorPawn::StaticClass()))
		{
			UClass* PawnClass = DefaultPawnClass ? DefaultPawnClass.Get() : AProjectOrganoidCharacter::StaticClass();
			FTransform SpawnTM = FTransform::Identity;
			if (IsEpitopeMap())
			{
				if (UProjectOrganoidLevelManagerSubsystem* Levels = GetWorld()->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
				{
					SpawnTM = Levels->GetCampaignOpeningTransform();
				}
			}
			else if (AActor* Start = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass()))
			{
				SpawnTM = Start->GetActorTransform();
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.Owner = PlayerController;
			if (APawn* Spawned = GetWorld()->SpawnActor<APawn>(PawnClass, SpawnTM, SpawnParams))
			{
				PlayerController->Possess(Spawned);
			}
		}
	}

	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PlayerController))
	{
		OrganoidPC->SetPauseMenuAllowed(true);
		OrganoidPC->EnterGameplayControl();
	}
}

void AProjectOrganoidGameMode::SpawnHUDForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalPlayerController())
	{
		return;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_MainMenu"), ESearchCase::IgnoreCase))
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

	HUDWidget->AddToViewport(0);
	HUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PlayerHUDWidgets.Add(PlayerController, HUDWidget);

	UProjectOrganoidGameplayHUDController* Controller = NewObject<UProjectOrganoidGameplayHUDController>(this);
	PlayerHUDControllers.Add(PlayerController, Controller);

	TryBindHUDToPawn(PlayerController);

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

		if (TObjectPtr<UProjectOrganoidGameplayHUDController>* Found = PlayerHUDControllers.Find(PlayerController))
		{
			if (UProjectOrganoidGameplayHUDController* Controller = Found->Get())
			{
				Controller->Initialize(Avery, HUDWidget);
			}
		}

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

UProjectOrganoidGameplayHUDController* AProjectOrganoidGameMode::GetHUDControllerForPlayer(APlayerController* PlayerController) const
{
	if (const TObjectPtr<UProjectOrganoidGameplayHUDController>* Found = PlayerHUDControllers.Find(PlayerController))
	{
		return Found->Get();
	}
	return nullptr;
}
