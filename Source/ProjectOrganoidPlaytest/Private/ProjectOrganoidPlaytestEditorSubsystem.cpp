#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestApi.h"
#include "ProjectOrganoidPlaytestLogSink.h"
#include "ProjectOrganoidPlaytestRegistry.h"
#include "OrganoidAIBridgePlaytestHost.h"
#include "Editor.h"
#include "PlayInEditorDataTypes.h"
#include "HAL/PlatformTime.h"
#include "Misc/Guid.h"
#include "Misc/OutputDeviceRedirector.h"
#include "UObject/Package.h"

namespace
{
	TSharedRef<FJsonObject> Ok(const TSharedRef<FJsonObject>& Data)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), true);
		Root->SetObjectField(TEXT("data"), Data);
		return Root;
	}

	TSharedRef<FJsonObject> Fail(const FString& Code, const FString& Message)
	{
		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetBoolField(TEXT("ok"), false);
		Root->SetStringField(TEXT("error_code"), Code);
		Root->SetStringField(TEXT("error"), Message);
		return Root;
	}

	FString NewRunId()
	{
		return FString::Printf(TEXT("ptr_%s"), *FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower());
	}
}

void UProjectOrganoidPlaytestEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OrganoidAIBridgePlaytest::RegisterHost(this);
}

void UProjectOrganoidPlaytestEditorSubsystem::Deinitialize()
{
	if (OrganoidAIBridgePlaytest::GetHost() == this)
	{
		OrganoidAIBridgePlaytest::RegisterHost(nullptr);
	}
	if (ActiveCase.IsValid())
	{
		ActiveCase->Abort(*this);
		ActiveCase.Reset();
	}
	RequestEndPieIfStarted();
	DetachLogSink();
	ActiveRecord.Reset();
	Super::Deinitialize();
}

TStatId UProjectOrganoidPlaytestEditorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectOrganoidPlaytestEditorSubsystem, STATGROUP_Tickables);
}

bool UProjectOrganoidPlaytestEditorSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject)
		&& ActiveRecord.IsValid()
		&& !OrganoidPlaytestStateIsTerminal(ActiveRecord->State);
}

void UProjectOrganoidPlaytestEditorSubsystem::Tick(float DeltaTime)
{
	if (!ActiveRecord.IsValid() || OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		return;
	}

	ActiveRecord->RefreshElapsed();
	if (ActiveRecord->State == EOrganoidPlaytestState::Queued)
	{
		ActiveRecord->State = EOrganoidPlaytestState::Running;
		SetStage(TEXT("Start"));
		if (ActiveCase.IsValid())
		{
			ActiveCase->Start(*this);
		}
	}

	if (bStopRequested && ActiveRecord.IsValid() && !OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		if (ActiveCase.IsValid())
		{
			ActiveCase->Abort(*this);
		}
		CompleteActive(EOrganoidPlaytestState::Aborted, TEXT("stop_playtest"));
		return;
	}

	if (ActiveCase.IsValid() && ActiveRecord.IsValid() && !OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		ActiveCase->Tick(*this, DeltaTime);
	}

	if (ActiveRecord.IsValid() && OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		ActiveCase.Reset();
	}
}

TSharedRef<FJsonObject> UProjectOrganoidPlaytestEditorSubsystem::ListPlaytestsJson() const
{
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Tests;
	for (const FOrganoidPlaytestCatalogEntry& Entry : FOrganoidPlaytestRegistry::GetEntries())
	{
		TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("test_id"), Entry.TestId);
		Item->SetStringField(TEXT("display_name"), Entry.DisplayName);
		Item->SetStringField(TEXT("map"), Entry.MapPackage);
		Item->SetBoolField(TEXT("requires_pie"), true);
		Item->SetBoolField(TEXT("mutates_assets"), false);
		Tests.Add(MakeShared<FJsonValueObject>(Item));
	}
	Data->SetArrayField(TEXT("tests"), Tests);
	if (ActiveRecord.IsValid() && !OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		Data->SetStringField(TEXT("active_run_id"), ActiveRecord->RunId);
	}
	else
	{
		Data->SetField(TEXT("active_run_id"), MakeShared<FJsonValueNull>());
	}
	return Ok(Data);
}

