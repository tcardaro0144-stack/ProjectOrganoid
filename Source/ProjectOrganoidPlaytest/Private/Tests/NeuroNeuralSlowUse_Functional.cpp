#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "NavigationSystem.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace NeuroNeuralSlowUseFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroNeuralSlowUse_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Neural Slow Use Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR IntroMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro.DA_Mission_NeuroResearchStationIntro");
	constexpr TCHAR UseMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse.DA_Mission_NeuroNeuralSlowUse");
	constexpr TCHAR AdaptationPath[] =
		TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");
	constexpr TCHAR IntroEvent[] = TEXT("Event_NeuralSlowEquipped");
	constexpr TCHAR UseMissionId[] = TEXT("Mission_NeuroNeuralSlowUse");
	constexpr TCHAR UseObjectiveId[] = TEXT("Obj_ApplyNeuralSlow");
	constexpr TCHAR UseEvent[] = TEXT("Event_NeuralSlowApplied");
	constexpr TCHAR SubjectLabel[] = TEXT("Host_Neuro_AdaptationSubject");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroNeuralSlowUseTest");
	constexpr TCHAR ExpectedLine[] =
		TEXT("Neural Slow took hold. The Host is moving slower, and it did not cost a shot.");
	const FVector SubjectLocation(850.f, -2050.f, -1100.f);
	const FVector PlayerStand(850.f, -1850.f, -1100.f);
	const FVector WalkTarget(850.f, -1900.f, -1100.f);
	const FVector FarStand(650.f, -400.f, -1100.f);
	constexpr float CameraSettleSeconds = 0.35f;
	constexpr float InputTimeoutSeconds = 2.0f;
	constexpr float MoveTimeoutSeconds = 8.0f;
	constexpr float CooldownTimeoutSeconds = 10.0f;
	constexpr float SampleSeconds = 0.50f;
	constexpr float SurveyedFloorZ = -1190.f;
	constexpr float ProductionAimConeCosine = 0.9063f;
	constexpr float NoTargetPitchDegrees = 65.f;
	constexpr float ViewMatchToleranceDegrees = 1.f;
	constexpr float ViewMatchTimeoutSeconds = 2.0f;
	constexpr float NoSpendFrameSeconds = 0.05f;
	constexpr float NeuralSlowPECost = 20.f;

	FString BoolText(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }

	const TCHAR* ReasonText(EProjectOrganoidBiologicalAdaptationFailReason Reason)
	{
		switch (Reason)
		{
		case EProjectOrganoidBiologicalAdaptationFailReason::None: return TEXT("None");
		case EProjectOrganoidBiologicalAdaptationFailReason::Unequipped: return TEXT("Unequipped");
		case EProjectOrganoidBiologicalAdaptationFailReason::InsufficientPE: return TEXT("InsufficientPE");
		case EProjectOrganoidBiologicalAdaptationFailReason::Cooldown: return TEXT("Cooldown");
		case EProjectOrganoidBiologicalAdaptationFailReason::NoTarget: return TEXT("NoTarget");
		case EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange: return TEXT("OutOfRange");
		case EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget: return TEXT("InvalidTarget");
		case EProjectOrganoidBiologicalAdaptationFailReason::Dead: return TEXT("Dead");
		default: return TEXT("Unknown");
		}
	}

	const TCHAR* PowerText(EProjectOrganoidPowerState State)
	{
		switch (State)
		{
		case EProjectOrganoidPowerState::Online: return TEXT("Online");
		case EProjectOrganoidPowerState::Emergency: return TEXT("Emergency");
		case EProjectOrganoidPowerState::Blackout: return TEXT("Blackout");
		default: return TEXT("Unknown");
		}
	}

	int32 CountActiveId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives) return 0;
		for (const FProjectOrganoidObjective& Entry : Objectives->GetActiveObjectives())
		{
			if (Entry.ObjectiveId == Id) ++Count;
		}
		return Count;
	}

	int32 CountCompletedId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives) return 0;
		for (const FProjectOrganoidObjective& Entry : Objectives->GetCompletedObjectives())
		{
			if (Entry.ObjectiveId == Id) ++Count;
		}
		return Count;
	}

	struct FAimSample
	{
		FRotator Requested = FRotator::ZeroRotator;
		FRotator Control = FRotator::ZeroRotator;
		FVector ViewLocation = FVector::ZeroVector;
		FRotator ViewRotation = FRotator::ZeroRotator;
		FVector CameraLocation = FVector::ZeroVector;
		FRotator CameraRotation = FRotator::ZeroRotator;
		FVector TraceStart = FVector::ZeroVector;
		FVector TraceEnd = FVector::ZeroVector;
		FString HitActor;
		FString Candidate;
		FString NearestHost;
		AProjectOrganoidHostBase* CandidateHost = nullptr;
		float NearestDot = -1.f;
		float NearestDistance = -1.f;
		float SubjectDot = -1.f;
		float ActorDistance = -1.f;
		EProjectOrganoidBiologicalAdaptationFailReason Reason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
		bool bViewMatchesRequest = false;
	};

	class FNeuroNeuralSlowUseFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Phase = EPhase::RuntimePlacement;
			WaitSeconds = 0.f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			bPlaced = false;
			bInjected = false;
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			if (DeadHost.IsValid()) DeadHost->Destroy();
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
			Owner.SetStage(TEXT("Abort"));
		}

		virtual void Tick(UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) override
		{
			FOrganoidPlaytestRecord* Record = Owner.GetActiveRecord();
			if (!Record) return;
			switch (Stage)
			{
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie: TickStartPie(Owner, *Record); break;
			case EStage::WaitReady: TickWaitReady(Owner, *Record, DeltaTime); break;
			case EStage::Proof: TickProof(Owner, *Record, DeltaTime); break;
			case EStage::EndSession:
				if (DeadHost.IsValid()) DeadHost->Destroy();
				DeadHost.Reset();
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitBetween;
				break;
			case EStage::WaitBetween:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress())
				{
					if (bAnyAssertFailed)
					{
						Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record->FailureReason);
						return;
					}
					bSecondSession = true;
					bRequestedNeuroStream = false;
					Stage = EStage::StartPie;
					return;
				}
				if (WaitSeconds > 20.f)
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("First PIE session did not end."));
				}
				break;
			case EStage::EndPie:
				if (DeadHost.IsValid()) DeadHost->Destroy();
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitStopped;
				break;
			case EStage::WaitStopped:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress() || WaitSeconds > 20.f)
				{
					UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
					if (UGameplayStatics::DoesSaveGameExist(SaveSlot, 0))
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty()) Record->FailureReason = TEXT("Test save slot remained after cleanup.");
					}
					TArray<UPackage*> WorldDirty;
					TArray<UPackage*> ContentDirty;
					FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
					FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
					if (WorldDirty.Num() + ContentDirty.Num() > 0)
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty()) Record->FailureReason = TEXT("A package was dirty after the test.");
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, EndSession, WaitBetween, EndPie, WaitStopped };
		enum class EPhase : uint8
		{
			RuntimePlacement,
			Handoff,
			SettleUnequipped,
			WaitUnequipped,
			SettleNoTarget,
			WaitNoTarget,
			SettleOutOfRange,
			WaitOutOfRange,
			SettleDead,
			WaitDead,
			SettleLowPE,
			WaitLowPE,
			SettleWrongHost,
			WaitWrongHost,
			HoldBeforeExpiry,
			WaitNaturalClear,
			IssueHostRestore,
			WaitHostRestoreMoving,
			MeasureHostRestore,
			SettleCooldown,
			WaitCooldown,
			WaitCooldownExpire,
			IssueBaselineLeg,
			WaitBaselineMoving,
			MeasureBaseline,
			SettleSuccess,
			WaitSuccess,
			MeasureSlow,
			SaveWhileActive,
			FreshLoad,
			SettleReplay,
			WaitReplay,
			Done
		};

		EStage Stage = EStage::Preflight;
		EPhase Phase = EPhase::RuntimePlacement;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bSecondSession = false;
		bool bPlaced = false;
		bool bInjected = false;
		bool bAimCommanded = false;
		bool bTargetGateStable = false;
		TWeakObjectPtr<AProjectOrganoidHostBase> StableTargetHost;
		EProjectOrganoidBiologicalAdaptationFailReason StableTargetReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
		bool bFreshEvaluated = false;
		float PEAnchor = 100.f;
		float NoSpendAnchor = 0.f;
		float NoSpendMin = 0.f;
		float NoSpendMax = 100.f;
		float NoSpendRate = 0.f;
		double NoSpendStartSeconds = 0.0;
		float SpendAnchor = 0.f;
		float SpendMin = 0.f;
		float SpendMax = 100.f;
		float SpendRate = 0.f;
		double SpendStartSeconds = 0.0;
		float SavedPE = 100.f;
		float CooldownAtSave = 0.f;
		double EffectAppliedAt = 0.0;
		int32 PostLoadFire = 0;
		int32 PostLoadNotes = 0;
		float BaselineDisplacement = 0.f;
		float SlowDisplacement = 0.f;
		FVector MoveAnchor = FVector::ZeroVector;
		FVector LegTarget = FVector::ZeroVector;
		FRotator RequestedAim = FRotator::ZeroRotator;
		FString LastAimText;
		float ReportedActorDistance = -1.f;
		float ReportedConeDot = -1.f;
		TWeakObjectPtr<AProjectOrganoidHostBase> DeadHost;

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty()) Record.FailureReason = Reason;
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
		}

		bool AssertTrue(FOrganoidPlaytestRecord& Record, const FString& Id, bool bPassed, const FString& Expected, const FString& Actual, const FString& ActorId)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, false);
			if (!bPassed)
			{
				bAnyAssertFailed = true;
				if (Record.FailureReason.IsEmpty()) Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
			}
			return bPassed;
		}

		void StopIfFailed()
		{
			if (bAnyAssertFailed) Stage = EStage::EndPie;
		}

		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUDController = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUDController ? HUDController->GetBoundHUDWidget() : nullptr;
		}

		void SetCameraLagEnabled(AProjectOrganoidCharacter* Character, bool bEnabled) const
		{
			if (USpringArmComponent* Boom = Character ? Character->GetCameraBoom() : nullptr)
			{
				Boom->bEnableCameraLag = bEnabled;
			}
		}

		void Face(AProjectOrganoidCharacter* Character, AActor* LookAt) const
		{
			if (Character && LookAt)
			{
				OrganoidPlaytestActions::FaceActor(Character, LookAt);
			}
		}

		void Place(AProjectOrganoidCharacter* Character, const FVector& Loc, AActor* LookAt) const
		{
			Character->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
			Face(Character, LookAt);
		}

		bool PressQ(AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
			UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
			UInputAction* Action = nullptr;
			if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Character ? Character->GetClass() : nullptr, TEXT("AbilityAction")))
			{
				Action = Cast<UInputAction>(Prop->GetObjectPropertyValue_InContainer(Character));
			}
			if (!Input || !Action) return false;
			Input->InjectInputForAction(Action, FInputActionValue(true), {}, {});
			return true;
		}

		void BeginSettle()
		{
			WaitSeconds = 0.f;
			bPlaced = false;
			bInjected = false;
			bAimCommanded = false;
			bTargetGateStable = false;
			StableTargetHost = nullptr;
			StableTargetReason = EProjectOrganoidBiologicalAdaptationFailReason::None;
		}

		bool Settle(AProjectOrganoidCharacter* Character, const FVector& Loc, AActor* LookAt, float DeltaTime)
		{
			if (!bPlaced)
			{
				Place(Character, Loc, LookAt);
				SetCameraLagEnabled(Character, false);
				bPlaced = true;
				WaitSeconds = 0.f;
			}
			Face(Character, LookAt);
			WaitSeconds += DeltaTime;
			return WaitSeconds >= CameraSettleSeconds;
		}

		bool NoCampaignCredit(AProjectOrganoidHostBase* Subject, UProjectOrganoidObjectiveSubsystem* Objectives) const
		{
			return Subject
				&& Subject->AdaptationCampaignEventFireCount == 0
				&& Subject->AdaptationCampaignNotificationCount == 0
				&& CountCompletedId(Objectives, FName(UseObjectiveId)) == 0
				&& CountActiveId(Objectives, FName(UseObjectiveId)) == 1
				&& Objectives->GetActiveMissionId() == FName(UseMissionId);
		}

		FString CreditActual(AProjectOrganoidHostBase* Subject, UProjectOrganoidObjectiveSubsystem* Objectives, UProjectOrganoidBiologicalAdaptationComponent* Adapt, AProjectOrganoidCharacter* Character) const
		{
			return FString::Printf(
				TEXT("reason=%s pe=%.1f slow=%d fire=%d note=%d completed=%d mission=%s"),
				ReasonText(Adapt ? Adapt->GetLastFailReason() : EProjectOrganoidBiologicalAdaptationFailReason::None),
				Character ? Character->GetPEEnergy() : -1.f,
				Subject && Subject->IsBiologicalLocomotorSlowActive() ? 1 : 0,
				Subject ? Subject->AdaptationCampaignEventFireCount : -1,
				Subject ? Subject->AdaptationCampaignNotificationCount : -1,
				CountCompletedId(Objectives, FName(UseObjectiveId)),
				Objectives ? *Objectives->GetActiveMissionId().ToString() : TEXT("none"));
		}

		bool ChooseLeg(UWorld* World, AProjectOrganoidHostBase* Subject)
		{
			UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			if (!Nav || !Subject) return false;
			const FVector Start = Subject->GetActorLocation();
			const FVector Offsets[] = {
				FVector(0.f, -600.f, 0.f),
				FVector(600.f, 0.f, 0.f),
				FVector(-600.f, 0.f, 0.f),
				FVector(0.f, 600.f, 0.f),
				FVector(450.f, -450.f, 0.f),
				FVector(-450.f, -450.f, 0.f)
			};
			for (const FVector& Offset : Offsets)
			{
				double PathLength = 0.0;
				const FVector Candidate = Start + Offset;
				if (Nav->GetPathLength(Start, Candidate, PathLength) == ENavigationQueryResult::Success && PathLength > 450.0)
				{
					LegTarget = Candidate;
					return true;
				}
			}
			return false;
		}

		bool RotationNear(const FRotator& A, const FRotator& B) const
		{
			return FMath::Abs(FMath::FindDeltaAngleDegrees(A.Pitch, B.Pitch)) <= ViewMatchToleranceDegrees
				&& FMath::Abs(FMath::FindDeltaAngleDegrees(A.Yaw, B.Yaw)) <= ViewMatchToleranceDegrees;
		}

		FString HostName(const AActor* Actor) const
		{
			return Actor ? Actor->GetActorNameOrLabel() : TEXT("none");
		}

		int32 CountSlowed(UWorld* World) const
		{
			int32 Count = 0;
			if (!World) return 0;
			for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
			{
				if (*It && (*It)->IsBiologicalLocomotorSlowActive()) ++Count;
			}
			return Count;
		}

		void BeginNoSpendWatch(UWorld* World, AProjectOrganoidCharacter* Character)
		{
			NoSpendAnchor = Character->GetPEEnergy();
			NoSpendMin = NoSpendAnchor;
			NoSpendMax = Character->GetMaxPEEnergy();
			NoSpendRate = Character->GetPERechargeRate();
			NoSpendStartSeconds = World->GetTimeSeconds();
			PEAnchor = NoSpendAnchor;
		}

		void SampleNoSpend(AProjectOrganoidCharacter* Character)
		{
			if (Character)
			{
				NoSpendMin = FMath::Min(NoSpendMin, Character->GetPEEnergy());
			}
		}

		float NoSpendElapsed(UWorld* World) const
		{
			return World ? FMath::Max(0.f, static_cast<float>(World->GetTimeSeconds() - NoSpendStartSeconds)) : 0.f;
		}

		float NoSpendTolerance() const
		{
			return FMath::Max(0.05f, NoSpendRate * NoSpendFrameSeconds);
		}

		float NoSpendExpected(UWorld* World) const
		{
			return FMath::Min(NoSpendMax, NoSpendAnchor + NoSpendRate * NoSpendElapsed(World));
		}

		bool NoSpendHolds(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			const float FinalPE = Character->GetPEEnergy();
			const float Tolerance = NoSpendTolerance();
			const float Drop = NoSpendAnchor - NoSpendMin;
			const bool bNoDrop = NoSpendMin >= NoSpendAnchor - Tolerance;
			const bool bEnvelope = FMath::Abs(FinalPE - NoSpendExpected(World)) <= Tolerance;
			const bool bNoTwenty = Drop < NeuralSlowPECost - Tolerance;
			return bNoDrop && bEnvelope && bNoTwenty;
		}

		FString NoSpendText(UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidBiologicalAdaptationComponent* Adapt) const
		{
			const float FinalPE = Character ? Character->GetPEEnergy() : -1.f;
			return FString::Printf(
				TEXT("anchor=%.3f min=%.3f final=%.3f elapsed=%.3f rate=%.3f expected=%.3f reason=%s"),
				NoSpendAnchor,
				NoSpendMin,
				FinalPE,
				NoSpendElapsed(World),
				NoSpendRate,
				NoSpendExpected(World),
				ReasonText(Adapt ? Adapt->GetLastFailReason() : EProjectOrganoidBiologicalAdaptationFailReason::None));
		}

		void InjectNoSpendQ(UWorld* World, AProjectOrganoidCharacter* Character)
		{
			BeginNoSpendWatch(World, Character);
			if (!bInjected) bInjected = PressQ(Character);
			SampleNoSpend(Character);
		}

		bool NoSpendRejection(UWorld* World, AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* Subject, UProjectOrganoidObjectiveSubsystem* Objectives) const
		{
			return bInjected && NoCampaignCredit(Subject, Objectives) && CountSlowed(World) == 0 && NoSpendHolds(World, Character);
		}

		void BeginSpendWatch(UWorld* World, AProjectOrganoidCharacter* Character)
		{
			SpendAnchor = Character->GetPEEnergy();
			SpendMin = SpendAnchor;
			SpendMax = Character->GetMaxPEEnergy();
			SpendRate = Character->GetPERechargeRate();
			SpendStartSeconds = World->GetTimeSeconds();
			PEAnchor = SpendAnchor;
		}

		void SampleSpend(AProjectOrganoidCharacter* Character)
		{
			if (Character)
			{
				SpendMin = FMath::Min(SpendMin, Character->GetPEEnergy());
			}
		}

		float SpendElapsed(UWorld* World) const
		{
			return World ? FMath::Max(0.f, static_cast<float>(World->GetTimeSeconds() - SpendStartSeconds)) : 0.f;
		}

		float SpendTolerance() const
		{
			return FMath::Max(0.05f, SpendRate * NoSpendFrameSeconds);
		}

		float SpendExpected(UWorld* World) const
		{
			return FMath::Min(SpendMax, SpendAnchor - NeuralSlowPECost + SpendRate * SpendElapsed(World));
		}

		float SpendReconstructed(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			const float FinalPE = Character ? Character->GetPEEnergy() : SpendMin;
			return (SpendAnchor - FinalPE) + SpendRate * SpendElapsed(World);
		}

		bool SpendCostHolds(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			const float FinalPE = Character->GetPEEnergy();
			const float Elapsed = SpendElapsed(World);
			const float Tolerance = SpendTolerance();
			const float Expected = SpendExpected(World);
			const float Reconstructed = SpendReconstructed(World, Character);
			const bool bAtCap = SpendAnchor >= SpendMax - Tolerance;
			const float CapSlack = bAtCap ? SpendRate * Elapsed : 0.f;
			const float Drop = SpendAnchor - SpendMin;
			const bool bDown = Drop + CapSlack + Tolerance >= NeuralSlowPECost && Drop <= NeuralSlowPECost + Tolerance;
			const bool bCost = FMath::Abs(Reconstructed - NeuralSlowPECost) <= Tolerance + CapSlack;
			const float LowerFinal = bAtCap ? (SpendAnchor - NeuralSlowPECost - Tolerance) : (Expected - Tolerance);
			const bool bFinal = FinalPE >= LowerFinal && FinalPE <= Expected + Tolerance;
			return bDown && bCost && bFinal;
		}

		FString SpendText(UWorld* World, AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* EffectHost, AProjectOrganoidHostBase* Subject, UProjectOrganoidObjectiveSubsystem* Objectives, UProjectOrganoidBiologicalAdaptationComponent* Adapt, float Speed) const
		{
			const float FinalPE = Character ? Character->GetPEEnergy() : -1.f;
			const int32 Fire = Subject ? Subject->AdaptationCampaignEventFireCount : -1;
			const int32 Note = Subject ? Subject->AdaptationCampaignNotificationCount : -1;
			const int32 Completed = CountCompletedId(Objectives, FName(UseObjectiveId));
			const int32 Credit = FMath::Max(0, Fire) + FMath::Max(0, Note) + Completed;
			return FString::Printf(
				TEXT("target=%s injection=%.3f min=%.3f final=%.3f elapsed=%.3f rate=%.3f expected=%.3f cost=%.3f speed=%.1f slow=%d cooldown=%d credit=%d"),
				*HostName(EffectHost),
				SpendAnchor,
				SpendMin,
				FinalPE,
				SpendElapsed(World),
				SpendRate,
				SpendExpected(World),
				SpendReconstructed(World, Character),
				Speed,
				EffectHost && EffectHost->IsBiologicalLocomotorSlowActive() ? 1 : 0,
				Adapt && Adapt->IsOnCooldown() ? 1 : 0,
				Credit);
		}

		FVector CapsuleCenter(const AActor* Actor) const
		{
			if (const ACharacter* AsCharacter = Cast<ACharacter>(Actor))
			{
				if (const UCapsuleComponent* Capsule = AsCharacter->GetCapsuleComponent())
				{
					return Capsule->GetComponentLocation();
				}
			}
			return Actor ? Actor->GetActorLocation() : FVector::ZeroVector;
		}

		void PlaceStill(AProjectOrganoidCharacter* Character, const FVector& Location) const
		{
			Character->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
			StopPawn(Character);
			SetCameraLagEnabled(Character, false);
		}

		FString TargetGateText(const TCHAR* PhaseName, EProjectOrganoidBiologicalAdaptationFailReason Expected, AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* Host, const FAimSample& Sample) const
		{
			const float CameraDistance = Host ? FVector::Dist(Sample.TraceStart, Host->GetActorLocation()) : -1.f;
			return FString::Printf(
				TEXT("phase=%s intended=%s candidate=%s expected=%s actual=%s pawn=%s camLoc=%s requested=%s viewRot=%s camRot=%s hit=%s dot=%.4f actor=%.1f cameraDist=%.1f pe=%.1f"),
				PhaseName,
				*HostName(Host),
				*Sample.Candidate,
				ReasonText(Expected),
				ReasonText(Sample.Reason),
				Character ? *Character->GetActorLocation().ToCompactString() : TEXT("none"),
				*Sample.CameraLocation.ToCompactString(),
				*Sample.Requested.ToString(),
				*Sample.ViewRotation.ToString(),
				*Sample.CameraRotation.ToString(),
				*Sample.HitActor,
				Sample.SubjectDot,
				Sample.ActorDistance,
				CameraDistance,
				Character ? Character->GetPEEnergy() : -1.f);
		}

		bool AcquireLiveTarget(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			AProjectOrganoidHostBase* Host,
			EProjectOrganoidBiologicalAdaptationFailReason Expected,
			float Range,
			float DeltaTime,
			const TCHAR* PhaseName,
			FAimSample& OutSample)
		{
			if (!Host)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("setup.target missing host phase=%s"), PhaseName));
				return false;
			}
			FAimSample Live;
			if (!ReadLiveAim(World, Character, FRotator::ZeroRotator, Range, Host, Live))
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("setup.target camera unavailable phase=%s"), PhaseName));
				return false;
			}
			const FVector Center = CapsuleCenter(Host);
			const FRotator Desired = (Center - Live.ViewLocation).Rotation();
			Live.Requested = Desired;
			const float CameraDistance = FVector::Dist(Live.TraceStart, Host->GetActorLocation());
			const bool bTraceHit = Live.HitActor == HostName(Host);
			const bool bCone = Live.SubjectDot >= ProductionAimConeCosine;
			const bool bAlive = !Host->bIsDead && !Host->bIsIncapacitated;
			bool bEvidence = false;
			if (Expected == EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget)
			{
				bEvidence = !bAlive && (bTraceHit || bCone);
			}
			else if (Expected == EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange)
			{
				const bool bTraceOut = bTraceHit && Live.ActorDistance > Range;
				const bool bConeOut = bCone && CameraDistance > Range;
				bEvidence = bAlive && (bTraceOut || bConeOut);
			}
			else
			{
				const bool bTraceIn = bTraceHit && Live.ActorDistance <= Range;
				const bool bConeIn = bCone && CameraDistance <= Range && Live.ActorDistance <= Range;
				bEvidence = bAlive && (bTraceIn || bConeIn);
			}
			const bool bClassified = Live.CandidateHost == Host && Live.Reason == Expected && bEvidence;
			if (bClassified && bTargetGateStable && StableTargetHost.Get() == Host && StableTargetReason == Expected)
			{
				OutSample = Live;
				LastAimText = TargetGateText(PhaseName, Expected, Character, Host, Live);
				return true;
			}
			bTargetGateStable = bClassified;
			StableTargetHost = bClassified ? Host : nullptr;
			StableTargetReason = bClassified ? Expected : EProjectOrganoidBiologicalAdaptationFailReason::None;
			ApplyControlAim(Character, FRotator(Desired.Pitch, Desired.Yaw, 0.f));
			bAimCommanded = true;
			RequestedAim = FRotator(Desired.Pitch, Desired.Yaw, 0.f);
			WaitSeconds += DeltaTime;
			if (WaitSeconds > ViewMatchTimeoutSeconds)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("setup.target timed out %s"), *TargetGateText(PhaseName, Expected, Character, Host, Live)));
			}
			return false;
		}

		void ApplyControlAim(AProjectOrganoidCharacter* Character, const FRotator& Rot) const
		{
			if (!Character) return;
			if (AController* Controller = Character->GetController())
			{
				Controller->SetControlRotation(Rot);
			}
			Character->SetActorRotation(FRotator(0.f, Rot.Yaw, 0.f));
		}

		void StopPawn(AProjectOrganoidCharacter* Character) const
		{
			if (UCharacterMovementComponent* Move = Character ? Character->GetCharacterMovement() : nullptr)
			{
				Move->StopMovementImmediately();
			}
		}

		FAimSample ClassifyAim(
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			const FVector& Start,
			const FVector& Direction,
			float Range,
			AProjectOrganoidHostBase* Subject) const
		{
			FAimSample Sample;
			const FVector Dir = Direction.GetSafeNormal();
			Sample.TraceStart = Start;
			Sample.TraceEnd = Start + Dir * Range;
			Sample.ActorDistance = Character && Subject ? FVector::Dist(Character->GetActorLocation(), Subject->GetActorLocation()) : -1.f;

			FCollisionQueryParams Params(SCENE_QUERY_STAT(NeuralSlowUseAimPreflight), false, Character);
			FHitResult Hit;
			AProjectOrganoidHostBase* TraceHost = nullptr;
			if (World && World->LineTraceSingleByChannel(Hit, Sample.TraceStart, Sample.TraceEnd, ECC_Pawn, Params))
			{
				Sample.HitActor = HostName(Hit.GetActor());
				TraceHost = Cast<AProjectOrganoidHostBase>(Hit.GetActor());
			}
			else
			{
				Sample.HitActor = TEXT("none");
			}

			if (TraceHost)
			{
				Sample.CandidateHost = TraceHost;
				Sample.Candidate = HostName(TraceHost);
				if (TraceHost->bIsDead || TraceHost->bIsIncapacitated)
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget;
				}
				else if (Character && FVector::Dist(Character->GetActorLocation(), TraceHost->GetActorLocation()) > Range)
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange;
				}
				else
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::None;
				}
			}

			AProjectOrganoidHostBase* BestHost = nullptr;
			float BestDot = ProductionAimConeCosine;
			bool bSawDeadAimedHost = false;
			bool bSawOutOfRangeAimedHost = false;
			AProjectOrganoidHostBase* BestOutHost = nullptr;
			float BestOutDot = -2.f;
			AProjectOrganoidHostBase* BestDeadHost = nullptr;
			float BestDeadDot = -2.f;
			AProjectOrganoidHostBase* Nearest = nullptr;
			float NearestDot = -2.f;
			float NearestDist = -1.f;

			for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
			{
				AProjectOrganoidHostBase* Host = *It;
				if (!Host) continue;
				const FVector ToHost = Host->GetActorLocation() - Start;
				const float Dist = ToHost.Size();
				if (Dist <= KINDA_SMALL_NUMBER) continue;
				const float Dot = FVector::DotProduct(Dir, ToHost / Dist);
				if (Host == Subject) Sample.SubjectDot = Dot;
				if (Dot > NearestDot)
				{
					NearestDot = Dot;
					Nearest = Host;
					NearestDist = Dist;
				}
				if (Dot < ProductionAimConeCosine) continue;
				if (Host->bIsDead || Host->bIsIncapacitated)
				{
					bSawDeadAimedHost = true;
					if (Dot > BestDeadDot)
					{
						BestDeadDot = Dot;
						BestDeadHost = Host;
					}
					continue;
				}
				if (Dist > Range)
				{
					bSawOutOfRangeAimedHost = true;
					if (Dot > BestOutDot)
					{
						BestOutDot = Dot;
						BestOutHost = Host;
					}
					continue;
				}
				if (Dot > BestDot)
				{
					BestDot = Dot;
					BestHost = Host;
				}
			}

			Sample.NearestHost = HostName(Nearest);
			Sample.NearestDot = Nearest ? NearestDot : -1.f;
			Sample.NearestDistance = Nearest ? NearestDist : -1.f;

			if (!TraceHost)
			{
				if (BestHost)
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::None;
					Sample.CandidateHost = BestHost;
					Sample.Candidate = HostName(BestHost);
				}
				else if (bSawDeadAimedHost)
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget;
					Sample.CandidateHost = BestDeadHost;
					Sample.Candidate = HostName(BestDeadHost);
				}
				else if (bSawOutOfRangeAimedHost)
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange;
					Sample.CandidateHost = BestOutHost;
					Sample.Candidate = HostName(BestOutHost);
				}
				else
				{
					Sample.Reason = EProjectOrganoidBiologicalAdaptationFailReason::NoTarget;
					Sample.CandidateHost = nullptr;
					Sample.Candidate = TEXT("none");
				}
			}
			return Sample;
		}

		bool ReadLiveAim(
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			const FRotator& Requested,
			float Range,
			AProjectOrganoidHostBase* Subject,
			FAimSample& OutSample) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			if (!PC || !PC->PlayerCameraManager) return false;
			FVector ViewLoc = FVector::ZeroVector;
			FRotator ViewRot = FRotator::ZeroRotator;
			PC->GetPlayerViewPoint(ViewLoc, ViewRot);
			FVector ManagerLoc = FVector::ZeroVector;
			FRotator ManagerRot = FRotator::ZeroRotator;
			PC->PlayerCameraManager->GetCameraViewPoint(ManagerLoc, ManagerRot);
			OutSample = ClassifyAim(World, Character, ViewLoc, ViewRot.Vector(), Range, Subject);
			OutSample.Requested = Requested;
			OutSample.Control = PC->GetControlRotation();
			OutSample.ViewLocation = ViewLoc;
			OutSample.ViewRotation = ViewRot;
			OutSample.CameraLocation = ManagerLoc;
			OutSample.CameraRotation = ManagerRot;
			OutSample.bViewMatchesRequest =
				RotationNear(ViewRot, Requested)
				&& RotationNear(ManagerRot, Requested)
				&& ViewLoc.Equals(ManagerLoc, 1.f);
			return true;
		}

		FString FormatAim(const FAimSample& Sample) const
		{
			return FString::Printf(
				TEXT("requested=%s control=%s camLoc=%s camRot=%s viewRot=%s start=%s end=%s hit=%s nearest=%s dot=%.4f dist=%.1f candidate=%s reason=%s actor=%.1f subjectDot=%.4f"),
				*Sample.Requested.ToString(),
				*Sample.Control.ToString(),
				*Sample.CameraLocation.ToCompactString(),
				*Sample.CameraRotation.ToString(),
				*Sample.ViewRotation.ToString(),
				*Sample.TraceStart.ToCompactString(),
				*Sample.TraceEnd.ToCompactString(),
				*Sample.HitActor,
				*Sample.NearestHost,
				Sample.NearestDot,
				Sample.NearestDistance,
				*Sample.Candidate,
				ReasonText(Sample.Reason),
				Sample.ActorDistance,
				Sample.SubjectDot);
		}

		bool SelectNoTargetYaw(
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			float Range,
			AProjectOrganoidHostBase* Subject,
			float& OutYaw,
			FAimSample& OutSample) const
		{
			USpringArmComponent* Boom = Character ? Character->GetCameraBoom() : nullptr;
			const float ArmLength = Boom ? Boom->TargetArmLength : 320.f;
			const FVector Relative = Boom ? Boom->GetRelativeLocation() : FVector(0.f, 0.f, 80.f);
			const FVector Origin = Character->GetActorLocation() + Character->GetActorRotation().RotateVector(Relative);
			bool bFound = false;
			float ChosenYaw = 0.f;
			float ClearestDot = 2.f;
			FAimSample Clearest;
			FAimSample Fallback;
			bool bHaveFallback = false;
			float FallbackDot = 2.f;
			for (int32 Step = 0; Step < 24; ++Step)
			{
				const float Yaw = static_cast<float>(Step) * 15.f;
				const FRotator Rot(NoTargetPitchDegrees, Yaw, 0.f);
				const FVector Dir = Rot.Vector();
				FAimSample Sample = ClassifyAim(World, Character, Origin - Dir * ArmLength, Dir, Range, Subject);
				Sample.Requested = Rot;
				const bool bBetterFallback = !bHaveFallback || Sample.NearestDot < FallbackDot - 0.0001f || (FMath::Abs(Sample.NearestDot - FallbackDot) <= 0.0001f && Yaw < Fallback.Requested.Yaw);
				if (bBetterFallback)
				{
					bHaveFallback = true;
					FallbackDot = Sample.NearestDot;
					Fallback = Sample;
				}
				if (Sample.Reason != EProjectOrganoidBiologicalAdaptationFailReason::NoTarget) continue;
				const bool bClearer = !bFound || Sample.NearestDot < ClearestDot - 0.0001f || (FMath::Abs(Sample.NearestDot - ClearestDot) <= 0.0001f && Yaw < ChosenYaw);
				if (bClearer)
				{
					bFound = true;
					ClearestDot = Sample.NearestDot;
					ChosenYaw = Yaw;
					Clearest = Sample;
				}
			}
			OutYaw = ChosenYaw;
			OutSample = bFound ? Clearest : Fallback;
			return bFound;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Intro = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroMissionSoftPath);
			UProjectOrganoidObjectiveDataAsset* Use = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, UseMissionSoftPath);
			UProjectOrganoidBiologicalAdaptationData* Neural = UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve();
			AssertTrue(Record, TEXT("asset.intro_next"), Intro && Intro->NextMissionAsset.ToSoftObjectPath().ToString() == UseMissionSoftPath, UseMissionSoftPath, Intro ? Intro->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.use_id"), Use && Use->MissionId == FName(UseMissionId), UseMissionId, Use ? Use->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.use_title"), Use && Use->MissionTitle.ToString() == TEXT("Use Neural Slow"), TEXT("Use Neural Slow"), Use ? Use->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.use_next_null"), Use && Use->NextMissionAsset.IsNull(), TEXT("null"), Use && Use->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.use_one_task"), Use && Use->Tasks.Num() == 1 && Use->Tasks[0].EventTriggers.Num() == 1 && Use->Tasks[0].EventTriggers[0].EventId == FName(UseEvent), UseEvent, Use && Use->Tasks.Num() == 1 && Use->Tasks[0].EventTriggers.Num() == 1 ? Use->Tasks[0].EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.neural_data"), Neural && Neural->GetPathName() == AdaptationPath, AdaptationPath, Neural ? Neural->GetPathName() : TEXT("missing"), TEXT("adaptation"));
			UProjectOrganoidBiologicalAdaptation_NeuralSlow* Slow = Cast<UProjectOrganoidBiologicalAdaptation_NeuralSlow>(Neural);
			AssertTrue(Record, TEXT("asset.pe_cost"), Slow && FMath::IsNearlyEqual(Slow->PECost, 20.f), TEXT("20"), Slow ? FString::SanitizeFloat(Slow->PECost) : TEXT("missing"), TEXT("adaptation"));
			AssertTrue(Record, TEXT("asset.cooldown"), Slow && FMath::IsNearlyEqual(Slow->CooldownSeconds, 8.f), TEXT("8"), Slow ? FString::SanitizeFloat(Slow->CooldownSeconds) : TEXT("missing"), TEXT("adaptation"));
			AssertTrue(Record, TEXT("asset.range"), Slow && FMath::IsNearlyEqual(Slow->MaxTargetRange, 800.f), TEXT("800"), Slow ? FString::SanitizeFloat(Slow->MaxTargetRange) : TEXT("missing"), TEXT("adaptation"));
			AssertTrue(Record, TEXT("asset.duration"), Slow && FMath::IsNearlyEqual(Slow->DurationSeconds, 4.f), TEXT("4"), Slow ? FString::SanitizeFloat(Slow->DurationSeconds) : TEXT("missing"), TEXT("adaptation"));
			AssertTrue(Record, TEXT("asset.speed_multiplier"), Slow && FMath::IsNearlyEqual(Slow->LocomotorSpeedMultiplier, 0.6f), TEXT("0.6"), Slow ? FString::SanitizeFloat(Slow->LocomotorSpeedMultiplier) : TEXT("missing"), TEXT("adaptation"));
			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Authored = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, SubjectLabel) : TArray<AActor*>();
			AActor* AuthoredActor = Authored.Num() == 1 ? Authored[0] : nullptr;
			const FVector AuthoredLoc = AuthoredActor ? AuthoredActor->GetActorLocation() : FVector::ZeroVector;
			const FRotator AuthoredRot = AuthoredActor ? AuthoredActor->GetActorRotation() : FRotator::ZeroRotator;
			const FVector AuthoredScale = AuthoredActor ? AuthoredActor->GetActorScale3D() : FVector::ZeroVector;
			const bool bAuthored =
				AuthoredActor
				&& AuthoredLoc.Equals(SubjectLocation, 0.05f)
				&& FMath::IsNearlyEqual(AuthoredRot.Yaw, 90.f, 0.05f)
				&& AuthoredScale.Equals(FVector::OneVector, 0.01f);
			AssertTrue(
				Record,
				TEXT("place.editor"),
				bAuthored,
				TEXT("850,-2050,-1100 yaw=90 scale=1"),
				AuthoredActor
					? FString::Printf(TEXT("%s yaw=%.3f scale=%s"), *AuthoredLoc.ToString(), AuthoredRot.Yaw, *AuthoredScale.ToString())
					: TEXT("missing"),
				SubjectLabel);
			if (bAnyAssertFailed || !Intro || !Use || !Neural)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 10 mission contract missing.") : Record.FailureReason);
				return;
			}
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			(void)Record;
			if (!Owner.RequestStartPie(MapPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			WaitSeconds = 0.f;
			bRequestedNeuroStream = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedNeuroStream)
			{
				const FName NeuroName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics);
				if (!NeuroName.IsNone())
				{
					Levels->AddStreamRequest(NeuroName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedNeuroStream = true;
				}
			}
			if (Character && bRequestedNeuroStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, SubjectLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				Phase = bSecondSession ? EPhase::FreshLoad : EPhase::RuntimePlacement;
				bFreshEvaluated = false;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Owner, Record, TEXT("PIE did not become ready with Host_Neuro_AdaptationSubject."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			TArray<AActor*> Subjects = World ? OrganoidPlaytestActions::FindActorsByLabel(World, SubjectLabel) : TArray<AActor*>();
			AProjectOrganoidHostBase* Subject = Subjects.Num() == 1 ? Cast<AProjectOrganoidHostBase>(Subjects[0]) : nullptr;
			TArray<AActor*> Host1s = World ? OrganoidPlaytestActions::FindActorsByLabel(World, Host1Label) : TArray<AActor*>();
			AProjectOrganoidHostBase* Host1 = Host1s.Num() == 1 ? Cast<AProjectOrganoidHostBase>(Host1s[0]) : nullptr;
			UProjectOrganoidBiologicalAdaptationData* Neural = UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve();
			if (!World || !Character || !Objectives || !Saves || !Power || !Adapt || !Subject || !Host1 || !Neural)
			{
				FailAndStop(Owner, Record, TEXT("Beat 10 actors or subsystems missing."));
				return;
			}

			auto Advance = [&](EPhase Next)
			{
				Phase = Next;
				BeginSettle();
			};

			switch (Phase)
			{
			case EPhase::RuntimePlacement:
			{
				SetCameraLagEnabled(Character, false);
				const FVector Loc = Subject->GetActorLocation();
				const bool bXY = FMath::Abs(Loc.X - SubjectLocation.X) <= 5.f && FMath::Abs(Loc.Y - SubjectLocation.Y) <= 5.f;
				AssertTrue(Record, TEXT("place.xy"), bXY, TEXT("850,-2050 +/-5"), FString::Printf(TEXT("X=%.3f Y=%.3f"), Loc.X, Loc.Y), SubjectLabel);
				UCapsuleComponent* Capsule = Subject->GetCapsuleComponent();
				const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
				FHitResult FloorHit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(Beat10Floor), false, Subject);
				const bool bFloor = World->LineTraceSingleByChannel(
					FloorHit,
					Loc + FVector(0.f, 0.f, 30.f),
					Loc - FVector(0.f, 0.f, 400.f),
					ECC_Visibility,
					Params);
				const float BottomZ = Loc.Z - HalfHeight;
				const float Gap = bFloor ? BottomZ - FloorHit.ImpactPoint.Z : -1000.f;
				const bool bFloorFit =
					bFloor
					&& FMath::IsNearlyEqual(HalfHeight, 96.f, 1.f)
					&& FMath::Abs(FloorHit.ImpactPoint.Z - SurveyedFloorZ) <= 8.f
					&& Gap >= 0.5f
					&& Gap <= 4.0f;
				AssertTrue(
					Record,
					TEXT("place.floor"),
					bFloorFit,
					TEXT("capsule 96, floor near -1190, bottom gap 0.5-4"),
					FString::Printf(TEXT("Z=%.3f half=%.3f floor=%.3f gap=%.3f"), Loc.Z, HalfHeight, bFloor ? FloorHit.ImpactPoint.Z : 0.f, Gap),
					SubjectLabel);
				AssertTrue(Record, TEXT("place.dormant"), !Subject->IsEncounterActivated(), TEXT("dormant"), Subject->IsEncounterActivated() ? TEXT("active") : TEXT("dormant"), SubjectLabel);
				bool bQ = false;
				UInputAction* Action = nullptr;
				if (FObjectProperty* Prop = FindFProperty<FObjectProperty>(Character->GetClass(), TEXT("AbilityAction")))
				{
					Action = Cast<UInputAction>(Prop->GetObjectPropertyValue_InContainer(Character));
				}
				if (FObjectProperty* MapProp = FindFProperty<FObjectProperty>(Character->GetClass(), TEXT("RuntimeMappingContext")))
				{
					if (UInputMappingContext* Context = Cast<UInputMappingContext>(MapProp->GetObjectPropertyValue_InContainer(Character)))
					{
						for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
						{
							if (Mapping.Action == Action && Mapping.Key == EKeys::Q) bQ = true;
						}
					}
				}
				AssertTrue(Record, TEXT("input.q_bound"), Action && Action->GetName() == TEXT("IA_Ability_Runtime") && bQ, TEXT("Q"), Action ? Action->GetName() : TEXT("none"), TEXT("input"));
				UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				double PathLength = 0.0;
				const bool bPath = Nav && Nav->GetPathLength(SubjectLocation, WalkTarget, PathLength) == ENavigationQueryResult::Success && PathLength > 80.0;
				AssertTrue(Record, TEXT("place.path"), bPath, TEXT("path"), FString::SanitizeFloat(PathLength), SubjectLabel);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				Advance(EPhase::Handoff);
				break;
			}
			case EPhase::Handoff:
			{
				UProjectOrganoidObjectiveDataAsset* Intro = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroMissionSoftPath);
				if (!Intro || !Objectives->LoadMission(Intro, false))
				{
					FailAndStop(Owner, Record, TEXT("Failed to load Beat 9 mission."));
					return;
				}
				Objectives->TriggerEvent(FName(IntroEvent));
				AssertTrue(Record, TEXT("handoff.use_current"), Objectives->GetActiveMissionId() == FName(UseMissionId), UseMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
				AssertTrue(Record, TEXT("handoff.objective_active"), CountActiveId(Objectives, FName(UseObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(UseObjectiveId))), UseObjectiveId);
				AssertTrue(Record, TEXT("handoff.still_dormant"), !Subject->IsEncounterActivated(), TEXT("dormant"), Subject->IsEncounterActivated() ? TEXT("active") : TEXT("dormant"), SubjectLabel);
				Adapt->UnlockAdaptation(Neural);
				Adapt->UnequipAdaptation();
				StopIfFailed();
				if (bAnyAssertFailed) return;
				Advance(EPhase::SettleUnequipped);
				break;
			}
			case EPhase::SettleUnequipped:
				if (!Settle(Character, PlayerStand, Subject, DeltaTime)) return;
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitUnequipped;
				break;
			case EPhase::WaitUnequipped:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::Unequipped)
				{
					AssertTrue(Record, TEXT("reject.unequipped"), NoSpendRejection(World, Character, Subject, Objectives), TEXT("Unequipped no 20-spend inside recharge envelope"), NoSpendText(World, Character, Adapt), TEXT("input"));
					StopIfFailed();
					if (!bAnyAssertFailed) { Adapt->EquipAdaptation(Neural); Advance(EPhase::SettleNoTarget); }
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.unequipped timed out %s"), *NoSpendText(World, Character, Adapt)));
				break;
			case EPhase::SettleNoTarget:
			{
				const float Range = Neural->MaxTargetRange;
				if (!bPlaced)
				{
					Character->SetActorLocation(PlayerStand, false, nullptr, ETeleportType::TeleportPhysics);
					StopPawn(Character);
					SetCameraLagEnabled(Character, false);
					float Yaw = 0.f;
					FAimSample Predicted;
					if (!SelectNoTargetYaw(World, Character, Range, Subject, Yaw, Predicted))
					{
						FailAndStop(Owner, Record, FString::Printf(TEXT("setup.no_target yaw selection failed %s"), *FormatAim(Predicted)));
						return;
					}
					RequestedAim = FRotator(NoTargetPitchDegrees, Yaw, 0.f);
					bPlaced = true;
					WaitSeconds = 0.f;
				}
				ApplyControlAim(Character, RequestedAim);
				FAimSample Live;
				if (!ReadLiveAim(World, Character, RequestedAim, Range, Subject, Live))
				{
					FailAndStop(Owner, Record, TEXT("setup.no_target camera unavailable"));
					return;
				}
				if (!Live.bViewMatchesRequest)
				{
					WaitSeconds += DeltaTime;
					if (WaitSeconds > ViewMatchTimeoutSeconds)
					{
						FailAndStop(Owner, Record, FString::Printf(TEXT("setup.no_target camera timed out %s"), *FormatAim(Live)));
					}
					return;
				}
				if (Live.Reason != EProjectOrganoidBiologicalAdaptationFailReason::NoTarget)
				{
					FailAndStop(Owner, Record, FString::Printf(TEXT("setup.no_target preflight %s"), *FormatAim(Live)));
					return;
				}
				LastAimText = FormatAim(Live);
				AssertTrue(Record, TEXT("setup.no_target"), true, TEXT("NoTarget"), LastAimText, TEXT("camera"));
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitNoTarget;
				break;
			}
			case EPhase::WaitNoTarget:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::NoTarget)
				{
					AssertTrue(Record, TEXT("reject.no_target"), NoSpendRejection(World, Character, Subject, Objectives), TEXT("NoTarget no 20-spend inside recharge envelope"), FString::Printf(TEXT("%s %s"), *NoSpendText(World, Character, Adapt), *LastAimText), TEXT("input"));
					StopIfFailed();
					if (!bAnyAssertFailed) Advance(EPhase::SettleOutOfRange);
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.no_target timed out %s %s"), *NoSpendText(World, Character, Adapt), *LastAimText));
				break;
			case EPhase::SettleOutOfRange:
			{
				const float Range = Neural->MaxTargetRange;
				if (!bPlaced)
				{
					PlaceStill(Character, FarStand);
					bPlaced = true;
					bAimCommanded = false;
					bTargetGateStable = false;
					StableTargetHost = nullptr;
					WaitSeconds = 0.f;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, Subject, EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange, Range, DeltaTime, TEXT("SettleOutOfRange"), Live)) return;
				ReportedActorDistance = Live.ActorDistance;
				ReportedConeDot = Live.SubjectDot;
				LastAimText = TargetGateText(TEXT("SettleOutOfRange"), EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange, Character, Subject, Live);
				AssertTrue(Record, TEXT("setup.out_of_range"), Live.CandidateHost == Subject && Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange, TEXT("OutOfRange"), LastAimText, SubjectLabel);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitOutOfRange;
				break;
			}
			case EPhase::WaitOutOfRange:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange)
				{
					AssertTrue(Record, TEXT("reject.out_of_range"), NoSpendRejection(World, Character, Subject, Objectives), TEXT("OutOfRange no 20-spend inside recharge envelope"), FString::Printf(TEXT("actor=%.1f dot=%.4f %s"), ReportedActorDistance, ReportedConeDot, *NoSpendText(World, Character, Adapt)), TEXT("input"));
					StopIfFailed();
					if (!bAnyAssertFailed) Advance(EPhase::SettleDead);
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.out_of_range timed out actor=%.1f dot=%.4f %s"), ReportedActorDistance, ReportedConeDot, *NoSpendText(World, Character, Adapt)));
				break;
			case EPhase::SettleDead:
			{
				if (!bPlaced)
				{
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					DeadHost = World->SpawnActor<AProjectOrganoidHostBase>(Subject->GetClass(), FVector(2200.f, -1850.f, -1100.f), FRotator::ZeroRotator, Params);
					if (DeadHost.IsValid()) DeadHost->bIsDead = true;
					PlaceStill(Character, FVector(2050.f, -1850.f, -1100.f));
					bPlaced = true;
					bAimCommanded = false;
					WaitSeconds = 0.f;
				}
				if (!DeadHost.IsValid())
				{
					FailAndStop(Owner, Record, TEXT("setup.dead host was not spawned."));
					return;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, DeadHost.Get(), EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget, Neural->MaxTargetRange, DeltaTime, TEXT("SettleDead"), Live)) return;
				AssertTrue(Record, TEXT("setup.dead"), Live.CandidateHost == DeadHost.Get() && Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget, TEXT("InvalidTarget"), LastAimText, TEXT("input"));
				StopIfFailed();
				if (bAnyAssertFailed) return;
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitDead;
				break;
			}
			case EPhase::WaitDead:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget)
				{
					AssertTrue(Record, TEXT("reject.dead"), DeadHost.IsValid() && NoSpendRejection(World, Character, Subject, Objectives), TEXT("InvalidTarget no 20-spend inside recharge envelope"), NoSpendText(World, Character, Adapt), TEXT("input"));
					StopIfFailed();
					if (!bAnyAssertFailed)
					{
						Character->ApplyPEEnergyDelta(5.f - Character->GetPEEnergy());
						Advance(EPhase::SettleLowPE);
					}
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.dead timed out %s"), *NoSpendText(World, Character, Adapt)));
				break;
			case EPhase::SettleLowPE:
				if (!Settle(Character, PlayerStand, Subject, DeltaTime)) return;
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitLowPE;
				break;
			case EPhase::WaitLowPE:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::InsufficientPE)
				{
					const UProjectOrganoidBiologicalAdaptationData* Equipped = Adapt->GetEquippedAdaptation();
					const float Cost = Equipped ? Equipped->PECost : -1.f;
					const bool bBelowCost = FMath::IsNearlyEqual(Cost, NeuralSlowPECost, 0.01f) && NoSpendAnchor < Cost;
					AssertTrue(Record, TEXT("reject.low_pe"), bBelowCost && NoSpendRejection(World, Character, Subject, Objectives), TEXT("PE below 20, InsufficientPE, recharge only"), FString::Printf(TEXT("cost=%.3f %s"), Cost, *NoSpendText(World, Character, Adapt)), SubjectLabel);
					StopIfFailed();
					if (!bAnyAssertFailed)
					{
						Character->ApplyPEEnergyDelta(100.f - Character->GetPEEnergy());
						if (DeadHost.IsValid()) DeadHost->Destroy();
						Advance(EPhase::SettleWrongHost);
					}
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.low_pe timed out %s"), *NoSpendText(World, Character, Adapt)));
				break;
			case EPhase::SettleWrongHost:
			{
				if (!bPlaced)
				{
					PlaceStill(Character, Host1->GetActorLocation() + FVector(180.f, 0.f, 0.f));
					bPlaced = true;
					bAimCommanded = false;
					WaitSeconds = 0.f;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, Host1, EProjectOrganoidBiologicalAdaptationFailReason::None, Neural->MaxTargetRange, DeltaTime, TEXT("SettleWrongHost"), Live)) return;
				const float CameraDistance = FVector::Dist(Live.TraceStart, Host1->GetActorLocation());
				const bool bTraceHit = Live.HitActor == HostName(Host1);
				const bool bCone = Live.SubjectDot >= ProductionAimConeCosine;
				const UProjectOrganoidBiologicalAdaptationData* Equipped = Adapt->GetEquippedAdaptation();
				const float Cost = Equipped ? Equipped->PECost : -1.f;
				const bool bReady =
					HostName(Host1) == Host1Label
					&& !Host1->bIsDead
					&& !Host1->bIsIncapacitated
					&& Live.CandidateHost == Host1
					&& Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::None
					&& Live.ActorDistance <= Neural->MaxTargetRange
					&& (bTraceHit || (bCone && CameraDistance <= Neural->MaxTargetRange))
					&& FMath::IsNearlyEqual(Cost, NeuralSlowPECost, 0.01f)
					&& Character->GetPEEnergy() >= Cost
					&& !Adapt->IsOnCooldown();
				AssertTrue(
					Record,
					TEXT("setup.wrong_host"),
					bReady,
					TEXT("Host_Neuro_1 alive, in range, cooldown ready, PE>=20"),
					FString::Printf(TEXT("alive=%d incapacitated=%d actor=%.1f cameraDist=%.1f dot=%.4f hit=%s pe=%.1f cooldown=%d %s"), Host1->bIsDead ? 0 : 1, Host1->bIsIncapacitated ? 1 : 0, Live.ActorDistance, CameraDistance, Live.SubjectDot, *Live.HitActor, Character->GetPEEnergy(), Adapt->IsOnCooldown() ? 1 : 0, *LastAimText),
					Host1Label);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				BeginSpendWatch(World, Character);
				if (!bInjected) bInjected = PressQ(Character);
				SampleSpend(Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitWrongHost;
				break;
			}
			case EPhase::WaitWrongHost:
				WaitSeconds += DeltaTime;
				SampleSpend(Character);
				if (Host1->IsBiologicalLocomotorSlowActive() && Adapt->IsOnCooldown())
				{
					const float Speed = Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f;
					AssertTrue(Record, TEXT("wrong_host.effect"), SpendCostHolds(World, Character) && FMath::IsNearlyEqual(Speed, 210.f, 8.f) && Host1->IsBiologicalLocomotorSlowActive() && NoCampaignCredit(Subject, Objectives), TEXT("20 pe, slow, cooldown, credit=0"), SpendText(World, Character, Host1, Subject, Objectives, Adapt, Speed), Host1Label);
					StopIfFailed();
					if (!bAnyAssertFailed)
					{
						EffectAppliedAt = FPlatformTime::Seconds();
						WaitSeconds = 0.f;
						Phase = EPhase::HoldBeforeExpiry;
					}
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("wrong_host.effect timed out %s"), *SpendText(World, Character, Host1, Subject, Objectives, Adapt, Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f)));
				break;
			case EPhase::HoldBeforeExpiry:
			{
				const double Elapsed = FPlatformTime::Seconds() - EffectAppliedAt;
				if (!Host1->IsBiologicalLocomotorSlowActive())
				{
					FailAndStop(Owner, Record, FString::Printf(TEXT("effect.still_active cleared early at %.3fs"), Elapsed));
					return;
				}
				if (Elapsed >= 3.2)
				{
					const float Speed = Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f;
					AssertTrue(Record, TEXT("effect.still_active"), FMath::IsNearlyEqual(Speed, 210.f, 8.f), TEXT("active before 4s"), FString::Printf(TEXT("elapsed=%.3f speed=%.1f"), Elapsed, Speed), Host1Label);
					StopIfFailed();
					if (!bAnyAssertFailed) { WaitSeconds = 0.f; Phase = EPhase::WaitNaturalClear; }
					return;
				}
				if (Elapsed > 4.0) FailAndStop(Owner, Record, TEXT("effect.still_active did not sample before expiry."));
				break;
			}
			case EPhase::WaitNaturalClear:
			{
				const double Elapsed = FPlatformTime::Seconds() - EffectAppliedAt;
				if (!Host1->IsBiologicalLocomotorSlowActive())
				{
					const float Speed = Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f;
					AssertTrue(Record, TEXT("effect.cleared"), Elapsed >= 3.95 && Elapsed <= 5.0 && FMath::IsNearlyEqual(Speed, 350.f, 8.f), TEXT("cleared near 4s at 350"), FString::Printf(TEXT("elapsed=%.3f speed=%.1f"), Elapsed, Speed), Host1Label);
					StopIfFailed();
					if (!bAnyAssertFailed) Advance(EPhase::SettleCooldown);
					return;
				}
				if (Elapsed > 5.0) FailAndStop(Owner, Record, FString::Printf(TEXT("Neural Slow still active at %.3fs"), Elapsed));
				break;
			}
			case EPhase::IssueHostRestore:
			{
				Place(Character, Host1->GetActorLocation() + FVector(150.f, 0.f, 0.f), Host1);
				Host1->TryActivateEncounterFromProximity(Character);
				Place(Character, FarStand, nullptr);
				FVector RestoreTarget = Host1->GetActorLocation() + FVector(500.f, 0.f, 0.f);
				UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				double PathLength = 0.0;
				if (!Nav || Nav->GetPathLength(Host1->GetActorLocation(), RestoreTarget, PathLength) != ENavigationQueryResult::Success || PathLength < 200.0)
				{
					RestoreTarget = Host1->GetActorLocation() + FVector(0.f, 500.f, 0.f);
				}
				if (AProjectOrganoidHostAIController* AI = Cast<AProjectOrganoidHostAIController>(Host1->GetController()))
				{
					AI->RequestInvestigateAt(RestoreTarget);
				}
				MoveAnchor = Host1->GetActorLocation();
				WaitSeconds = 0.f;
				Phase = EPhase::WaitHostRestoreMoving;
				break;
			}
			case EPhase::WaitHostRestoreMoving:
				WaitSeconds += DeltaTime;
				if (FVector::Dist2D(Host1->GetActorLocation(), MoveAnchor) > 20.f)
				{
					MoveAnchor = Host1->GetActorLocation();
					WaitSeconds = 0.f;
					Phase = EPhase::MeasureHostRestore;
					return;
				}
				if (WaitSeconds > MoveTimeoutSeconds) FailAndStop(Owner, Record, TEXT("Ordinary host did not resume movement after Neural Slow."));
				break;
			case EPhase::MeasureHostRestore:
				WaitSeconds += DeltaTime;
				if (WaitSeconds < SampleSeconds) return;
				{
					const float RestoredDisplacement = FVector::Dist2D(Host1->GetActorLocation(), MoveAnchor);
					const float Speed = Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f;
					AssertTrue(Record, TEXT("effect.restored_move"), RestoredDisplacement > 8.f && FMath::IsNearlyEqual(Speed, 350.f, 8.f), TEXT("restored displacement"), FString::Printf(TEXT("moved=%.2f speed=%.1f"), RestoredDisplacement, Speed), Host1Label);
				}
				StopIfFailed();
				if (!bAnyAssertFailed) { WaitSeconds = 0.f; Phase = EPhase::WaitCooldownExpire; }
				break;
			case EPhase::SettleCooldown:
			{
				if (!bPlaced)
				{
					PlaceStill(Character, Host1->GetActorLocation() + FVector(180.f, 0.f, 0.f));
					bPlaced = true;
					bAimCommanded = false;
					WaitSeconds = 0.f;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, Host1, EProjectOrganoidBiologicalAdaptationFailReason::None, Neural->MaxTargetRange, DeltaTime, TEXT("SettleCooldown"), Live)) return;
				AssertTrue(Record, TEXT("setup.cooldown_target"), Live.CandidateHost == Host1 && Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::None && Adapt->IsOnCooldown(), TEXT("Host_Neuro_1 in range before cooldown reject"), LastAimText, Host1Label);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				InjectNoSpendQ(World, Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitCooldown;
				break;
			}
			case EPhase::WaitCooldown:
				WaitSeconds += DeltaTime;
				SampleNoSpend(Character);
				if (Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::Cooldown)
				{
					AssertTrue(Record, TEXT("reject.cooldown"), NoSpendRejection(World, Character, Subject, Objectives) && Adapt->IsOnCooldown(), TEXT("Cooldown no 20-spend inside recharge envelope"), NoSpendText(World, Character, Adapt), TEXT("input"));
					StopIfFailed();
					if (!bAnyAssertFailed) { WaitSeconds = 0.f; Phase = EPhase::IssueHostRestore; }
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("reject.cooldown timed out %s"), *NoSpendText(World, Character, Adapt)));
				break;
			case EPhase::WaitCooldownExpire:
				WaitSeconds += DeltaTime;
				if (!Adapt->IsOnCooldown())
				{
					AssertTrue(Record, TEXT("cooldown.expired"), !Host1->IsBiologicalLocomotorSlowActive() || WaitSeconds >= 4.f, TEXT("ready"), Adapt->IsOnCooldown() ? TEXT("cooling") : TEXT("ready"), TEXT("cooldown"));
					StopIfFailed();
					if (!bAnyAssertFailed) Advance(EPhase::IssueBaselineLeg);
					return;
				}
				if (WaitSeconds > CooldownTimeoutSeconds) FailAndStop(Owner, Record, TEXT("Production 8s cooldown did not expire."));
				break;
			case EPhase::IssueBaselineLeg:
				if (!ChooseLeg(World, Subject))
				{
					FailAndStop(Owner, Record, TEXT("No nav leg longer than 450uu from the campaign host."));
					return;
				}
				Place(Character, Subject->GetActorLocation() + FVector(0.f, 150.f, 0.f), Subject);
				Subject->TryActivateEncounterFromProximity(Character);
				Place(Character, FarStand, nullptr);
				if (AProjectOrganoidHostAIController* AI = Cast<AProjectOrganoidHostAIController>(Subject->GetController()))
				{
					AI->RequestInvestigateAt(LegTarget);
				}
				MoveAnchor = Subject->GetActorLocation();
				WaitSeconds = 0.f;
				Phase = EPhase::WaitBaselineMoving;
				break;
			case EPhase::WaitBaselineMoving:
				WaitSeconds += DeltaTime;
				if (FVector::Dist2D(Subject->GetActorLocation(), MoveAnchor) > 40.f)
				{
					MoveAnchor = Subject->GetActorLocation();
					WaitSeconds = 0.f;
					Phase = EPhase::MeasureBaseline;
					return;
				}
				if (WaitSeconds > MoveTimeoutSeconds) FailAndStop(Owner, Record, TEXT("Campaign host did not start the baseline leg."));
				break;
			case EPhase::MeasureBaseline:
				WaitSeconds += DeltaTime;
				if (WaitSeconds < SampleSeconds) return;
				BaselineDisplacement = FVector::Dist2D(Subject->GetActorLocation(), MoveAnchor);
				AssertTrue(Record, TEXT("move.before_slow"), Subject->IsEncounterActivated() && BaselineDisplacement > 8.f, TEXT("moving"), FString::SanitizeFloat(BaselineDisplacement), SubjectLabel);
				StopIfFailed();
				if (!bAnyAssertFailed)
				{
					Character->ApplyPEEnergyDelta(100.f - Character->GetPEEnergy());
					Advance(EPhase::SettleSuccess);
				}
				break;
			case EPhase::SettleSuccess:
			{
				if (!bPlaced)
				{
					PlaceStill(Character, Subject->GetActorLocation() + FVector(180.f, 0.f, 0.f));
					bPlaced = true;
					bAimCommanded = false;
					WaitSeconds = 0.f;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, Subject, EProjectOrganoidBiologicalAdaptationFailReason::None, Neural->MaxTargetRange, DeltaTime, TEXT("SettleSuccess"), Live)) return;
				AssertTrue(Record, TEXT("setup.success_target"), Live.CandidateHost == Subject && Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::None, TEXT("campaign host in range"), LastAimText, SubjectLabel);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				BeginSpendWatch(World, Character);
				if (!bInjected) bInjected = PressQ(Character);
				SampleSpend(Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitSuccess;
				break;
			}
			case EPhase::WaitSuccess:
				WaitSeconds += DeltaTime;
				SampleSpend(Character);
				if (Subject->IsBiologicalLocomotorSlowActive() && Adapt->IsOnCooldown())
				{
					const float Speed = Subject->GetCharacterMovement() ? Subject->GetCharacterMovement()->MaxWalkSpeed : 0.f;
					UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
					const FString Line = Widget ? Widget->GetLastResourceNotification().ToString() : FString(TEXT("no hud"));
					const float RemainingNote = Widget ? Widget->GetTransientNotificationSecondsRemaining() : -1.f;
					AssertTrue(Record, TEXT("apply.identity"), Adapt->GetEquippedAdaptationPath().ToString() == AdaptationPath, AdaptationPath, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
					AssertTrue(Record, TEXT("apply.pe"), SpendCostHolds(World, Character), TEXT("20"), SpendText(World, Character, Subject, Subject, Objectives, Adapt, Speed), TEXT("PE"));
					AssertTrue(Record, TEXT("apply.cooldown"), Adapt->IsOnCooldown() && Adapt->GetCooldownRemaining() > 6.f, TEXT("8"), FString::SanitizeFloat(Adapt->GetCooldownRemaining()), TEXT("cooldown"));
					AssertTrue(Record, TEXT("apply.speed"), FMath::IsNearlyEqual(Speed, 210.f, 8.f), TEXT("210"), FString::SanitizeFloat(Speed), SubjectLabel);
					AssertTrue(Record, TEXT("apply.event_once"), Subject->AdaptationCampaignEventFireCount == 1, TEXT("1"), FString::FromInt(Subject->AdaptationCampaignEventFireCount), SubjectLabel);
					AssertTrue(Record, TEXT("apply.nathan"), Line.Contains(ExpectedLine) && Subject->AdaptationCampaignNotificationCount == 1, ExpectedLine, Line, TEXT("HUD"));
					AssertTrue(Record, TEXT("apply.nathan_duration"), FMath::IsNearlyEqual(Subject->AdaptationCampaignNotificationDurationSeconds, 7.f, 0.05f) && RemainingNote > 6.f && RemainingNote <= 7.f, TEXT("7"), FString::SanitizeFloat(RemainingNote), TEXT("HUD"));
					AssertTrue(Record, TEXT("apply.objective"), CountCompletedId(Objectives, FName(UseObjectiveId)) == 1 && CountActiveId(Objectives, FName(UseObjectiveId)) == 0, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(UseObjectiveId))), UseObjectiveId);
					AssertTrue(Record, TEXT("apply.mission"), Objectives->IsMissionComplete(FName(UseMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
					StopIfFailed();
					if (!bAnyAssertFailed)
					{
						MoveAnchor = Subject->GetActorLocation();
						WaitSeconds = 0.f;
						Phase = EPhase::MeasureSlow;
					}
					return;
				}
				if (WaitSeconds > InputTimeoutSeconds) FailAndStop(Owner, Record, FString::Printf(TEXT("apply timed out %s"), *CreditActual(Subject, Objectives, Adapt, Character)));
				break;
			case EPhase::MeasureSlow:
				WaitSeconds += DeltaTime;
				if (WaitSeconds < SampleSeconds) return;
				SlowDisplacement = FVector::Dist2D(Subject->GetActorLocation(), MoveAnchor);
				AssertTrue(
					Record,
					TEXT("apply.observable_slow"),
					Subject->IsBiologicalLocomotorSlowActive() && SlowDisplacement > 8.f && SlowDisplacement < BaselineDisplacement * 0.80f,
					TEXT("slower than baseline"),
					FString::Printf(TEXT("slow=%.2f baseline=%.2f"), SlowDisplacement, BaselineDisplacement),
					SubjectLabel);
				StopIfFailed();
				if (!bAnyAssertFailed) { WaitSeconds = 0.f; Phase = EPhase::SaveWhileActive; }
				break;
			case EPhase::SaveWhileActive:
			{
				Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
				Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
				const EProjectOrganoidPowerState NeuroState = Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics);
				const EProjectOrganoidPowerState CryoState = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo);
				AssertTrue(Record, TEXT("power.neuro_online"), NeuroState == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(NeuroState), TEXT("Power"));
				AssertTrue(Record, TEXT("power.cryo_blackout"), CryoState == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(CryoState), TEXT("Power"));
				CooldownAtSave = Adapt->GetCooldownRemaining();
				AssertTrue(Record, TEXT("save.cooldown_active"), Subject->IsBiologicalLocomotorSlowActive() && CooldownAtSave > 5.f, TEXT("slow and cooldown"), FString::Printf(TEXT("slow=%d cooldown=%.2f"), Subject->IsBiologicalLocomotorSlowActive() ? 1 : 0, CooldownAtSave), TEXT("cooldown"));
				Saves->DeleteSave(SaveSlot);
				const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
				SavedPE = Character->GetPEEnergy();
				AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
				AssertTrue(
					Record,
					TEXT("save.captured"),
					bSaved && Subject->IsBiologicalLocomotorSlowActive() && CooldownAtSave > 5.f,
					TEXT("equipped, PE, mission complete, guard closed, slow, cooldown"),
					FString::Printf(
						TEXT("path=%s pe=%.1f cooldown=%.2f slow=1 mission=%s completed=%d active=%d neuro=%s cryo=%s"),
						*Adapt->GetEquippedAdaptationPath().ToString(),
						SavedPE,
						CooldownAtSave,
						*Objectives->GetActiveMissionId().ToString(),
						CountCompletedId(Objectives, FName(UseObjectiveId)),
						CountActiveId(Objectives, FName(UseObjectiveId)),
						PowerText(NeuroState),
						PowerText(CryoState)),
					TEXT("save"));
				StopIfFailed();
				if (!bAnyAssertFailed)
				{
					Stage = EStage::EndSession;
				}
				break;
			}
			case EPhase::FreshLoad:
			{
				if (bFreshEvaluated) return;
				bFreshEvaluated = true;
				const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				Adapt = Character->GetBiologicalAdaptationComponent();
				Subject = Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, SubjectLabel));
				Host1 = Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, Host1Label));
				int32 SlowHosts = 0;
				for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
				{
					if (It->IsBiologicalLocomotorSlowActive()) ++SlowHosts;
				}
				const float CooldownRemaining = Adapt ? Adapt->GetCooldownRemaining() : -1.f;
				const float SubjectSpeed = Subject && Subject->GetCharacterMovement() ? Subject->GetCharacterMovement()->MaxWalkSpeed : 0.f;
				const float Host1Speed = Host1 && Host1->GetCharacterMovement() ? Host1->GetCharacterMovement()->MaxWalkSpeed : 0.f;
				AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
				AssertTrue(Record, TEXT("save.mission"), Objectives->IsMissionComplete(FName(UseMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
				AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(UseObjectiveId)) == 1 && CountActiveId(Objectives, FName(UseObjectiveId)) == 0, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(UseObjectiveId))), UseObjectiveId);
				AssertTrue(Record, TEXT("save.equipped"), Adapt && Adapt->GetEquippedAdaptationPath().ToString() == AdaptationPath, AdaptationPath, Adapt ? Adapt->GetEquippedAdaptationPath().ToString() : TEXT("missing"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("save.pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), SavedPE, 1.f), FString::SanitizeFloat(SavedPE), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
				AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
				AssertTrue(Record, TEXT("save.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
				AssertTrue(Record, TEXT("save.replay_guard"), CountCompletedId(Objectives, FName(UseObjectiveId)) == 1 && CountActiveId(Objectives, FName(UseObjectiveId)) == 0, TEXT("closed"), FString::Printf(TEXT("completed=%d active=%d"), CountCompletedId(Objectives, FName(UseObjectiveId)), CountActiveId(Objectives, FName(UseObjectiveId))), UseObjectiveId);
				AssertTrue(Record, TEXT("save.cooldown_ready"), Adapt && !Adapt->IsOnCooldown() && CooldownRemaining <= 0.05f, TEXT("0"), FString::SanitizeFloat(CooldownRemaining), TEXT("cooldown"));
				AssertTrue(Record, TEXT("save.slow_absent"), SlowHosts == 0 && Subject && !Subject->IsBiologicalLocomotorSlowActive() && Host1 && !Host1->IsBiologicalLocomotorSlowActive(), TEXT("0"), FString::FromInt(SlowHosts), TEXT("hosts"));
				AssertTrue(Record, TEXT("save.speed_normal"), FMath::IsNearlyEqual(SubjectSpeed, 350.f, 8.f) && FMath::IsNearlyEqual(Host1Speed, 350.f, 8.f), TEXT("350"), FString::Printf(TEXT("subject=%.1f host1=%.1f"), SubjectSpeed, Host1Speed), SubjectLabel);
				PostLoadFire = Subject ? Subject->AdaptationCampaignEventFireCount : -1;
				PostLoadNotes = Subject ? Subject->AdaptationCampaignNotificationCount : -1;
				StopIfFailed();
				if (!bAnyAssertFailed) Advance(EPhase::SettleReplay);
				break;
			}
			case EPhase::SettleReplay:
			{
				if (!bPlaced)
				{
					PlaceStill(Character, PlayerStand);
					bPlaced = true;
					bAimCommanded = false;
					WaitSeconds = 0.f;
				}
				FAimSample Live;
				if (!AcquireLiveTarget(Owner, Record, World, Character, Subject, EProjectOrganoidBiologicalAdaptationFailReason::None, Neural->MaxTargetRange, DeltaTime, TEXT("SettleReplay"), Live)) return;
				AssertTrue(Record, TEXT("setup.replay_target"), Live.CandidateHost == Subject && Live.Reason == EProjectOrganoidBiologicalAdaptationFailReason::None, TEXT("campaign host in range"), LastAimText, SubjectLabel);
				StopIfFailed();
				if (bAnyAssertFailed) return;
				if (!bInjected) bInjected = PressQ(Character);
				WaitSeconds = 0.f;
				Phase = EPhase::WaitReplay;
				break;
			}
			case EPhase::WaitReplay:
				WaitSeconds += DeltaTime;
				{
					const int32 Completed = CountCompletedId(Objectives, FName(UseObjectiveId));
					const bool bDuplicate = Subject->AdaptationCampaignEventFireCount != PostLoadFire
						|| Subject->AdaptationCampaignNotificationCount != PostLoadNotes
						|| Completed != 1
						|| CountActiveId(Objectives, FName(UseObjectiveId)) != 0;
					if (bDuplicate)
					{
						AssertTrue(Record, TEXT("replay.no_second_credit"), false, TEXT("no second credit"), FString::Printf(TEXT("fire=%d note=%d completed=%d active=%d"), Subject->AdaptationCampaignEventFireCount, Subject->AdaptationCampaignNotificationCount, Completed, CountActiveId(Objectives, FName(UseObjectiveId))), SubjectLabel);
						StopIfFailed();
						return;
					}
					if (WaitSeconds < 1.5f) return;
					AssertTrue(Record, TEXT("replay.no_second_credit"), true, TEXT("no second credit"), FString::Printf(TEXT("fire=%d note=%d completed=%d pe=%.1f slow=%d"), PostLoadFire, PostLoadNotes, Completed, Character->GetPEEnergy(), Subject->IsBiologicalLocomotorSlowActive() ? 1 : 0), SubjectLabel);
					Saves->DeleteSave(SaveSlot);
					AssertTrue(Record, TEXT("cleanup.slot"), !Saves->DoesSaveExist(SaveSlot), TEXT("absent"), Saves->DoesSaveExist(SaveSlot) ? TEXT("present") : TEXT("absent"), TEXT("save"));
					if (DeadHost.IsValid()) DeadHost->Destroy();
					StopIfFailed();
					if (!bAnyAssertFailed) Phase = EPhase::Done;
				}
				break;
			case EPhase::Done:
				Stage = EStage::EndPie;
				break;
			default:
				FailAndStop(Owner, Record, TEXT("Unknown Beat 10 phase."));
				break;
			}
		}
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroNeuralSlowUseFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister GRegister;
}
