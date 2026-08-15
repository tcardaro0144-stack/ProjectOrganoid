// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDialogueSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidTelemetrySubsystem.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void UProjectOrganoidDialogueSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UProjectOrganoidDialogueSubsystem::Deinitialize()
{
	if (ActiveState.bIsActive)
	{
		EndDialogue();
	}
	Super::Deinitialize();
}

APlayerController* UProjectOrganoidDialogueSubsystem::ResolveListenerController() const
{
	if (AProjectOrganoidCharacter* Listener = ActiveListener.Get())
	{
		return Cast<APlayerController>(Listener->GetController());
	}
	return nullptr;
}

bool UProjectOrganoidDialogueSubsystem::StartDialogue(
	UProjectOrganoidDialogueDataAsset* Conversation,
	AActor* Speaker,
	AProjectOrganoidCharacter* Listener)
{
	if (!Conversation || Conversation->ConversationId.IsNone() || !Speaker || !Listener)
	{
		return false;
	}

	if (ActiveState.bIsActive)
	{
		EndDialogue();
	}

	ActiveConversation = Conversation;
	ActiveSpeaker = Speaker;
	ActiveListener = Listener;
	ActiveState.ConversationId = Conversation->ConversationId;
	ActiveState.bIsActive = true;

	OnDialogueStarted.Broadcast(Conversation->ConversationId);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidTelemetrySubsystem* Telemetry = GI->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
		{
			Telemetry->ReportGameplayEvent(TEXT("DialogueStart"), Conversation->ConversationId.ToString());
		}
	}

	const FName Entry = Conversation->EntryNodeId.IsNone() && Conversation->Nodes.Num() > 0
		? Conversation->Nodes[0].NodeId
		: Conversation->EntryNodeId;

	return AdvanceToNode(Entry);
}

bool UProjectOrganoidDialogueSubsystem::AdvanceToNode(FName NodeId)
{
	if (!ActiveConversation || NodeId.IsNone())
	{
		return false;
	}

	FProjectOrganoidDialogueNode Node;
	if (!ActiveConversation->FindNode(NodeId, Node))
	{
		return false;
	}

	ApplyNode(Node);
	return true;
}

void UProjectOrganoidDialogueSubsystem::ApplyNode(const FProjectOrganoidDialogueNode& Node)
{
	ActiveState.CurrentNodeId = Node.NodeId;
	ActiveState.CurrentNode = Node;

	FrameCameraForShot(Node.CameraShot, ActiveSpeaker.Get(), ActiveListener.Get());
	OnDialogueNodeChanged.Broadcast(Node, Node.Emotion);
}

bool UProjectOrganoidDialogueSubsystem::SelectChoice(int32 ChoiceIndex)
{
	if (!ActiveState.bIsActive)
	{
		return false;
	}

	const TArray<FProjectOrganoidDialogueChoice>& Choices = ActiveState.CurrentNode.Choices;
	if (!Choices.IsValidIndex(ChoiceIndex))
	{
		// No choices — advance via NextNodeId or end
		if (ActiveState.CurrentNode.bEndsConversation || ActiveState.CurrentNode.NextNodeId.IsNone())
		{
			EndDialogue();
			return true;
		}
		return AdvanceToNode(ActiveState.CurrentNode.NextNodeId);
	}

	const FProjectOrganoidDialogueChoice& Choice = Choices[ChoiceIndex];
	OnDialogueChoiceSelected.Broadcast(ChoiceIndex, Choice.NextNodeId);

	if (!Choice.GameplayEventId.IsNone())
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(Choice.GameplayEventId);
		}
	}

	if (Choice.NextNodeId.IsNone())
	{
		EndDialogue();
		return true;
	}

	return AdvanceToNode(Choice.NextNodeId);
}

