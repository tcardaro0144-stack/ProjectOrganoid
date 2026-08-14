// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidSecurityTypes.h"
#include "ProjectOrganoidSecuritySubsystem.generated.h"

class AProjectOrganoidSecurityGate;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidFacilityLockdownChanged, bool, bIsLockdownActive, FName, LockdownId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidSecurityGateStateChanged, AProjectOrganoidSecurityGate*, Gate, EProjectOrganoidSecurityGateState, NewState, EProjectOrganoidSecurityOverrideMethod, Method);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidSecurityGateOverrideAttempt, AProjectOrganoidSecurityGate*, Gate, AProjectOrganoidCharacter*, Character, bool, bSucceeded);

/**
 *  Facility-wide lockdown director — registers security gates, engages / lifts
 *  lockdowns, and broadcasts gate override events for objectives / UI.
 */
UCLASS()
class UProjectOrganoidSecuritySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Security")
	FOnProjectOrganoidFacilityLockdownChanged OnFacilityLockdownChanged;

	UPROPERTY(BlueprintAssignable, Category = "Security")
	FOnProjectOrganoidSecurityGateStateChanged OnSecurityGateStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Security")
	FOnProjectOrganoidSecurityGateOverrideAttempt OnSecurityGateOverrideAttempt;

	UFUNCTION(BlueprintCallable, Category = "Security")
	void RegisterSecurityGate(AProjectOrganoidSecurityGate* Gate);

	UFUNCTION(BlueprintCallable, Category = "Security")
	void UnregisterSecurityGate(AProjectOrganoidSecurityGate* Gate);

	UFUNCTION(BlueprintPure, Category = "Security")
	bool IsFacilityLockdownActive() const { return bFacilityLockdownActive; }

	UFUNCTION(BlueprintPure, Category = "Security")
	FName GetActiveLockdownId() const { return ActiveLockdownId; }

	/** Seal all registered gates (optional zone filter via LockdownId match). */
	UFUNCTION(BlueprintCallable, Category = "Security")
	void EngageFacilityLockdown(FName LockdownId = TEXT("FacilityWide"));

	/** Lift lockdown and optionally unlock every gate that was sealed by it. */
	UFUNCTION(BlueprintCallable, Category = "Security")
	void LiftFacilityLockdown(bool bUnlockAllGates = false);

	/** Force-open / unlock a gate by id (scripted overrides, objectives). */
	UFUNCTION(BlueprintCallable, Category = "Security")
	bool OverrideGateById(FName GateId, EProjectOrganoidSecurityOverrideMethod Method = EProjectOrganoidSecurityOverrideMethod::SubsystemOverride);

	UFUNCTION(BlueprintCallable, Category = "Security")
	bool OverrideGate(AProjectOrganoidSecurityGate* Gate, EProjectOrganoidSecurityOverrideMethod Method = EProjectOrganoidSecurityOverrideMethod::SubsystemOverride);

	UFUNCTION(BlueprintPure, Category = "Security")
	AProjectOrganoidSecurityGate* FindGateById(FName GateId) const;

	UFUNCTION(BlueprintPure, Category = "Security")
	TArray<AProjectOrganoidSecurityGate*> GetRegisteredGates() const;

	/** Called by gates after a successful / failed player override attempt. */
	void NotifyGateOverrideAttempt(AProjectOrganoidSecurityGate* Gate, AProjectOrganoidCharacter* Character, bool bSucceeded);

	/** Called by gates whenever their sealed/open state changes. */
	void NotifyGateStateChanged(AProjectOrganoidSecurityGate* Gate, EProjectOrganoidSecurityGateState NewState, EProjectOrganoidSecurityOverrideMethod Method);

protected:

	UPROPERTY()
	TArray<TObjectPtr<AProjectOrganoidSecurityGate>> RegisteredGates;

	UPROPERTY(BlueprintReadOnly, Category = "Security")
	bool bFacilityLockdownActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Security")
	FName ActiveLockdownId = NAME_None;

	void PruneInvalidGates();
};
