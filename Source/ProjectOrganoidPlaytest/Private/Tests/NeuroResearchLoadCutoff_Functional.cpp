#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/PackageName.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveGame.h"
#include "UObject/Package.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("NeuroResearchLoadCutoff_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Research Load Cutoff Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");

	constexpr TCHAR MissionId[] = TEXT("Mission_NeuroGenetics");
	constexpr TCHAR MissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR MissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	constexpr TCHAR MissionTitle[] = TEXT("NeuroGenetics");
	constexpr TCHAR MissionDescription[] =
		TEXT("Isolate the unstable research load before reconnecting the primary feed.");

	constexpr TCHAR IsolateId[] = TEXT("Obj_IsolateNeuroResearchLoad");
	constexpr TCHAR IsolateTitle[] = TEXT("Isolate the NeuroGenetics research load");
	constexpr TCHAR IsolateDescription[] =
		TEXT("Find the emergency cutoff feeding the unstable research equipment.");
	constexpr TCHAR IsolateEvent[] = TEXT("Event_NeuroResearchLoadIsolated");

	constexpr TCHAR TraceId[] = TEXT("Obj_TraceNeuralMappingSignal");
	constexpr TCHAR TraceTitle[] = TEXT("Trace the neural mapping signal");
	constexpr TCHAR TraceDescription[] =
		TEXT("Use the research-floor systems to determine what the array was monitoring.");
	constexpr TCHAR TraceEvent[] = TEXT("Event_NeuralMappingSignalTraced");

	constexpr TCHAR FollowId[] = TEXT("Obj_FollowNeuralSignature");
	constexpr TCHAR FollowTitle[] = TEXT("Follow the neural signature");
	constexpr TCHAR FollowDescription[] =
		TEXT("Track the matching neural pattern deeper into the research wing.");
	constexpr TCHAR FollowEvent[] = TEXT("Event_NeuralSignatureFollowed");

	constexpr TCHAR ExamineId[] = TEXT("Obj_ExamineNeuralChangeEvidence");
	constexpr TCHAR ExamineTitle[] = TEXT("Examine the neural-change evidence");
	constexpr TCHAR ExamineDescription[] =
		TEXT("Inspect the research-wing evidence linked to the matching neural signature.");

	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");

	constexpr TCHAR CutoffLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad");
	constexpr TCHAR CutoffReloadLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad_ReloadClone");
	constexpr TCHAR ArrayLabel[] = TEXT("NeuralMappingArray_NeuroGenetics");
	constexpr TCHAR ArrayOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR ObservationNodeLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR PanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR GateNoneLabel[] = TEXT("EmergencyCutoff_GateNone_Test");
	constexpr TCHAR GateNoneObjId[] = TEXT("Obj_CutoffGateNone_Test");
	constexpr TCHAR GateNoneEvent[] = TEXT("Event_CutoffGateNone_Test");

	constexpr TCHAR CutoffInspectPrompt[] = TEXT("Isolate Research Load");
	constexpr TCHAR CutoffReviewPrompt[] = TEXT("Research Load Isolated");
	constexpr TCHAR ArrayInspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ArrayReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	constexpr TCHAR CutoffLine[] =
		TEXT("That cut the feed. The array\u2019s offline, but its last mapping data should still be here.");
	constexpr TCHAR CutoffRendered[] =
		TEXT("Nathan: That cut the feed. The array\u2019s offline, but its last mapping data should still be here.");
	constexpr TCHAR ArrayLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");

	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	const FVector CutoffLocation(-100.0f, -600.0f, -1100.0f);
	constexpr float CutoffInteractionRange = 175.0f;
	constexpr float CutoffNotifySeconds = 4.0f;
	const FVector CutoffPedestalRel(0.0f, 0.0f, 35.0f);
	const FVector CutoffPedestalScale(0.45f, 0.45f, 0.70f);
	const FVector CutoffColumnRel(0.0f, 0.0f, 100.0f);
	const FVector CutoffColumnScale(0.65f, 0.30f, 0.60f);
	const FVector CutoffHeadRel(0.0f, 0.0f, 150.0f);
	const FVector CutoffHeadScale(0.80f, 0.40f, 0.25f);

	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);
	constexpr float ArrayInteractionRange = 200.0f;
	constexpr float ArrayNotifySeconds = 4.0f;
	const FVector ArrayPedestalRel(0.0f, 0.0f, 40.0f);
	const FVector ArrayPedestalScale(1.2f, 1.2f, 0.8f);
	const FVector ArrayColumnRel(0.0f, 0.0f, 110.0f);
	const FVector ArrayColumnScale(0.35f, 0.35f, 1.4f);
	const FVector ArrayHeadRel(0.0f, 0.0f, 192.5f);
	const FVector ArrayHeadScale(1.6f, 1.6f, 0.25f);
	const FVector StationLocation(800.0f, -1600.0f, -1100.0f);

	const FVector TerminalLocation(300.0f, -600.0f, -1100.0f);
	constexpr float TerminalInteractionRange = 175.0f;
	constexpr float TerminalNotifySeconds = 4.0f;
	constexpr TCHAR TerminalInspectPrompt[] = TEXT("Trace Neural Mapping Signal");
	constexpr TCHAR TerminalReviewPrompt[] = TEXT("Signal Trace Complete");
	constexpr TCHAR TerminalLine[] =
		TEXT("These scans line up with the victims\u2019 neural changes. Something\u2019s been tracking the same pattern across all of them.");
	const FVector TerminalPedestalRel(0.0f, 0.0f, 30.0f);
	const FVector TerminalPedestalScale(0.55f, 0.45f, 0.60f);
	const FVector TerminalColumnRel(0.0f, 0.0f, 90.0f);
	const FVector TerminalColumnScale(0.50f, 0.25f, 0.60f);
	const FVector TerminalHeadRel(0.0f, 0.0f, 145.0f);
	const FVector TerminalHeadScale(0.75f, 0.35f, 0.25f);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	void CollectDirtyPackageNames(TArray<FString>& Out)
	{
		Out.Reset();
		TArray<UPackage*> WorldDirty;
		TArray<UPackage*> ContentDirty;
		FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
		FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
		auto Add = [&Out](const TArray<UPackage*>& Packages)
		{
			for (UPackage* Package : Packages)
			{
				if (Package)
				{
					Out.AddUnique(Package->GetName());
				}
			}
		};
		Add(WorldDirty);
		Add(ContentDirty);
		Out.Sort();
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString PowerStateName(EProjectOrganoidPowerState State)
	{
		switch (State)
		{
		case EProjectOrganoidPowerState::Online: return TEXT("Online");
		case EProjectOrganoidPowerState::Emergency: return TEXT("Emergency");
		case EProjectOrganoidPowerState::Blackout: return TEXT("Blackout");
		default: return TEXT("Unknown");
		}
	}

	FString ObjectiveStateName(EProjectOrganoidObjectiveState State)
	{
		return UEnum::GetValueAsString(State);
	}

	int32 CountActiveId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives)
		{
			return 0;
		}
		for (const FProjectOrganoidObjective& Entry : Objectives->GetActiveObjectives())
		{
			if (Entry.ObjectiveId == Id)
			{
				++Count;
			}
		}
		return Count;
	}

	int32 CountCompletedId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives)
		{
			return 0;
		}
		for (const FProjectOrganoidObjective& Entry : Objectives->GetCompletedObjectives())
		{
			if (Entry.ObjectiveId == Id)
			{
				++Count;
			}
		}
		return Count;
	}

	bool VecNear(const FVector& A, const FVector& B, float Eps = 0.51f)
	{
		return FMath::Abs(A.X - B.X) <= Eps
			&& FMath::Abs(A.Y - B.Y) <= Eps
			&& FMath::Abs(A.Z - B.Z) <= Eps;
	}

	bool RotNear(const FRotator& A, const FRotator& B, float Eps = 0.51f)
	{
		return FMath::Abs(A.Pitch - B.Pitch) <= Eps
			&& FMath::Abs(A.Yaw - B.Yaw) <= Eps
			&& FMath::Abs(A.Roll - B.Roll) <= Eps;
	}

	bool MeshPathMatches(UStaticMeshComponent* Mesh, const TCHAR* ExpectedPath)
	{
		if (!Mesh || !Mesh->GetStaticMesh())
		{
			return false;
		}
		const FString Path = Mesh->GetStaticMesh()->GetPathName();
		return Path.Equals(ExpectedPath, ESearchCase::CaseSensitive) || Path.Contains(ExpectedPath);
	}

	UProjectOrganoidObjectiveDataAsset* LoadPersistedMissionAsset()
	{
		return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionSoftPath);
	}

	class FNeuroResearchLoadCutoffFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			DirtyBefore.Reset();
			PersistedCutoff.Reset();
			TransientActors.Reset();
			OwnedHudWidget.Reset();
			OpeningMissionProbe.Reset();
			OpeningMissionObjectives.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			UnbindOpeningMissionProbe();
			DestroyTransientActors();
			Owner.SetStage(TEXT("Abort"));
		}

		virtual void Tick(UProjectOrganoidPlaytestEditorSubsystem& Owner, float DeltaTime) override
		{
			FOrganoidPlaytestRecord* Record = Owner.GetActiveRecord();
			if (!Record)
			{
				return;
			}

			switch (Stage)
			{
			case EStage::Preflight:
				TickPreflight(Owner, *Record);
				break;
			case EStage::StartPie:
				TickStartPie(Owner, *Record);
				break;
			case EStage::WaitReady:
				TickWaitReady(Owner, *Record, DeltaTime);
				break;
			case EStage::Proof:
				TickProof(Owner, *Record);
				break;
			case EStage::EndPie:
				UnbindOpeningMissionProbe();
				DestroyTransientActors();
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitStopped;
				Owner.SetStage(TEXT("WaitPieStopped"));
				break;
			case EStage::WaitStopped:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress() || WaitSeconds > 20.0f)
				{
					Stage = EStage::AssertDurable;
				}
				break;
			case EStage::AssertDurable:
				TickAssertDurable(Owner, *Record);
				break;
			case EStage::Finalize:
				Owner.CompleteActive(
					bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass,
					Record->FailureReason);
				break;
			}
		}

	private:
		enum class EStage : uint8
		{
			Preflight,
			StartPie,
			WaitReady,
			Proof,
			EndPie,
			WaitStopped,
			AssertDurable,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidInspectableInstrument> PersistedCutoff;
		TArray<TWeakObjectPtr<AProjectOrganoidInspectableInstrument>> TransientActors;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> OwnedHudWidget;
		TStrongObjectPtr<UOrganoidNeuroResearchFloorArrayMissionCompletionProbe> OpeningMissionProbe;
		TWeakObjectPtr<UProjectOrganoidObjectiveSubsystem> OpeningMissionObjectives;

		void UnbindOpeningMissionProbe()
		{
			if (UProjectOrganoidObjectiveSubsystem* Objectives = OpeningMissionObjectives.Get())
			{
				if (UOrganoidNeuroResearchFloorArrayMissionCompletionProbe* LiveProbe = OpeningMissionProbe.Get())
				{
					Objectives->OnMissionCompleted.RemoveDynamic(
						LiveProbe,
						&UOrganoidNeuroResearchFloorArrayMissionCompletionProbe::HandleMissionCompleted);
				}
			}
			OpeningMissionProbe.Reset();
			OpeningMissionObjectives.Reset();
		}

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			UnbindOpeningMissionProbe();
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
		}

		bool AssertTrue(
			FOrganoidPlaytestRecord& Record,
			const FString& Id,
			bool bPassed,
			const FString& Expected,
			const FString& Actual,
			const FString& ActorId)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, false);
			if (!bPassed)
			{
				bAnyAssertFailed = true;
				if (Record.FailureReason.IsEmpty())
				{
					Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
				}
			}
			return bPassed;
		}

		void DestroyTransientActors()
		{
			for (const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& Weak : TransientActors)
			{
				if (AProjectOrganoidInspectableInstrument* Live = Weak.Get())
				{
					const FString Label = OrganoidPlaytestActions::ActorLabel(Live);
					if (!Label.Equals(CutoffLabel, ESearchCase::CaseSensitive)
						&& !Label.Equals(ArrayLabel, ESearchCase::CaseSensitive))
					{
						Live->Destroy();
					}
				}
			}
			TransientActors.Reset();
			OwnedHudWidget.Reset();
		}

		void Track(AProjectOrganoidInspectableInstrument* Actor)
		{
			if (Actor)
			{
				TransientActors.Add(Actor);
			}
		}

		void ApplyPresentationMesh(
			UStaticMeshComponent* Mesh,
			const TCHAR* Path,
			const FVector& Rel,
			const FVector& Scale) const
		{
			if (!Mesh)
			{
				return;
			}
			if (UStaticMesh* Asset = LoadObject<UStaticMesh>(nullptr, Path))
			{
				Mesh->SetStaticMesh(Asset);
			}
			Mesh->SetRelativeLocation(Rel);
			Mesh->SetRelativeRotation(FRotator::ZeroRotator);
			Mesh->SetRelativeScale3D(Scale);
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetGenerateOverlapEvents(false);
		}

		void ConfigureCutoffIdentically(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(CutoffLocation);
			Actor->SetActorRotation(FRotator::ZeroRotator);
			Actor->SetActorScale3D(FVector::OneVector);
			Actor->InteractionRange = CutoffInteractionRange;
			Actor->RequiredActiveObjectiveId = FName(IsolateId);
			Actor->ObjectiveEventId = FName(IsolateEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(IsolateId);
			Actor->InspectionPrompt = FText::FromString(CutoffInspectPrompt);
			Actor->ReviewPrompt = FText::FromString(CutoffReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(CutoffLine);
			Actor->NotificationDurationSeconds = CutoffNotifySeconds;
			Actor->bHasBeenInspected = false;
			Actor->InspectionNotificationCount = 0;
			Actor->ObjectiveEventFireCount = 0;
			ApplyPresentationMesh(Actor->PedestalMesh, CubeMeshPath, CutoffPedestalRel, CutoffPedestalScale);
			ApplyPresentationMesh(Actor->ColumnMesh, CubeMeshPath, CutoffColumnRel, CutoffColumnScale);
			ApplyPresentationMesh(Actor->ArrayHeadMesh, CubeMeshPath, CutoffHeadRel, CutoffHeadScale);
			Actor->RefreshPrompt();
		}

		void ConfigureArrayIdentically(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(ArrayLocation);
			Actor->SetActorRotation(FRotator::ZeroRotator);
			Actor->SetActorScale3D(FVector::OneVector);
			Actor->InteractionRange = ArrayInteractionRange;
			Actor->RequiredActiveObjectiveId = NAME_None;
			Actor->ObjectiveEventId = FName(ArrayEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(ResearchFloorId);
			Actor->InspectionPrompt = FText::FromString(ArrayInspectPrompt);
			Actor->ReviewPrompt = FText::FromString(ArrayReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(ArrayLine);
			Actor->NotificationDurationSeconds = ArrayNotifySeconds;
			Actor->bHasBeenInspected = false;
			Actor->InspectionNotificationCount = 0;
			Actor->ObjectiveEventFireCount = 0;
			ApplyPresentationMesh(Actor->PedestalMesh, CubeMeshPath, ArrayPedestalRel, ArrayPedestalScale);
			ApplyPresentationMesh(Actor->ColumnMesh, CylinderMeshPath, ArrayColumnRel, ArrayColumnScale);
			ApplyPresentationMesh(Actor->ArrayHeadMesh, CylinderMeshPath, ArrayHeadRel, ArrayHeadScale);
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredCutoffClone(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			const FTransform Xform(FRotator::ZeroRotator, CutoffLocation, FVector::OneVector);
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(), Xform);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString());
			ConfigureCutoffIdentically(Actor);
			Actor->FinishSpawning(Xform);
			ConfigureCutoffIdentically(Actor);
			Track(Actor);
			return Actor;
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredArrayClone(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			const FTransform Xform(FRotator::ZeroRotator, ArrayLocation, FVector::OneVector);
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(), Xform);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString());
			ConfigureArrayIdentically(Actor);
			Actor->FinishSpawning(Xform);
			ConfigureArrayIdentically(Actor);
			Track(Actor);
			return Actor;
		}

		AProjectOrganoidInspectableInstrument* SpawnGateNone(UWorld* World)
		{
			if (!World)
			{
				return nullptr;
			}
			const FTransform Xform(FRotator::ZeroRotator, CutoffLocation + FVector(200.0f, 0.0f, 0.0f), FVector::OneVector);
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(), Xform);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(GateNoneLabel);
			Actor->InteractionRange = CutoffInteractionRange;
			Actor->RequiredActiveObjectiveId = NAME_None;
			Actor->ObjectiveEventId = FName(GateNoneEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(GateNoneObjId);
			Actor->InspectionPrompt = FText::FromString(TEXT("Gate None Probe"));
			Actor->ReviewPrompt = FText::FromString(TEXT("Gate None Review"));
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(TEXT("Gate none probe line."));
			Actor->NotificationDurationSeconds = CutoffNotifySeconds;
			Actor->bHasBeenInspected = false;
			Actor->FinishSpawning(Xform);
			Actor->RequiredActiveObjectiveId = NAME_None;
			Actor->RefreshPrompt();
			Track(Actor);
			return Actor;
		}

		UProjectOrganoidHUDWidget* ResolveOwnedHud(APlayerController* PC)
		{
			if (UProjectOrganoidHUDWidget* Existing = OwnedHudWidget.Get())
			{
				return Existing;
			}
			if (!PC)
			{
				return nullptr;
			}
			if (AProjectOrganoidGameMode* GameMode = PC->GetWorld()
				? PC->GetWorld()->GetAuthGameMode<AProjectOrganoidGameMode>()
				: nullptr)
			{
				if (UProjectOrganoidGameplayHUDController* Controller = GameMode->GetHUDControllerForPlayer(PC))
				{
					if (UProjectOrganoidHUDWidget* Bound = Controller->GetBoundHUDWidget())
					{
						OwnedHudWidget = Bound;
						return Bound;
					}
				}
			}
			return nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			CollectDirtyPackageNames(DirtyBefore);
			AssertTrue(
				Record, TEXT("preflight.map_package"),
				FPackageName::DoesPackageExist(MapPackage),
				TEXT("exists"),
				FPackageName::DoesPackageExist(MapPackage) ? TEXT("exists") : TEXT("missing"),
				MapPackage);
			AssertTrue(
				Record, TEXT("preflight.mission_package"),
				FPackageName::DoesPackageExist(MissionPackage),
				TEXT("exists"),
				FPackageName::DoesPackageExist(MissionPackage) ? TEXT("exists") : TEXT("missing"),
				MissionPackage);
			Stage = EStage::StartPie;
			Owner.SetStage(TEXT("StartPie"));
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			if (!Owner.RequestStartPie(MapPackage))
			{
				FailAndStop(Owner, Record, TEXT("Failed to start PIE on Lvl_Epitope."));
				return;
			}
			WaitSeconds = 0.0f;
			bRequestedNeuroStream = false;
			Stage = EStage::WaitReady;
			Owner.SetStage(TEXT("WaitPieReady"));
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World
				? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>()
				: nullptr;

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

			const TArray<AActor*> Cutoffs = World
				? OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel)
				: TArray<AActor*>();
			const TArray<AActor*> Arrays = World
				? OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel)
				: TArray<AActor*>();
			const TArray<AActor*> Terminals = World
				? OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel)
				: TArray<AActor*>();
			if (World && Character && Cutoffs.Num() == 1 && Arrays.Num() == 1 && Terminals.Num() == 1)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				FailAndStop(
					Owner, Record,
					TEXT("Timed out waiting for PIE player, EmergencyCutoff_NeuroResearchLoad, NeuralMappingArray_NeuroGenetics, and NeuralMappingTerminal_NeuroGenetics. Persist Beat 4 + cutoff + terminal saves before this test can pass."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			if (!World || !Character || !PC || !Power || !Objectives)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE world/character/power/objectives."));
				return;
			}

			UProjectOrganoidHUDWidget* HUD = ResolveOwnedHud(PC);
			AssertTrue(
				Record, TEXT("hud.owned_resolved"),
				HUD != nullptr,
				TEXT("owned HUD"),
				HUD ? TEXT("owned HUD") : TEXT("null"),
				TEXT("HUD"));
			if (!HUD)
			{
				FailAndStop(Owner, Record, TEXT("Failed to resolve owned HUD."));
				return;
			}

			const TArray<AActor*> StationsBefore = OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel);
			const TArray<AActor*> PanelsBefore = OrganoidPlaytestActions::FindActorsByLabel(World, PanelLabel);
			const TArray<AActor*> ArraysBefore = OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel);
			const FVector StationLocBefore = StationsBefore.Num() == 1 ? StationsBefore[0]->GetActorLocation() : FVector::ZeroVector;
			const FVector PanelLocBefore = PanelsBefore.Num() == 1 ? PanelsBefore[0]->GetActorLocation() : FVector::ZeroVector;
			const FVector ArrayLocBefore = ArraysBefore.Num() == 1 ? ArraysBefore[0]->GetActorLocation() : FVector::ZeroVector;

			AssertTrue(
				Record, TEXT("preserve.neuro_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("preserve.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));

			// ---- A. Persisted Beat 4 mission asset ----
			UProjectOrganoidObjectiveDataAsset* Mission = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("mission.asset_loaded"),
				Mission != nullptr,
				TEXT("loaded"),
				Mission ? TEXT("loaded") : TEXT("missing"),
				MissionSoftPath);
			if (!Mission)
			{
				FailAndStop(
					Owner, Record,
					TEXT("DA_Mission_NeuroGenetics missing. Require Beat 4 expand+save, cutoff spawn+save, and mapping terminal spawn+save before this test can pass."));
				return;
			}
			AssertTrue(
				Record, TEXT("mission.exact_path"),
				FSoftObjectPath(Mission).ToString() == MissionSoftPath,
				MissionSoftPath,
				FSoftObjectPath(Mission).ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.exact_class"),
				Mission->GetClass() == UProjectOrganoidObjectiveDataAsset::StaticClass(),
				TEXT("ProjectOrganoidObjectiveDataAsset"),
				Mission->GetClass() ? Mission->GetClass()->GetName() : TEXT("null"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.id"),
				Mission->MissionId == FName(MissionId),
				MissionId,
				Mission->MissionId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.title"),
				Mission->MissionTitle.ToString() == MissionTitle,
				MissionTitle,
				Mission->MissionTitle.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.description"),
				Mission->MissionDescription.ToString() == MissionDescription,
				MissionDescription,
				Mission->MissionDescription.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.next_power_restore"),
				Mission->NextMissionAsset.ToSoftObjectPath().ToString()
					== TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore"),
				TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore"),
				Mission->NextMissionAsset.ToSoftObjectPath().IsValid()
					? Mission->NextMissionAsset.ToSoftObjectPath().ToString()
					: TEXT("null"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_count_four"),
				Mission->Tasks.Num() == 4,
				TEXT("4"),
				FString::FromInt(Mission->Tasks.Num()),
				TEXT("DA"));
			if (Mission->Tasks.Num() != 4)
			{
				FailAndStop(
					Owner, Record,
					TEXT("DA_Mission_NeuroGenetics is not exact Beat 5 (task count != 4). Require expand_neurogenetics_mission_beat5 + save before this test can pass."));
				return;
			}

			const FProjectOrganoidMissionTaskDefinition& Task1 = Mission->Tasks[0];
			const FProjectOrganoidMissionTaskDefinition& Task2 = Mission->Tasks[1];
			const FProjectOrganoidMissionTaskDefinition& Task3 = Mission->Tasks[2];
			const FProjectOrganoidMissionTaskDefinition& Task4 = Mission->Tasks[3];
			AssertTrue(
				Record, TEXT("mission.task1_objective_id"),
				Task1.Objective.ObjectiveId == FName(IsolateId),
				IsolateId,
				Task1.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_title"),
				Task1.Objective.Title.ToString() == IsolateTitle,
				IsolateTitle,
				Task1.Objective.Title.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_description"),
				Task1.Objective.Description.ToString() == IsolateDescription,
				IsolateDescription,
				Task1.Objective.Description.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_category_main"),
				Task1.Objective.Type == EProjectOrganoidObjectiveType::Main,
				TEXT("Main"),
				UEnum::GetValueAsString(Task1.Objective.Type),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_target_one"),
				Task1.Objective.TargetProgress == 1,
				TEXT("1"),
				FString::FromInt(Task1.Objective.TargetProgress),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_auto_activate"),
				Task1.bAutoActivate,
				TEXT("true"),
				BoolText(Task1.bAutoActivate),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_no_prerequisites"),
				Task1.Objective.PrerequisiteObjectiveIds.Num() == 0,
				TEXT("0"),
				FString::FromInt(Task1.Objective.PrerequisiteObjectiveIds.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_event_triggers_exact"),
				Task1.EventTriggers.Num() == 1
					&& Task1.EventTriggers[0].EventId == FName(IsolateEvent)
					&& Task1.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete
					&& (Task1.EventTriggers[0].ObjectiveId.IsNone()
						|| Task1.EventTriggers[0].ObjectiveId == FName(IsolateId)),
				TEXT("Event_NeuroResearchLoadIsolated/Complete"),
				Task1.EventTriggers.Num() == 1
					? FString::Printf(
						TEXT("%s/%s"),
						*Task1.EventTriggers[0].EventId.ToString(),
						*UEnum::GetValueAsString(Task1.EventTriggers[0].Action))
					: FString::FromInt(Task1.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_initially_incomplete"),
				Task1.Objective.State == EProjectOrganoidObjectiveState::Inactive
					&& Task1.Objective.CurrentProgress == 0,
				TEXT("Inactive/0"),
				FString::Printf(
					TEXT("%s/%d"),
					*ObjectiveStateName(Task1.Objective.State),
					Task1.Objective.CurrentProgress),
				TEXT("DA"));

			AssertTrue(
				Record, TEXT("mission.task2_objective_id"),
				Task2.Objective.ObjectiveId == FName(TraceId),
				TraceId,
				Task2.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_title"),
				Task2.Objective.Title.ToString() == TraceTitle,
				TraceTitle,
				Task2.Objective.Title.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_description"),
				Task2.Objective.Description.ToString() == TraceDescription,
				TraceDescription,
				Task2.Objective.Description.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_category_main"),
				Task2.Objective.Type == EProjectOrganoidObjectiveType::Main,
				TEXT("Main"),
				UEnum::GetValueAsString(Task2.Objective.Type),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_target_one"),
				Task2.Objective.TargetProgress == 1,
				TEXT("1"),
				FString::FromInt(Task2.Objective.TargetProgress),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_auto_activate"),
				Task2.bAutoActivate,
				TEXT("true"),
				BoolText(Task2.bAutoActivate),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_prereq_isolate"),
				Task2.Objective.PrerequisiteObjectiveIds.Num() == 1
					&& Task2.Objective.PrerequisiteObjectiveIds[0] == FName(IsolateId),
				IsolateId,
				Task2.Objective.PrerequisiteObjectiveIds.Num() == 1
					? Task2.Objective.PrerequisiteObjectiveIds[0].ToString()
					: FString::FromInt(Task2.Objective.PrerequisiteObjectiveIds.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_event_triggers_exact"),
				Task2.EventTriggers.Num() == 1
					&& Task2.EventTriggers[0].EventId == FName(TraceEvent)
					&& Task2.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete
					&& (Task2.EventTriggers[0].ObjectiveId.IsNone()
						|| Task2.EventTriggers[0].ObjectiveId == FName(TraceId)),
				TEXT("Event_NeuralMappingSignalTraced/Complete"),
				Task2.EventTriggers.Num() == 1
					? Task2.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task2.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_initially_incomplete"),
				Task2.Objective.State == EProjectOrganoidObjectiveState::Inactive
					&& Task2.Objective.CurrentProgress == 0,
				TEXT("Inactive/0"),
				FString::Printf(
					TEXT("%s/%d"),
					*ObjectiveStateName(Task2.Objective.State),
					Task2.Objective.CurrentProgress),
				TEXT("DA"));

			AssertTrue(
				Record, TEXT("mission.task3_objective_id"),
				Task3.Objective.ObjectiveId == FName(FollowId),
				FollowId,
				Task3.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_title"),
				Task3.Objective.Title.ToString() == FollowTitle,
				FollowTitle,
				Task3.Objective.Title.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_description"),
				Task3.Objective.Description.ToString() == FollowDescription,
				FollowDescription,
				Task3.Objective.Description.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_category_main"),
				Task3.Objective.Type == EProjectOrganoidObjectiveType::Main,
				TEXT("Main"),
				UEnum::GetValueAsString(Task3.Objective.Type),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_target_one"),
				Task3.Objective.TargetProgress == 1,
				TEXT("1"),
				FString::FromInt(Task3.Objective.TargetProgress),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_auto_activate"),
				Task3.bAutoActivate,
				TEXT("true"),
				BoolText(Task3.bAutoActivate),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_prereq_trace"),
				Task3.Objective.PrerequisiteObjectiveIds.Num() == 1
					&& Task3.Objective.PrerequisiteObjectiveIds[0] == FName(TraceId),
				TraceId,
				Task3.Objective.PrerequisiteObjectiveIds.Num() == 1
					? Task3.Objective.PrerequisiteObjectiveIds[0].ToString()
					: FString::FromInt(Task3.Objective.PrerequisiteObjectiveIds.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_event_followed"),
				Task3.EventTriggers.Num() == 1
					&& Task3.EventTriggers[0].EventId == FName(FollowEvent)
					&& Task3.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete
					&& (Task3.EventTriggers[0].ObjectiveId.IsNone()
						|| Task3.EventTriggers[0].ObjectiveId == FName(FollowId)),
				TEXT("Event_NeuralSignatureFollowed/Complete"),
				Task3.EventTriggers.Num() == 1
					? Task3.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task3.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_initially_incomplete"),
				Task3.Objective.State == EProjectOrganoidObjectiveState::Inactive
					&& Task3.Objective.CurrentProgress == 0,
				TEXT("Inactive/0"),
				FString::Printf(
					TEXT("%s/%d"),
					*ObjectiveStateName(Task3.Objective.State),
					Task3.Objective.CurrentProgress),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_objective_id"),
				Task4.Objective.ObjectiveId == FName(ExamineId),
				ExamineId,
				Task4.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_title"),
				Task4.Objective.Title.ToString() == ExamineTitle,
				ExamineTitle,
				Task4.Objective.Title.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_description"),
				Task4.Objective.Description.ToString() == ExamineDescription,
				ExamineDescription,
				Task4.Objective.Description.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_prereq_follow"),
				Task4.Objective.PrerequisiteObjectiveIds.Num() == 1
					&& Task4.Objective.PrerequisiteObjectiveIds[0] == FName(FollowId),
				FollowId,
				Task4.Objective.PrerequisiteObjectiveIds.Num() == 1
					? Task4.Objective.PrerequisiteObjectiveIds[0].ToString()
					: FString::FromInt(Task4.Objective.PrerequisiteObjectiveIds.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_event_examined"),
				Task4.EventTriggers.Num() == 1
					&& Task4.EventTriggers[0].EventId == FName(TEXT("Event_NeuralChangeEvidenceExamined"))
					&& Task4.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete,
				TEXT("Event_NeuralChangeEvidenceExamined/Complete"),
				Task4.EventTriggers.Num() == 1
					? Task4.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task4.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.no_fifth_task"),
				Mission->Tasks.Num() == 4,
				TEXT("4"),
				FString::FromInt(Mission->Tasks.Num()),
				TEXT("DA"));

			// ---- B. Persisted cutoff actor ----
			const TArray<AActor*> CutoffMatches = OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_unique"),
				CutoffMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(CutoffMatches.Num()),
				CutoffLabel);
			AProjectOrganoidInspectableInstrument* Cutoff = CutoffMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(CutoffMatches[0])
				: nullptr;
			PersistedCutoff = Cutoff;
			if (!Cutoff)
			{
				FailAndStop(
					Owner, Record,
					TEXT("EmergencyCutoff_NeuroResearchLoad missing or wrong class. Require spawn_neuro_research_load_cutoff + save before this test can pass."));
				return;
			}
			AssertTrue(
				Record, TEXT("map.cutoff_native_class"),
				Cutoff->GetClass() == AProjectOrganoidInspectableInstrument::StaticClass(),
				TEXT("ProjectOrganoidInspectableInstrument"),
				Cutoff->GetClass() ? Cutoff->GetClass()->GetName() : TEXT("null"),
				CutoffLabel);
			const FString CutoffPackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Cutoff));
			AssertTrue(
				Record, TEXT("map.cutoff_neuro_package"),
				CutoffPackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				CutoffPackage,
				CutoffLabel);
			const FString WorldPackage = OrganoidPlaytestActions::NormalizePackage(
				World->GetOutermost() ? World->GetOutermost()->GetName() : FString());
			AssertTrue(
				Record, TEXT("map.persistent_lvl_epitope"),
				WorldPackage.Equals(MapPackage, ESearchCase::CaseSensitive),
				MapPackage,
				WorldPackage,
				TEXT("world"));
			AssertTrue(
				Record, TEXT("map.cutoff_transform"),
				VecNear(Cutoff->GetActorLocation(), CutoffLocation)
					&& RotNear(Cutoff->GetActorRotation(), FRotator::ZeroRotator)
					&& VecNear(Cutoff->GetActorScale3D(), FVector::OneVector),
				TEXT("(-100,-600,-1100)/(0,0,0)/(1,1,1)"),
				FString::Printf(
					TEXT("(%s)/(%s)/(%s)"),
					*Cutoff->GetActorLocation().ToCompactString(),
					*Cutoff->GetActorRotation().ToCompactString(),
					*Cutoff->GetActorScale3D().ToCompactString()),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_interaction_range"),
				FMath::IsNearlyEqual(Cutoff->InteractionRange, CutoffInteractionRange, 0.05f),
				TEXT("175"),
				FString::SanitizeFloat(Cutoff->InteractionRange),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_required_active_id"),
				Cutoff->RequiredActiveObjectiveId == FName(IsolateId),
				IsolateId,
				Cutoff->RequiredActiveObjectiveId.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_objective_event_id"),
				Cutoff->ObjectiveEventId == FName(IsolateEvent),
				IsolateEvent,
				Cutoff->ObjectiveEventId.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_replay_guard_id"),
				Cutoff->CompletedObjectiveIdForReplayGuard == FName(IsolateId),
				IsolateId,
				Cutoff->CompletedObjectiveIdForReplayGuard.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_inspection_prompt"),
				Cutoff->InspectionPrompt.ToString() == CutoffInspectPrompt,
				CutoffInspectPrompt,
				Cutoff->InspectionPrompt.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_review_prompt"),
				Cutoff->ReviewPrompt.ToString() == CutoffReviewPrompt,
				CutoffReviewPrompt,
				Cutoff->ReviewPrompt.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_speaker_nathan"),
				Cutoff->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Cutoff->SpeakerLabel.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_response_text_u2019"),
				Cutoff->InspectionResponseText.ToString() == CutoffLine,
				CutoffLine,
				Cutoff->InspectionResponseText.ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_notification_duration"),
				FMath::IsNearlyEqual(Cutoff->NotificationDurationSeconds, CutoffNotifySeconds, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(Cutoff->NotificationDurationSeconds),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_initially_uninspected"),
				!Cutoff->bHasBeenInspected,
				TEXT("false"),
				BoolText(Cutoff->bHasBeenInspected),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_components_exist"),
				Cutoff->SceneRoot && Cutoff->PedestalMesh && Cutoff->ColumnMesh && Cutoff->ArrayHeadMesh,
				TEXT("SceneRoot+Pedestal+Column+ArrayHead"),
				TEXT("present"),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_pedestal_mesh"),
				MeshPathMatches(Cutoff->PedestalMesh, CubeMeshPath)
					&& VecNear(Cutoff->PedestalMesh->GetRelativeLocation(), CutoffPedestalRel)
					&& RotNear(Cutoff->PedestalMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Cutoff->PedestalMesh->GetRelativeScale3D(), CutoffPedestalScale)
					&& Cutoff->PedestalMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Cutoff->PedestalMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_column_mesh"),
				MeshPathMatches(Cutoff->ColumnMesh, CubeMeshPath)
					&& VecNear(Cutoff->ColumnMesh->GetRelativeLocation(), CutoffColumnRel)
					&& RotNear(Cutoff->ColumnMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Cutoff->ColumnMesh->GetRelativeScale3D(), CutoffColumnScale)
					&& Cutoff->ColumnMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Cutoff->ColumnMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_array_head_mesh"),
				MeshPathMatches(Cutoff->ArrayHeadMesh, CubeMeshPath)
					&& VecNear(Cutoff->ArrayHeadMesh->GetRelativeLocation(), CutoffHeadRel)
					&& RotNear(Cutoff->ArrayHeadMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Cutoff->ArrayHeadMesh->GetRelativeScale3D(), CutoffHeadScale)
					&& Cutoff->ArrayHeadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Cutoff->ArrayHeadMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				CutoffLabel);

			// ---- C. Persisted NeuralMappingArray (NRFA map-actor basics) ----
			const TArray<AActor*> ArrayMatches = OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_unique"),
				ArrayMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(ArrayMatches.Num()),
				ArrayLabel);
			AProjectOrganoidInspectableInstrument* Array = ArrayMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(ArrayMatches[0])
				: nullptr;
			if (!Array)
			{
				FailAndStop(
					Owner, Record,
					TEXT("NeuralMappingArray_NeuroGenetics missing or wrong class. Persist neuro_neural_mapping_array_v1 before this test can pass."));
				return;
			}
			AssertTrue(
				Record, TEXT("map.array_native_class"),
				Array->GetClass() == AProjectOrganoidInspectableInstrument::StaticClass(),
				TEXT("ProjectOrganoidInspectableInstrument"),
				Array->GetClass() ? Array->GetClass()->GetName() : TEXT("null"),
				ArrayLabel);
			const FString ArrayPackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Array));
			AssertTrue(
				Record, TEXT("map.array_neuro_package"),
				ArrayPackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				ArrayPackage,
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_transform"),
				VecNear(Array->GetActorLocation(), ArrayLocation)
					&& RotNear(Array->GetActorRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->GetActorScale3D(), FVector::OneVector),
				TEXT("(-500,-600,-1100)/(0,0,0)/(1,1,1)"),
				FString::Printf(
					TEXT("(%s)/(%s)/(%s)"),
					*Array->GetActorLocation().ToCompactString(),
					*Array->GetActorRotation().ToCompactString(),
					*Array->GetActorScale3D().ToCompactString()),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_interaction_range"),
				FMath::IsNearlyEqual(Array->InteractionRange, ArrayInteractionRange, 0.05f),
				TEXT("200"),
				FString::SanitizeFloat(Array->InteractionRange),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_objective_event_id"),
				Array->ObjectiveEventId == FName(ArrayEvent),
				ArrayEvent,
				Array->ObjectiveEventId.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_replay_guard_id"),
				Array->CompletedObjectiveIdForReplayGuard == FName(ResearchFloorId),
				ResearchFloorId,
				Array->CompletedObjectiveIdForReplayGuard.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_inspection_prompt"),
				Array->InspectionPrompt.ToString() == ArrayInspectPrompt,
				ArrayInspectPrompt,
				Array->InspectionPrompt.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_review_prompt"),
				Array->ReviewPrompt.ToString() == ArrayReviewPrompt,
				ArrayReviewPrompt,
				Array->ReviewPrompt.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_speaker_nathan"),
				Array->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Array->SpeakerLabel.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_response_text_u2019"),
				Array->InspectionResponseText.ToString() == ArrayLine,
				ArrayLine,
				Array->InspectionResponseText.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_notification_duration"),
				FMath::IsNearlyEqual(Array->NotificationDurationSeconds, ArrayNotifySeconds, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(Array->NotificationDurationSeconds),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_initially_uninspected"),
				!Array->bHasBeenInspected,
				TEXT("false"),
				BoolText(Array->bHasBeenInspected),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_components_exist"),
				Array->SceneRoot && Array->PedestalMesh && Array->ColumnMesh && Array->ArrayHeadMesh,
				TEXT("SceneRoot+Pedestal+Column+ArrayHead"),
				TEXT("present"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_pedestal_mesh"),
				MeshPathMatches(Array->PedestalMesh, CubeMeshPath)
					&& VecNear(Array->PedestalMesh->GetRelativeLocation(), ArrayPedestalRel)
					&& RotNear(Array->PedestalMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->PedestalMesh->GetRelativeScale3D(), ArrayPedestalScale)
					&& Array->PedestalMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->PedestalMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_column_mesh"),
				MeshPathMatches(Array->ColumnMesh, CylinderMeshPath)
					&& VecNear(Array->ColumnMesh->GetRelativeLocation(), ArrayColumnRel)
					&& RotNear(Array->ColumnMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->ColumnMesh->GetRelativeScale3D(), ArrayColumnScale)
					&& Array->ColumnMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->ColumnMesh->GetGenerateOverlapEvents(),
				TEXT("Cylinder@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_array_head_mesh"),
				MeshPathMatches(Array->ArrayHeadMesh, CylinderMeshPath)
					&& VecNear(Array->ArrayHeadMesh->GetRelativeLocation(), ArrayHeadRel)
					&& RotNear(Array->ArrayHeadMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->ArrayHeadMesh->GetRelativeScale3D(), ArrayHeadScale)
					&& Array->ArrayHeadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->ArrayHeadMesh->GetGenerateOverlapEvents(),
				TEXT("Cylinder@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);


			// ---- C2. Persisted NeuralMappingTerminal (exact; untouched by this test) ----
			const TArray<AActor*> TerminalMatches = OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_unique"),
				TerminalMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(TerminalMatches.Num()),
				TerminalLabel);
			AProjectOrganoidInspectableInstrument* Terminal = TerminalMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(TerminalMatches[0])
				: nullptr;
			if (!Terminal)
			{
				FailAndStop(
					Owner, Record,
					TEXT("NeuralMappingTerminal_NeuroGenetics missing or wrong class. Require spawn_neuro_neural_mapping_terminal + save before this test can pass."));
				return;
			}
			AssertTrue(
				Record, TEXT("map.terminal_native_class"),
				Terminal->GetClass() == AProjectOrganoidInspectableInstrument::StaticClass(),
				TEXT("ProjectOrganoidInspectableInstrument"),
				Terminal->GetClass() ? Terminal->GetClass()->GetName() : TEXT("null"),
				TerminalLabel);
			const FString TerminalPackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Terminal));
			AssertTrue(
				Record, TEXT("map.terminal_neuro_package"),
				TerminalPackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				TerminalPackage,
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_transform"),
				VecNear(Terminal->GetActorLocation(), TerminalLocation)
					&& RotNear(Terminal->GetActorRotation(), FRotator::ZeroRotator)
					&& VecNear(Terminal->GetActorScale3D(), FVector::OneVector),
				TEXT("(300,-600,-1100)/(0,0,0)/(1,1,1)"),
				FString::Printf(
					TEXT("(%s)/(%s)/(%s)"),
					*Terminal->GetActorLocation().ToCompactString(),
					*Terminal->GetActorRotation().ToCompactString(),
					*Terminal->GetActorScale3D().ToCompactString()),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_interaction_range"),
				FMath::IsNearlyEqual(Terminal->InteractionRange, TerminalInteractionRange, 0.05f),
				TEXT("175"),
				FString::SanitizeFloat(Terminal->InteractionRange),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_required_active_id"),
				Terminal->RequiredActiveObjectiveId == FName(TraceId),
				TraceId,
				Terminal->RequiredActiveObjectiveId.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_objective_event_id"),
				Terminal->ObjectiveEventId == FName(TraceEvent),
				TraceEvent,
				Terminal->ObjectiveEventId.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_replay_guard_id"),
				Terminal->CompletedObjectiveIdForReplayGuard == FName(TraceId),
				TraceId,
				Terminal->CompletedObjectiveIdForReplayGuard.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_inspection_prompt"),
				Terminal->InspectionPrompt.ToString() == TerminalInspectPrompt,
				TerminalInspectPrompt,
				Terminal->InspectionPrompt.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_review_prompt"),
				Terminal->ReviewPrompt.ToString() == TerminalReviewPrompt,
				TerminalReviewPrompt,
				Terminal->ReviewPrompt.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_speaker_nathan"),
				Terminal->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Terminal->SpeakerLabel.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_response_text_u2019"),
				Terminal->InspectionResponseText.ToString() == TerminalLine,
				TerminalLine,
				Terminal->InspectionResponseText.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_notification_duration"),
				FMath::IsNearlyEqual(Terminal->NotificationDurationSeconds, TerminalNotifySeconds, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(Terminal->NotificationDurationSeconds),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_initially_uninspected"),
				!Terminal->bHasBeenInspected,
				TEXT("false"),
				BoolText(Terminal->bHasBeenInspected),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_components_exist"),
				Terminal->SceneRoot && Terminal->PedestalMesh && Terminal->ColumnMesh && Terminal->ArrayHeadMesh,
				TEXT("SceneRoot+Pedestal+Column+ArrayHead"),
				TEXT("present"),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_pedestal_mesh"),
				MeshPathMatches(Terminal->PedestalMesh, CubeMeshPath)
					&& VecNear(Terminal->PedestalMesh->GetRelativeLocation(), TerminalPedestalRel)
					&& RotNear(Terminal->PedestalMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Terminal->PedestalMesh->GetRelativeScale3D(), TerminalPedestalScale)
					&& Terminal->PedestalMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Terminal->PedestalMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_column_mesh"),
				MeshPathMatches(Terminal->ColumnMesh, CubeMeshPath)
					&& VecNear(Terminal->ColumnMesh->GetRelativeLocation(), TerminalColumnRel)
					&& RotNear(Terminal->ColumnMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Terminal->ColumnMesh->GetRelativeScale3D(), TerminalColumnScale)
					&& Terminal->ColumnMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Terminal->ColumnMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_array_head_mesh"),
				MeshPathMatches(Terminal->ArrayHeadMesh, CubeMeshPath)
					&& VecNear(Terminal->ArrayHeadMesh->GetRelativeLocation(), TerminalHeadRel)
					&& RotNear(Terminal->ArrayHeadMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Terminal->ArrayHeadMesh->GetRelativeScale3D(), TerminalHeadScale)
					&& Terminal->ArrayHeadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Terminal->ArrayHeadMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				TerminalLabel);
			const int32 TerminalEventsBefore = Terminal->ObjectiveEventFireCount;
			const int32 TerminalLinesBefore = Terminal->InspectionNotificationCount;

			// ---- D. Distinct gate_none probe (RequiredActiveObjectiveId missing) ----
			FProjectOrganoidObjective GateNoneObj;
			GateNoneObj.ObjectiveId = FName(GateNoneObjId);
			GateNoneObj.Title = FText::FromString(TEXT("Cutoff gate-none probe"));
			GateNoneObj.Type = EProjectOrganoidObjectiveType::Main;
			GateNoneObj.State = EProjectOrganoidObjectiveState::Inactive;
			GateNoneObj.TargetProgress = 1;
			GateNoneObj.bShowInJournal = false;
			Objectives->RegisterObjective(GateNoneObj);
			FProjectOrganoidObjectiveEventTrigger GateNoneTrig;
			GateNoneTrig.EventId = FName(GateNoneEvent);
			GateNoneTrig.ObjectiveId = FName(GateNoneObjId);
			GateNoneTrig.Action = EProjectOrganoidObjectiveEventAction::Complete;
			Objectives->RegisterEventTrigger(GateNoneTrig);
			Objectives->ActivateObjective(FName(GateNoneObjId));

			AProjectOrganoidInspectableInstrument* GateNoneActor = SpawnGateNone(World);
			AssertTrue(
				Record, TEXT("gate_none.spawned"),
				GateNoneActor != nullptr,
				TEXT("spawned"),
				GateNoneActor ? TEXT("spawned") : TEXT("null"),
				GateNoneLabel);
			if (!GateNoneActor)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn gate-none instrument."));
				return;
			}
			AssertTrue(
				Record, TEXT("gate_none.required_is_none"),
				GateNoneActor->RequiredActiveObjectiveId.IsNone(),
				TEXT("None"),
				GateNoneActor->RequiredActiveObjectiveId.ToString(),
				TEXT("gate"));
			const bool bGateNoneCan = GateNoneActor->CanInteract(Character);
			AssertTrue(
				Record, TEXT("gate_none.can_interact"),
				bGateNoneCan,
				TEXT("true"),
				BoolText(bGateNoneCan),
				TEXT("gate"));
			const bool bGateNoneInteracted = GateNoneActor->Interact(Character);
			AssertTrue(
				Record, TEXT("gate_none.interact_accepted"),
				bGateNoneInteracted,
				TEXT("true"),
				BoolText(bGateNoneInteracted),
				TEXT("gate"));
			FProjectOrganoidObjective GateNoneAfter;
			Objectives->GetObjective(FName(GateNoneObjId), GateNoneAfter);
			AssertTrue(
				Record, TEXT("gate_none.completed"),
				GateNoneAfter.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				ObjectiveStateName(GateNoneAfter.State),
				GateNoneObjId);
			AssertTrue(
				Record, TEXT("gate_none.event_once"),
				GateNoneActor->ObjectiveEventFireCount == 1,
				TEXT("1"),
				FString::FromInt(GateNoneActor->ObjectiveEventFireCount),
				TEXT("gate"));
			AssertTrue(
				Record, TEXT("gate_none.line_once"),
				GateNoneActor->InspectionNotificationCount == 1,
				TEXT("1"),
				FString::FromInt(GateNoneActor->InspectionNotificationCount),
				TEXT("gate"));
			GateNoneActor->Destroy();
			TransientActors.RemoveAll([](const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& W) { return !W.IsValid(); });

			// ---- E. Before Mission_NeuroGenetics: persisted cutoff gated ----
			AssertTrue(
				Record, TEXT("pre.cutoff_mission_not_neuro"),
				Objectives->GetActiveMissionId() != FName(MissionId),
				TEXT("not Mission_NeuroGenetics"),
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			const int32 PreEvents = Cutoff->ObjectiveEventFireCount;
			const int32 PreLines = Cutoff->InspectionNotificationCount;
			const bool bPreCan = Cutoff->CanInteract(Character);
			AssertTrue(
				Record, TEXT("pre.cutoff_can_interact_false"),
				!bPreCan,
				TEXT("false"),
				BoolText(bPreCan),
				CutoffLabel);
			const bool bPreInteract = Cutoff->Interact(Character);
			AssertTrue(
				Record, TEXT("pre.cutoff_interact_rejected"),
				!bPreInteract,
				TEXT("false"),
				BoolText(bPreInteract),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("pre.cutoff_no_event"),
				Cutoff->ObjectiveEventFireCount == PreEvents,
				TEXT("0"),
				FString::FromInt(Cutoff->ObjectiveEventFireCount - PreEvents),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("pre.cutoff_no_line"),
				Cutoff->InspectionNotificationCount == PreLines,
				TEXT("0"),
				FString::FromInt(Cutoff->InspectionNotificationCount - PreLines),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("pre.cutoff_not_inspected"),
				!Cutoff->bHasBeenInspected,
				TEXT("false"),
				BoolText(Cutoff->bHasBeenInspected),
				CutoffLabel);

			// ---- F. Ordered OpeningFoundation → NeuroGenetics handoff (NRFA-style) ----

			const TArray<AActor*> ObservationMatches = OrganoidPlaytestActions::FindActorsByLabel(World, ObservationNodeLabel);
			AssertTrue(
				Record, TEXT("map.observation_unique"),
				ObservationMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(ObservationMatches.Num()),
				ObservationNodeLabel);
			AProjectOrganoidInspectableInstrument* ObservationNode = ObservationMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(ObservationMatches[0])
				: nullptr;
			AssertTrue(
				Record, TEXT("map.observation_present_exact"),
				ObservationNode != nullptr
					&& ObservationNode->GetClass() == AProjectOrganoidInspectableInstrument::StaticClass()
					&& !ObservationNode->bHasBeenInspected
					&& ObservationNode->RequiredActiveObjectiveId == FName(FollowId)
					&& ObservationNode->ObjectiveEventId == FName(FollowEvent)
					&& ObservationNode->CompletedObjectiveIdForReplayGuard == FName(FollowId),
				TEXT("exact uninspected Follow-gated node"),
				ObservationNode ? TEXT("present") : TEXT("missing"),
				ObservationNodeLabel);
			const int32 ObservationEventsBefore = ObservationNode ? ObservationNode->ObjectiveEventFireCount : -1;
			const int32 ObservationLinesBefore = ObservationNode ? ObservationNode->InspectionNotificationCount : -1;

			Objectives->LoadDefaultMission();
			AssertTrue(
				Record, TEXT("ordered.reseeds_opening"),
				Objectives->GetActiveMissionId() == TEXT("Mission_OpeningFoundation"),
				TEXT("Mission_OpeningFoundation"),
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("ordered.pending_soft_path"),
				Objectives->GetPendingNextMissionAssetPath().ToString() == MissionSoftPath,
				MissionSoftPath,
				Objectives->GetPendingNextMissionAssetPath().ToString(),
				TEXT("objectives"));
			UProjectOrganoidObjectiveDataAsset* RealMission = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("ordered.real_da_loaded"),
				RealMission != nullptr
					&& RealMission->MissionId == FName(MissionId)
					&& RealMission->Tasks.Num() == 4
					&& FSoftObjectPath(RealMission).ToString() == MissionSoftPath,
				MissionSoftPath,
				RealMission ? FSoftObjectPath(RealMission).ToString() : TEXT("null"),
				TEXT("DA"));

			AProjectOrganoidInspectableInstrument* OrderedArray =
				SpawnConfiguredArrayClone(World, FName(ArrayOrderedLabel));
			AssertTrue(
				Record, TEXT("ordered.array_clone_spawned"),
				OrderedArray != nullptr,
				TEXT("spawned"),
				OrderedArray ? TEXT("spawned") : TEXT("null"),
				ArrayOrderedLabel);
			if (!OrderedArray)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn ordered-path array clone."));
				return;
			}

			UOrganoidNeuroResearchFloorArrayMissionCompletionProbe* LiveOpeningProbe =
				NewObject<UOrganoidNeuroResearchFloorArrayMissionCompletionProbe>(World);
			OpeningMissionProbe.Reset(LiveOpeningProbe);
			OpeningMissionObjectives = Objectives;
			Objectives->OnMissionCompleted.AddDynamic(
				LiveOpeningProbe,
				&UOrganoidNeuroResearchFloorArrayMissionCompletionProbe::HandleMissionCompleted);

			Objectives->TriggerEvent(FName(DiscoveryEvent));
			Objectives->TriggerEvent(FName(DiagnosisEvent));
			FProjectOrganoidObjective ResearchAfterDiagnosis;
			Objectives->GetObjective(FName(ResearchFloorId), ResearchAfterDiagnosis);
			AssertTrue(
				Record, TEXT("ordered.diagnosis_activates_research"),
				ResearchAfterDiagnosis.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				ObjectiveStateName(ResearchAfterDiagnosis.State),
				ResearchFloorId);
			AssertTrue(
				Record, TEXT("ordered.mission_incomplete_after_diagnosis"),
				!Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation")),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation"))),
				TEXT("mission"));

			const bool bOrderedInteracted = OrderedArray->Interact(Character);
			AssertTrue(
				Record, TEXT("ordered.array_completes_research"),
				bOrderedInteracted,
				TEXT("true"),
				BoolText(bOrderedInteracted),
				TEXT("array"));
			FProjectOrganoidObjective ResearchAfterInspect;
			Objectives->GetObjective(FName(ResearchFloorId), ResearchAfterInspect);
			AssertTrue(
				Record, TEXT("ordered.research_completed"),
				ResearchAfterInspect.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				ObjectiveStateName(ResearchAfterInspect.State),
				ResearchFloorId);

			Objectives->TriggerEvent(FName(ReceptionEvent));
			Objectives->TriggerEvent(FName(SecurityEvent));

			AssertTrue(
				Record, TEXT("ordered.next_mission_current"),
				Objectives->GetActiveMissionId() == FName(MissionId),
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("ordered.real_da_drives_handoff"),
				Objectives->GetActiveMissionId() == FName(MissionId)
					&& LoadPersistedMissionAsset() != nullptr
					&& LoadPersistedMissionAsset()->Tasks.Num() == 4,
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("ordered.isolate_active_once"),
				CountActiveId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("ordered.trace_inactive"),
				CountActiveId(Objectives, FName(TraceId)) == 0
					&& CountCompletedId(Objectives, FName(TraceId)) == 0,
				TEXT("active=0 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(TraceId)),
					CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			FProjectOrganoidObjective TraceAfterHandoff;
			Objectives->GetObjective(FName(TraceId), TraceAfterHandoff);
			AssertTrue(
				Record, TEXT("ordered.trace_state_inactive"),
				TraceAfterHandoff.State == EProjectOrganoidObjectiveState::Inactive,
				TEXT("Inactive"),
				ObjectiveStateName(TraceAfterHandoff.State),
				TraceId);
			AssertTrue(
				Record, TEXT("ordered.follow_inactive"),
				CountActiveId(Objectives, FName(FollowId)) == 0
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("active=0 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("ordered.examine_inactive"),
				CountActiveId(Objectives, FName(ExamineId)) == 0
					&& CountCompletedId(Objectives, FName(ExamineId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				ExamineId);

			const int32 OpeningCompletedCount = LiveOpeningProbe
				? LiveOpeningProbe->OpeningFoundationCompletedCount
				: -1;
			AssertTrue(
				Record, TEXT("ordered.opening_completed_once"),
				OpeningCompletedCount == 1,
				TEXT("1"),
				FString::FromInt(OpeningCompletedCount),
				TEXT("mission"));
			UnbindOpeningMissionProbe();

			if (OrderedArray)
			{
				OrderedArray->Destroy();
			}
			TransientActors.RemoveAll([](const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& W) { return !W.IsValid(); });

			// ---- G. Interact persisted cutoff ----
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCutoffCan = Cutoff->CanInteract(Character);
			AssertTrue(
				Record, TEXT("cutoff.can_interact"),
				bCutoffCan,
				TEXT("true"),
				BoolText(bCutoffCan),
				CutoffLabel);
			const int32 CutoffEventsBefore = Cutoff->ObjectiveEventFireCount;
			const int32 CutoffLinesBefore = Cutoff->InspectionNotificationCount;
			const bool bCutoffInteract = Cutoff->Interact(Character);
			AssertTrue(
				Record, TEXT("cutoff.interact_accepted"),
				bCutoffInteract,
				TEXT("true"),
				BoolText(bCutoffInteract),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("cutoff.isolate_completed_once"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("cutoff.event_once"),
				Cutoff->ObjectiveEventFireCount == CutoffEventsBefore + 1,
				TEXT("1"),
				FString::FromInt(Cutoff->ObjectiveEventFireCount - CutoffEventsBefore),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("cutoff.line_once"),
				Cutoff->InspectionNotificationCount == CutoffLinesBefore + 1,
				TEXT("1"),
				FString::FromInt(Cutoff->InspectionNotificationCount - CutoffLinesBefore),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("cutoff.nathan_exact_text"),
				HUD->GetLastResourceNotification().ToString() == CutoffRendered,
				CutoffRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("cutoff.review_prompt"),
				Cutoff->GetInteractionPrompt().ToString() == CutoffReviewPrompt,
				CutoffReviewPrompt,
				Cutoff->GetInteractionPrompt().ToString(),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("cutoff.trace_active_once"),
				CountActiveId(Objectives, FName(TraceId)) == 1
					&& CountCompletedId(Objectives, FName(TraceId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(TraceId)),
					CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("cutoff.follow_still_inactive"),
				CountActiveId(Objectives, FName(FollowId)) == 0
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("active=0 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("cutoff.examine_still_inactive"),
				CountActiveId(Objectives, FName(ExamineId)) == 0
					&& CountCompletedId(Objectives, FName(ExamineId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				ExamineId);
			AssertTrue(
				Record, TEXT("cutoff.observation_untouched"),
				ObservationNode
					&& !ObservationNode->bHasBeenInspected
					&& ObservationNode->ObjectiveEventFireCount == ObservationEventsBefore
					&& ObservationNode->InspectionNotificationCount == ObservationLinesBefore,
				TEXT("uninspected/no event/no line"),
				TEXT("checked"),
				ObservationNodeLabel);
			AssertTrue(
				Record, TEXT("cutoff.terminal_still_uninspected"),
				Terminal && !Terminal->bHasBeenInspected
					&& Terminal->ObjectiveEventFireCount == TerminalEventsBefore
					&& Terminal->InspectionNotificationCount == TerminalLinesBefore,
				TEXT("untouched"),
				Terminal
					? FString::Printf(
						TEXT("inspected=%s events=%d lines=%d"),
						*BoolText(Terminal->bHasBeenInspected),
						Terminal->ObjectiveEventFireCount,
						Terminal->InspectionNotificationCount)
					: TEXT("null"),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("cutoff.mission_still_neuro"),
				Objectives->GetActiveMissionId() == FName(MissionId),
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("cutoff.mission_incomplete"),
				!Objectives->IsMissionComplete(FName(MissionId)),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(FName(MissionId))),
				TEXT("mission"));

			// ---- H. Repeat interact: no replay ----
			const int32 RepeatEventsBefore = Cutoff->ObjectiveEventFireCount;
			const int32 RepeatLinesBefore = Cutoff->InspectionNotificationCount;
			const int32 TraceActiveBeforeRepeat = CountActiveId(Objectives, FName(TraceId));
			const bool bRepeatInteract = Cutoff->Interact(Character);
			AssertTrue(
				Record, TEXT("repeat.interact_ok"),
				bRepeatInteract,
				TEXT("true"),
				BoolText(bRepeatInteract),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("repeat.no_event_replay"),
				Cutoff->ObjectiveEventFireCount == RepeatEventsBefore,
				TEXT("0"),
				FString::FromInt(Cutoff->ObjectiveEventFireCount - RepeatEventsBefore),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("repeat.no_line_replay"),
				Cutoff->InspectionNotificationCount == RepeatLinesBefore,
				TEXT("0"),
				FString::FromInt(Cutoff->InspectionNotificationCount - RepeatLinesBefore),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("repeat.isolate_completed"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("repeat.trace_active"),
				CountActiveId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("repeat.no_duplicate_activation"),
				CountActiveId(Objectives, FName(TraceId)) == TraceActiveBeforeRepeat
					&& CountActiveId(Objectives, FName(TraceId)) == 1
					&& CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("trace_active=1 isolate_completed=1"),
				FString::Printf(
					TEXT("trace_active=%d isolate_completed=%d"),
					CountActiveId(Objectives, FName(TraceId)),
					CountCompletedId(Objectives, FName(IsolateId))),
				TEXT("objectives"));

			// ---- I. Save/load via identically configured transient cutoff clone ----
			UProjectOrganoidSaveGame* SaveGame = NewObject<UProjectOrganoidSaveGame>(GetTransientPackage());
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.isolate_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(IsolateId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(IsolateId))),
				TEXT("save"));

			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
			AProjectOrganoidInspectableInstrument* Reloaded =
				SpawnConfiguredCutoffClone(World, FName(CutoffReloadLabel));
			AssertTrue(
				Record, TEXT("saveload.reload_spawned"),
				Reloaded != nullptr,
				TEXT("spawned"),
				Reloaded ? TEXT("spawned") : TEXT("null"),
				CutoffReloadLabel);
			if (!Reloaded)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn identically configured cutoff reload clone."));
				return;
			}
			AssertTrue(
				Record, TEXT("saveload.starts_review_prompt"),
				Reloaded->GetInteractionPrompt().ToString() == CutoffReviewPrompt,
				CutoffReviewPrompt,
				Reloaded->GetInteractionPrompt().ToString(),
				TEXT("cutoff"));
			AssertTrue(
				Record, TEXT("saveload.not_marked_inspected_without_interact"),
				!Reloaded->bHasBeenInspected,
				TEXT("false"),
				BoolText(Reloaded->bHasBeenInspected),
				TEXT("cutoff"));
			AssertTrue(
				Record, TEXT("saveload.no_transient_mission_da"),
				LoadPersistedMissionAsset() == Mission
					&& FSoftObjectPath(Mission).ToString() == MissionSoftPath
					&& Mission->Tasks.Num() == 4,
				MissionSoftPath,
				FSoftObjectPath(Mission).ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("saveload.isolate_completed"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("saveload.trace_active"),
				CountActiveId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(TraceId))),
				TraceId);

			const int32 ReloadEventsBefore = Reloaded->ObjectiveEventFireCount;
			const int32 ReloadLinesBefore = Reloaded->InspectionNotificationCount;
			const bool bReloadInteract = Reloaded->Interact(Character);
			AssertTrue(
				Record, TEXT("saveload.review_interact"),
				bReloadInteract,
				TEXT("true"),
				BoolText(bReloadInteract),
				TEXT("cutoff"));
			AssertTrue(
				Record, TEXT("saveload.no_event_replay"),
				Reloaded->ObjectiveEventFireCount == ReloadEventsBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->ObjectiveEventFireCount - ReloadEventsBefore),
				TEXT("cutoff"));
			AssertTrue(
				Record, TEXT("saveload.no_line_replay"),
				Reloaded->InspectionNotificationCount == ReloadLinesBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->InspectionNotificationCount - ReloadLinesBefore),
				TEXT("cutoff"));

			// ---- J. Preservation ----
			AssertTrue(
				Record, TEXT("preserve.neuro_still_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("preserve.cryo_still_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("preserve.restore_event_unhandled"),
				Objectives->TriggerEvent(FName(RestoreEvent)) == 0,
				TEXT("0"),
				FString::FromInt(0),
				RestoreEvent);

			const TArray<AActor*> StationsAfter = OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel);
			const TArray<AActor*> PanelsAfter = OrganoidPlaytestActions::FindActorsByLabel(World, PanelLabel);
			const TArray<AActor*> ArraysAfter = OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel);
			AssertTrue(
				Record, TEXT("preserve.station_untouched"),
				StationsAfter.Num() == StationsBefore.Num()
					&& (StationsAfter.Num() != 1 || StationsAfter[0]->GetActorLocation().Equals(StationLocBefore, 0.5f))
					&& (StationsAfter.Num() != 1 || VecNear(StationsAfter[0]->GetActorLocation(), StationLocation)),
				TEXT("unchanged"),
				FString::FromInt(StationsAfter.Num()),
				StationLabel);
			AssertTrue(
				Record, TEXT("preserve.panel_untouched"),
				PanelsAfter.Num() == PanelsBefore.Num()
					&& (PanelsAfter.Num() != 1 || PanelsAfter[0]->GetActorLocation().Equals(PanelLocBefore, 0.5f)),
				TEXT("unchanged"),
				FString::FromInt(PanelsAfter.Num()),
				PanelLabel);
			AssertTrue(
				Record, TEXT("preserve.array_untouched"),
				ArraysAfter.Num() == ArraysBefore.Num()
					&& (ArraysAfter.Num() != 1 || ArraysAfter[0]->GetActorLocation().Equals(ArrayLocBefore, 0.5f)),
				TEXT("unchanged"),
				FString::FromInt(ArraysAfter.Num()),
				ArrayLabel);

			AssertTrue(
				Record, TEXT("playtest_mutates_assets"),
				true,
				TEXT("false"),
				TEXT("false"),
				TEXT(""));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);

			AssertTrue(
				Record, TEXT("dirty.admin_unchanged"),
				!PackageIsDirty(AdminPackage),
				TEXT("clean"),
				PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"),
				AdminPackage);
			AssertTrue(
				Record, TEXT("dirty.neuro_unchanged"),
				!PackageIsDirty(NeuroPackage),
				TEXT("clean"),
				PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"),
				NeuroPackage);
			AssertTrue(
				Record, TEXT("dirty.lvl_epitope_unchanged"),
				!PackageIsDirty(MapPackage),
				TEXT("clean"),
				PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"),
				MapPackage);
			AssertTrue(
				Record, TEXT("dirty.mission_unchanged"),
				!PackageIsDirty(MissionPackage),
				TEXT("clean"),
				PackageIsDirty(MissionPackage) ? TEXT("dirty") : TEXT("clean"),
				MissionPackage);
			AssertTrue(
				Record, TEXT("dirty.no_new_editor_dirt"),
				DirtyAfter.Num() <= DirtyBefore.Num(),
				FString::FromInt(DirtyBefore.Num()),
				FString::FromInt(DirtyAfter.Num()),
				TEXT("editor"));

			Stage = EStage::Finalize;
			Owner.SetStage(TEXT("Finalize"));
		}
	};

	struct FNeuroResearchLoadCutoffAutoRegister
	{
		FNeuroResearchLoadCutoffAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroResearchLoadCutoffFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroResearchLoadCutoffAutoRegister GRegisterNeuroResearchLoadCutoff;
}
