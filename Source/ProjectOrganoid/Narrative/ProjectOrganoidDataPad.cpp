// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDataPad.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
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

bool AProjectOrganoidDataPad::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (RequiredObjectiveIdForInteraction.IsNone())
	{
		return true;
	}

	const UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	if (!GI)
	{
		return false;
	}

	const UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
	if (!Objectives)
	{
		return false;
	}

	FProjectOrganoidObjective Objective;
	if (!Objectives->GetObjective(RequiredObjectiveIdForInteraction, Objective))
	{
		return false;
	}

	return Objective.State == EProjectOrganoidObjectiveState::Active
		|| Objective.State == EProjectOrganoidObjectiveState::Completed;
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

	if (bFirstRead)
	{
		if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
		{
			if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
			{
				const bool bAlreadyComplete = IsRequiredObjectiveCompleted(Objectives);
				if (bBroadcastGenericDataPadEvent)
				{
					Objectives->TriggerEvent(TEXT("Event_DataPadRead"));
				}

				if (!ObjectiveEventId.IsNone())
				{
					Objectives->TriggerEvent(ObjectiveEventId);
				}

				if (!bAlreadyComplete && IsRequiredObjectiveCompleted(Objectives))
				{
					PresentCompletionNotification(Interactor);
				}
			}
		}
	}

	BP_OnDataPadRead(Interactor, LogEntry);
	return true;
}

bool AProjectOrganoidDataPad::IsRequiredObjectiveCompleted(const UProjectOrganoidObjectiveSubsystem* Objectives) const
{
	if (RequiredObjectiveIdForInteraction.IsNone() || !Objectives)
	{
		return false;
	}
	FProjectOrganoidObjective Objective;
	return Objectives->GetObjective(RequiredObjectiveIdForInteraction, Objective)
		&& Objective.State == EProjectOrganoidObjectiveState::Completed;
}

void AProjectOrganoidDataPad::PresentCompletionNotification(AProjectOrganoidCharacter* Interactor)
{
	if (CompletionNotificationText.IsEmpty() || CompletionNotificationCount > 0)
	{
		return;
	}
	APlayerController* PC = Interactor ? Cast<APlayerController>(Interactor->GetController()) : nullptr;
	UWorld* World = GetWorld();
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD && HUD->ShowTransientNotification(CompletionNotificationSpeaker, CompletionNotificationText, CompletionNotificationDurationSeconds))
	{
		++CompletionNotificationCount;
	}
}
