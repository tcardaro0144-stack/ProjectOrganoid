// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidSecurityTypes.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSecurityGate.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidGateStateChanged, AProjectOrganoidSecurityGate*, Gate, EProjectOrganoidSecurityGateState, NewState, EProjectOrganoidSecurityOverrideMethod, Method);

/**
 *  Interactive progression barrier — sealed during lockdown until Avery
 *  presents a keycard or terminal hacking tool of sufficient clearance.
 */
UCLASS(Blueprintable)
class AProjectOrganoidSecurityGate : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidSecurityGate();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> GateMesh;

	/** Collision volume that blocks Avery while the gate is sealed */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BarrierVolume;

	/** Unique id for subsystem lookups / lockdown groups */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	FName GateId = TEXT("Gate_Unnamed");

	/** Optional lockdown group — matches EngageFacilityLockdown(LockdownId) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	FName LockdownGroupId = TEXT("FacilityWide");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	EProjectOrganoidSecurityTier RequiredSecurityTier = EProjectOrganoidSecurityTier::Level1_Admin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Security|Gate")
	EProjectOrganoidSecurityGateState GateState = EProjectOrganoidSecurityGateState::Sealed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	bool bAllowKeycardOverride = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	bool bAllowHackingToolOverride = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	bool bConsumeKeycardOnOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	bool bConsumeHackingToolOnOverride = true;

	/** If true, facility lockdown re-seals this gate even after a prior override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate")
	bool bResealOnFacilityLockdown = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate|Power")
	EProjectOrganoidPowerSector PowerSector = EProjectOrganoidPowerSector::Admin;

	/** Maglocks lose power in blackout — barrier collision opens until power returns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security|Gate|Power")
	bool bFailOpenOnBlackout = true;

	UPROPERTY(BlueprintAssignable, Category = "Security|Gate")
	FOnProjectOrganoidGateStateChanged OnGateStateChanged;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "Security|Gate")
	bool IsSealed() const { return GateState == EProjectOrganoidSecurityGateState::Sealed; }

	UFUNCTION(BlueprintPure, Category = "Security|Gate")
	bool IsOpen() const { return GateState == EProjectOrganoidSecurityGateState::Open; }

	UFUNCTION(BlueprintCallable, Category = "Security|Gate")
	void SetGateState(EProjectOrganoidSecurityGateState NewState, EProjectOrganoidSecurityOverrideMethod Method = EProjectOrganoidSecurityOverrideMethod::Manual);

	UFUNCTION(BlueprintCallable, Category = "Security|Gate")
	bool TryOverrideWithInventory(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Security|Gate")
	void ApplyFacilityLockdown();

	UFUNCTION(BlueprintCallable, Category = "Security|Gate")
	void ClearFacilityLockdownSeal(bool bUnlock);

	UFUNCTION(BlueprintCallable, Category = "Security|Gate|Power")
	void HandlePowerStateChanged(EProjectOrganoidPowerState NewState, EProjectOrganoidPowerState PreviousState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Security|Gate")
	void BP_OnGateOpened(AProjectOrganoidCharacter* Interactor, EProjectOrganoidSecurityOverrideMethod Method);

	UFUNCTION(BlueprintImplementableEvent, Category = "Security|Gate")
	void BP_OnAccessDenied(AProjectOrganoidCharacter* Interactor, EProjectOrganoidSecurityTier RequiredTier);

	UFUNCTION(BlueprintImplementableEvent, Category = "Security|Gate")
	void BP_OnGateStateChanged(EProjectOrganoidSecurityGateState NewState, EProjectOrganoidSecurityOverrideMethod Method);

	UFUNCTION(BlueprintImplementableEvent, Category = "Security|Gate|Power")
	void BP_OnPowerStateChanged(EProjectOrganoidPowerState NewState);

protected:

	void RefreshBarrierCollision();
	void RefreshInteractionPrompt();
	void NotifyObjectiveEvent(FName EventId) const;
};
