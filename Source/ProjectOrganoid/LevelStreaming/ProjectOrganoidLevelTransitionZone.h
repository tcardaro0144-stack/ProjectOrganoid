// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidLevelTransitionZone.generated.h"

class UBoxComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTransitionTriggered, AProjectOrganoidCharacter*, Character, EProjectOrganoidSubLevelTag, TargetTag);

/**
 *  Trigger volume that streams Epitope sub-levels and autosaves Avery on entry.
 */
UCLASS(Blueprintable)
class AProjectOrganoidLevelTransitionZone : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidLevelTransitionZone();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** Destination facility floor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	EProjectOrganoidSubLevelTag TargetSubLevelTag = EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics;

	/**
	 *  Streaming level to load. If None, uses the registered definition for TargetSubLevelTag.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FName TargetStreamingLevelName = NAME_None;

	/** Streaming levels to unload after the target finishes loading */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TArray<FName> StreamingLevelsToUnload;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bMakeVisibleAfterLoad = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bTeleportOnArrival = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (MakeEditWidget = true))
	FTransform ArrivalTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bTriggerOnce = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition")
	bool bHasTriggered = false;

	UPROPERTY(BlueprintAssignable, Category = "Transition")
	FOnProjectOrganoidTransitionTriggered OnTransitionTriggered;

	UFUNCTION(BlueprintCallable, Category = "Transition")
	bool TriggerTransition(AProjectOrganoidCharacter* Character);

protected:

	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
