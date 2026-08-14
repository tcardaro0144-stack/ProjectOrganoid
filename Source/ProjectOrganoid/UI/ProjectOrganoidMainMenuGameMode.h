// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ProjectOrganoidMainMenuGameMode.generated.h"

class UProjectOrganoidMainMenuWidget;
class APlayerController;

/**
 *  Title-screen GameMode — no Avery pawn / vitals HUD; spawns the main menu widget.
 */
UCLASS()
class AProjectOrganoidMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	AProjectOrganoidMainMenuGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UProjectOrganoidMainMenuWidget> MainMenuWidgetClass;

protected:

	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, TObjectPtr<UProjectOrganoidMainMenuWidget>> PlayerMenuWidgets;

	void SpawnMainMenuForPlayer(APlayerController* PlayerController);
};
