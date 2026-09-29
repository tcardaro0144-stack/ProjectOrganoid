// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSterlingEscapeCinematic.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidNodeZeroCore.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AProjectOrganoidSterlingEscapeCinematic::AProjectOrganoidSterlingEscapeCinematic()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

bool AProjectOrganoidSterlingEscapeCinematic::PlayEscapeSequence(EProjectOrganoidNodeZeroFate Fate)
{
	if (Fate != EProjectOrganoidNodeZeroFate::Destroy && Fate != EProjectOrganoidNodeZeroFate::Extract)
	{
		return false;
	}
	LastPlayedFate = Fate;
	++PlayCount;
	if (Fate == EProjectOrganoidNodeZeroFate::Destroy)
	{
		PresentLine(TEXT("Sterling"), TEXT("Core overload. Lights die with the basin. Run for the pod."));
	}
	else
	{
		PresentLine(TEXT("Sterling"), TEXT("The heart is yours. Next loop, they remember you."));
	}
	return true;
}

void AProjectOrganoidSterlingEscapeCinematic::PresentLine(const FString& Speaker, const FString& Line) const
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

AProjectOrganoidSterlingEscapeTrigger::AProjectOrganoidSterlingEscapeTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(120.f, 120.f, 160.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetCanEverAffectNavigation(false);
}

void AProjectOrganoidSterlingEscapeTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	NotifyPlayerOverlap(OtherActor);
}

void AProjectOrganoidSterlingEscapeTrigger::NotifyPlayerOverlap(AActor* OtherActor)
{
	if (bConsumed || !Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	AProjectOrganoidNodeZeroCore* Core = nullptr;
	for (TActorIterator<AProjectOrganoidNodeZeroCore> It(World); It; ++It)
	{
		Core = *It;
		break;
	}
	AProjectOrganoidSterlingEscapeCinematic* Cinematic = nullptr;
	for (TActorIterator<AProjectOrganoidSterlingEscapeCinematic> It(World); It; ++It)
	{
		Cinematic = *It;
		break;
	}
	if (!Core || !Cinematic || Core->GetChosenFate() == EProjectOrganoidNodeZeroFate::None)
	{
		return;
	}
	if (Cinematic->PlayEscapeSequence(Core->GetChosenFate()))
	{
		bConsumed = true;
	}
}
