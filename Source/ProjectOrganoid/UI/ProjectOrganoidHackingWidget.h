// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidHackingTypes.h"
#include "ProjectOrganoidHackingWidget.generated.h"

class AProjectOrganoidTerminal;
class AProjectOrganoidCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidHackingStateChanged, EProjectOrganoidHackingUIState, NewState, EProjectOrganoidHackingUIState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHackingProgressChanged, float, ProgressNormalized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidHackingFinished, bool, bSucceeded);

/**
 *  Facility hacking UI state machine — node-matching or password decryption.
 *  C++ drives puzzle logic; UMG / Blueprint paints the board.
 */
UCLASS()
class UProjectOrganoidHackingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable, Category = "Hacking|UI")
	FOnProjectOrganoidHackingStateChanged OnHackingStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Hacking|UI")
	FOnProjectOrganoidHackingProgressChanged OnHackingProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Hacking|UI")
	FOnProjectOrganoidHackingFinished OnHackingFinished;

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void BindToTerminal(AProjectOrganoidTerminal* InTerminal, AProjectOrganoidCharacter* InCharacter, const FProjectOrganoidHackingSessionConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void UnbindFromTerminal();

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void BeginHackingSession();

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void AbortHackingSession();

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void SetHackingState(EProjectOrganoidHackingUIState NewState);

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void CloseHackingUI();

	/** Node-match: submit a node index from the active pool */
	UFUNCTION(BlueprintCallable, Category = "Hacking|NodeMatch")
	bool SubmitNodeSelection(int32 NodeIndex);

	/** Password decrypt: submit a guessed character / cipher key */
	UFUNCTION(BlueprintCallable, Category = "Hacking|Password")
	bool SubmitDecryptKey(FString Key);

	/** Debug / designer shortcut — mark puzzle solved */
	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void ForceSucceed();

	UFUNCTION(BlueprintCallable, Category = "Hacking|UI")
	void ForceFail();

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	EProjectOrganoidHackingUIState GetHackingState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	EProjectOrganoidHackingMiniGame GetMiniGame() const { return SessionConfig.MiniGame; }

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	float GetProgressNormalized() const;

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	int32 GetAttemptsRemaining() const { return AttemptsRemaining; }

	UFUNCTION(BlueprintPure, Category = "Hacking|NodeMatch")
	TArray<int32> GetTargetNodeSequence() const { return TargetNodeSequence; }

	UFUNCTION(BlueprintPure, Category = "Hacking|NodeMatch")
	TArray<int32> GetPlayerNodeSequence() const { return PlayerNodeSequence; }

	UFUNCTION(BlueprintPure, Category = "Hacking|Password")
	FString GetScrambledPassword() const { return ScrambledPassword; }

	UFUNCTION(BlueprintPure, Category = "Hacking|Password")
	FString GetRevealedPassword() const { return RevealedPassword; }

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	AProjectOrganoidTerminal* GetBoundTerminal() const { return BoundTerminal; }

	UFUNCTION(BlueprintPure, Category = "Hacking|UI")
	AProjectOrganoidCharacter* GetBoundCharacter() const { return BoundCharacter; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Hacking|UI")
	void OnHackingUIOpened(EProjectOrganoidHackingMiniGame MiniGame);

	UFUNCTION(BlueprintImplementableEvent, Category = "Hacking|UI")
	void OnHackingUIClosed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hacking|UI")
	void OnHackingBoardNeedsRefresh();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hacking|UI")
	void BP_OnHackingStateChanged(EProjectOrganoidHackingUIState NewState, EProjectOrganoidHackingUIState PreviousState);

protected:

	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|UI")
	TObjectPtr<AProjectOrganoidTerminal> BoundTerminal;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|UI")
	TObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|UI")
	FProjectOrganoidHackingSessionConfig SessionConfig;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|UI")
	EProjectOrganoidHackingUIState CurrentState = EProjectOrganoidHackingUIState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|NodeMatch")
	TArray<int32> TargetNodeSequence;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|NodeMatch")
	TArray<int32> PlayerNodeSequence;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|Password")
	FString ScrambledPassword;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|Password")
	FString RevealedPassword;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|UI")
	int32 AttemptsRemaining = 3;

	UPROPERTY(BlueprintReadOnly, Category = "Hacking|Password")
	int32 DecryptStepsCompleted = 0;

	FTimerHandle BootTimerHandle;
	FTimerHandle ResultTimerHandle;

	void PreparePuzzle();
	void HandlePuzzleSuccess();
	void HandlePuzzleFailure();
	void NotifyProgressChanged();
	void FinishAndNotify(bool bSucceeded);

	UFUNCTION()
	void HandleBootFinished();

	UFUNCTION()
	void HandleResultHoldFinished();
};
