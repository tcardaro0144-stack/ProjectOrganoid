// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidAdminRoomTrigger.generated.h"

class UBoxComponent;

/**
 *  Section 15 room-identity volume. BeginOverlap sets Admin_SectorController CurrentRoom.
 *  EndOverlap does nothing (sticky identity).
 */
UCLASS(Blueprintable)
class AProjectOrganoidAdminRoomTrigger : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidAdminRoomTrigger();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Room")
	FName RoomID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Room")
	FText RoomDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Admin|Room")
	bool bTriggerOnce = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Admin|Room")
	bool bHasTriggered = false;

protected:

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
