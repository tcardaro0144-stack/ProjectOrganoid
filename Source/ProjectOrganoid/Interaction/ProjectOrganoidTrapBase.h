// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidTrapTypes.h"
#include "ProjectOrganoidTrapBase.generated.h"

class USceneComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTrapTriggered, AActor*, TrapActor, AActor*, TriggeringActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidTrapArmedChanged, bool, bArmed);

/**
 *  Base facility trap — arm/disarm, trigger cooldown, hazard application helpers.
 */
UCLASS(Abstract, Blueprintable)
class AProjectOrganoidTrapBase : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidTrapBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> TrapRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	EProjectOrganoidTrapType TrapType = EProjectOrganoidTrapType::LaserTripwire;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	FName CorridorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	bool bArmed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap")
	bool bSingleUse = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0.0"))
	float RetriggerCooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Damage")
	EProjectOrganoidHazardType LinkedHazard = EProjectOrganoidHazardType::UVCRadiation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Damage", meta = (ClampMin = "0.0"))
	float TriggerDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Damage", meta = (ClampMin = "0.0"))
	float TriggerIntensity = 1.0f;

	UPROPERTY(BlueprintAssignable, Category = "Trap")
	FOnProjectOrganoidTrapTriggered OnTrapTriggered;

	UPROPERTY(BlueprintAssignable, Category = "Trap")
	FOnProjectOrganoidTrapArmedChanged OnTrapArmedChanged;

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void SetArmed(bool bNewArmed);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	bool TriggerTrap(AActor* TriggeringActor);

	UFUNCTION(BlueprintPure, Category = "Trap")
	bool CanTrigger() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Trap")
	void BP_OnTrapTriggered(AActor* TriggeringActor);

protected:

	float LastTriggerTime = -1000.0f;

	virtual void ApplyTrapEffects(AActor* TriggeringActor);
	void ApplyHazardToActor(AActor* Target, float DamageOverride = -1.0f) const;
};
