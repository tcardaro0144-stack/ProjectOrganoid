// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidDoorLock.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidDoorStateChanged, bool, bIsUnlocked);

/**
 *  Facility door / airlock lock requiring a keycard of a minimum security tier.
 */
UCLASS(Blueprintable)
class AProjectOrganoidDoorLock : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidDoorLock();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	/** Minimum keycard clearance required to unlock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Security")
	EProjectOrganoidSecurityTier RequiredSecurityTier = EProjectOrganoidSecurityTier::Level1_Admin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door|Security")
	bool bIsLocked = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Door|Security")
	bool bIsOpen = false;

	/** If true, consume the matching keycard on successful unlock */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door|Security")
	bool bConsumeKeycardOnUnlock = false;

	UPROPERTY(BlueprintAssignable, Category = "Door")
	FOnProjectOrganoidDoorStateChanged OnDoorStateChanged;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetLocked(bool bNewLocked);

	UFUNCTION(BlueprintCallable, Category = "Door")
	void SetOpen(bool bNewOpen);

	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void BP_OnDoorUnlocked(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Door")
	void BP_OnAccessDenied(AProjectOrganoidCharacter* Interactor, EProjectOrganoidSecurityTier RequiredTier);
};
