#include "ProjectOrganoidPlaytestReport.h"
#include "HAL/PlatformTime.h"

namespace
{
	TArray<TSharedPtr<FJsonValue>> StringArray(const TArray<FString>& Values)
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FString& Value : Values)
		{
			Out.Add(MakeShared<FJsonValueString>(Value));
		}
		return Out;
	}
}

const TCHAR* OrganoidPlaytestStateName(EOrganoidPlaytestState State)
{
	switch (State)
	{
	case EOrganoidPlaytestState::Queued: return TEXT("queued");
	case EOrganoidPlaytestState::Running: return TEXT("running");
	case EOrganoidPlaytestState::Pass: return TEXT("pass");
	case EOrganoidPlaytestState::Fail: return TEXT("fail");
	case EOrganoidPlaytestState::Blocked: return TEXT("blocked");
	case EOrganoidPlaytestState::Aborted: return TEXT("aborted");
	default: return TEXT("unknown");
	}
}

bool OrganoidPlaytestStateIsTerminal(EOrganoidPlaytestState State)
{
	return State == EOrganoidPlaytestState::Pass
		|| State == EOrganoidPlaytestState::Fail
		|| State == EOrganoidPlaytestState::Blocked
		|| State == EOrganoidPlaytestState::Aborted;
}

void FOrganoidPlaytestRecord::AddActor(const FString& Key, const FString& Value)
{
	Actors.Add(Key, Value);
}

FOrganoidPlaytestAssertion& FOrganoidPlaytestRecord::AddAssertion(
	const FString& Id,
	bool bPassed,
	const FString& Expected,
	const FString& Actual,
	const FString& ActorId,
	bool bDurableConfig)
{
	FOrganoidPlaytestAssertion Assertion;
	Assertion.Id = Id;
	Assertion.bPassed = bPassed;
	Assertion.Expected = Expected;
	Assertion.Actual = Actual;
	Assertion.ActorId = ActorId;
	Assertion.bDurableConfig = bDurableConfig;
	const int32 Index = Assertions.Add(Assertion);
	if (!bPassed && FailureReason.IsEmpty())
	{
		FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
	}
	if (!bPassed && bDurableConfig)
	{
		bNeedsApproval = true;
	}
	return Assertions[Index];
}

void FOrganoidPlaytestRecord::MarkNeedsApproval(const FString& Reason, const FString& NextAction)
{
	bNeedsApproval = true;
	if (FailureReason.IsEmpty())
	{
		FailureReason = Reason;
	}
	RecommendedNextAction = NextAction;
}

void FOrganoidPlaytestRecord::RefreshElapsed()
{
	if (StartSeconds > 0.0)
	{
		ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
	}
}

void FOrganoidPlaytestRecord::FinalizeHeadline()
{
	RefreshElapsed();
	const FString StateName = FString(OrganoidPlaytestStateName(State)).ToUpper();
	if (State == EOrganoidPlaytestState::Pass)
	{
		Headline = FString::Printf(TEXT("%s — PASS"), *TestId);
	}
	else if (FailureReason.Len() > 0)
	{
		Headline = FString::Printf(TEXT("%s — %s — %s"), *TestId, *StateName, *FailureReason);
	}
	else
	{
		Headline = FString::Printf(TEXT("%s — %s"), *TestId, *StateName);
	}

	if (bNeedsApproval && RecommendedNextAction.IsEmpty())
	{
		RecommendedNextAction = TEXT(
			"Durable project mutation required. Use OrganoidAIBridge prepare_write, dual approve_write, then execute_write. The playtest bot will not mutate.");
	}
}

TSharedRef<FJsonObject> FOrganoidPlaytestRecord::ToJson() const
{
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("test_id"), TestId);
	Data->SetStringField(TEXT("display_name"), DisplayName);
	Data->SetStringField(TEXT("run_id"), RunId);
	Data->SetStringField(TEXT("state"), OrganoidPlaytestStateName(State));
	Data->SetStringField(TEXT("current_stage"), CurrentStage);
	Data->SetNumberField(TEXT("elapsed_ms"), ElapsedMs);
	Data->SetBoolField(TEXT("needs_approval"), bNeedsApproval);
	if (FailureReason.IsEmpty())
	{
		Data->SetField(TEXT("failure_reason"), MakeShared<FJsonValueNull>());
	}
	else
	{
		Data->SetStringField(TEXT("failure_reason"), FailureReason);
	}
	if (RecommendedNextAction.IsEmpty())
	{
		Data->SetField(TEXT("recommended_next_action"), MakeShared<FJsonValueNull>());
	}
	else
	{
		Data->SetStringField(TEXT("recommended_next_action"), RecommendedNextAction);
	}
	Data->SetStringField(TEXT("headline"), Headline);

	TSharedRef<FJsonObject> ActorsJson = MakeShared<FJsonObject>();
	for (const TPair<FString, FString>& Pair : Actors)
	{
		ActorsJson->SetStringField(Pair.Key, Pair.Value);
	}
	Data->SetObjectField(TEXT("actors"), ActorsJson);

	TArray<TSharedPtr<FJsonValue>> AssertionJson;
	for (const FOrganoidPlaytestAssertion& Assertion : Assertions)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("id"), Assertion.Id);
		Entry->SetBoolField(TEXT("passed"), Assertion.bPassed);
		Entry->SetStringField(TEXT("expected"), Assertion.Expected);
		Entry->SetStringField(TEXT("actual"), Assertion.Actual);
		if (Assertion.ActorId.IsEmpty())
		{
			Entry->SetField(TEXT("actor_id"), MakeShared<FJsonValueNull>());
		}
		else
		{
			Entry->SetStringField(TEXT("actor_id"), Assertion.ActorId);
		}
		Entry->SetBoolField(TEXT("durable_config"), Assertion.bDurableConfig);
		AssertionJson.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Data->SetArrayField(TEXT("assertions"), AssertionJson);
	Data->SetArrayField(TEXT("warnings"), StringArray(Warnings));
	Data->SetArrayField(TEXT("errors"), StringArray(Errors));
	return Data;
}

TSharedRef<FJsonObject> FOrganoidPlaytestRecord::ToStatusJson() const
{
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("test_id"), TestId);
	Data->SetStringField(TEXT("run_id"), RunId);
	Data->SetStringField(TEXT("state"), OrganoidPlaytestStateName(State));
	Data->SetStringField(TEXT("current_stage"), CurrentStage);
	Data->SetNumberField(TEXT("elapsed_ms"), ElapsedMs);
	Data->SetBoolField(TEXT("needs_approval"), bNeedsApproval);
	Data->SetStringField(TEXT("headline"), Headline);
	if (FailureReason.IsEmpty())
	{
		Data->SetField(TEXT("failure_reason"), MakeShared<FJsonValueNull>());
	}
	else
	{
		Data->SetStringField(TEXT("failure_reason"), FailureReason);
	}
	return Data;
}
