// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDataPad.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
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

FText AProjectOrganoidDataPad::GetInteractionPrompt() const
{
	if (WeaponRosterWeaponId.IsNone())
	{
		return Super::GetInteractionPrompt();
	}
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			FProjectOrganoidObjective Objective;
			if (Objectives->GetObjective(TEXT("Obj_RecoverWeaponRoster"), Objective)
				&& Objective.State == EProjectOrganoidObjectiveState::Active)
			{
				return FText::FromString(TEXT("Recover Weapon Roster"));
			}
		}
	}
	return Super::GetInteractionPrompt();
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

	TryAwardWeaponRosterCredit(Interactor);
	BP_OnDataPadRead(Interactor, LogEntry);
	return true;
}

void AProjectOrganoidDataPad::TryAwardWeaponRosterCredit(AProjectOrganoidCharacter* Interactor)
{
	if (WeaponRosterWeaponId.IsNone() || WeaponRosterCreditCount > 0 || !Interactor)
	{
		return;
	}
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
	if (!Objectives)
	{
		return;
	}
	FProjectOrganoidObjective Objective;
	if (!Objectives->GetObjective(TEXT("Obj_RecoverWeaponRoster"), Objective)
		|| Objective.State != EProjectOrganoidObjectiveState::Active)
	{
		return;
	}
	UProjectOrganoidWeaponComponent* Weapons = Interactor->GetWeaponComponent();
	if (!Weapons || !Weapons->UnlockWeaponRoster(WeaponRosterWeaponId))
	{
		return;
	}

	int32 PriorNotifications = 0;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AProjectOrganoidDataPad> It(World); It; ++It)
		{
			PriorNotifications += It->WeaponRosterNotificationCount;
		}
	}

	Objectives->TriggerEvent(TEXT("Event_WeaponRosterRecovered"));
	++WeaponRosterCreditCount;

	if (PriorNotifications == 0)
	{
		APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
		UWorld* World = GetWorld();
		AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
		UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
		if (HUD && HUD->ShowTransientNotification(
			FText::FromString(TEXT("Nathan")),
			FText::FromString(TEXT("Five more. Each with a different job. Lytic was the emergency — these are the toolkit. No unlimited ammo.")),
			7.0f))
		{
			++WeaponRosterNotificationCount;
		}
	}
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
