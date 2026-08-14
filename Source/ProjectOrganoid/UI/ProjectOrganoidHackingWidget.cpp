// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHackingWidget.h"
#include "ProjectOrganoidTerminal.h"
#include "ProjectOrganoidCharacter.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UProjectOrganoidHackingWidget::BindToTerminal(
	AProjectOrganoidTerminal* InTerminal,
	AProjectOrganoidCharacter* InCharacter,
	const FProjectOrganoidHackingSessionConfig& InConfig)
{
	BoundTerminal = InTerminal;
	BoundCharacter = InCharacter;
	SessionConfig = InConfig;
	AttemptsRemaining = FMath::Max(1, SessionConfig.MaxAttempts);
	DecryptStepsCompleted = 0;
	PlayerNodeSequence.Reset();
	TargetNodeSequence.Reset();
	ScrambledPassword.Reset();
	RevealedPassword.Reset();
	CurrentState = EProjectOrganoidHackingUIState::Idle;

	OnHackingUIOpened(SessionConfig.MiniGame);
	BeginHackingSession();
}

void UProjectOrganoidHackingWidget::UnbindFromTerminal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BootTimerHandle);
		World->GetTimerManager().ClearTimer(ResultTimerHandle);
	}

	BoundTerminal = nullptr;
	BoundCharacter = nullptr;
	SetHackingState(EProjectOrganoidHackingUIState::Idle);
}

void UProjectOrganoidHackingWidget::BeginHackingSession()
{
	if (CurrentState != EProjectOrganoidHackingUIState::Idle
		&& CurrentState != EProjectOrganoidHackingUIState::Failed
		&& CurrentState != EProjectOrganoidHackingUIState::Closing)
	{
		return;
	}

	AttemptsRemaining = FMath::Max(1, SessionConfig.MaxAttempts);
	DecryptStepsCompleted = 0;
	PlayerNodeSequence.Reset();
	PreparePuzzle();
	SetHackingState(EProjectOrganoidHackingUIState::Booting);
	OnHackingBoardNeedsRefresh();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BootTimerHandle);
		if (SessionConfig.BootDurationSeconds <= KINDA_SMALL_NUMBER)
		{
			HandleBootFinished();
		}
		else
		{
			World->GetTimerManager().SetTimer(
				BootTimerHandle,
				this,
				&UProjectOrganoidHackingWidget::HandleBootFinished,
				SessionConfig.BootDurationSeconds,
				false);
		}
	}
	else
	{
		HandleBootFinished();
	}
}

void UProjectOrganoidHackingWidget::AbortHackingSession()
{
	if (CurrentState == EProjectOrganoidHackingUIState::Closing
		|| CurrentState == EProjectOrganoidHackingUIState::Idle)
	{
		return;
	}

	FinishAndNotify(false);
}

void UProjectOrganoidHackingWidget::CloseHackingUI()
{
	if (CurrentState != EProjectOrganoidHackingUIState::Closing)
	{
		SetHackingState(EProjectOrganoidHackingUIState::Closing);
		OnHackingUIClosed();
	}

	RemoveFromParent();
	UnbindFromTerminal();
}

bool UProjectOrganoidHackingWidget::SubmitNodeSelection(int32 NodeIndex)
{
	if (CurrentState != EProjectOrganoidHackingUIState::Playing
		|| SessionConfig.MiniGame != EProjectOrganoidHackingMiniGame::NodeMatch)
	{
		return false;
	}

	if (NodeIndex < 0 || NodeIndex >= SessionConfig.NodePoolSize)
	{
		return false;
	}

	const int32 ExpectedIndex = PlayerNodeSequence.Num();
	if (!TargetNodeSequence.IsValidIndex(ExpectedIndex))
	{
		return false;
	}

	if (TargetNodeSequence[ExpectedIndex] != NodeIndex)
	{
		PlayerNodeSequence.Reset();
		--AttemptsRemaining;
		NotifyProgressChanged();
		OnHackingBoardNeedsRefresh();

		if (AttemptsRemaining <= 0)
		{
			HandlePuzzleFailure();
		}
		return false;
	}

	PlayerNodeSequence.Add(NodeIndex);
	NotifyProgressChanged();
	OnHackingBoardNeedsRefresh();

	if (PlayerNodeSequence.Num() >= TargetNodeSequence.Num())
	{
		HandlePuzzleSuccess();
	}

	return true;
}

