// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProjectOrganoidDialogueTypes.h"
#include "ProjectOrganoidDialogueSubsystem.generated.h"

class AActor;
class APlayerController;
class AProjectOrganoidCharacter;
class UCameraComponent;
class UProjectOrganoidDialogueDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidDialogueStarted, FName, ConversationId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidDialogueEnded, FName, ConversationId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidDialogueNodeChanged, const FProjectOrganoidDialogueNode&, Node, EProjectOrganoidSpeakerEmotion, Emotion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidDialogueChoiceSelected, int32, ChoiceIndex, FName, NextNodeId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidDialogueCameraFramed, EProjectOrganoidDialogueCameraShot, Shot, AActor*, Speaker);

/**
 *  Branching NPC dialogue director — emotion flags, choice routing, cinematic framing.
 */
UCLASS()
class UProjectOrganoidDialogueSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidDialogueStarted OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidDialogueEnded OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidDialogueNodeChanged OnDialogueNodeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidDialogueChoiceSelected OnDialogueChoiceSelected;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidDialogueCameraFramed OnDialogueCameraFramed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Camera", meta = (ClampMin = "0.0"))
	float CameraBlendTime = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Camera", meta = (ClampMin = "50.0"))
	float CloseUpDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Camera", meta = (ClampMin = "50.0"))
	float OverShoulderDistance = 220.0f;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(UProjectOrganoidDialogueDataAsset* Conversation, AActor* Speaker, AProjectOrganoidCharacter* Listener);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool AdvanceToNode(FName NodeId);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool SelectChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return ActiveState.bIsActive; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FProjectOrganoidActiveDialogueState GetActiveDialogueState() const { return ActiveState; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	EProjectOrganoidSpeakerEmotion GetCurrentEmotion() const { return ActiveState.CurrentNode.Emotion; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	AActor* GetActiveSpeaker() const { return ActiveSpeaker.Get(); }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	AProjectOrganoidCharacter* GetActiveListener() const { return ActiveListener.Get(); }

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidDialogueDataAsset> ActiveConversation;

	UPROPERTY()
	TWeakObjectPtr<AActor> ActiveSpeaker;

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> ActiveListener;

	UPROPERTY()
	FProjectOrganoidActiveDialogueState ActiveState;

	UPROPERTY()
	TObjectPtr<AActor> DialogueCameraActor;

	TWeakObjectPtr<AActor> CachedViewTarget;
	bool bHasCachedViewTarget = false;

	void ApplyNode(const FProjectOrganoidDialogueNode& Node);
	void FrameCameraForShot(EProjectOrganoidDialogueCameraShot Shot, AActor* Speaker, AProjectOrganoidCharacter* Listener);
	void RestoreGameplayCamera();
	APlayerController* ResolveListenerController() const;
};
