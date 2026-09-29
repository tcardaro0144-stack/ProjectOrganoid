// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidConclusionChoiceTrigger.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCreditsRoll.h"
#include "ProjectOrganoidNodeZeroCore.h"
#include "ProjectOrganoidSaveCleanup.h"

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AProjectOrganoidConclusionChoiceTrigger::AProjectOrganoidConclusionChoiceTrigger()
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

void AProjectOrganoidConclusionChoiceTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	NotifyPlayerOverlap(OtherActor);
}

void AProjectOrganoidConclusionChoiceTrigger::NotifyPlayerOverlap(AActor* OtherActor)
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
		if (It->GetChosenFate() != EProjectOrganoidNodeZeroFate::None)
		{
			Core = *It;
			break;
		}
	}
	if (!Core)
	{
		return;
	}
	ResolveConclusionChoice(Core->GetChosenFate());
}

bool AProjectOrganoidConclusionChoiceTrigger::ResolveConclusionChoice(EProjectOrganoidNodeZeroFate Fate)
{
	if (bConsumed || (Fate != EProjectOrganoidNodeZeroFate::Destroy && Fate != EProjectOrganoidNodeZeroFate::Extract))
	{
		return false;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	AProjectOrganoidCreditsRoll* Credits = nullptr;
	AProjectOrganoidSaveCleanup* Cleanup = nullptr;
	for (TActorIterator<AProjectOrganoidCreditsRoll> It(World); It; ++It)
	{
		Credits = *It;
		break;
	}
	for (TActorIterator<AProjectOrganoidSaveCleanup> It(World); It; ++It)
	{
		Cleanup = *It;
		break;
	}
	if (!Credits || !Cleanup)
	{
		return false;
	}
	if (!Credits->PlayCredits(Fate) || !Cleanup->ApplyCleanup(Fate))
	{
		return false;
	}
	ResolvedFate = Fate;
	bConsumed = true;
	return true;
}
