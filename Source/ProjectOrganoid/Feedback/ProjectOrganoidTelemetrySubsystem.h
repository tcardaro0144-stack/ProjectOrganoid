// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ProjectOrganoidTelemetrySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTelemetryEvent, FName, EventType, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidCrashReportWritten, const FString&, FilePath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidPerformanceBottleneck, float, FrameMs, const FString&, Context);

UENUM(BlueprintType)
enum class EProjectOrganoidTelemetrySeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error"),
	Fatal UMETA(DisplayName = "Fatal"),
	Performance UMETA(DisplayName = "Performance"),
	Assertion UMETA(DisplayName = "Assertion")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidTelemetryRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry")
	FDateTime Timestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry")
	EProjectOrganoidTelemetrySeverity Severity = EProjectOrganoidTelemetrySeverity::Info;

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry")
	FName Category = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Telemetry")
	FString Message;
};

/**
 *  End-to-end telemetry + local crash / error reporting.
 *  Captures engine ensures, gameplay errors, and frame-time bottlenecks to Saved/ProjectOrganoid/Telemetry/.
 */
UCLASS()
class UProjectOrganoidTelemetrySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }

	UPROPERTY(BlueprintAssignable, Category = "Telemetry")
	FOnProjectOrganoidTelemetryEvent OnTelemetryEvent;

	UPROPERTY(BlueprintAssignable, Category = "Telemetry")
	FOnProjectOrganoidCrashReportWritten OnCrashReportWritten;

	UPROPERTY(BlueprintAssignable, Category = "Telemetry")
	FOnProjectOrganoidPerformanceBottleneck OnPerformanceBottleneck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "8.0"))
	float BottleneckFrameMs = 33.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "0.1"))
	float BottleneckReportCooldownSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bCaptureEngineErrors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bCaptureEnsures = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "16", ClampMax = "1024"))
	int32 MaxInMemoryRecords = 256;

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void ReportGameplayEvent(FName EventType, const FString& Message, EProjectOrganoidTelemetrySeverity Severity = EProjectOrganoidTelemetrySeverity::Info);

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void ReportGameplayError(const FString& Message, FName Category = TEXT("Gameplay"));

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void ReportPerformanceBottleneck(float FrameMs, const FString& Context = TEXT("Frame"));

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void ReportAssertion(const FString& Expression, const FString& File, int32 Line);

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	FString FlushSessionLogToDisk();

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	FString WriteCrashReport(const FString& Reason);

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	FString GetTelemetryDirectory() const;

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	TArray<FProjectOrganoidTelemetryRecord> GetRecentRecords() const { return RecentRecords; }

protected:

	UPROPERTY()
	TArray<FProjectOrganoidTelemetryRecord> RecentRecords;

	FString SessionId;
	FString SessionLogPath;
	float BottleneckCooldownRemaining = 0.0f;
	bool bOutputDeviceInstalled = false;

	void EnsureTelemetryDirectory() const;
	void AppendRecord(EProjectOrganoidTelemetrySeverity Severity, FName Category, const FString& Message);
	void AppendLineToSessionLog(const FString& Line) const;
	FString SeverityToString(EProjectOrganoidTelemetrySeverity Severity) const;

	void HandleEngineEnsure(const FString& Message);
	FDelegateHandle EnsureHandle;
};