TSharedRef<FJsonObject> UProjectOrganoidPlaytestEditorSubsystem::RequestRun(const FString& TestId)
{
	if (ActiveRecord.IsValid() && !OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		return Fail(TEXT("already_running"), FString::Printf(
			TEXT("Playtest %s is already %s (run_id=%s)."),
			*ActiveRecord->TestId,
			OrganoidPlaytestStateName(ActiveRecord->State),
			*ActiveRecord->RunId));
	}

	const FOrganoidPlaytestCatalogEntry* Entry = FOrganoidPlaytestRegistry::Find(TestId);
	if (!Entry)
	{
		return Fail(TEXT("unknown_test"), FString::Printf(TEXT("No registered playtest '%s'."), *TestId));
	}

	TSharedPtr<IOrganoidPlaytestCase> Case = FOrganoidPlaytestRegistry::Create(TestId);
	if (!Case.IsValid())
	{
		return Fail(TEXT("create_failed"), FString::Printf(TEXT("Failed to create playtest '%s'."), *TestId));
	}

	TSharedPtr<FOrganoidPlaytestRecord> Record = MakeShared<FOrganoidPlaytestRecord>();
	Record->TestId = Entry->TestId;
	Record->DisplayName = Entry->DisplayName;
	Record->RunId = NewRunId();
	Record->State = EOrganoidPlaytestState::Queued;
	Record->CurrentStage = TEXT("queued");
	Record->StartSeconds = FPlatformTime::Seconds();
	Record->Headline = FString::Printf(TEXT("%s — QUEUED"), *Record->TestId);

	ArchiveActive();
	ActiveRecord = Record;
	ActiveCase = Case;
	bStopRequested = false;
	bStartedPieOurselves = false;
	AttachLogSink();

	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("run_id"), Record->RunId);
	Data->SetStringField(TEXT("test_id"), Record->TestId);
	Data->SetStringField(TEXT("state"), TEXT("queued"));
	Data->SetStringField(TEXT("current_stage"), Record->CurrentStage);
	return Ok(Data);
}

TSharedRef<FJsonObject> UProjectOrganoidPlaytestEditorSubsystem::GetStatusJson(const FString& RunId) const
{
	const FOrganoidPlaytestRecord* Record = FindRecord(RunId);
	if (!Record)
	{
		return Fail(TEXT("not_found"), TEXT("Unknown playtest run_id."));
	}
	return Ok(Record->ToStatusJson());
}

TSharedRef<FJsonObject> UProjectOrganoidPlaytestEditorSubsystem::GetResultJson(const FString& RunId) const
{
	const FOrganoidPlaytestRecord* Record = FindRecord(RunId);
	if (!Record)
	{
		return Fail(TEXT("not_found"), TEXT("Unknown playtest run_id."));
	}
	return Ok(Record->ToJson());
}

TSharedRef<FJsonObject> UProjectOrganoidPlaytestEditorSubsystem::RequestStop(const FString& RunId)
{
	if (!ActiveRecord.IsValid())
	{
		return Fail(TEXT("not_found"), TEXT("No active playtest run."));
	}
	if (!RunId.IsEmpty() && !ActiveRecord->RunId.Equals(RunId, ESearchCase::IgnoreCase))
	{
		return Fail(TEXT("run_mismatch"), TEXT("run_id does not match the active playtest."));
	}
	if (OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		return Ok(ActiveRecord->ToJson());
	}
	bStopRequested = true;
	TSharedRef<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("run_id"), ActiveRecord->RunId);
	Data->SetStringField(TEXT("state"), TEXT("running"));
	Data->SetStringField(TEXT("current_stage"), TEXT("stopping"));
	return Ok(Data);
}

void UProjectOrganoidPlaytestEditorSubsystem::SetStage(const FString& Stage)
{
	if (ActiveRecord.IsValid())
	{
		ActiveRecord->CurrentStage = Stage;
		ActiveRecord->RefreshElapsed();
	}
}

void UProjectOrganoidPlaytestEditorSubsystem::CompleteActive(EOrganoidPlaytestState State, const FString& Reason)
{
	if (!ActiveRecord.IsValid() || OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		return;
	}
	ActiveRecord->State = State;
	if (!Reason.IsEmpty() && ActiveRecord->FailureReason.IsEmpty())
	{
		ActiveRecord->FailureReason = Reason;
	}
	if (LogSink)
	{
		ActiveRecord->Errors = LogSink->Errors;
		ActiveRecord->Warnings = LogSink->Warnings;
	}
	ActiveRecord->FinalizeHeadline();
	RequestEndPieIfStarted();
	DetachLogSink();
	ArchiveActive();
}

bool UProjectOrganoidPlaytestEditorSubsystem::RequestStartPie(const FString& MapPackage)
{
	if (!GEditor)
	{
		return false;
	}
	if (GEditor->IsPlaySessionInProgress())
	{
		return false;
	}
	FRequestPlaySessionParams Params;
	Params.WorldType = EPlaySessionWorldType::PlayInEditor;
	Params.SessionDestination = EPlaySessionDestinationType::InProcess;
	Params.GlobalMapOverride = MapPackage;
	GEditor->RequestPlaySession(Params);
	bStartedPieOurselves = true;
	return true;
}

