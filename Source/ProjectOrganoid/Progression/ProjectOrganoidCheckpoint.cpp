// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidCheckpoint::AProjectOrganoidCheckpoint()
{
	InteractionPrompt = FText::FromString(TEXT("Use Checkpoint"));

	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CheckpointMesh"));
	CheckpointMesh->SetupAttachment(InteractionSphere);
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AutosaveVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("AutosaveVolume"));
	AutosaveVolume->SetupAttachment(InteractionSphere);
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
		AutosaveVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidCheckpoint::OnAutosaveVolumeBeginOverlap);
	}

	if (!CheckpointDisplayName.IsEmpty())
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

	const bool bSucceeded = TriggerCheckpointSave(Interactor);
	OnInteracted.Broadcast(this, Interactor);
	return bSucceeded;
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

	if (bRestoreHealthOnSave)
	{
		const float Missing = Character->GetMaxHealth() - Character->GetHealth();
		if (Missing > KINDA_SMALL_NUMBER)
		{
			Character->ApplyHealthDelta(Missing);
		}
	}

	const bool bSucceeded = SaveSubsystem->SaveAtCheckpoint(Character, this, ResolveSaveSlot());
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
