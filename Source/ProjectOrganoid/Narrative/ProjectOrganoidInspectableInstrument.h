// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidInspectableInstrument.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidObjectiveSubsystem;
class USceneComponent;
class UStaticMeshComponent;

/**
 *  World instrument that completes an objective event on first inspection and
 *  presents a one-shot HUD transient notification. Replay uses ReviewPrompt.
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidInspectableInstrument : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidInspectableInstrument();

	/**
	 * Temporary presentation hierarchy root. Mesh static-mesh assets and relative
	 * transforms are left unset for a later editor-authoring step.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Presentation")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Temporary pedestal blockout mesh. Optional; no Engine mesh assigned in C++. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Presentation")
	TObjectPtr<UStaticMeshComponent> PedestalMesh;

	/** Temporary column blockout mesh. Optional; no Engine mesh assigned in C++. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Presentation")
	TObjectPtr<UStaticMeshComponent> ColumnMesh;

	/** Temporary array-head blockout mesh. Optional; no Engine mesh assigned in C++. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Presentation")
	TObjectPtr<UStaticMeshComponent> ArrayHeadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Objectives")
	FName ObjectiveEventId = FName(TEXT("Event_NeuroResearchArrayLocated"));

	/** When this objective is already Completed, start/replay in ReviewPrompt mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Objectives")
	FName CompletedObjectiveIdForReplayGuard = FName(TEXT("Obj_InvestigateNeuroResearchFloor"));

	/**
	 * Optional gate: when set, first-time inspection requires this objective to be Active.
	 * None preserves legacy ungated behavior. Completed replay-guard still allows Review.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Objectives")
	FName RequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Presentation")
	FText InspectionPrompt = FText::FromString(TEXT("Inspect Neural Mapping Array"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Presentation")
	FText ReviewPrompt = FText::FromString(TEXT("Review Neural Mapping Array"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Presentation")
	FText SpeakerLabel = FText::FromString(TEXT("Nathan"));

	/** Curly apostrophe as ASCII \u2019 escape (bytes 5C 75 32 30 31 39). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Presentation")
	FText InspectionResponseText = FText::FromString(
		TEXT("The spikes are coming from this array. It\u2019s still mapping something."));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Presentation", meta = (ClampMin = "0.0"))
	float NotificationDurationSeconds = 4.0f;

	/** Transient presentation flag only — not SaveGame authority. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instrument|Presentation", Transient)
	bool bHasBeenInspected = false;

	/** Test / diagnostics: how many first-inspection HUD lines were presented. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instrument|Diagnostics", Transient)
	int32 InspectionNotificationCount = 0;

	/** Test / diagnostics: how many times ObjectiveEventId was fired. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instrument|Diagnostics", Transient)
	int32 ObjectiveEventFireCount = 0;

	/**
	 * Optional second campaign hook. All three ids None disables it.
	 * Runs only after the primary replay guard is already Completed, and only when
	 * its own objective is Active and its prerequisite objective is Completed.
	 * Does not replace the primary ObjectiveEventId path.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup")
	FName FollowupRequiredActiveObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup")
	FName FollowupPrerequisiteCompletedObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup")
	FName FollowupSuccessEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup")
	FText FollowupSpeakerLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup")
	FText FollowupResponseText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instrument|Followup", meta = (ClampMin = "0.0"))
	float FollowupNotificationDurationSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instrument|Followup", Transient)
	int32 FollowupEventFireCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Instrument|Followup", Transient)
	int32 FollowupNotificationCount = 0;

	virtual void BeginPlay() override;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Instrument")
	void BP_OnInspected(AProjectOrganoidCharacter* Interactor, bool bFirstInspection);

	void RefreshPrompt();

protected:

	UProjectOrganoidObjectiveSubsystem* GetObjectiveSubsystem() const;
	bool IsGuardedObjectiveCompleted() const;
	bool IsRequiredObjectiveActive() const;
	bool IsFollowupConnectionConfigured() const;
	bool IsNamedObjectiveInState(FName ObjectiveId, EProjectOrganoidObjectiveState State) const;
	bool TryFollowupConnection(AProjectOrganoidCharacter* Interactor);
	void PresentInspectionNotification(AProjectOrganoidCharacter* Interactor);
	void PresentFollowupNotification(AProjectOrganoidCharacter* Interactor);
	void ConfigurePresentationMesh(UStaticMeshComponent* Mesh) const;
};