bool UProjectOrganoidHackingWidget::SubmitDecryptKey(FString Key)
{
	if (CurrentState != EProjectOrganoidHackingUIState::Playing
		|| SessionConfig.MiniGame != EProjectOrganoidHackingMiniGame::PasswordDecrypt)
	{
		return false;
	}

	Key.TrimStartAndEndInline();
	if (Key.IsEmpty())
	{
		return false;
	}

	const FString Target = SessionConfig.TargetPassword.ToUpper();
	const FString Guess = Key.ToUpper();

	const int32 RevealIndex = DecryptStepsCompleted;
	const bool bCharMatch = Target.IsValidIndex(RevealIndex)
		&& Guess.Len() > 0
		&& Target[RevealIndex] == Guess[0];
	const bool bFullMatch = Guess == Target;

	if (!bCharMatch && !bFullMatch)
	{
		--AttemptsRemaining;
		NotifyProgressChanged();
		OnHackingBoardNeedsRefresh();

		if (AttemptsRemaining <= 0)
		{
			HandlePuzzleFailure();
		}
		return false;
	}

	if (bFullMatch)
	{
		DecryptStepsCompleted = SessionConfig.DecryptStepsRequired;
		RevealedPassword = Target;
	}
	else
	{
		++DecryptStepsCompleted;
		RevealedPassword.Reset();
		for (int32 i = 0; i < Target.Len(); ++i)
		{
			RevealedPassword.AppendChar(i < DecryptStepsCompleted ? Target[i] : TEXT('*'));
		}
	}

	NotifyProgressChanged();
	OnHackingBoardNeedsRefresh();

	if (DecryptStepsCompleted >= SessionConfig.DecryptStepsRequired || RevealedPassword == Target)
	{
		HandlePuzzleSuccess();
	}

	return true;
}

void UProjectOrganoidHackingWidget::ForceSucceed()
{
	if (CurrentState == EProjectOrganoidHackingUIState::Playing
		|| CurrentState == EProjectOrganoidHackingUIState::Booting)
	{
		HandlePuzzleSuccess();
	}
}

void UProjectOrganoidHackingWidget::ForceFail()
{
	if (CurrentState == EProjectOrganoidHackingUIState::Playing
		|| CurrentState == EProjectOrganoidHackingUIState::Booting)
	{
		HandlePuzzleFailure();
	}
}

float UProjectOrganoidHackingWidget::GetProgressNormalized() const
{
	switch (SessionConfig.MiniGame)
	{
	case EProjectOrganoidHackingMiniGame::NodeMatch:
		return TargetNodeSequence.Num() > 0
			? static_cast<float>(PlayerNodeSequence.Num()) / static_cast<float>(TargetNodeSequence.Num())
			: 0.0f;
	case EProjectOrganoidHackingMiniGame::PasswordDecrypt:
		return SessionConfig.DecryptStepsRequired > 0
			? static_cast<float>(DecryptStepsCompleted) / static_cast<float>(SessionConfig.DecryptStepsRequired)
			: 0.0f;
	default:
		return 0.0f;
	}
}

void UProjectOrganoidHackingWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BootTimerHandle);
		World->GetTimerManager().ClearTimer(ResultTimerHandle);
	}
	Super::NativeDestruct();
}

void UProjectOrganoidHackingWidget::SetHackingState(EProjectOrganoidHackingUIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	const EProjectOrganoidHackingUIState Previous = CurrentState;
	CurrentState = NewState;
	OnHackingStateChanged.Broadcast(NewState, Previous);
	BP_OnHackingStateChanged(NewState, Previous);
}

