// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "ProjectOrganoidLoadingScreenWidget.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidMainMenuGameMode.h"
#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidLevelTypes.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UProjectOrganoidFlowManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FlowState = EProjectOrganoidFlowState::Boot;
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &UProjectOrganoidFlowManagerSubsystem::HandlePostLoadMap);
}

void UProjectOrganoidFlowManagerSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	HideLoadingScreen();
	Super::Deinitialize();
}

void UProjectOrganoidFlowManagerSubsystem::SetFlowState(EProjectOrganoidFlowState NewState)
{
	if (FlowState == NewState)
	{
		return;
	}

	const EProjectOrganoidFlowState Previous = FlowState;
	FlowState = NewState;
	OnFlowStateChanged.Broadcast(FlowState, Previous);
}

void UProjectOrganoidFlowManagerSubsystem::EnterTitleState()
{
	SetFlowState(EProjectOrganoidFlowState::Title);
}

void UProjectOrganoidFlowManagerSubsystem::ShowLoadingScreen(const FText& InitialStatus)
{
	HideLoadingScreen();

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	TSubclassOf<UProjectOrganoidLoadingScreenWidget> ClassToSpawn = LoadingScreenClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidLoadingScreenWidget::StaticClass();
	}

	// GameInstance-owned so the overlay survives OpenLevel and can be dismissed after possession.
	ActiveLoadingScreen = CreateWidget<UProjectOrganoidLoadingScreenWidget>(GI, ClassToSpawn);
	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->AddToViewport(1000);
		SetLoadingProgress(0.05f, InitialStatus);
	}
}

void UProjectOrganoidFlowManagerSubsystem::HideLoadingScreen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideLoadingHandle);
	}

	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->OnLoadingFinished();
		ActiveLoadingScreen->RemoveFromParent();
		ActiveLoadingScreen = nullptr;
	}
}

void UProjectOrganoidFlowManagerSubsystem::SetLoadingProgress(float Progress01, const FText& StatusText)
{
	OnLoadingProgress.Broadcast(Progress01, StatusText);
	if (ActiveLoadingScreen)
	{
		ActiveLoadingScreen->SetStatus(StatusText, Progress01);
	}
}

void UProjectOrganoidFlowManagerSubsystem::PrepareForTravel()
{
	DismissLeftoverTitleWidgets();

	if (UWorld* World = GetWorld())
	{
		if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(
			UGameplayStatics::GetPlayerController(World, 0)))
		{
			PC->DismissTitleMainMenu();
			PC->SetPause(false);
			PC->FlushPressedKeys();
		}

		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->RemoveAllViewportWidgets();
		}
	}
}

void UProjectOrganoidFlowManagerSubsystem::DismissLeftoverTitleWidgets()
{
	if (UWorld* World = GetWorld())
	{
		if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(
			UGameplayStatics::GetPlayerController(World, 0)))
		{
			PC->DismissTitleMainMenu();
		}
	}
}

void UProjectOrganoidFlowManagerSubsystem::TravelToGameplayLevel(FName LevelName)
{
	if (FlowState == EProjectOrganoidFlowState::Loading)
	{
		return;
	}

	const FName Target = LevelName.IsNone() ? GameplayLevelName : LevelName;
	PendingGameplayLevel = Target;
	SetFlowState(EProjectOrganoidFlowState::Loading);
	PrepareForTravel();
	ShowLoadingScreen(FText::FromString(TEXT("Initializing Epitope lockdown...")));
	SetLoadingProgress(0.35f, FText::FromString(TEXT("Streaming facility sectors...")));
	LoadingStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	TWeakObjectPtr<UProjectOrganoidFlowManagerSubsystem> WeakThis(this);
	const FName TravelTarget = Target;
	auto DoTravel = [WeakThis, TravelTarget]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		UGameplayStatics::OpenLevel(WeakThis.Get(), TravelTarget);
	};

	if (UWorld* World = GetWorld())
	{
		// Next tick so the New Game click / UI capture is fully released first.
		World->GetTimerManager().SetTimerForNextTick(DoTravel);
	}
	else
	{
		DoTravel();
	}
}

void UProjectOrganoidFlowManagerSubsystem::StartNewGame(FName OverrideGameplayLevel)
{
	PendingLoadSlot.Reset();
	TravelToGameplayLevel(OverrideGameplayLevel);
}

void UProjectOrganoidFlowManagerSubsystem::LoadGameAndTravel(const FString& SlotName, FName OverrideGameplayLevel)
{
	PendingLoadSlot = SlotName;
	if (UProjectOrganoidSaveSubsystem* Save = GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>())
	{
		Save->RequestLoadOnNextTravel(SlotName);
	}
	TravelToGameplayLevel(OverrideGameplayLevel);
}

void UProjectOrganoidFlowManagerSubsystem::ReturnToTitle()
{
	PendingLoadSlot.Reset();
	SetFlowState(EProjectOrganoidFlowState::Loading);
	PrepareForTravel();
	ShowLoadingScreen(FText::FromString(TEXT("Returning to title...")));
	UGameplayStatics::OpenLevel(this, TitleLevelName);
	SetFlowState(EProjectOrganoidFlowState::Title);
}