void UProjectOrganoidDialogueSubsystem::EndDialogue()
{
	const FName EndedId = ActiveState.ConversationId;
	RestoreGameplayCamera();

	ActiveState = FProjectOrganoidActiveDialogueState();
	ActiveConversation = nullptr;
	ActiveSpeaker.Reset();
	ActiveListener.Reset();

	if (!EndedId.IsNone())
	{
		OnDialogueEnded.Broadcast(EndedId);
	}
}

void UProjectOrganoidDialogueSubsystem::FrameCameraForShot(
	EProjectOrganoidDialogueCameraShot Shot,
	AActor* Speaker,
	AProjectOrganoidCharacter* Listener)
{
	APlayerController* PC = ResolveListenerController();
	if (!PC || !Speaker || Shot == EProjectOrganoidDialogueCameraShot::None)
	{
		return;
	}

	if (!bHasCachedViewTarget)
	{
		CachedViewTarget = PC->GetViewTarget();
		bHasCachedViewTarget = true;
	}

	UWorld* World = Speaker->GetWorld();
	if (!World)
	{
		return;
	}

	if (!DialogueCameraActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		DialogueCameraActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Speaker->GetActorTransform(), Params);
	}

	ACameraActor* CamActor = Cast<ACameraActor>(DialogueCameraActor);
	if (!CamActor)
	{
		return;
	}

	const FVector SpeakerLoc = Speaker->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f);
	const FVector ListenerLoc = Listener
		? Listener->GetActorLocation() + FVector(0.0f, 0.0f, 70.0f)
		: SpeakerLoc + Speaker->GetActorForwardVector() * -200.0f;

	FVector CamLoc = SpeakerLoc;
	FRotator CamRot = (SpeakerLoc - ListenerLoc).Rotation();

	switch (Shot)
	{
	case EProjectOrganoidDialogueCameraShot::CloseUp:
		CamLoc = SpeakerLoc - Speaker->GetActorForwardVector() * CloseUpDistance + FVector(0.0f, 40.0f, 10.0f);
		CamRot = (SpeakerLoc - CamLoc).Rotation();
		break;
	case EProjectOrganoidDialogueCameraShot::TwoShot:
		CamLoc = (SpeakerLoc + ListenerLoc) * 0.5f - ((SpeakerLoc - ListenerLoc).GetSafeNormal() ^ FVector::UpVector) * 280.0f + FVector(0.0f, 0.0f, 40.0f);
		CamRot = (((SpeakerLoc + ListenerLoc) * 0.5f) - CamLoc).Rotation();
		break;
	case EProjectOrganoidDialogueCameraShot::Profile:
		CamLoc = SpeakerLoc + Speaker->GetActorRightVector() * CloseUpDistance + FVector(0.0f, 0.0f, 20.0f);
		CamRot = (SpeakerLoc - CamLoc).Rotation();
		break;
	case EProjectOrganoidDialogueCameraShot::OverShoulder:
	default:
		CamLoc = ListenerLoc - (ListenerLoc - SpeakerLoc).GetSafeNormal() * OverShoulderDistance + FVector(0.0f, 35.0f, 25.0f);
		CamRot = (SpeakerLoc - CamLoc).Rotation();
		break;
	}

	CamActor->SetActorLocationAndRotation(CamLoc, CamRot);
	PC->SetViewTargetWithBlend(CamActor, CameraBlendTime);
	OnDialogueCameraFramed.Broadcast(Shot, Speaker);
}

void UProjectOrganoidDialogueSubsystem::RestoreGameplayCamera()
{
	if (APlayerController* PC = ResolveListenerController())
	{
		if (bHasCachedViewTarget)
		{
			AActor* Target = CachedViewTarget.Get();
			if (!Target && ActiveListener.IsValid())
			{
				Target = ActiveListener.Get();
			}
			if (Target)
			{
				PC->SetViewTargetWithBlend(Target, CameraBlendTime);
			}
		}
	}

	bHasCachedViewTarget = false;
	CachedViewTarget.Reset();

	if (DialogueCameraActor)
	{
		DialogueCameraActor->Destroy();
		DialogueCameraActor = nullptr;
	}
}
