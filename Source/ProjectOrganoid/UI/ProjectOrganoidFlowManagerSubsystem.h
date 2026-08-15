// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidFlowManagerSubsystem.generated.h"

class UUserWidget;
class UProjectOrganoidLoadingScreenWidget;
class AProjectOrganoidGameMode;

UENUM(BlueprintType)
enum class EProjectOrganoidFlowState : uint8
{
	Boot UMETA(DisplayName = "Boot"),
	Title UMETA(DisplayName = "Title"),
	Loading UMETA(DisplayName = "Loading"),
	Gameplay UMETA(DisplayName = "Gameplay"),
	SectorTransition UMETA(DisplayName = "Sector Transition")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidFlowStateChanged, EProjectOrganoidFlowState, NewState, EProjectOrganoidFlowState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidLoadingProgress, float, Progress01, const FText&, StatusText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidSectorTravelRequested, EProjectOrganoidSubLevelTag, TargetTag);

/**
 *  Title → loading → gameplay → sector streaming orchestrator.
 */
UCLASS()
class UProjectOrganoidFlowManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Maps")
	FName TitleLevelName = FName(TEXT("/Game/Maps/Lvl_MainMenu"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|Maps")
	FName GameplayLevelName = FName(TEXT("/Game/ThirdPerson/Lvl_ThirdPerson"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow|UI")
	TSubclassOf<UProjectOrganoidLoadingScreenWidget> LoadingScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flow", meta = (ClampMin = "0.0"))
	float MinimumLoadingSeconds = 0.75f;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnProjectOrganoidFlowStateChanged OnFlowStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnProjectOrganoidLoadingProgress OnLoadingProgress;

	UPROPERTY(BlueprintAssignable, Category = "Flow")
	FOnProjectOrganoidSectorTravelRequested OnSectorTravelRequested;

	UFUNCTION(BlueprintPure, Category = "Flow")
	EProjectOrganoidFlowState GetFlowState() const { return FlowState; }

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void EnterTitleState();

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void StartNewGame(FName OverrideGameplayLevel = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void LoadGameAndTravel(const FString& SlotName, FName OverrideGameplayLevel = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void ReturnToTitle();

	/** Called by gameplay GameMode once the world is ready */
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void NotifyGameplayMapReady(AProjectOrganoidGameMode* GameMode);

	UFUNCTION(BlueprintCallable, Category = "Flow|Sector")
	bool RequestSectorTransition(EProjectOrganoidSubLevelTag TargetTag, bool bTeleportToDestination = true);

	UFUNCTION(BlueprintCallable, Category = "Flow|UI")
	void SetLoadingProgress(float Progress01, const FText& StatusText);

protected:

	EProjectOrganoidFlowState FlowState = EProjectOrganoidFlowState::Boot;

	UPROPERTY()
	TObjectPtr<UProjectOrganoidLoadingScreenWidget> ActiveLoadingScreen;

	FString PendingLoadSlot;
	FName PendingGameplayLevel = NAME_None;
	float LoadingStartTime = 0.0f;
	FTimerHandle HideLoadingHandle;

	void SetFlowState(EProjectOrganoidFlowState NewState);
	void ShowLoadingScreen(const FText& InitialStatus);
	void HideLoadingScreen();
	void TravelToGameplayLevel(FName LevelName);
};
