// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDataPad.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidDataPad::AProjectOrganoidDataPad()
{
	InteractionPrompt = FText::FromString(TEXT("Read Data Pad"));

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	PadMesh->SetupAttachment(InteractionSphere);
	PadMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	LogEntry.EntryId = TEXT("Pad_Unnamed");
	LogEntry.Title = FText::FromString(TEXT("Untitled Facility Log"));
	LogEntry.Body = FText::FromString(TEXT("Corrupted entry."));
	LogEntry.Author = FText::FromString(TEXT("Unknown"));
	LogEntry.Category = TEXT("Facility");
}

bool AProjectOrganoidDataPad::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::Interact_Implementation(Interactor) || !Interactor)
	{
		return false;
	}

	if (UProjectOrganoidLogComponent* LogComponent = Interactor->GetLogComponent())
	{
		FProjectOrganoidLogEntry EntryCopy = LogEntry;
		EntryCopy.bIsRead = true;
		LogComponent->CollectLogEntry(EntryCopy);
		LogComponent->MarkEntryRead(EntryCopy.EntryId);
	}

	const bool bFirstRead = !bHasBeenRead;
	bHasBeenRead = true;

	if (bFirstRead && !ObjectiveEventId.IsNone())
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
			{
				Objectives->TriggerEvent(ObjectiveEventId);
			}
		}
	}

	BP_OnDataPadRead(Interactor, LogEntry);
	return true;
}
