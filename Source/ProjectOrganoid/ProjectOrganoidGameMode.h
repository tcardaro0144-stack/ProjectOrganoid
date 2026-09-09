// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "ProjectOrganoidGameMode.generated.h"

class UProjectOrganoidHUDWidget;
class UProjectOrganoidGameplayHUDController;
class APlayerController;

/**
 *  ProjectOrganoid GameMode — Avery Vance pawn, Organoid player controller, diegetic HUD.
 */
UCLASS()
class AProjectOrganoidGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AProjectOrganoidGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	/** Spawn / possess Avery if travel left the controller without a gameplay pawn. */
	void EnsurePossessedGameplayPawn(APlayerController* PlayerController);

	/** HUD widget class spawned for local players (defaults to UProjectOrganoidHUDWidget) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UProjectOrganoidHUDWidget> HUDWidgetClass;

	UFUNCTION(BlueprintPure, Category = "UI")
	UProjectOrganoidGameplayHUDController* GetHUDControllerForPlayer(APlayerController* PlayerController) const;

protected:

	/** Active HUD widgets keyed by player controller */
	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, TObjectPtr<UProjectOrganoidHUDWidget>> PlayerHUDWidgets;

	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, TObjectPtr<UProjectOrganoidGameplayHUDController>> PlayerHUDControllers;

	void SpawnHUDForPlayer(APlayerController* PlayerController);
	void TryBindHUDToPawn(APlayerController* PlayerController);
	void TryStartEpitopePlay();
	bool IsEpitopeMap() const;

	FTimerHandle EpitopeReadyTimerHandle;
	int32 EpitopeReadyAttempts = 0;
};
