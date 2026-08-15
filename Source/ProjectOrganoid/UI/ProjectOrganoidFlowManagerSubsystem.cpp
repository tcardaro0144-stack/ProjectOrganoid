// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "ProjectOrganoidLoadingScreenWidget.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidLevelTypes.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

void UProjectOrganoidFlowManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FlowState = EProjectOrganoidFlowState::Boot;
}

void UProjectOrganoidFlowManagerSubsystem::Deinitialize()
{
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

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return;
	}

	TSubclassOf<UProjectOrganoidLoadingScreenWidget> ClassToSpawn = LoadingScreenClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidLoadingScreenWidget::StaticClass();
	}

	ActiveLoadingScreen = CreateWidget<UProjectOrganoidLoadingScreenWidget>(PC, ClassToSpawn);
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

void UProjectOrganoidFlowManagerSubsystem::TravelToGameplayLevel(FName LevelName)
{
	const FName Target = LevelName.IsNone() ? GameplayLevelName : LevelName;
	PendingGameplayLevel = Target;
	SetFlowState(EProjectOrganoidFlowState::Loading);
	ShowLoadingScreen(FText::FromString(TEXT("Initializing Epitope lockdown...")));
	SetLoadingProgress(0.35f, FText::FromString(TEXT("Streaming facility sectors...")));
	LoadingStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	UGameplayStatics::OpenLevel(this, Target);
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
	ShowLoadingScreen(FText::FromString(TEXT("Returning to title...")));
	UGameplayStatics::OpenLevel(this, TitleLevelName);
	SetFlowState(EProjectOrganoidFlowState::Title);
}

void UProjectOrganoidFlowManagerSubsystem::NotifyGameplayMapReady(AProjectOrganoidGameMode* /*GameMode*/)
{
	SetFlowState(EProjectOrganoidFlowState::Gameplay);
	SetLoadingProgress(0.9f, FText::FromString(TEXT("Synchronizing Avery's suit telemetry...")));

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
	TArray<FName> Unload;
	SetFlowState(EProjectOrganoidFlowState::SectorTransition);
	OnSectorTravelRequested.Broadcast(TargetTag);
	ShowLoadingScreen(FText::FromString(TEXT("Traversing sector airlocks...")));
	SetLoadingProgress(0.4f, FText::FromName(Def.StreamingLevelName));

	const bool bOk = Levels->RequestSubLevelTransition(
		Avery,
		TargetTag,
		Def.StreamingLevelName,
		Unload,
		true,
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
