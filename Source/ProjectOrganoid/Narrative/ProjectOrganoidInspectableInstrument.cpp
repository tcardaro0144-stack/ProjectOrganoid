// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidInspectableInstrument::AProjectOrganoidInspectableInstrument()
{
	InteractionPrompt = InspectionPrompt;
	InteractionRange = 220.0f;

	// Temporary presentation hierarchy. Static mesh assets / transforms are editor-authored later.
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetupAttachment(InteractionSphere.Get());

	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	PedestalMesh->SetupAttachment(SceneRoot);
	ConfigurePresentationMesh(PedestalMesh);

	ColumnMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ColumnMesh"));
	ColumnMesh->SetupAttachment(SceneRoot);
	ConfigurePresentationMesh(ColumnMesh);

	ArrayHeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrayHeadMesh"));
	ArrayHeadMesh->SetupAttachment(SceneRoot);
	ConfigurePresentationMesh(ArrayHeadMesh);
}

void AProjectOrganoidInspectableInstrument::ConfigurePresentationMesh(UStaticMeshComponent* Mesh) const
{
	if (!Mesh)
	{
		return;
	}

	// No Engine mesh assignment — assets stay optional for later authoring.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
}

void AProjectOrganoidInspectableInstrument::BeginPlay()
{
	Super::BeginPlay();
	RefreshPrompt();
}

bool AProjectOrganoidInspectableInstrument::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}

	// Review always available once the guarded objective is Completed.
	if (IsGuardedObjectiveCompleted())
	{
		return true;
	}

	// Optional first-time gate: require Active objective when configured.
	if (!RequiredActiveObjectiveId.IsNone() && !IsRequiredObjectiveActive())
	{
		return false;
	}

	return true;
}

bool AProjectOrganoidInspectableInstrument::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	if (IsGuardedObjectiveCompleted())
	{
		if (TryFollowupConnection(Interactor))
		{
			RefreshPrompt();
			BP_OnInspected(Interactor, true);
			return true;
		}

		RefreshPrompt();
		BP_OnInspected(Interactor, false);
		return true;
	}

	// Reject first-time path without marking inspected or firing events.
	if (!RequiredActiveObjectiveId.IsNone() && !IsRequiredObjectiveActive())
	{
		return false;
	}

	UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectiveSubsystem();
	if (!Objectives || ObjectiveEventId.IsNone() || CompletedObjectiveIdForReplayGuard.IsNone())
	{
		return true;
	}

	Objectives->TriggerEvent(ObjectiveEventId);
	++ObjectiveEventFireCount;

	if (!IsGuardedObjectiveCompleted())
	{
		// Failure: leave inspect/review state untouched.
		return true;
	}

	bHasBeenInspected = true;
	PresentInspectionNotification(Interactor);
	RefreshPrompt();
	BP_OnInspected(Interactor, true);
	return true;
}

void AProjectOrganoidInspectableInstrument::RefreshPrompt()
{
	if (IsGuardedObjectiveCompleted() || bHasBeenInspected)
	{
		InteractionPrompt = ReviewPrompt.IsEmpty()
			? FText::FromString(TEXT("Review Neural Mapping Array"))
			: ReviewPrompt;
	}
	else
	{
		InteractionPrompt = InspectionPrompt.IsEmpty()
			? FText::FromString(TEXT("Inspect Neural Mapping Array"))
			: InspectionPrompt;
	}
	bIsInteractable = true;
}

UProjectOrganoidObjectiveSubsystem* AProjectOrganoidInspectableInstrument::GetObjectiveSubsystem() const
{
	if (const UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
	}
	return nullptr;
}

bool AProjectOrganoidInspectableInstrument::IsGuardedObjectiveCompleted() const
{
	if (CompletedObjectiveIdForReplayGuard.IsNone())
	{
		return false;
	}

	const UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectiveSubsystem();
	if (!Objectives)
	{
		return false;
	}

	FProjectOrganoidObjective Objective;
	if (!Objectives->GetObjective(CompletedObjectiveIdForReplayGuard, Objective))
	{
		return false;
	}

	return Objective.State == EProjectOrganoidObjectiveState::Completed;
}

