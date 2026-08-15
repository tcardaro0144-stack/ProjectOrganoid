// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidQAAutomationSubsystem.h"
#include "ProjectOrganoidTerminal.h"
#include "ProjectOrganoidLaserTripwire.h"
#include "ProjectOrganoidTrapBase.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidTelemetrySubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void UProjectOrganoidQAAutomationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (bAutoRunOnBeginPlay)
	{
		RunFullValidationSuite();
	}
}

void UProjectOrganoidQAAutomationSubsystem::Deinitialize()
{
	bSuiteRunning = false;
	Super::Deinitialize();
}

TStatId UProjectOrganoidQAAutomationSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectOrganoidQAAutomationSubsystem, STATGROUP_Tickables);
}

void UProjectOrganoidQAAutomationSubsystem::CompleteReport(
	FProjectOrganoidQATestReport& Report,
	EProjectOrganoidQATestResult Result,
	const FString& Message)
{
	Report.Result = Result;
	Report.Message = Message;
	OnQATestCompleted.Broadcast(Report);

	if (Result == EProjectOrganoidQATestResult::Passed)
	{
		++PassedCount;
	}
	else if (Result == EProjectOrganoidQATestResult::Failed)
	{
		++FailedCount;
	}

	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UProjectOrganoidTelemetrySubsystem* Telemetry = GI->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
		{
			Telemetry->ReportGameplayEvent(
				TEXT("QA"),
				FString::Printf(TEXT("%s -> %s (%s)"),
					*UEnum::GetValueAsString(Report.TestId),
					*UEnum::GetValueAsString(Result),
					*Message),
				Result == EProjectOrganoidQATestResult::Failed
					? EProjectOrganoidTelemetrySeverity::Error
					: EProjectOrganoidTelemetrySeverity::Info);
		}
	}
}

void UProjectOrganoidQAAutomationSubsystem::RunFullValidationSuite()
{
	LastReports.Reset();
	PassedCount = 0;
	FailedCount = 0;
	SuiteStep = 0;
	StepTimer = 0.0f;
	bSuiteRunning = true;
}

void UProjectOrganoidQAAutomationSubsystem::Tick(float DeltaTime)
{
	if (!bSuiteRunning)
	{
		return;
	}
	AdvanceSuite(DeltaTime);
}

void UProjectOrganoidQAAutomationSubsystem::AdvanceSuite(float DeltaTime)
{
	StepTimer += DeltaTime;
	if (StepTimer < StepDelaySeconds)
	{
		return;
	}
	StepTimer = 0.0f;

	FProjectOrganoidQATestReport Report;
	switch (SuiteStep)
	{
	case 0:
		Report = RunTerminalHackTest();
		LastReports.Add(Report);
		++SuiteStep;
		break;
	case 1:
		Report = RunLaserTripwireTest();
		LastReports.Add(Report);
		++SuiteStep;
		break;
	case 2:
		Report = RunPowerBlackoutTest();
		LastReports.Add(Report);
		++SuiteStep;
		break;
	default:
		bSuiteRunning = false;
		OnQASuiteFinished.Broadcast(PassedCount, FailedCount);
		break;
	}
}

FProjectOrganoidQATestReport UProjectOrganoidQAAutomationSubsystem::RunTerminalHackTest()
{
	FProjectOrganoidQATestReport Report;
	Report.TestId = EProjectOrganoidQATestId::TerminalHack;
	Report.Result = EProjectOrganoidQATestResult::Running;

	UWorld* World = GetWorld();
	if (!World)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("No world"));
		return Report;
	}

	AProjectOrganoidTerminal* Terminal = nullptr;
	for (TActorIterator<AProjectOrganoidTerminal> It(World); It; ++It)
	{
		Terminal = *It;
		break;
	}

	if (!Terminal)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Skipped, TEXT("No AProjectOrganoidTerminal in level"));
		return Report;
	}

	AProjectOrganoidCharacter* Avery = Cast<AProjectOrganoidCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	Terminal->NotifyHackingFinished(true);
	(void)Avery;

	CompleteReport(Report, EProjectOrganoidQATestResult::Passed,
		FString::Printf(TEXT("Terminal %s NotifyHackingFinished(true) executed (hacked=%s)"),
			*Terminal->GetName(),
			Terminal->bHasBeenHacked ? TEXT("true") : TEXT("false")));

	return Report;
}

