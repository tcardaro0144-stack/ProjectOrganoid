// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDialogueNPC.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidDialogueSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AProjectOrganoidDialogueNPC::AProjectOrganoidDialogueNPC()
{
	InteractionPrompt = FText::FromString(TEXT("Talk"));
	InteractionRange = 220.0f;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(InteractionSphere);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void AProjectOrganoidDialogueNPC::BeginPlay()
{
	Super::BeginPlay();
	CurrentEmotion = DefaultEmotion;
	BindDialogueEvents();
}

void AProjectOrganoidDialogueNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindDialogueEvents();
	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidDialogueNPC::BindDialogueEvents()
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			Dialogue->OnDialogueNodeChanged.AddDynamic(this, &AProjectOrganoidDialogueNPC::HandleDialogueNodeChanged);
			Dialogue->OnDialogueEnded.AddDynamic(this, &AProjectOrganoidDialogueNPC::HandleDialogueEnded);
		}
	}
}

void AProjectOrganoidDialogueNPC::UnbindDialogueEvents()
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			Dialogue->OnDialogueNodeChanged.RemoveDynamic(this, &AProjectOrganoidDialogueNPC::HandleDialogueNodeChanged);
			Dialogue->OnDialogueEnded.RemoveDynamic(this, &AProjectOrganoidDialogueNPC::HandleDialogueEnded);
		}
	}
}

bool AProjectOrganoidDialogueNPC::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor) || !ConversationAsset)
	{
		return false;
	}

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			if (Dialogue->IsDialogueActive())
			{
				return false;
			}
		}
	}

	return true;
}

bool AProjectOrganoidDialogueNPC::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!CanInteract_Implementation(Interactor))
	{
		return false;
	}

	if (UGameInstance* GI = Interactor->GetGameInstance())
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			return Dialogue->StartDialogue(ConversationAsset, this, Interactor);
		}
	}

	return false;
}

void AProjectOrganoidDialogueNPC::SetEmotion(EProjectOrganoidSpeakerEmotion NewEmotion)
{
	if (CurrentEmotion == NewEmotion)
	{
		return;
	}

	const EProjectOrganoidSpeakerEmotion Previous = CurrentEmotion;
	CurrentEmotion = NewEmotion;
	OnEmotionChanged.Broadcast(CurrentEmotion, Previous);
	BP_OnEmotionChanged(CurrentEmotion);
}

void AProjectOrganoidDialogueNPC::HandleDialogueNodeChanged(const FProjectOrganoidDialogueNode& /*Node*/, EProjectOrganoidSpeakerEmotion Emotion)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			if (Dialogue->GetActiveSpeaker() != this)
			{
				return;
			}
		}
	}

	SetEmotion(Emotion);
}

void AProjectOrganoidDialogueNPC::HandleDialogueEnded(FName /*ConversationId*/)
{
	SetEmotion(DefaultEmotion);
}