void UProjectOrganoidHackingWidget::PreparePuzzle()
{
	TargetNodeSequence.Reset();
	PlayerNodeSequence.Reset();
	DecryptStepsCompleted = 0;

	if (SessionConfig.MiniGame == EProjectOrganoidHackingMiniGame::NodeMatch)
	{
		const int32 Pool = FMath::Max(3, SessionConfig.NodePoolSize);
		const int32 Length = FMath::Clamp(SessionConfig.NodeSequenceLength, 2, Pool);
		TargetNodeSequence.Reserve(Length);
		for (int32 i = 0; i < Length; ++i)
		{
			TargetNodeSequence.Add(FMath::RandRange(0, Pool - 1));
		}
	}
	else
	{
		FString Target = SessionConfig.TargetPassword.ToUpper();
		if (Target.IsEmpty())
		{
			Target = TEXT("EPITOPE");
			SessionConfig.TargetPassword = Target;
		}

		TArray<TCHAR> Chars;
		Chars.Reserve(Target.Len());
		for (int32 i = 0; i < Target.Len(); ++i)
		{
			Chars.Add(Target[i]);
		}
		for (int32 i = Chars.Num() - 1; i > 0; --i)
		{
			const int32 j = FMath::RandRange(0, i);
			Chars.Swap(i, j);
		}
		ScrambledPassword.Reset();
		for (const TCHAR C : Chars)
		{
			ScrambledPassword.AppendChar(C);
		}

		RevealedPassword.Reset();
		for (int32 i = 0; i < Target.Len(); ++i)
		{
			RevealedPassword.AppendChar(TEXT('*'));
		}
	}
}

void UProjectOrganoidHackingWidget::HandleBootFinished()
{
	if (CurrentState != EProjectOrganoidHackingUIState::Booting)
	{
		return;
	}

	SetHackingState(EProjectOrganoidHackingUIState::Playing);
	NotifyProgressChanged();
	OnHackingBoardNeedsRefresh();
}

void UProjectOrganoidHackingWidget::HandlePuzzleSuccess()
{
	SetHackingState(EProjectOrganoidHackingUIState::Success);
	NotifyProgressChanged();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResultTimerHandle);
		World->GetTimerManager().SetTimer(
			ResultTimerHandle,
			this,
			&UProjectOrganoidHackingWidget::HandleResultHoldFinished,
			FMath::Max(0.05f, SessionConfig.ResultHoldSeconds),
			false);
	}
	else
	{
		FinishAndNotify(true);
	}
}

void UProjectOrganoidHackingWidget::HandlePuzzleFailure()
{
	SetHackingState(EProjectOrganoidHackingUIState::Failed);
	NotifyProgressChanged();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResultTimerHandle);
		World->GetTimerManager().SetTimer(
			ResultTimerHandle,
			this,
			&UProjectOrganoidHackingWidget::HandleResultHoldFinished,
			FMath::Max(0.05f, SessionConfig.ResultHoldSeconds),
			false);
	}
	else
	{
		FinishAndNotify(false);
	}
}

void UProjectOrganoidHackingWidget::HandleResultHoldFinished()
{
	const bool bSucceeded = CurrentState == EProjectOrganoidHackingUIState::Success;
	FinishAndNotify(bSucceeded);
}

void UProjectOrganoidHackingWidget::NotifyProgressChanged()
{
	OnHackingProgressChanged.Broadcast(GetProgressNormalized());
}

void UProjectOrganoidHackingWidget::FinishAndNotify(bool bSucceeded)
{
	OnHackingFinished.Broadcast(bSucceeded);

	if (AProjectOrganoidTerminal* Terminal = BoundTerminal)
	{
		Terminal->NotifyHackingFinished(bSucceeded);
	}
}
