// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidTerminal.h"
#include "ProjectOrganoidHackingWidget.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidSecurityGate.h"
#include "ProjectOrganoidDoorLock.h"
#include "ProjectOrganoidSecuritySubsystem.h"
#include "ProjectOrganoidSecurityTypes.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidTerminal::AProjectOrganoidTerminal()
{
	InteractionPrompt = FText::FromString(TEXT("Hack Terminal"));

	TerminalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TerminalMesh"));
	TerminalMesh->SetupAttachment(InteractionSphere);
	TerminalMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	HackingWidgetClass = UProjectOrganoidHackingWidget::StaticClass();
}

void AProjectOrganoidTerminal::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->RegisterTerminal(this);
			HandlePowerStateChanged(Power->GetSectorPowerState(PowerSector), EProjectOrganoidPowerState::Online);
		}
	}
}

void AProjectOrganoidTerminal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->UnregisterTerminal(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool AProjectOrganoidTerminal::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (bSingleUse && bHasBeenHacked)
	{
		return false;
	}

	return ActiveHackingWidget == nullptr;
}

bool AProjectOrganoidTerminal::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	ActiveHacker = Interactor;
	OnTerminalOpened.Broadcast(this, Interactor);
	BP_OnTerminalOpened(Interactor);
	OpenHackingUI(Interactor);
	return true;
}

UProjectOrganoidHackingWidget* AProjectOrganoidTerminal::OpenHackingUI(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || !HackingWidgetClass)
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!PC)
	{
		PC = UGameplayStatics::GetPlayerController(this, 0);
	}
	if (!PC)
	{
		return nullptr;
	}

	CloseHackingUI();

	ActiveHackingWidget = CreateWidget<UProjectOrganoidHackingWidget>(PC, HackingWidgetClass);
	if (!ActiveHackingWidget)
	{
		return nullptr;
	}

	ActiveHackingWidget->AddToViewport(50);
	ActiveHackingWidget->BindToTerminal(this, Interactor, HackingConfig);
	SetInputModeForUI(Interactor, true);
	return ActiveHackingWidget;
}

void AProjectOrganoidTerminal::CloseHackingUI()
{
	if (ActiveHackingWidget)
	{
		UProjectOrganoidHackingWidget* Widget = ActiveHackingWidget;
		ActiveHackingWidget = nullptr;
		Widget->CloseHackingUI();
	}

	if (AProjectOrganoidCharacter* Character = ActiveHacker.Get())
	{
		SetInputModeForUI(Character, false);
	}
}

void AProjectOrganoidTerminal::NotifyHackingFinished(bool bSucceeded)
{
	AProjectOrganoidCharacter* Character = ActiveHacker.Get();
	OnHackFinished.Broadcast(this, bSucceeded);

	if (bSucceeded)
	{
		ApplyHackRewards(Character);
		BP_OnHackSucceeded(Character);
	}
	else
	{
		BP_OnHackFailed(Character);
	}

	CloseHackingUI();
	ActiveHacker.Reset();
}

void AProjectOrganoidTerminal::ApplyHackRewards(AProjectOrganoidCharacter* Character)
{
	if (bHasBeenHacked && bSingleUse)
	{
		return;
	}

	bHasBeenHacked = true;
	UnlockLinkedSecurity();

	if (bGrantRewardLogOnSuccess)
	{
		GrantRewardLog(Character);
	}

	NotifyObjectiveEvent(SuccessObjectiveEventId);

	if (bSingleUse)
	{
		bIsInteractable = false;
		InteractionPrompt = FText::FromString(TEXT("Terminal Compromised"));
	}
}

void AProjectOrganoidTerminal::UnlockLinkedSecurity()
{
	if (AProjectOrganoidDoorLock* Door = LinkedDoorLock.Get())
	{
		Door->SetLocked(false);
		Door->SetOpen(true);
		BP_OnDoorUnlockedByTerminal(Door);
	}
	else if (!LinkedDoorLock.IsNull())
	{
		if (AProjectOrganoidDoorLock* LoadedDoor = LinkedDoorLock.LoadSynchronous())
		{
			LoadedDoor->SetLocked(false);
			LoadedDoor->SetOpen(true);
			BP_OnDoorUnlockedByTerminal(LoadedDoor);
		}
	}

	AProjectOrganoidSecurityGate* Gate = LinkedSecurityGate.Get();
	if (!Gate && !LinkedSecurityGate.IsNull())
	{
		Gate = LinkedSecurityGate.LoadSynchronous();
	}

	if (!Gate && !LinkedSecurityGateId.IsNone())
	{
		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
			{
				Gate = Security->FindGateById(LinkedSecurityGateId);
			}
		}
	}

	if (Gate)
	{
		Gate->SetGateState(EProjectOrganoidSecurityGateState::Open, EProjectOrganoidSecurityOverrideMethod::TerminalHack);
		BP_OnGateUnlockedByTerminal(Gate);

		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
			{
				Security->NotifyGateOverrideAttempt(Gate, ActiveHacker.Get(), true);
			}
		}
	}
}

void AProjectOrganoidTerminal::GrantRewardLog(AProjectOrganoidCharacter* Character)
{
	if (!Character || RewardLogEntry.EntryId.IsNone())
	{
		return;
	}

	if (UProjectOrganoidLogComponent* Logs = Character->GetLogComponent())
	{
		FProjectOrganoidLogEntry EntryCopy = RewardLogEntry;
		Logs->CollectLogEntry(EntryCopy);
		Logs->MarkEntryRead(EntryCopy.EntryId);
	}
}

void AProjectOrganoidTerminal::NotifyObjectiveEvent(FName EventId) const
{
	if (EventId.IsNone())
	{
		return;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(EventId);
		}
	}
}

void AProjectOrganoidTerminal::SetInputModeForUI(AProjectOrganoidCharacter* Character, bool bUIOnly)
{
	if (!Character)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	if (bUIOnly)
	{
		FInputModeUIOnly InputMode;
		if (ActiveHackingWidget)
		{
			InputMode.SetWidgetToFocus(ActiveHackingWidget->TakeWidget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = true;
		PC->SetPause(false);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}
}

void AProjectOrganoidTerminal::HandlePowerStateChanged(EProjectOrganoidPowerState NewState, EProjectOrganoidPowerState PreviousState)
{
	if (bDisableDuringBlackout && !(bSingleUse && bHasBeenHacked))
	{
		bIsInteractable = NewState != EProjectOrganoidPowerState::Blackout;
		InteractionPrompt = bIsInteractable
			? FText::FromString(TEXT("Hack Terminal"))
			: FText::FromString(TEXT("Terminal Offline"));
	}

	BP_OnPowerStateChanged(NewState);
}
