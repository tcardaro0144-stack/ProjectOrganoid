// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidTelemetrySubsystem.h"
#include "ProjectOrganoid.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/CoreDelegates.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/OutputDevice.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogProjectOrganoidTelemetry, Log, All);

namespace ProjectOrganoidTelemetry
{
	class FErrorTeeOutputDevice : public FOutputDevice
	{
	public:
		TWeakObjectPtr<UProjectOrganoidTelemetrySubsystem> Owner;

		virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
		{
			if (!Owner.IsValid() || !Owner->bCaptureEngineErrors)
			{
				return;
			}

			if (Verbosity > ELogVerbosity::Warning)
			{
				return;
			}

			const EProjectOrganoidTelemetrySeverity Severity =
				(Verbosity <= ELogVerbosity::Error)
					? EProjectOrganoidTelemetrySeverity::Error
					: EProjectOrganoidTelemetrySeverity::Warning;

			Owner->ReportGameplayEvent(Category, FString(V), Severity);
		}
	};

	static TUniquePtr<FErrorTeeOutputDevice> GErrorTee;
}

void UProjectOrganoidTelemetrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SessionId = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	EnsureTelemetryDirectory();
	SessionLogPath = GetTelemetryDirectory() / FString::Printf(TEXT("session_%s.log"), *SessionId);

	AppendLineToSessionLog(FString::Printf(TEXT("=== ProjectOrganoid telemetry session %s ==="), *SessionId));
	ReportGameplayEvent(TEXT("SessionStart"), TEXT("Telemetry subsystem initialized"));

	if (bCaptureEnsures)
	{
		EnsureHandle = FCoreDelegates::OnHandleSystemError.AddLambda([this]()
		{
			HandleEngineEnsure(TEXT("Engine system error / ensure"));
		});
	}

	if (!ProjectOrganoidTelemetry::GErrorTee.IsValid())
	{
		ProjectOrganoidTelemetry::GErrorTee = MakeUnique<ProjectOrganoidTelemetry::FErrorTeeOutputDevice>();
	}
	ProjectOrganoidTelemetry::GErrorTee->Owner = this;
	if (!bOutputDeviceInstalled)
	{
		GLog->AddOutputDevice(ProjectOrganoidTelemetry::GErrorTee.Get());
		bOutputDeviceInstalled = true;
	}
}

void UProjectOrganoidTelemetrySubsystem::Deinitialize()
{
	FlushSessionLogToDisk();

	if (EnsureHandle.IsValid())
	{
		FCoreDelegates::OnHandleSystemError.Remove(EnsureHandle);
		EnsureHandle.Reset();
	}

	if (bOutputDeviceInstalled && ProjectOrganoidTelemetry::GErrorTee.IsValid())
	{
		GLog->RemoveOutputDevice(ProjectOrganoidTelemetry::GErrorTee.Get());
		ProjectOrganoidTelemetry::GErrorTee->Owner.Reset();
		bOutputDeviceInstalled = false;
	}

	Super::Deinitialize();
}

TStatId UProjectOrganoidTelemetrySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectOrganoidTelemetrySubsystem, STATGROUP_Tickables);
}

void UProjectOrganoidTelemetrySubsystem::Tick(float DeltaTime)
{
	BottleneckCooldownRemaining = FMath::Max(0.0f, BottleneckCooldownRemaining - DeltaTime);

	const float FrameMs = DeltaTime * 1000.0f;
	if (FrameMs >= BottleneckFrameMs && BottleneckCooldownRemaining <= KINDA_SMALL_NUMBER)
	{
		ReportPerformanceBottleneck(FrameMs, TEXT("GameThreadFrame"));
		BottleneckCooldownRemaining = BottleneckReportCooldownSeconds;
	}
}

FString UProjectOrganoidTelemetrySubsystem::GetTelemetryDirectory() const
{
	return FPaths::ProjectSavedDir() / TEXT("ProjectOrganoid") / TEXT("Telemetry");
}

void UProjectOrganoidTelemetrySubsystem::EnsureTelemetryDirectory() const
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*GetTelemetryDirectory());
}

FString UProjectOrganoidTelemetrySubsystem::SeverityToString(EProjectOrganoidTelemetrySeverity Severity) const
{
	switch (Severity)
	{
	case EProjectOrganoidTelemetrySeverity::Warning: return TEXT("Warning");
	case EProjectOrganoidTelemetrySeverity::Error: return TEXT("Error");
	case EProjectOrganoidTelemetrySeverity::Fatal: return TEXT("Fatal");
	case EProjectOrganoidTelemetrySeverity::Performance: return TEXT("Performance");
	case EProjectOrganoidTelemetrySeverity::Assertion: return TEXT("Assertion");
	case EProjectOrganoidTelemetrySeverity::Info:
	default: return TEXT("Info");
	}
}

void UProjectOrganoidTelemetrySubsystem::AppendLineToSessionLog(const FString& Line) const
{
	EnsureTelemetryDirectory();
	FFileHelper::SaveStringToFile(
		Line + LINE_TERMINATOR,
		*SessionLogPath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		FILEWRITE_Append);
}

