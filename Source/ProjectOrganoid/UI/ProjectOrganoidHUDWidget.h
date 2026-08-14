// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidHUDWidget.generated.h"

class AProjectOrganoidCharacter;

/**
 *  Diegetic vitals overlay for Avery Vance.
 *  Bind to AProjectOrganoidCharacter to mirror Suit Vitals and Tactical Mode state.
 */
UCLASS()
class UProjectOrganoidHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Bind this HUD to Avery's character and start receiving vitals / tactical events */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void BindToCharacter(AProjectOrganoidCharacter* InCharacter);

	/** Clear the character binding and stop listening for tactical mode changes */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void UnbindFromCharacter();

	/** Pull current Suit Vitals from the bound character into the overlay */
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals")
	void UpdateVitalsFromCharacter();

	/** Push heart rate (BPM) into Blueprint visual/audio presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetHeartRateBPM(float BPM);

	/** Push toxicity percentage into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetToxicityPercent(float Percent);

	/** Push current health into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetHealth(float HealthValue);

	/** Push current / max PE Energy into Blueprint presentation */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Vitals")
	void SetPEEnergy(float CurrentEnergy, float MaxEnergy);

	/** Fired when Tactical Mode engages — implement audio/visual effects in Blueprint */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Tactical")
	void OnTacticalModeActivated();

	/** Fired when Tactical Mode disengages — implement audio/visual effects in Blueprint */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Tactical")
	void OnTacticalModeDeactivated();

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	/** Bound Avery Vance character providing Suit Vitals */
	UPROPERTY(BlueprintReadOnly, Category = "HUD|Vitals")
	TObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UFUNCTION()
	void HandleTacticalModeChanged(bool bIsActive);
};
