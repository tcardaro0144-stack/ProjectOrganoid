// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidSecurityGate.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidSecuritySubsystem.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidSecurityGate::AProjectOrganoidSecurityGate()
{
	InteractionPrompt = FText::FromString(TEXT("Override Security Gate"));

	GateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GateMesh"));
	GateMesh->SetupAttachment(InteractionSphere);
	GateMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	BarrierVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BarrierVolume"));
	BarrierVolume->SetupAttachment(InteractionSphere);
	BarrierVolume->InitBoxExtent(FVector(50.0f, 150.0f, 140.0f));
	BarrierVolume->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BarrierVolume->SetCollisionResponseToAllChannels(ECR_Block);
	BarrierVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	BarrierVolume->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	RefreshBarrierCollision();
	RefreshInteractionPrompt();
}

void AProjectOrganoidSecurityGate::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
		{
			Security->RegisterSecurityGate(this);
		}

		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->RegisterSecurityGate(this);
			HandlePowerStateChanged(Power->GetSectorPowerState(PowerSector), EProjectOrganoidPowerState::Online);
		}
	}

	RefreshBarrierCollision();
	RefreshInteractionPrompt();
}

void AProjectOrganoidSecurityGate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
		{
			Security->UnregisterSecurityGate(this);
		}

		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->UnregisterSecurityGate(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool AProjectOrganoidSecurityGate::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	// Allow sealed attempts so access-denied feedback can fire.
	return true;
}

bool AProjectOrganoidSecurityGate::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (!IsSealed())
	{
		const EProjectOrganoidSecurityGateState ToggleState =
			(GateState == EProjectOrganoidSecurityGateState::Open)
				? EProjectOrganoidSecurityGateState::Unlocked
				: EProjectOrganoidSecurityGateState::Open;
		SetGateState(ToggleState, EProjectOrganoidSecurityOverrideMethod::Manual);
		OnInteracted.Broadcast(this, Interactor);
		return true;
	}

	const bool bSucceeded = TryOverrideWithInventory(Interactor);
	OnInteracted.Broadcast(this, Interactor);
	return bSucceeded;
}

bool AProjectOrganoidSecurityGate::TryOverrideWithInventory(AProjectOrganoidCharacter* Interactor)
{
	UProjectOrganoidSecuritySubsystem* Security = GetWorld()
		? GetWorld()->GetSubsystem<UProjectOrganoidSecuritySubsystem>()
		: nullptr;

	if (!Interactor)
	{
		if (Security)
		{
			Security->NotifyGateOverrideAttempt(this, nullptr, false);
		}
		return false;
	}

	UProjectOrganoidInventoryComponent* Inventory = Interactor->GetInventoryComponent();
	if (!Inventory)
	{
		BP_OnAccessDenied(Interactor, RequiredSecurityTier);
		if (Security)
		{
			Security->NotifyGateOverrideAttempt(this, Interactor, false);
		}
		return false;
	}

	EProjectOrganoidSecurityOverrideMethod Method = EProjectOrganoidSecurityOverrideMethod::None;

	const bool bHasKeycard = bAllowKeycardOverride && Inventory->HasKeycardOfTier(RequiredSecurityTier);
	const bool bHasHackTool = bAllowHackingToolOverride && Inventory->HasSecurityOverrideTool(RequiredSecurityTier);

	if (bHasKeycard)
	{
		Method = EProjectOrganoidSecurityOverrideMethod::Keycard;
		if (bConsumeKeycardOnOverride)
		{
			Inventory->ConsumeKeycardOfTier(RequiredSecurityTier);
		}
	}
	else if (bHasHackTool)
	{
		Method = EProjectOrganoidSecurityOverrideMethod::HackingTool;
		if (bConsumeHackingToolOnOverride)
		{
			Inventory->ConsumeSecurityOverrideTool(RequiredSecurityTier);
		}
	}
	else
	{
		BP_OnAccessDenied(Interactor, RequiredSecurityTier);
		if (Security)
		{
			Security->NotifyGateOverrideAttempt(this, Interactor, false);
		}
		return false;
	}

	SetGateState(EProjectOrganoidSecurityGateState::Open, Method);
	BP_OnGateOpened(Interactor, Method);
	NotifyObjectiveEvent(TEXT("Event_SecurityGateOpened"));

	if (Security)
	{
		Security->NotifyGateOverrideAttempt(this, Interactor, true);
	}

	return true;
}

void AProjectOrganoidSecurityGate::SetGateState(EProjectOrganoidSecurityGateState NewState, EProjectOrganoidSecurityOverrideMethod Method)
{
	if (GateState == NewState)
	{
		RefreshBarrierCollision();
		RefreshInteractionPrompt();
		return;
	}

	GateState = NewState;
	RefreshBarrierCollision();
	RefreshInteractionPrompt();

	OnGateStateChanged.Broadcast(this, GateState, Method);
	BP_OnGateStateChanged(GateState, Method);

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
		{
			Security->NotifyGateStateChanged(this, GateState, Method);
		}
	}
}

void AProjectOrganoidSecurityGate::ApplyFacilityLockdown()
{
	SetGateState(EProjectOrganoidSecurityGateState::Sealed, EProjectOrganoidSecurityOverrideMethod::SubsystemOverride);
}

void AProjectOrganoidSecurityGate::ClearFacilityLockdownSeal(bool bUnlock)
{
	if (bUnlock)
	{
		SetGateState(EProjectOrganoidSecurityGateState::Unlocked, EProjectOrganoidSecurityOverrideMethod::SubsystemOverride);
	}
}

void AProjectOrganoidSecurityGate::RefreshBarrierCollision()
{
	if (!BarrierVolume)
	{
		return;
	}

	const bool bBlock = IsSealed();
	BarrierVolume->SetCollisionEnabled(bBlock ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BarrierVolume->SetHiddenInGame(!bBlock);

	if (GateMesh)
	{
		GateMesh->SetVisibility(bBlock || GateState != EProjectOrganoidSecurityGateState::Open, true);
	}
}

void AProjectOrganoidSecurityGate::RefreshInteractionPrompt()
{
	switch (GateState)
	{
	case EProjectOrganoidSecurityGateState::Sealed:
		InteractionPrompt = FText::FromString(TEXT("Override Security Gate"));
		break;
	case EProjectOrganoidSecurityGateState::Unlocked:
		InteractionPrompt = FText::FromString(TEXT("Open Security Gate"));
		break;
	case EProjectOrganoidSecurityGateState::Open:
		InteractionPrompt = FText::FromString(TEXT("Close Security Gate"));
		break;
	default:
		break;
	}
}

void AProjectOrganoidSecurityGate::NotifyObjectiveEvent(FName EventId) const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(EventId);
		}
	}
}

void AProjectOrganoidSecurityGate::HandlePowerStateChanged(EProjectOrganoidPowerState NewState, EProjectOrganoidPowerState PreviousState)
{
	if (bFailOpenOnBlackout && BarrierVolume)
	{
		if (NewState == EProjectOrganoidPowerState::Blackout)
		{
			BarrierVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			bIsInteractable = false;
			InteractionPrompt = FText::FromString(TEXT("Gate Maglock Offline"));
		}
		else
		{
			RefreshBarrierCollision();
			bIsInteractable = true;
			RefreshInteractionPrompt();
		}
	}

	BP_OnPowerStateChanged(NewState);
}
