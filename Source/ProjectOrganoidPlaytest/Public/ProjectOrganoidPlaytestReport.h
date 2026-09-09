#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

enum class EOrganoidPlaytestState : uint8
{
	Queued,
	Running,
	Pass,
	Fail,
	Blocked,
	Aborted
};

struct FOrganoidPlaytestAssertion
{
	FString Id;
	bool bPassed = false;
	FString Expected;
	FString Actual;
	FString ActorId;
	bool bDurableConfig = false;
};

struct FOrganoidPlaytestRecord
{
	FString TestId;
	FString DisplayName;
	FString RunId;
	EOrganoidPlaytestState State = EOrganoidPlaytestState::Queued;
	FString CurrentStage;
	double StartSeconds = 0.0;
	double ElapsedMs = 0.0;
	bool bNeedsApproval = false;
	FString FailureReason;
	FString RecommendedNextAction;
	FString Headline;
	TMap<FString, FString> Actors;
	TArray<FOrganoidPlaytestAssertion> Assertions;
	TArray<FString> Warnings;
	TArray<FString> Errors;

	void AddActor(const FString& Key, const FString& Value);
	FOrganoidPlaytestAssertion& AddAssertion(
		const FString& Id,
		bool bPassed,
		const FString& Expected,
		const FString& Actual,
		const FString& ActorId = FString(),
		bool bDurableConfig = false);
	void MarkNeedsApproval(const FString& Reason, const FString& NextAction);
	void RefreshElapsed();
	void FinalizeHeadline();
	TSharedRef<FJsonObject> ToJson() const;
	TSharedRef<FJsonObject> ToStatusJson() const;
};

const TCHAR* OrganoidPlaytestStateName(EOrganoidPlaytestState State);
bool OrganoidPlaytestStateIsTerminal(EOrganoidPlaytestState State);
