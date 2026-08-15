// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidDialogueTypes.h"
#include "ProjectOrganoidDialogueNPC.generated.h"

class USkeletalMeshComponent;
class UProjectOrganoidDialogueDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidNPCEmotionChanged, EProjectOrganoidSpeakerEmotion, NewEmotion, EProjectOrganoidSpeakerEmotion, PreviousEmotion);

/**
 *  Talkable facility NPC — starts a dialogue tree on interact and tracks reaction emotion.
 */
UCLASS(Blueprintable)
class AProjectOrganoidDialogueNPC : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidDialogueNPC();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TObjectPtr<UProjectOrganoidDialogueDataAsset> ConversationAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EProjectOrganoidSpeakerEmotion DefaultEmotion = EProjectOrganoidSpeakerEmotion::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue")
	EProjectOrganoidSpeakerEmotion CurrentEmotion = EProjectOrganoidSpeakerEmotion::Neutral;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnProjectOrganoidNPCEmotionChanged OnEmotionChanged;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetEmotion(EProjectOrganoidSpeakerEmotion NewEmotion);

	UFUNCTION(BlueprintImplementableEvent, Category = "Dialogue")
	void BP_OnEmotionChanged(EProjectOrganoidSpeakerEmotion NewEmotion);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleDialogueNodeChanged(const FProjectOrganoidDialogueNode& Node, EProjectOrganoidSpeakerEmotion Emotion);

	UFUNCTION()
	void HandleDialogueEnded(FName ConversationId);

	void BindDialogueEvents();
	void UnbindDialogueEvents();
};
