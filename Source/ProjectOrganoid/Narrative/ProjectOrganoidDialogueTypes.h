// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProjectOrganoidDialogueTypes.generated.h"

/** Speaker emotional state for animation / subtitle styling */
UENUM(BlueprintType)
enum class EProjectOrganoidSpeakerEmotion : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Calm UMETA(DisplayName = "Calm"),
	Tense UMETA(DisplayName = "Tense"),
	Fearful UMETA(DisplayName = "Fearful"),
	Angry UMETA(DisplayName = "Angry"),
	Injured UMETA(DisplayName = "Injured"),
	Urgent UMETA(DisplayName = "Urgent")
};

UENUM(BlueprintType)
enum class EProjectOrganoidDialogueCameraShot : uint8
{
	None UMETA(DisplayName = "None"),
	OverShoulder UMETA(DisplayName = "Over Shoulder"),
	CloseUp UMETA(DisplayName = "Close Up"),
	TwoShot UMETA(DisplayName = "Two Shot"),
	Profile UMETA(DisplayName = "Profile")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText ChoiceText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NextNodeId = NAME_None;

	/** Optional objective / telemetry event fired when this choice is selected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName GameplayEventId = NAME_None;
};

USTRUCT(BlueprintType)
struct FProjectOrganoidDialogueNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText LineText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EProjectOrganoidSpeakerEmotion Emotion = EProjectOrganoidSpeakerEmotion::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EProjectOrganoidDialogueCameraShot CameraShot = EProjectOrganoidDialogueCameraShot::OverShoulder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (ClampMin = "0.5"))
	float AutoAdvanceSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FProjectOrganoidDialogueChoice> Choices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NextNodeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	bool bEndsConversation = false;
};

USTRUCT(BlueprintType)
struct FProjectOrganoidActiveDialogueState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName ConversationId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName CurrentNodeId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FProjectOrganoidDialogueNode CurrentNode;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bIsActive = false;
};

/**
 *  Designer-authored dialogue tree (branching nodes + emotions + camera shots).
 */
UCLASS(BlueprintType)
class UProjectOrganoidDialogueDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
	FName ConversationId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
	FText ConversationTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
	FName EntryNodeId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
	TArray<FProjectOrganoidDialogueNode> Nodes;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool FindNode(FName NodeId, FProjectOrganoidDialogueNode& OutNode) const
	{
		for (const FProjectOrganoidDialogueNode& Node : Nodes)
		{
			if (Node.NodeId == NodeId)
			{
				OutNode = Node;
				return true;
			}
		}
		return false;
	}
};
