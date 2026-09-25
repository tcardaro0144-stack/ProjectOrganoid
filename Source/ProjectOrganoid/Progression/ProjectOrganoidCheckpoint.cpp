// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidCheckpoint::AProjectOrganoidCheckpoint()
{
	InteractionPrompt = FText::FromString(TEXT("Use Checkpoint"));

	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMesh"));
	CheckpointMesh->SetupAttachment(InteractionSphere.Get());
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AutosaveVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AutosaveVolume"));
	AutosaveVolume->SetupAttachment(InteractionSphere.Get());
	AutosaveVolume->InitBoxExtent(FVector(120.0f, 120.0f, 100.0f));
	AutosaveVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AutosaveVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	AutosaveVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	AutosaveVolume->SetGenerateOverlapEvents(true);
}

void AProjectOrganoidCheckpoint::BeginPlay()
{
	Super::BeginPlay();

	if (AutosaveVolume)
	{
		AutosaveVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &AProjectOrganoidCheckpoint::OnAutosaveVolumeBeginOverlap);
	}

	if (!CampaignRequiredActiveObjectiveId.IsNone() && !CampaignEntryPrompt.IsEmpty())
	{
		InteractionPrompt = CampaignEntryPrompt;
	}
	else if (!CheckpointDisplayName.IsEmpty())
	{
		InteractionPrompt = FText::Format(NSLOCTEXT("ProjectOrganoid", "CheckpointPrompt", "Save — {0}"), CheckpointDisplayName);
	}
}

bool AProjectOrganoidCheckpoint::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	return Super::CanInteract_Implementation(Interactor) && bSaveOnInteract;
}

bool AProjectOrganoidCheckpoint::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	TryCampaignEntry(Interactor);
	const bool bSucceeded = TriggerCheckpointSave(Interactor);
	OnInteracted.Broadcast(this, Interactor);
	return bSucceeded;
}

void AProjectOrganoidCheckpoint::TryCampaignEntry(AProjectOrganoidCharacter* Character)
{
	if (CampaignRequiredActiveObjectiveId.IsNone() || CampaignSuccessEventId.IsNone() || !Character)
	{
		return;
	}
	if (!IsCampaignObjectiveActive() || IsCampaignObjectiveCompleted())
	{
		return;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(CampaignSuccessEventId);
			++CampaignEventFireCount;
		}
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	UWorld* World = GetWorld();
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD && HUD->ShowTransientNotification(CampaignNotificationSpeaker, CampaignNotificationText, CampaignNotificationDurationSeconds))
	{
		++CampaignNotificationCount;
	}
}

bool AProjectOrganoidCheckpoint::IsCampaignObjectiveActive() const
{
	if (CampaignRequiredActiveObjectiveId.IsNone())
	{
		return false;
	}
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
	if (!Objectives)
	{
		return false;
	}
	FProjectOrganoidObjective Objective;
	return Objectives->GetObjective(CampaignRequiredActiveObjectiveId, Objective)
		&& Objective.State == EProjectOrganoidObjectiveState::Active;
}

bool AProjectOrganoidCheckpoint::IsCampaignObjectiveCompleted() const
{
	if (CampaignRequiredActiveObjectiveId.IsNone())
	{
		return false;
	}
	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
	if (!Objectives)
	{
		return false;
	}
	FProjectOrganoidObjective Objective;
	return Objectives->GetObjective(CampaignRequiredActiveObjectiveId, Objective)
		&& Objective.State == EProjectOrganoidObjectiveState::Completed;
}

bool AProjectOrganoidCheckpoint::TriggerCheckpointSave(AProjectOrganoidCharacter* Character)
{
	if (!Character)
	{
		return false;
	}

	UGameInstance* GI = UGameplayStatics::GetGameInstance(this);
	UProjectOrganoidSaveSubsystem* SaveSubsystem = GI
		? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>()
		: nullptr;

	if (!SaveSubsystem)
	{
		BP_OnCheckpointSaved(Character, false);
		OnCheckpointUsed.Broadcast(this, Character, false);
		return false;
	}

	ApplyHealthStabilizationFloor(Character);

	const FString Slot = ResolveSaveSlot();
	const bool bSucceeded = SaveSubsystem->SaveAtCheckpoint(Character, this, Slot);
	if (bSucceeded)
	{
		Character->NotifyCheckpointActivated(this, Slot);
	}
	BP_OnCheckpointSaved(Character, bSucceeded);
	OnCheckpointUsed.Broadcast(this, Character, bSucceeded);
	return bSucceeded;
}

void AProjectOrganoidCheckpoint::OnAutosaveVolumeBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bSaveOnOverlapEnter)
	{
		return;
	}

	AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now - LastOverlapAutosaveTime < OverlapAutosaveCooldownSeconds)
	{
		return;
	}

	LastOverlapAutosaveTime = Now;
	TriggerCheckpointSave(Character);
}

void AProjectOrganoidCheckpoint::ApplyHealthStabilizationFloor(AProjectOrganoidCharacter* Character) const
{
	if (!Character)
	{
		return;
	}

	const float MaxHealth = Character->GetMaxHealth();
	if (MaxHealth <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float FloorPercent = FMath::Clamp(HealthStabilizationFloorPercent, 0.0f, 1.0f);
	if (FloorPercent <= 0.0f)
	{
		return;
	}

	const float FloorHealth = MaxHealth * FloorPercent;
	const float Current = Character->GetHealth();
	if (Current + KINDA_SMALL_NUMBER >= FloorHealth)
	{
		return;
	}

	Character->ApplyHealthDelta(FloorHealth - Current);
}

FString AProjectOrganoidCheckpoint::ResolveSaveSlot() const
{
	if (!SaveSlotOverride.IsEmpty())
	{
		return SaveSlotOverride;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
		{
			return SaveSubsystem->AutosaveSlotName;
		}
	}

	return TEXT("OrganoidAutosave");
}