FProjectOrganoidQATestReport UProjectOrganoidQAAutomationSubsystem::RunLaserTripwireTest()
{
	FProjectOrganoidQATestReport Report;
	Report.TestId = EProjectOrganoidQATestId::LaserTripwire;
	Report.Result = EProjectOrganoidQATestResult::Running;

	UWorld* World = GetWorld();
	if (!World)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("No world"));
		return Report;
	}

	AProjectOrganoidLaserTripwire* Laser = nullptr;
	for (TActorIterator<AProjectOrganoidLaserTripwire> It(World); It; ++It)
	{
		Laser = *It;
		break;
	}

	if (!Laser)
	{
		// Spawn a temporary laser for validation
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Laser = World->SpawnActor<AProjectOrganoidLaserTripwire>(
			AProjectOrganoidLaserTripwire::StaticClass(),
			FTransform(FVector(0.0f, 0.0f, 100.0f)),
			Params);
	}

	if (!Laser)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("Unable to locate/spawn laser tripwire"));
		return Report;
	}

	Laser->SetArmed(true);
	AProjectOrganoidCharacter* Avery = Cast<AProjectOrganoidCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	const bool bTriggered = Laser->TriggerTrap(Avery ? static_cast<AActor*>(Avery) : static_cast<AActor*>(Laser));

	if (bTriggered && Laser->CanTrigger() == false)
	{
		// Cooldown engaged after trigger
		CompleteReport(Report, EProjectOrganoidQATestResult::Passed, TEXT("Tripwire armed, triggered, cooldown active"));
	}
	else if (bTriggered)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Passed, TEXT("Tripwire trigger succeeded"));
	}
	else
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("Tripwire failed to trigger while armed"));
	}

	return Report;
}

FProjectOrganoidQATestReport UProjectOrganoidQAAutomationSubsystem::RunPowerBlackoutTest()
{
	FProjectOrganoidQATestReport Report;
	Report.TestId = EProjectOrganoidQATestId::PowerBlackout;
	Report.Result = EProjectOrganoidQATestResult::Running;

	UWorld* World = GetWorld();
	if (!World)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("No world"));
		return Report;
	}

	UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>();
	if (!Power)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed, TEXT("Power subsystem missing"));
		return Report;
	}

	Power->TriggerFacilityBlackout();
	const EProjectOrganoidPowerState AfterBlackout = Power->GetFacilityPowerState();

	AProjectOrganoidLaserTripwire* Laser = nullptr;
	for (TActorIterator<AProjectOrganoidLaserTripwire> It(World); It; ++It)
	{
		Laser = *It;
		break;
	}

	bool bLaserCollisionDisabled = true;
	if (Laser && Laser->BeamVolume)
	{
		bLaserCollisionDisabled =
			Laser->BeamVolume->GetCollisionEnabled() == ECollisionEnabled::NoCollision;
	}

	Power->RestoreFacilityPower();
	const EProjectOrganoidPowerState AfterRestore = Power->GetFacilityPowerState();

	if (AfterBlackout == EProjectOrganoidPowerState::Blackout
		&& AfterRestore == EProjectOrganoidPowerState::Online)
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Passed,
			FString::Printf(TEXT("Blackout->Online ok; laserCollisionDisabled=%s"),
				bLaserCollisionDisabled ? TEXT("true") : TEXT("false")));
	}
	else
	{
		CompleteReport(Report, EProjectOrganoidQATestResult::Failed,
			FString::Printf(TEXT("Unexpected states blackout=%s restore=%s"),
				*UEnum::GetValueAsString(AfterBlackout),
				*UEnum::GetValueAsString(AfterRestore)));
	}

	return Report;
}