void UProjectOrganoidFlowManagerSubsystem::ApplyGameplayHandoff(bool bHideLoadingNow)
{
	DismissLeftoverTitleWidgets();

	UWorld* World = GetWorld();
	if (!World)
	{
		if (bHideLoadingNow)
		{
			HideLoadingScreen();
		}
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (AProjectOrganoidGameMode* GameplayGM = Cast<AProjectOrganoidGameMode>(World->GetAuthGameMode()))
	{
		GameplayGM->EnsurePossessedGameplayPawn(PC);
	}
	else if (PC)
	{
		APawn* Existing = PC->GetPawn();
		if (!Existing || Existing->IsA(ASpectatorPawn::StaticClass()))
		{
			if (Existing)
			{
				PC->UnPossess();
				Existing->Destroy();
			}

			UClass* PawnClass = AProjectOrganoidCharacter::StaticClass();
			FTransform SpawnTM = FTransform::Identity;
			if (AActor* Start = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
			{
				SpawnTM = Start->GetActorTransform();
			}

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			if (APawn* Spawned = World->SpawnActor<APawn>(PawnClass, SpawnTM, SpawnParams))
			{
				PC->Possess(Spawned);
			}
		}

		if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PC))
		{
			OrganoidPC->SetPauseMenuAllowed(true);
			OrganoidPC->EnterGameplayControl();
		}
		else
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
			PC->SetShowMouseCursor(false);
		}
	}

	if (bHideLoadingNow)
	{
		HideLoadingScreen();
	}
}

void UProjectOrganoidFlowManagerSubsystem::NotifyGameplayMapReady(AProjectOrganoidGameMode* /*GameMode*/)
{
	UWorld* CurrentWorld = GetWorld();
	const FString MapName = CurrentWorld
		? UGameplayStatics::GetCurrentLevelName(CurrentWorld, /*bRemovePrefixString=*/true)
		: FString();
	if (MapName.Contains(TEXT("Lvl_MainMenu"), ESearchCase::IgnoreCase))
	{
		EnterTitleState();
		return;
	}

	SetFlowState(EProjectOrganoidFlowState::Gameplay);
	SetLoadingProgress(0.9f, FText::FromString(TEXT("Synchronizing Avery's suit telemetry...")));
	ApplyGameplayHandoff(false);

	UWorld* World = GetWorld();
	if (!World)
	{
		HideLoadingScreen();
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - LoadingStartTime;
	const float Remaining = FMath::Max(0.0f, MinimumLoadingSeconds - Elapsed);

	TWeakObjectPtr<UProjectOrganoidFlowManagerSubsystem> WeakThis(this);
	World->GetTimerManager().SetTimer(HideLoadingHandle, [WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->SetLoadingProgress(1.0f, FText::FromString(TEXT("Ready")));
			WeakThis->HideLoadingScreen();
		}
	}, Remaining > KINDA_SMALL_NUMBER ? Remaining : 0.05f, false);
}

void UProjectOrganoidFlowManagerSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	if (!LoadedWorld->IsGameWorld())
	{
		return;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(LoadedWorld, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_MainMenu"), ESearchCase::IgnoreCase))
	{
		HideLoadingScreen();
		EnterTitleState();

		TWeakObjectPtr<UWorld> WeakWorld(LoadedWorld);
		LoadedWorld->GetTimerManager().SetTimerForNextTick([WeakWorld]()
		{
			UWorld* World = WeakWorld.Get();
			if (!World)
			{
				return;
			}
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(It->Get()))
				{
					OrganoidPC->SetPauseMenuAllowed(false);
					OrganoidPC->EnsureTitleMainMenu();
				}
			}
		});
		return;
	}

	if (FlowState == EProjectOrganoidFlowState::Loading
		|| MapName.Equals(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		SetFlowState(EProjectOrganoidFlowState::Gameplay);

		const bool bWrongGameMode = LoadedWorld->GetAuthGameMode()
			&& LoadedWorld->GetAuthGameMode()->IsA(AProjectOrganoidMainMenuGameMode::StaticClass());

		TWeakObjectPtr<UProjectOrganoidFlowManagerSubsystem> WeakThis(this);
		LoadedWorld->GetTimerManager().SetTimerForNextTick([WeakThis, bWrongGameMode]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->ApplyGameplayHandoff(bWrongGameMode);
				if (bWrongGameMode)
				{
					WeakThis->HideLoadingScreen();
				}
			}
		});
	}
}

bool UProjectOrganoidFlowManagerSubsystem::RequestSectorTransition(EProjectOrganoidSubLevelTag TargetTag, bool bTeleportToDestination)
{
	UWorld* World = GetWorld();
	if (!World || TargetTag == EProjectOrganoidSubLevelTag::None)
	{
		return false;
	}

	UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>();
	if (!Levels)
	{
		return false;
	}

	FProjectOrganoidSubLevelDefinition Def;
	if (!Levels->GetSubLevelDefinition(TargetTag, Def))
	{
		return false;
	}

	AProjectOrganoidCharacter* Avery = Cast<AProjectOrganoidCharacter>(UGameplayStatics::GetPlayerPawn(World, 0));
	SetFlowState(EProjectOrganoidFlowState::SectorTransition);
	OnSectorTravelRequested.Broadcast(TargetTag);
	ShowLoadingScreen(FText::FromString(TEXT("Traversing sector airlocks...")));
	SetLoadingProgress(0.4f, FText::FromName(Def.StreamingLevelName));

	const bool bOk = Levels->RequestDebugWarpToRegion(
		Avery,
		TargetTag,
		bTeleportToDestination,
		FTransform::Identity);

	if (bOk)
	{
		SetLoadingProgress(1.0f, FText::FromString(TEXT("Sector online")));
		HideLoadingScreen();
		SetFlowState(EProjectOrganoidFlowState::Gameplay);
	}
	else
	{
		HideLoadingScreen();
		SetFlowState(EProjectOrganoidFlowState::Gameplay);
	}

	return bOk;
}
