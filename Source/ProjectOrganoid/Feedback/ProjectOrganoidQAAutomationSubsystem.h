// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "ProjectOrganoidQAAutomationSubsystem.generated.h"

UENUM(BlueprintType)
enum class EProjectOrganoidQATestId : uint8
{
	TerminalHack UMETA(DisplayName = "Interactive Terminal Hack"),
	LaserTripwire UMETA(DisplayName = "Hazard Laser Tripwire"),
	PowerBlackout UMETA(DisplayName = "Power Blackout Transition")
};

UENUM(BlueprintType)
enum class EProjectOrganoidQATestResult : uint8
{
	Pending UMETA(DisplayName = "Pending"),
	Running UMETA(DisplayName = "Running"),
	Passed UMETA(DisplayName = "Passed"),
	Failed UMETA(DisplayName = "Failed"),
	Skipped UMETA(DisplayName = "Skipped")
};

USTRUCT(BlueprintType)
struct FProjectOrganoidQATestReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "QA")
	EProjectOrganoidQATestId TestId = EProjectOrganoidQATestId::TerminalHack;

	UPROPERTY(BlueprintReadOnly, Category = "QA")
	EProjectOrganoidQATestResult Result = EProjectOrganoidQATestResult::Pending;

	UPROPERTY(BlueprintReadOnly, Category = "QA")
	FString Message;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidQATestCompleted, const FProjectOrganoidQATestReport&, Report);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidQASuiteFinished, int32, PassedCount, int32, FailedCount);

/**
 *  Automated QA harness — validates terminals, laser tripwires, and power blackout
 *  transitions without manual playtesting.
 */
UCLASS()
class UProjectOrganoidQAAutomationSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate() && bSuiteRunning; }
	virtual bool IsTickableInEditor() const override { return false; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QA")
	bool bAutoRunOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "QA", meta = (ClampMin = "0.05"))
	float StepDelaySeconds = 0.25f;

	UPROPERTY(BlueprintAssignable, Category = "QA")
	FOnProjectOrganoidQATestCompleted OnQATestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "QA")
	FOnProjectOrganoidQASuiteFinished OnQASuiteFinished;

	UFUNCTION(BlueprintCallable, Category = "QA")
	void RunFullValidationSuite();

	UFUNCTION(BlueprintCallable, Category = "QA")
	FProjectOrganoidQATestReport RunTerminalHackTest();

	UFUNCTION(BlueprintCallable, Category = "QA")
	FProjectOrganoidQATestReport RunLaserTripwireTest();

	UFUNCTION(BlueprintCallable, Category = "QA")
	FProjectOrganoidQATestReport RunPowerBlackoutTest();

	UFUNCTION(BlueprintPure, Category = "QA")
	TArray<FProjectOrganoidQATestReport> GetLastSuiteReports() const { return LastReports; }

	UFUNCTION(BlueprintPure, Category = "QA")
	bool IsSuiteRunning() const { return bSuiteRunning; }

protected:

	UPROPERTY()
	TArray<FProjectOrganoidQATestReport> LastReports;

	bool bSuiteRunning = false;
	int32 SuiteStep = 0;
	float StepTimer = 0.0f;
	int32 PassedCount = 0;
	int32 FailedCount = 0;

	void CompleteReport(FProjectOrganoidQATestReport& Report, EProjectOrganoidQATestResult Result, const FString& Message);
	void AdvanceSuite(float DeltaTime);
};