void UProjectOrganoidPlaytestEditorSubsystem::RequestEndPieIfStarted()
{
	if (!bStartedPieOurselves || !GEditor)
	{
		return;
	}
	if (GEditor->IsPlaySessionInProgress())
	{
		GEditor->RequestEndPlayMap();
	}
	bStartedPieOurselves = false;
}

const FOrganoidPlaytestRecord* UProjectOrganoidPlaytestEditorSubsystem::FindRecord(const FString& RunId) const
{
	if (ActiveRecord.IsValid() && (RunId.IsEmpty() || ActiveRecord->RunId.Equals(RunId, ESearchCase::IgnoreCase)))
	{
		return ActiveRecord.Get();
	}
	for (const TSharedPtr<FOrganoidPlaytestRecord>& Record : History)
	{
		if (Record.IsValid() && Record->RunId.Equals(RunId, ESearchCase::IgnoreCase))
		{
			return Record.Get();
		}
	}
	return nullptr;
}

void UProjectOrganoidPlaytestEditorSubsystem::AttachLogSink()
{
	DetachLogSink();
	LogSink = new FOrganoidPlaytestLogSink();
	if (GLog)
	{
		GLog->AddOutputDevice(LogSink);
	}
}

void UProjectOrganoidPlaytestEditorSubsystem::DetachLogSink()
{
	if (LogSink && GLog)
	{
		GLog->RemoveOutputDevice(LogSink);
	}
	delete LogSink;
	LogSink = nullptr;
}

void UProjectOrganoidPlaytestEditorSubsystem::ArchiveActive()
{
	if (!ActiveRecord.IsValid() || !OrganoidPlaytestStateIsTerminal(ActiveRecord->State))
	{
		return;
	}
	for (const TSharedPtr<FOrganoidPlaytestRecord>& Existing : History)
	{
		if (Existing == ActiveRecord)
		{
			return;
		}
	}
	History.Insert(ActiveRecord, 0);
	constexpr int32 MaxHistory = 8;
	if (History.Num() > MaxHistory)
	{
		History.SetNum(MaxHistory);
	}
}

TSharedRef<FJsonObject> OrganoidPlaytestApi::ListPlaytests()
{
	if (GEditor)
	{
		if (UProjectOrganoidPlaytestEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UProjectOrganoidPlaytestEditorSubsystem>())
		{
			return Sub->ListPlaytestsJson();
		}
	}
	return Fail(TEXT("playtest_unavailable"), TEXT("Playtest editor subsystem is not available."));
}

TSharedRef<FJsonObject> OrganoidPlaytestApi::RunPlaytest(const FString& TestId)
{
	if (TestId.IsEmpty())
	{
		return Fail(TEXT("bad_args"), TEXT("test_id is required."));
	}
	if (GEditor)
	{
		if (UProjectOrganoidPlaytestEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UProjectOrganoidPlaytestEditorSubsystem>())
		{
			return Sub->RequestRun(TestId);
		}
	}
	return Fail(TEXT("playtest_unavailable"), TEXT("Playtest editor subsystem is not available."));
}

TSharedRef<FJsonObject> OrganoidPlaytestApi::GetPlaytestStatus(const FString& RunId)
{
	if (GEditor)
	{
		if (UProjectOrganoidPlaytestEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UProjectOrganoidPlaytestEditorSubsystem>())
		{
			return Sub->GetStatusJson(RunId);
		}
	}
	return Fail(TEXT("playtest_unavailable"), TEXT("Playtest editor subsystem is not available."));
}

TSharedRef<FJsonObject> OrganoidPlaytestApi::GetPlaytestResult(const FString& RunId)
{
	if (GEditor)
	{
		if (UProjectOrganoidPlaytestEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UProjectOrganoidPlaytestEditorSubsystem>())
		{
			return Sub->GetResultJson(RunId);
		}
	}
	return Fail(TEXT("playtest_unavailable"), TEXT("Playtest editor subsystem is not available."));
}

TSharedRef<FJsonObject> OrganoidPlaytestApi::StopPlaytest(const FString& RunId)
{
	if (GEditor)
	{
		if (UProjectOrganoidPlaytestEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UProjectOrganoidPlaytestEditorSubsystem>())
		{
			return Sub->RequestStop(RunId);
		}
	}
	return Fail(TEXT("playtest_unavailable"), TEXT("Playtest editor subsystem is not available."));
}