bool AProjectOrganoidInspectableInstrument::IsRequiredObjectiveActive() const
{
	if (RequiredActiveObjectiveId.IsNone())
	{
		return true;
	}

	const UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectiveSubsystem();
	if (!Objectives)
	{
		return false;
	}

	FProjectOrganoidObjective Objective;
	if (!Objectives->GetObjective(RequiredActiveObjectiveId, Objective))
	{
		return false;
	}

	return Objective.State == EProjectOrganoidObjectiveState::Active;
}

bool AProjectOrganoidInspectableInstrument::IsFollowupConnectionConfigured() const
{
	return !FollowupRequiredActiveObjectiveId.IsNone()
		&& !FollowupPrerequisiteCompletedObjectiveId.IsNone()
		&& !FollowupSuccessEventId.IsNone();
}

bool AProjectOrganoidInspectableInstrument::IsNamedObjectiveInState(
	FName ObjectiveId,
	EProjectOrganoidObjectiveState State) const
{
	if (ObjectiveId.IsNone())
	{
		return false;
	}

	const UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectiveSubsystem();
	if (!Objectives)
	{
		return false;
	}

	FProjectOrganoidObjective Objective;
	if (!Objectives->GetObjective(ObjectiveId, Objective))
	{
		return false;
	}

	return Objective.State == State;
}

bool AProjectOrganoidInspectableInstrument::TryFollowupConnection(AProjectOrganoidCharacter* Interactor)
{
	if (!IsFollowupConnectionConfigured())
	{
		return false;
	}

	if (FollowupEventFireCount > 0
		|| IsNamedObjectiveInState(FollowupRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Completed))
	{
		return false;
	}

	if (!IsNamedObjectiveInState(FollowupRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Active)
		|| !IsNamedObjectiveInState(
			FollowupPrerequisiteCompletedObjectiveId,
			EProjectOrganoidObjectiveState::Completed))
	{
		return false;
	}

	UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectiveSubsystem();
	if (!Objectives)
	{
		return false;
	}

	Objectives->TriggerEvent(FollowupSuccessEventId);
	++FollowupEventFireCount;

	if (IsNamedObjectiveInState(FollowupRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Completed))
	{
		PresentFollowupNotification(Interactor);
	}

	return true;
}

void AProjectOrganoidInspectableInstrument::PresentInspectionNotification(AProjectOrganoidCharacter* Interactor)
{
	APlayerController* PC = Interactor ? Cast<APlayerController>(Interactor->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AProjectOrganoidGameMode* GameMode = World->GetAuthGameMode<AProjectOrganoidGameMode>();
	if (!GameMode)
	{
		return;
	}

	UProjectOrganoidGameplayHUDController* HUDController = GameMode->GetHUDControllerForPlayer(PC);
	if (!HUDController)
	{
		return;
	}

	if (HUDController->ShowTransientNotification(SpeakerLabel, InspectionResponseText, NotificationDurationSeconds))
	{
		++InspectionNotificationCount;
	}
}

void AProjectOrganoidInspectableInstrument::PresentFollowupNotification(AProjectOrganoidCharacter* Interactor)
{
	if (FollowupResponseText.IsEmpty() || FollowupNotificationDurationSeconds <= 0.0f)
	{
		return;
	}

	APlayerController* PC = Interactor ? Cast<APlayerController>(Interactor->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	UWorld* World = GetWorld();
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUDController = GameMode ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (!HUDController)
	{
		return;
	}

	const FText Speaker = FollowupSpeakerLabel.IsEmpty()
		? FText::GetEmpty()
		: FollowupSpeakerLabel;
	if (HUDController->ShowTransientNotification(Speaker, FollowupResponseText, FollowupNotificationDurationSeconds))
	{
		++FollowupNotificationCount;
	}
}
