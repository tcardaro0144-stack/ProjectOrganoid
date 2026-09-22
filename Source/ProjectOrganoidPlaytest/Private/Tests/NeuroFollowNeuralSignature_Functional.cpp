#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Blueprint/UserWidget.h"
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
	constexpr TCHAR TestId[] = TEXT("NeuroFollowNeuralSignature_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Follow Neural Signature Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	constexpr TCHAR MissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");

	constexpr TCHAR MissionId[] = TEXT("Mission_NeuroGenetics");
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

	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");

	constexpr TCHAR NodeLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	constexpr TCHAR NodeReloadLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics_ReloadClone");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR PanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR ArrayLabel[] = TEXT("NeuralMappingArray_NeuroGenetics");
	constexpr TCHAR CutoffLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad");
	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");
	constexpr TCHAR HostResearcherLabel[] = TEXT("Host_Neuro_Researcher");
	constexpr TCHAR HazardLabel[] = TEXT("Hazard_ScrubberLeak");
	constexpr TCHAR TrapLabel[] = TEXT("CorridorTraps_GowningRing");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");
	constexpr TCHAR PadContainmentLabel[] = TEXT("DataPad_NeuroContainment");
	constexpr TCHAR PadFailureLabel[] = TEXT("DataPad_NeuroResearchFailure");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_NeuroAirlock");

	constexpr TCHAR InspectPrompt[] = TEXT("Follow Neural Signature");
	constexpr TCHAR ReviewPrompt[] = TEXT("Neural Signature Located");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	// Exact line: one ASCII \u2019 escape (wasn't).
	constexpr TCHAR ObservationLine[] =
		TEXT("The signature continues into the research wing. Epitope wasn\u2019t just recording the damage. They were studying the same change in every subject.");
	constexpr TCHAR ObservationRendered[] =
		TEXT("Nathan: The signature continues into the research wing. Epitope wasn\u2019t just recording the damage. They were studying the same change in every subject.");

	constexpr float NodeInteractionRange = 175.0f;
	constexpr float NodeNotifySeconds = 4.0f;
	const FVector NodeLocation(500.0f, -2100.0f, -1100.0f);

	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FVector NodePedestalRel(0.0f, 0.0f, 30.0f);
	const FVector NodePedestalScale(0.60f, 0.50f, 0.60f);
	const FVector NodeColumnRel(0.0f, 0.0f, 95.0f);
	const FVector NodeColumnScale(0.30f, 0.30f, 1.00f);
	const FVector NodeHeadRel(0.0f, 0.0f, 155.0f);
	const FVector NodeHeadScale(0.90f, 0.45f, 0.25f);

	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");
	constexpr TCHAR ArrayOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR ArrayInspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ArrayReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ArrayLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);
	constexpr float ArrayInteractionRange = 200.0f;
	const FVector ArrayPedestalRel(0.0f, 0.0f, 40.0f);
	const FVector ArrayPedestalScale(1.2f, 1.2f, 0.8f);
	const FVector ArrayColumnRel(0.0f, 0.0f, 110.0f);
	const FVector ArrayColumnScale(0.35f, 0.35f, 1.4f);
	const FVector ArrayHeadRel(0.0f, 0.0f, 192.5f);
	const FVector ArrayHeadScale(1.6f, 1.6f, 0.25f);
	constexpr TCHAR CutoffInspectPrompt[] = TEXT("Isolate Research Load");
	constexpr TCHAR CutoffReviewPrompt[] = TEXT("Research Load Isolated");
	constexpr TCHAR TerminalInspectPrompt[] = TEXT("Trace Neural Mapping Signal");
	constexpr TCHAR TerminalReviewPrompt[] = TEXT("Signal Trace Complete");
	const FVector TerminalLocation(300.0f, -600.0f, -1100.0f);
	const FVector CutoffLocation(-100.0f, -600.0f, -1100.0f);

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

	int32 CountLabel(UWorld* World, const TCHAR* Label)
	{
		return World ? OrganoidPlaytestActions::FindActorsByLabel(World, Label).Num() : 0;
	}

	FVector LabelLocationOrZero(UWorld* World, const TCHAR* Label)
	{
		const TArray<AActor*> Found = World
			? OrganoidPlaytestActions::FindActorsByLabel(World, Label)
			: TArray<AActor*>();
		return Found.Num() == 1 ? Found[0]->GetActorLocation() : FVector::ZeroVector;
	}

	class FNeuroFollowNeuralSignatureFunctional : public IOrganoidPlaytestCase
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
			TransientActors.Reset();
			OwnedHudWidget.Reset();
			DecoyHudWidget.Reset();
			TransientMission.Reset();
			OpeningMissionProbe.Reset();
			OpeningMissionObjectives.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroyTransientActors();
			DestroyDecoyHud();
			UnbindOpeningMissionProbe();
			TransientMission.Reset();
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
				DestroyTransientActors();
				DestroyDecoyHud();
				UnbindOpeningMissionProbe();
				TransientMission.Reset();
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
		TArray<TWeakObjectPtr<AProjectOrganoidInspectableInstrument>> TransientActors;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> OwnedHudWidget;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> DecoyHudWidget;
		TStrongObjectPtr<UProjectOrganoidObjectiveDataAsset> TransientMission;
		TStrongObjectPtr<UOrganoidNeuroResearchFloorArrayMissionCompletionProbe> OpeningMissionProbe;
		TWeakObjectPtr<UProjectOrganoidObjectiveSubsystem> OpeningMissionObjectives;

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
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

		void DestroyDecoyHud()
		{
			if (UProjectOrganoidHUDWidget* Decoy = DecoyHudWidget.Get())
			{
				Decoy->RemoveFromParent();
			}
			DecoyHudWidget.Reset();
		}

		void DestroyTransientActors()
		{
			for (const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& Weak : TransientActors)
			{
				if (AProjectOrganoidInspectableInstrument* Live = Weak.Get())
				{
					Live->Destroy();
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

		UProjectOrganoidGameplayHUDController* ResolveHudController(APlayerController* PC) const
		{
			if (!PC || !PC->GetWorld())
			{
				return nullptr;
			}
			if (AProjectOrganoidGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AProjectOrganoidGameMode>())
			{
				return GameMode->GetHUDControllerForPlayer(PC);
			}
			return nullptr;
		}

		UProjectOrganoidHUDWidget* ResolveOwnedHud(APlayerController* PC)
		{
			if (UProjectOrganoidHUDWidget* Existing = OwnedHudWidget.Get())
			{
				return Existing;
			}
			if (UProjectOrganoidGameplayHUDController* Controller = ResolveHudController(PC))
			{
				if (UProjectOrganoidHUDWidget* Bound = Controller->GetBoundHUDWidget())
				{
					OwnedHudWidget = Bound;
					return Bound;
				}
			}
			return nullptr;
		}

		UProjectOrganoidHUDWidget* SpawnDecoyHud(APlayerController* PC)
		{
			if (!PC)
			{
				return nullptr;
			}
			UProjectOrganoidHUDWidget* Decoy = CreateWidget<UProjectOrganoidHUDWidget>(PC);
			if (!Decoy)
			{
				return nullptr;
			}
			Decoy->AddToViewport(1);
			DecoyHudWidget = Decoy;
			return Decoy;
		}

		void UnbindOpeningMissionProbe()
		{
			if (UOrganoidNeuroResearchFloorArrayMissionCompletionProbe* LiveProbe = OpeningMissionProbe.Get())
			{
				if (UProjectOrganoidObjectiveSubsystem* Objectives = OpeningMissionObjectives.Get())
				{
					Objectives->OnMissionCompleted.RemoveDynamic(
						LiveProbe,
						&UOrganoidNeuroResearchFloorArrayMissionCompletionProbe::HandleMissionCompleted);
				}
			}
			OpeningMissionProbe.Reset();
			OpeningMissionObjectives.Reset();
		}

		bool VecNear(const FVector& A, const FVector& B, float Tol = 0.5f) const
		{
			return A.Equals(B, Tol);
		}

		bool RotNear(const FRotator& A, const FRotator& B, float Tol = 0.5f) const
		{
			return A.Equals(B, Tol);
		}

		bool MeshPathMatches(UStaticMeshComponent* Mesh, const TCHAR* Path) const
		{
			if (!Mesh || !Mesh->GetStaticMesh())
			{
				return false;
			}
			const FString Live = Mesh->GetStaticMesh()->GetPathName();
			return Live.Equals(Path, ESearchCase::CaseSensitive)
				|| Live.StartsWith(FString(Path) + TEXT("."), ESearchCase::CaseSensitive);
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
			Actor->NotificationDurationSeconds = 4.0f;
			Actor->bHasBeenInspected = false;
			Actor->InspectionNotificationCount = 0;
			Actor->ObjectiveEventFireCount = 0;
			ApplyPresentationMesh(Actor->PedestalMesh, CubeMeshPath, ArrayPedestalRel, ArrayPedestalScale);
			ApplyPresentationMesh(Actor->ColumnMesh, CylinderMeshPath, ArrayColumnRel, ArrayColumnScale);
			ApplyPresentationMesh(Actor->ArrayHeadMesh, CylinderMeshPath, ArrayHeadRel, ArrayHeadScale);
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredArrayClone(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AProjectOrganoidInspectableInstrument* Actor = World->SpawnActor<AProjectOrganoidInspectableInstrument>(
				AProjectOrganoidInspectableInstrument::StaticClass(),
				ArrayLocation,
				FRotator::ZeroRotator,
				Params);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString(), true);
			Actor->SetActorScale3D(FVector::OneVector);
			ConfigureArrayIdentically(Actor);
			Track(Actor);
			return Actor;
		}

		UProjectOrganoidObjectiveDataAsset* LoadPersistedMissionAsset() const
		{
			return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionSoftPath);
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

		void ConfigureObservationNode(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(NodeLocation);
			Actor->SetActorRotation(FRotator::ZeroRotator);
			Actor->SetActorScale3D(FVector::OneVector);
			Actor->InteractionRange = NodeInteractionRange;
			Actor->RequiredActiveObjectiveId = FName(FollowId);
			Actor->ObjectiveEventId = FName(FollowEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(FollowId);
			Actor->InspectionPrompt = FText::FromString(InspectPrompt);
			Actor->ReviewPrompt = FText::FromString(ReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(ObservationLine);
			Actor->NotificationDurationSeconds = NodeNotifySeconds;
			Actor->bHasBeenInspected = false;
			Actor->InspectionNotificationCount = 0;
			Actor->ObjectiveEventFireCount = 0;
			ApplyPresentationMesh(Actor->PedestalMesh, CubeMeshPath, NodePedestalRel, NodePedestalScale);
			ApplyPresentationMesh(Actor->ColumnMesh, CylinderMeshPath, NodeColumnRel, NodeColumnScale);
			ApplyPresentationMesh(Actor->ArrayHeadMesh, CubeMeshPath, NodeHeadRel, NodeHeadScale);
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredNode(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			const FTransform Xform(FRotator::ZeroRotator, NodeLocation, FVector::OneVector);
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(), Xform);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString());
			ConfigureObservationNode(Actor);
			Actor->FinishSpawning(Xform);
			ConfigureObservationNode(Actor);
			Track(Actor);
			return Actor;
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
				Record, TEXT("preflight.neuro_package"),
				FPackageName::DoesPackageExist(NeuroPackage),
				TEXT("exists"),
				FPackageName::DoesPackageExist(NeuroPackage) ? TEXT("exists") : TEXT("missing"),
				NeuroPackage);
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

			const int32 StationCount = CountLabel(World, StationLabel);
			const int32 ArrayCount = CountLabel(World, ArrayLabel);
			const int32 CutoffCount = CountLabel(World, CutoffLabel);
			if (World && Character && StationCount >= 1 && ArrayCount == 1 && CutoffCount == 1)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				FailAndStop(
					Owner, Record,
					TEXT("Timed out waiting for PIE player, ResearchStation_NeuroGenetics, NeuralMappingArray_NeuroGenetics, and EmergencyCutoff_NeuroResearchLoad."));
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

			UProjectOrganoidGameplayHUDController* HudController = ResolveHudController(PC);
			UProjectOrganoidHUDWidget* HUD = ResolveOwnedHud(PC);
			UProjectOrganoidHUDWidget* Decoy = SpawnDecoyHud(PC);
			AssertTrue(
				Record, TEXT("hud.owned_resolved"),
				HUD != nullptr,
				TEXT("owned HUD"),
				HUD ? TEXT("owned HUD") : TEXT("null"),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.controller_bound_owned"),
				HudController != nullptr && HudController->GetBoundHUDWidget() == HUD,
				TEXT("bound==owned"),
				(HudController && HudController->GetBoundHUDWidget() == HUD) ? TEXT("bound==owned") : TEXT("mismatch"),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.decoy_distinct"),
				Decoy != nullptr && Decoy != HUD,
				TEXT("true"),
				BoolText(Decoy != nullptr && Decoy != HUD),
				TEXT("HUD"));
			if (!HUD || !HudController || !Decoy)
			{
				FailAndStop(Owner, Record, TEXT("Failed to resolve owned HUD/controller or decoy."));
				return;
			}

			const int32 StationCountBefore = CountLabel(World, StationLabel);
			const int32 PanelCountBefore = CountLabel(World, PanelLabel);
			const int32 ArrayCountBefore = CountLabel(World, ArrayLabel);
			const int32 CutoffCountBefore = CountLabel(World, CutoffLabel);
			const int32 TerminalCountBefore = CountLabel(World, TerminalLabel);
			const int32 Host1Before = CountLabel(World, Host1Label);
			const int32 Host2Before = CountLabel(World, Host2Label);
			const int32 Host3Before = CountLabel(World, Host3Label);
			const int32 HostResearcherBefore = CountLabel(World, HostResearcherLabel);
			const int32 HazardBefore = CountLabel(World, HazardLabel);
			const int32 TrapBefore = CountLabel(World, TrapLabel);
			const int32 GateBefore = CountLabel(World, GateLabel);
			const int32 PadCBefore = CountLabel(World, PadContainmentLabel);
			const int32 PadFBefore = CountLabel(World, PadFailureLabel);
			const int32 CheckpointBefore = CountLabel(World, CheckpointLabel);
			const FVector StationLocBefore = LabelLocationOrZero(World, StationLabel);
			const FVector PanelLocBefore = LabelLocationOrZero(World, PanelLabel);
			const FVector ArrayLocBefore = LabelLocationOrZero(World, ArrayLabel);
			const FVector CutoffLocBefore = LabelLocationOrZero(World, CutoffLabel);
			const FVector TerminalLocBefore = LabelLocationOrZero(World, TerminalLabel);

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

			// ---- A. Persisted Beat 5 four-task mission ----
			UProjectOrganoidObjectiveDataAsset* Fixture = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("mission.loaded_persisted"),
				Fixture != nullptr,
				TEXT("loaded"),
				Fixture ? TEXT("loaded") : TEXT("null"),
				TEXT("DA"));
			if (!Fixture)
			{
				FailAndStop(Owner, Record, TEXT("DA_Mission_NeuroGenetics missing. Require expand_neurogenetics_mission_beat5 + save."));
				return;
			}
			AssertTrue(
				Record, TEXT("mission.exact_soft_path"),
				FSoftObjectPath(Fixture).ToString() == MissionSoftPath,
				MissionSoftPath,
				FSoftObjectPath(Fixture).ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.package_clean"),
				!PackageIsDirty(MissionPackage),
				TEXT("clean"),
				PackageIsDirty(MissionPackage) ? TEXT("dirty") : TEXT("clean"),
				MissionPackage);
			AssertTrue(
				Record, TEXT("mission.id"),
				Fixture->MissionId == FName(MissionId),
				MissionId,
				Fixture->MissionId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.title"),
				Fixture->MissionTitle.ToString() == MissionTitle,
				MissionTitle,
				Fixture->MissionTitle.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.description"),
				Fixture->MissionDescription.ToString() == MissionDescription,
				MissionDescription,
				Fixture->MissionDescription.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.next_null"),
				!Fixture->NextMissionAsset.ToSoftObjectPath().IsValid(),
				TEXT("null"),
				Fixture->NextMissionAsset.ToSoftObjectPath().IsValid()
					? Fixture->NextMissionAsset.ToSoftObjectPath().ToString()
					: TEXT("null"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_count_four"),
				Fixture->Tasks.Num() == 4,
				TEXT("4"),
				FString::FromInt(Fixture->Tasks.Num()),
				TEXT("DA"));
			if (Fixture->Tasks.Num() != 4)
			{
				FailAndStop(Owner, Record, TEXT("DA_Mission_NeuroGenetics is not exact Beat 5 (task count != 4)."));
				return;
			}

			const FProjectOrganoidMissionTaskDefinition& Task1 = Fixture->Tasks[0];
			const FProjectOrganoidMissionTaskDefinition& Task2 = Fixture->Tasks[1];
			const FProjectOrganoidMissionTaskDefinition& Task3 = Fixture->Tasks[2];
			const FProjectOrganoidMissionTaskDefinition& Task4 = Fixture->Tasks[3];

			AssertTrue(
				Record, TEXT("mission.task1_id"),
				Task1.Objective.ObjectiveId == FName(IsolateId),
				IsolateId,
				Task1.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_event_exact"),
				Task1.EventTriggers.Num() == 1
					&& Task1.EventTriggers[0].EventId == FName(IsolateEvent)
					&& Task1.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete,
				TEXT("Event_NeuroResearchLoadIsolated/Complete"),
				Task1.EventTriggers.Num() == 1
					? Task1.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task1.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_id"),
				Task2.Objective.ObjectiveId == FName(TraceId),
				TraceId,
				Task2.Objective.ObjectiveId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task2_event_exact"),
				Task2.EventTriggers.Num() == 1
					&& Task2.EventTriggers[0].EventId == FName(TraceEvent)
					&& Task2.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete,
				TEXT("Event_NeuralMappingSignalTraced/Complete"),
				Task2.EventTriggers.Num() == 1
					? Task2.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task2.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task3_id"),
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
					&& Task3.EventTriggers[0].ObjectiveId == FName(FollowId),
				TEXT("Event_NeuralSignatureFollowed/Complete"),
				Task3.EventTriggers.Num() == 1
					? Task3.EventTriggers[0].EventId.ToString()
					: FString::FromInt(Task3.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task4_id"),
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
				Record, TEXT("mission.task4_no_events"),
				Task4.EventTriggers.Num() == 0,
				TEXT("0"),
				FString::FromInt(Task4.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.no_fifth_task"),
				Fixture->Tasks.Num() == 4,
				TEXT("4"),
				FString::FromInt(Fixture->Tasks.Num()),
				TEXT("DA"));

			// ---- B. Persisted observation node + upstream actors ----
			AssertTrue(
				Record, TEXT("map.array_present"),
				CountLabel(World, ArrayLabel) == 1,
				TEXT("1"),
				FString::FromInt(CountLabel(World, ArrayLabel)),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.cutoff_present"),
				CountLabel(World, CutoffLabel) == 1,
				TEXT("1"),
				FString::FromInt(CountLabel(World, CutoffLabel)),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("map.terminal_present"),
				CountLabel(World, TerminalLabel) == 1,
				TEXT("1"),
				FString::FromInt(CountLabel(World, TerminalLabel)),
				TerminalLabel);

			const TArray<AActor*> NodeMatches = OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel);
			AssertTrue(
				Record, TEXT("map.node_unique"),
				NodeMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(NodeMatches.Num()),
				NodeLabel);
			AProjectOrganoidInspectableInstrument* Node = NodeMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(NodeMatches[0])
				: nullptr;
			if (!Node)
			{
				FailAndStop(Owner, Record, TEXT("NeuralSignatureObservationNode_NeuroGenetics missing. Require spawn + save."));
				return;
			}
			AssertTrue(
				Record, TEXT("node.class_native"),
				Node->GetClass() == AProjectOrganoidInspectableInstrument::StaticClass(),
				TEXT("ProjectOrganoidInspectableInstrument"),
				Node->GetClass() ? Node->GetClass()->GetName() : TEXT("null"),
				NodeLabel);
			const FString NodePackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Node));
			AssertTrue(
				Record, TEXT("node.neuro_package"),
				NodePackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				NodePackage,
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.transform"),
				VecNear(Node->GetActorLocation(), NodeLocation)
					&& RotNear(Node->GetActorRotation(), FRotator::ZeroRotator)
					&& VecNear(Node->GetActorScale3D(), FVector::OneVector),
				TEXT("(500,-2100,-1100)/(0,0,0)/(1,1,1)"),
				FString::Printf(
					TEXT("(%s)/(%s)/(%s)"),
					*Node->GetActorLocation().ToCompactString(),
					*Node->GetActorRotation().ToCompactString(),
					*Node->GetActorScale3D().ToCompactString()),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.required_active_follow"),
				Node->RequiredActiveObjectiveId == FName(FollowId),
				FollowId,
				Node->RequiredActiveObjectiveId.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.objective_event_followed"),
				Node->ObjectiveEventId == FName(FollowEvent),
				FollowEvent,
				Node->ObjectiveEventId.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.replay_guard_follow"),
				Node->CompletedObjectiveIdForReplayGuard == FName(FollowId),
				FollowId,
				Node->CompletedObjectiveIdForReplayGuard.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.inspect_prompt"),
				Node->InspectionPrompt.ToString() == InspectPrompt,
				InspectPrompt,
				Node->InspectionPrompt.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.review_prompt"),
				Node->ReviewPrompt.ToString() == ReviewPrompt,
				ReviewPrompt,
				Node->ReviewPrompt.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.range_175"),
				FMath::IsNearlyEqual(Node->InteractionRange, NodeInteractionRange, 0.01f),
				TEXT("175"),
				FString::SanitizeFloat(Node->InteractionRange),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.speaker"),
				Node->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Node->SpeakerLabel.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.line_exact"),
				Node->InspectionResponseText.ToString() == ObservationLine,
				ObservationLine,
				Node->InspectionResponseText.ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.notify_4"),
				FMath::IsNearlyEqual(Node->NotificationDurationSeconds, NodeNotifySeconds, 0.01f),
				TEXT("4"),
				FString::SanitizeFloat(Node->NotificationDurationSeconds),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.initially_uninspected"),
				!Node->bHasBeenInspected,
				TEXT("false"),
				BoolText(Node->bHasBeenInspected),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.blockout_meshes"),
				MeshPathMatches(Node->PedestalMesh, CubeMeshPath)
					&& VecNear(Node->PedestalMesh->GetRelativeLocation(), NodePedestalRel)
					&& VecNear(Node->PedestalMesh->GetRelativeScale3D(), NodePedestalScale)
					&& Node->PedestalMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Node->PedestalMesh->GetGenerateOverlapEvents()
					&& MeshPathMatches(Node->ColumnMesh, CylinderMeshPath)
					&& VecNear(Node->ColumnMesh->GetRelativeLocation(), NodeColumnRel)
					&& VecNear(Node->ColumnMesh->GetRelativeScale3D(), NodeColumnScale)
					&& Node->ColumnMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Node->ColumnMesh->GetGenerateOverlapEvents()
					&& MeshPathMatches(Node->ArrayHeadMesh, CubeMeshPath)
					&& VecNear(Node->ArrayHeadMesh->GetRelativeLocation(), NodeHeadRel)
					&& VecNear(Node->ArrayHeadMesh->GetRelativeScale3D(), NodeHeadScale)
					&& Node->ArrayHeadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Node->ArrayHeadMesh->GetGenerateOverlapEvents(),
				TEXT("Cube/Cylinder/Cube NoCollision exact"),
				TEXT("checked"),
				NodeLabel);

			AProjectOrganoidInspectableInstrument* Terminal =
				Cast<AProjectOrganoidInspectableInstrument>(
					OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel)[0]);
			AProjectOrganoidInspectableInstrument* Cutoff =
				Cast<AProjectOrganoidInspectableInstrument>(
					OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel)[0]);
			AssertTrue(
				Record, TEXT("map.terminal_cutoff_cast"),
				Terminal != nullptr && Cutoff != nullptr,
				TEXT("present"),
				TEXT("checked"),
				TEXT("map"));

			// ---- C. Before prerequisites: node gated ----
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const int32 PreEvents = Node->ObjectiveEventFireCount;
			const int32 PreLines = Node->InspectionNotificationCount;
			const bool bCanBefore = Node->CanInteract(Character);
			const bool bInteractBefore = Node->Interact(Character);
			AssertTrue(
				Record, TEXT("pre.node_can_false"),
				!bCanBefore,
				TEXT("false"),
				BoolText(bCanBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("pre.node_interact_rejected"),
				!bInteractBefore,
				TEXT("false"),
				BoolText(bInteractBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("pre.no_follow_event"),
				Node->ObjectiveEventFireCount == PreEvents,
				TEXT("0"),
				FString::FromInt(Node->ObjectiveEventFireCount - PreEvents),
				NodeLabel);
			AssertTrue(
				Record, TEXT("pre.no_line"),
				Node->InspectionNotificationCount == PreLines,
				TEXT("0"),
				FString::FromInt(Node->InspectionNotificationCount - PreLines),
				NodeLabel);
			AssertTrue(
				Record, TEXT("pre.not_inspected"),
				!Node->bHasBeenInspected,
				TEXT("false"),
				BoolText(Node->bHasBeenInspected),
				NodeLabel);
			AssertTrue(
				Record, TEXT("pre.decoy_untouched"),
				Decoy->GetLastResourceNotification().IsEmpty()
					|| Decoy->GetLastResourceNotification().ToString() != ObservationRendered,
				TEXT("no observation line"),
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));

			// ---- D. OpeningFoundation → diagnosis → array → cutoff → terminal ----
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
				UEnum::GetValueAsString(ResearchAfterDiagnosis.State),
				ResearchFloorId);

			const bool bOrderedInteracted = OrderedArray->Interact(Character);
			AssertTrue(
				Record, TEXT("ordered.array_completes_research"),
				bOrderedInteracted,
				TEXT("true"),
				BoolText(bOrderedInteracted),
				TEXT("array"));
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
				Record, TEXT("runtime.isolate_active"),
				CountActiveId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(IsolateId))),
				IsolateId);

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

			AssertTrue(
				Record, TEXT("pre_cutoff.node_still_gated"),
				!Node->CanInteract(Character),
				TEXT("false"),
				BoolText(Node->CanInteract(Character)),
				NodeLabel);

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCutoffInteract = Cutoff->Interact(Character);
			AssertTrue(
				Record, TEXT("cutoff.interact_accepted"),
				bCutoffInteract,
				TEXT("true"),
				BoolText(bCutoffInteract),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("isolate.completed"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("isolate.trace_active"),
				CountActiveId(Objectives, FName(TraceId)) == 1
					&& CountCompletedId(Objectives, FName(TraceId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(TraceId)),
					CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("isolate.follow_still_inactive"),
				CountActiveId(Objectives, FName(FollowId)) == 0
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				FollowId);
			AssertTrue(
				Record, TEXT("isolate.examine_still_inactive"),
				CountActiveId(Objectives, FName(ExamineId)) == 0
					&& CountCompletedId(Objectives, FName(ExamineId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				ExamineId);
			AssertTrue(
				Record, TEXT("cutoff.review_prompt"),
				Cutoff->GetInteractionPrompt().ToString() == CutoffReviewPrompt,
				CutoffReviewPrompt,
				Cutoff->GetInteractionPrompt().ToString(),
				CutoffLabel);

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bTerminalInteract = Terminal->Interact(Character);
			AssertTrue(
				Record, TEXT("terminal.interact_accepted"),
				bTerminalInteract,
				TEXT("true"),
				BoolText(bTerminalInteract),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("trace.completed"),
				CountCompletedId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("trace.follow_active"),
				CountActiveId(Objectives, FName(FollowId)) == 1
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("trace.examine_inactive"),
				CountActiveId(Objectives, FName(ExamineId)) == 0
					&& CountCompletedId(Objectives, FName(ExamineId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				ExamineId);
			AssertTrue(
				Record, TEXT("terminal.review_prompt"),
				Terminal->GetInteractionPrompt().ToString() == TerminalReviewPrompt,
				TerminalReviewPrompt,
				Terminal->GetInteractionPrompt().ToString(),
				TerminalLabel);

			// ---- E. Observation node interaction ----
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCanAfter = Node->CanInteract(Character);
			AssertTrue(
				Record, TEXT("node.can_after_follow_active"),
				bCanAfter,
				TEXT("true"),
				BoolText(bCanAfter),
				NodeLabel);
			const int32 EventsBefore = Node->ObjectiveEventFireCount;
			const int32 LinesBefore = Node->InspectionNotificationCount;
			const bool bInteract = Node->Interact(Character);
			AssertTrue(
				Record, TEXT("node.interact_accepted"),
				bInteract,
				TEXT("true"),
				BoolText(bInteract),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.event_once"),
				Node->ObjectiveEventFireCount == EventsBefore + 1,
				TEXT("1"),
				FString::FromInt(Node->ObjectiveEventFireCount - EventsBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.follow_completed_once"),
				CountCompletedId(Objectives, FName(FollowId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("node.line_once"),
				Node->InspectionNotificationCount == LinesBefore + 1,
				TEXT("1"),
				FString::FromInt(Node->InspectionNotificationCount - LinesBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.nathan_exact_owned_hud"),
				HUD->GetLastResourceNotification().ToString() == ObservationRendered,
				ObservationRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("node.decoy_rejected"),
				Decoy->GetLastResourceNotification().ToString() != ObservationRendered,
				TEXT("decoy != owned line"),
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("node.review_prompt_after"),
				Node->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Node->GetInteractionPrompt().ToString(),
				NodeLabel);
			AssertTrue(
				Record, TEXT("node.examine_active_once"),
				CountActiveId(Objectives, FName(ExamineId)) == 1
					&& CountCompletedId(Objectives, FName(ExamineId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(ExamineId)),
					CountCompletedId(Objectives, FName(ExamineId))),
				ExamineId);
			AssertTrue(
				Record, TEXT("node.mission_still_neuro"),
				Objectives->GetActiveMissionId() == FName(MissionId),
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("node.mission_incomplete"),
				!Objectives->IsMissionComplete(FName(MissionId)),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(FName(MissionId))),
				TEXT("mission"));

			// ---- F. Repeat ----
			const int32 RepeatEventsBefore = Node->ObjectiveEventFireCount;
			const int32 RepeatLinesBefore = Node->InspectionNotificationCount;
			const int32 ExamineActiveBefore = CountActiveId(Objectives, FName(ExamineId));
			const bool bRepeat = Node->Interact(Character);
			AssertTrue(
				Record, TEXT("repeat.interact_ok"),
				bRepeat,
				TEXT("true"),
				BoolText(bRepeat),
				NodeLabel);
			AssertTrue(
				Record, TEXT("repeat.no_event_replay"),
				Node->ObjectiveEventFireCount == RepeatEventsBefore,
				TEXT("0"),
				FString::FromInt(Node->ObjectiveEventFireCount - RepeatEventsBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("repeat.no_line_replay"),
				Node->InspectionNotificationCount == RepeatLinesBefore,
				TEXT("0"),
				FString::FromInt(Node->InspectionNotificationCount - RepeatLinesBefore),
				NodeLabel);
			AssertTrue(
				Record, TEXT("repeat.follow_completed"),
				CountCompletedId(Objectives, FName(FollowId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("repeat.examine_active"),
				CountActiveId(Objectives, FName(ExamineId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(ExamineId))),
				ExamineId);
			AssertTrue(
				Record, TEXT("repeat.no_duplicate_activation"),
				CountActiveId(Objectives, FName(ExamineId)) == ExamineActiveBefore
					&& CountActiveId(Objectives, FName(ExamineId)) == 1
					&& CountCompletedId(Objectives, FName(FollowId)) == 1,
				TEXT("examine_active=1 follow_completed=1"),
				FString::Printf(
					TEXT("examine_active=%d follow_completed=%d"),
					CountActiveId(Objectives, FName(ExamineId)),
					CountCompletedId(Objectives, FName(FollowId))),
				TEXT("objectives"));

			// ---- G. Save/load reconstruction ----
			UProjectOrganoidSaveGame* SaveGame = NewObject<UProjectOrganoidSaveGame>(GetTransientPackage());
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.isolate_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(IsolateId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(IsolateId))),
				TEXT("save"));
			AssertTrue(
				Record, TEXT("saveload.trace_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(TraceId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(TraceId))),
				TEXT("save"));
			AssertTrue(
				Record, TEXT("saveload.follow_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(FollowId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(FollowId))),
				TEXT("save"));

			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.isolate_completed"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("saveload.trace_completed"),
				CountCompletedId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("saveload.follow_completed"),
				CountCompletedId(Objectives, FName(FollowId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("saveload.examine_active"),
				CountActiveId(Objectives, FName(ExamineId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(ExamineId))),
				ExamineId);

			AProjectOrganoidInspectableInstrument* Reloaded =
				SpawnConfiguredNode(World, FName(NodeReloadLabel));
			AssertTrue(
				Record, TEXT("saveload.reload_spawned"),
				Reloaded != nullptr,
				TEXT("spawned"),
				Reloaded ? TEXT("spawned") : TEXT("null"),
				NodeReloadLabel);
			if (!Reloaded)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn identically configured observation-node reload clone."));
				return;
			}
			AssertTrue(
				Record, TEXT("saveload.starts_review"),
				Reloaded->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Reloaded->GetInteractionPrompt().ToString(),
				NodeReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.not_marked_without_interact"),
				!Reloaded->bHasBeenInspected,
				TEXT("false"),
				BoolText(Reloaded->bHasBeenInspected),
				NodeReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_disk_path_mutation"),
				Fixture != nullptr
					&& Fixture->Tasks.Num() == 4
					&& FSoftObjectPath(Fixture).ToString() == MissionSoftPath
					&& !PackageIsDirty(MissionPackage),
				TEXT("persisted Beat5 clean"),
				FSoftObjectPath(Fixture).ToString(),
				TEXT("DA"));

			const int32 ReloadEventsBefore = Reloaded->ObjectiveEventFireCount;
			const int32 ReloadLinesBefore = Reloaded->InspectionNotificationCount;
			const bool bReloadInteract = Reloaded->Interact(Character);
			AssertTrue(
				Record, TEXT("saveload.review_interact"),
				bReloadInteract,
				TEXT("true"),
				BoolText(bReloadInteract),
				NodeReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_event_replay"),
				Reloaded->ObjectiveEventFireCount == ReloadEventsBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->ObjectiveEventFireCount - ReloadEventsBefore),
				NodeReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_line_replay"),
				Reloaded->InspectionNotificationCount == ReloadLinesBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->InspectionNotificationCount - ReloadLinesBefore),
				NodeReloadLabel);

			// ---- H. Preservation ----
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
			const int32 RestoreHandled = Objectives->TriggerEvent(FName(RestoreEvent));
			AssertTrue(
				Record, TEXT("preserve.restore_event_unhandled"),
				RestoreHandled == 0,
				TEXT("0"),
				FString::FromInt(RestoreHandled),
				RestoreEvent);

			AssertTrue(
				Record, TEXT("preserve.station_untouched"),
				CountLabel(World, StationLabel) == StationCountBefore
					&& LabelLocationOrZero(World, StationLabel).Equals(StationLocBefore, 0.5f),
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, StationLabel)),
				StationLabel);
			AssertTrue(
				Record, TEXT("preserve.panel_untouched"),
				CountLabel(World, PanelLabel) == PanelCountBefore
					&& LabelLocationOrZero(World, PanelLabel).Equals(PanelLocBefore, 0.5f),
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, PanelLabel)),
				PanelLabel);
			AssertTrue(
				Record, TEXT("preserve.array_untouched"),
				CountLabel(World, ArrayLabel) == ArrayCountBefore
					&& LabelLocationOrZero(World, ArrayLabel).Equals(ArrayLocBefore, 0.5f),
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, ArrayLabel)),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("preserve.cutoff_untouched"),
				CountLabel(World, CutoffLabel) == CutoffCountBefore
					&& LabelLocationOrZero(World, CutoffLabel).Equals(CutoffLocBefore, 0.5f),
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, CutoffLabel)),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("preserve.terminal_untouched"),
				CountLabel(World, TerminalLabel) == TerminalCountBefore
					&& LabelLocationOrZero(World, TerminalLabel).Equals(TerminalLocBefore, 0.5f),
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, TerminalLabel)),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("preserve.hosts_untouched"),
				CountLabel(World, Host1Label) == Host1Before
					&& CountLabel(World, Host2Label) == Host2Before
					&& CountLabel(World, Host3Label) == Host3Before
					&& CountLabel(World, HostResearcherLabel) == HostResearcherBefore,
				TEXT("unchanged"),
				FString::Printf(
					TEXT("%d/%d/%d/%d"),
					CountLabel(World, Host1Label),
					CountLabel(World, Host2Label),
					CountLabel(World, Host3Label),
					CountLabel(World, HostResearcherLabel)),
				TEXT("Hosts"));
			AssertTrue(
				Record, TEXT("preserve.hazard_traps_gate"),
				CountLabel(World, HazardLabel) == HazardBefore
					&& CountLabel(World, TrapLabel) == TrapBefore
					&& CountLabel(World, GateLabel) == GateBefore,
				TEXT("unchanged"),
				FString::Printf(
					TEXT("%d/%d/%d"),
					CountLabel(World, HazardLabel),
					CountLabel(World, TrapLabel),
					CountLabel(World, GateLabel)),
				TEXT("keep"));
			AssertTrue(
				Record, TEXT("preserve.pads_checkpoint"),
				CountLabel(World, PadContainmentLabel) == PadCBefore
					&& CountLabel(World, PadFailureLabel) == PadFBefore
					&& CountLabel(World, CheckpointLabel) == CheckpointBefore,
				TEXT("unchanged"),
				FString::Printf(
					TEXT("%d/%d/%d"),
					CountLabel(World, PadContainmentLabel),
					CountLabel(World, PadFailureLabel),
					CountLabel(World, CheckpointLabel)),
				TEXT("pads"));

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

	struct FNeuroFollowNeuralSignatureAutoRegister
	{
		FNeuroFollowNeuralSignatureAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroFollowNeuralSignatureFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroFollowNeuralSignatureAutoRegister GRegisterNeuroFollowNeuralSignature;
}
