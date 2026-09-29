// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCreditsRoll.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"

AProjectOrganoidCreditsRoll::AProjectOrganoidCreditsRoll()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

FString AProjectOrganoidCreditsRoll::BuildCreditsBody() const
{
	return FString(
		TEXT("Tom Cardaro\n")
		TEXT("Project Organoid\n")
		TEXT("Engine 5.8.3\n")
		TEXT("34 hashes\n")
		TEXT("63/63 COMPLETE_PASS 5423\n")
		TEXT("Beats 19-33\n")
		TEXT("Thank you"));
}

bool AProjectOrganoidCreditsRoll::PlayCredits(EProjectOrganoidNodeZeroFate Fate)
{
	if (Fate != EProjectOrganoidNodeZeroFate::Destroy && Fate != EProjectOrganoidNodeZeroFate::Extract)
	{
		return false;
	}
	LastPlayedFate = Fate;
	++PlayCount;
	LastCreditsText = BuildCreditsBody();
	bShowedCarrierLine = false;
	PresentLine(TEXT("Credits"), LastCreditsText);
	if (Fate == EProjectOrganoidNodeZeroFate::Extract)
	{
		PresentLine(TEXT("Sterling"), TEXT("You take it, you become the carrier."));
		bShowedCarrierLine = true;
	}
	else
	{
		PresentLine(TEXT("Sterling"), TEXT("Core overload. Lights die with the basin. Run for the pod."));
	}
	return true;
}

void AProjectOrganoidCreditsRoll::PresentLine(const FString& Speaker, const FString& Line) const
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD)
	{
		HUD->ShowTransientNotification(FText::FromString(Speaker), FText::FromString(Line), LineSeconds);
	}
}
