// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectOrganoidGameMode.generated.h"

class UProjectOrganoidHUDWidget;
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

	/** HUD widget class spawned for local players (defaults to UProjectOrganoidHUDWidget) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UProjectOrganoidHUDWidget> HUDWidgetClass;

protected:

	/** Active HUD widgets keyed by player controller */
	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, TObjectPtr<UProjectOrganoidHUDWidget>> PlayerHUDWidgets;

	void SpawnHUDForPlayer(APlayerController* PlayerController);
	void TryBindHUDToPawn(APlayerController* PlayerController);
};
