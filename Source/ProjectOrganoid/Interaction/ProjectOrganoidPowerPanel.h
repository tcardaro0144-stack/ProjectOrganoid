// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidPowerPanel.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AProjectOrganoidCharacter;

/**
 *  Self-powered breaker. Works during blackout so Avery can raise a dark sector
 *  to Emergency (or a chosen restored state) without a live terminal.
 */
UCLASS(Blueprintable)
class AProjectOrganoidPowerPanel : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidPowerPanel();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PanelMesh;

	/** Always-on indicator so the breaker stays findable in a blackout */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> StatusLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerSector PowerSector = EProjectOrganoidPowerSector::Cryo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerState RestoredState = EProjectOrganoidPowerState::Emergency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	bool bSingleUse = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Power")
	bool bHasBeenEngaged = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	FName SuccessObjectiveEventId = NAME_None;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Power")
	void BP_OnPowerPanelEngaged(AProjectOrganoidCharacter* Interactor, EProjectOrganoidPowerState NewState);

protected:

	void NotifyObjectiveEvent(FName EventId) const;
	void RefreshPrompt();
};
