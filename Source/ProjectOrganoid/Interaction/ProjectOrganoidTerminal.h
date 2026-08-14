// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidHackingTypes.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidTerminal.generated.h"

class UStaticMeshComponent;
class UProjectOrganoidHackingWidget;
class AProjectOrganoidSecurityGate;
class AProjectOrganoidDoorLock;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTerminalHackFinished, AProjectOrganoidTerminal*, Terminal, bool, bSucceeded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTerminalOpened, AProjectOrganoidTerminal*, Terminal, AProjectOrganoidCharacter*, Character);

/**
 *  Interactive facility terminal — opens a hacking mini-game UI and, on success,
 *  unlocks linked security doors/gates and rewards data-log entries.
 */
UCLASS(Blueprintable)
class AProjectOrganoidTerminal : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidTerminal();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TerminalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	FName TerminalId = TEXT("Terminal_Unnamed");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Hacking")
	FProjectOrganoidHackingSessionConfig HackingConfig;

	/** UMG class spawned for the mini-game (defaults to C++ base) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|UI")
	TSubclassOf<UProjectOrganoidHackingWidget> HackingWidgetClass;

	/** Soft-linked security gate unlocked on successful hack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	TSoftObjectPtr<AProjectOrganoidSecurityGate> LinkedSecurityGate;

	/** Soft-linked door lock unlocked on successful hack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	TSoftObjectPtr<AProjectOrganoidDoorLock> LinkedDoorLock;

	/** Optional gate id lookup via UProjectOrganoidSecuritySubsystem */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	FName LinkedSecurityGateId = NAME_None;

	/** Lore entry granted on success */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	FProjectOrganoidLogEntry RewardLogEntry;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	bool bGrantRewardLogOnSuccess = true;

	/** Objective event fired on first successful hack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Rewards")
	FName SuccessObjectiveEventId = TEXT("Event_TerminalHackSuccess");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal")
	bool bSingleUse = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Terminal")
	bool bHasBeenHacked = false;

	UPROPERTY(BlueprintAssignable, Category = "Terminal")
	FOnProjectOrganoidTerminalOpened OnTerminalOpened;

	UPROPERTY(BlueprintAssignable, Category = "Terminal")
	FOnProjectOrganoidTerminalHackFinished OnHackFinished;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintCallable, Category = "Terminal|UI")
	UProjectOrganoidHackingWidget* OpenHackingUI(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Terminal|UI")
	void CloseHackingUI();

	/** Called by the hacking widget when the FSM reaches Success / Failed */
	UFUNCTION(BlueprintCallable, Category = "Terminal")
	void NotifyHackingFinished(bool bSucceeded);

	UFUNCTION(BlueprintCallable, Category = "Terminal|Rewards")
	void ApplyHackRewards(AProjectOrganoidCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void BP_OnTerminalOpened(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void BP_OnHackSucceeded(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void BP_OnHackFailed(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void BP_OnDoorUnlockedByTerminal(AProjectOrganoidDoorLock* Door);

	UFUNCTION(BlueprintImplementableEvent, Category = "Terminal")
	void BP_OnGateUnlockedByTerminal(AProjectOrganoidSecurityGate* Gate);

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidHackingWidget> ActiveHackingWidget;

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> ActiveHacker;

	void UnlockLinkedSecurity();
	void GrantRewardLog(AProjectOrganoidCharacter* Character);
	void NotifyObjectiveEvent(FName EventId) const;
	void SetInputModeForUI(AProjectOrganoidCharacter* Character, bool bUIOnly);
};