void UProjectOrganoidTelemetrySubsystem::AppendRecord(
	EProjectOrganoidTelemetrySeverity Severity,
	FName Category,
	const FString& Message)
{
	FProjectOrganoidTelemetryRecord Record;
	Record.Timestamp = FDateTime::Now();
	Record.Severity = Severity;
	Record.Category = Category;
	Record.Message = Message;

	RecentRecords.Add(Record);
	while (RecentRecords.Num() > MaxInMemoryRecords)
	{
		RecentRecords.RemoveAt(0);
	}

	const FString Line = FString::Printf(
		TEXT("[%s][%s][%s] %s"),
		*Record.Timestamp.ToString(TEXT("%H:%M:%S.%s")),
		*SeverityToString(Severity),
		*Category.ToString(),
		*Message);

	AppendLineToSessionLog(Line);
	UE_LOG(LogProjectOrganoidTelemetry, Log, TEXT("%s"), *Line);
	OnTelemetryEvent.Broadcast(Category, Message);
}

void UProjectOrganoidTelemetrySubsystem::ReportGameplayEvent(
	FName EventType,
	const FString& Message,
	EProjectOrganoidTelemetrySeverity Severity)
{
	AppendRecord(Severity, EventType.IsNone() ? TEXT("Event") : EventType, Message);
}

void UProjectOrganoidTelemetrySubsystem::ReportGameplayError(const FString& Message, FName Category)
{
	AppendRecord(EProjectOrganoidTelemetrySeverity::Error, Category, Message);
}

void UProjectOrganoidTelemetrySubsystem::ReportPerformanceBottleneck(float FrameMs, const FString& Context)
{
	const FString Message = FString::Printf(TEXT("%s frame=%.2fms threshold=%.2fms"), *Context, FrameMs, BottleneckFrameMs);
	AppendRecord(EProjectOrganoidTelemetrySeverity::Performance, TEXT("Performance"), Message);
	OnPerformanceBottleneck.Broadcast(FrameMs, Context);
}

void UProjectOrganoidTelemetrySubsystem::ReportAssertion(const FString& Expression, const FString& File, int32 Line)
{
	const FString Message = FString::Printf(TEXT("%s @ %s:%d"), *Expression, *File, Line);
	AppendRecord(EProjectOrganoidTelemetrySeverity::Assertion, TEXT("Assert"), Message);
	WriteCrashReport(Message);
}

void UProjectOrganoidTelemetrySubsystem::HandleEngineEnsure(const FString& Message)
{
	AppendRecord(EProjectOrganoidTelemetrySeverity::Assertion, TEXT("Ensure"), Message);
	WriteCrashReport(Message);
}

FString UProjectOrganoidTelemetrySubsystem::FlushSessionLogToDisk()
{
	EnsureTelemetryDirectory();
	const FString SnapshotPath = GetTelemetryDirectory() / FString::Printf(TEXT("flush_%s.log"), *SessionId);

	FString Body;
	Body += FString::Printf(TEXT("ProjectOrganoid telemetry flush %s\n"), *SessionId);
	for (const FProjectOrganoidTelemetryRecord& Record : RecentRecords)
	{
		Body += FString::Printf(
			TEXT("[%s][%s][%s] %s\n"),
			*Record.Timestamp.ToString(),
			*SeverityToString(Record.Severity),
			*Record.Category.ToString(),
			*Record.Message);
	}

	FFileHelper::SaveStringToFile(Body, *SnapshotPath);
	return SnapshotPath;
}

FString UProjectOrganoidTelemetrySubsystem::WriteCrashReport(const FString& Reason)
{
	EnsureTelemetryDirectory();
	const FString Path = GetTelemetryDirectory() / FString::Printf(
		TEXT("crash_%s_%s.txt"),
		*SessionId,
		*FDateTime::Now().ToString(TEXT("%H%M%S")));

	FString Body;
	Body += TEXT("ProjectOrganoid Crash / Assertion Report\n");
	Body += FString::Printf(TEXT("Session: %s\n"), *SessionId);
	Body += FString::Printf(TEXT("Time: %s\n"), *FDateTime::Now().ToString());
	Body += FString::Printf(TEXT("Reason: %s\n\n"), *Reason);
	Body += TEXT("--- Recent telemetry ---\n");
	const int32 Start = FMath::Max(0, RecentRecords.Num() - 40);
	for (int32 Index = Start; Index < RecentRecords.Num(); ++Index)
	{
		const FProjectOrganoidTelemetryRecord& Record = RecentRecords[Index];
		Body += FString::Printf(
			TEXT("[%s][%s] %s\n"),
			*SeverityToString(Record.Severity),
			*Record.Category.ToString(),
			*Record.Message);
	}

	FFileHelper::SaveStringToFile(Body, *Path);
	OnCrashReportWritten.Broadcast(Path);
	UE_LOG(LogProjectOrganoid, Error, TEXT("Crash report written: %s"), *Path);
	return Path;
}
