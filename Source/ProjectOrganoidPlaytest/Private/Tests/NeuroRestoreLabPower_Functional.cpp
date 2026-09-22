#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.h"

#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/StrongObjectPtr.h"

namespace NeuroRestoreLabPowerFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroRestoreLabPower_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Restore Lab Power Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR NeuroGeneticsMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	constexpr TCHAR NeuroGeneticsMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR RestoreMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore");
	constexpr TCHAR RestoreMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore");

	constexpr TCHAR NeuroGeneticsMissionId[] = TEXT("Mission_NeuroGenetics");
	constexpr TCHAR RestoreMissionId[] = TEXT("Mission_NeuroPowerRestore");
	constexpr TCHAR RestoreMissionTitle[] = TEXT("Restore NeuroGenetics Power");
	constexpr TCHAR RestoreMissionDescription[] =
		TEXT("Bring the NeuroGenetics research wing back online using its backup power panel.");

	constexpr TCHAR RestoreObjectiveId[] = TEXT("Obj_RestoreNeuroLabPower");
	constexpr TCHAR RestoreObjectiveTitle[] = TEXT("Restore NeuroGenetics power");
	constexpr TCHAR RestoreObjectiveDescription[] =
		TEXT("Use the backup panel to bring the NeuroGenetics sector online.");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");

	constexpr TCHAR IsolateId[] = TEXT("Obj_IsolateNeuroResearchLoad");
	constexpr TCHAR TraceId[] = TEXT("Obj_TraceNeuralMappingSignal");
	constexpr TCHAR FollowId[] = TEXT("Obj_FollowNeuralSignature");
	constexpr TCHAR ExamineId[] = TEXT("Obj_ExamineNeuralChangeEvidence");
	constexpr TCHAR ExamineEvent[] = TEXT("Event_NeuralChangeEvidenceExamined");
	constexpr TCHAR FollowEvent[] = TEXT("Event_NeuralSignatureFollowed");

	constexpr TCHAR MapPanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR UndiscoveredCloneLabel[] = TEXT("PowerPanel_NeuroBackup_UndiscoveredClone");
	constexpr TCHAR WrongGatePanelLabel[] = TEXT("PowerPanel_NeuroBackup_WrongGateClone");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroRestoreLabPowerTest");

	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");
	constexpr TCHAR InvestigationObjectiveId[] = TEXT("Obj_InvestigateNeuroPowerFailure");

	constexpr TCHAR ExpectedInspectPrompt[] = TEXT("Inspect Power Controls");
	constexpr TCHAR ExpectedDiscoveryReviewPrompt[] = TEXT("Review Power Status");
	constexpr TCHAR ExpectedActiveRestorePrompt[] = TEXT("Restore NeuroGenetics power");
	constexpr TCHAR ExpectedCompletedReviewPrompt[] = TEXT("Review backup power status");
	constexpr TCHAR ExpectedFailureStatus[] = TEXT("PRIMARY FEED OFFLINE — EMERGENCY BACKUP ACTIVE");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	constexpr TCHAR ExpectedRestoreLine[] =
		TEXT("NeuroGenetics is back online. Cryo is still dark, but I can work with this.");
	constexpr TCHAR ExpectedRestoreRendered[] =
		TEXT("Nathan: NeuroGenetics is back online. Cryo is still dark, but I can work with this.");
	constexpr float ExpectedRestoreDuration = 6.0f;

	const FVector ExpectedPanelLocation(-500.0f, -2275.0f, -1100.0f);
	const FVector UndiscoveredCloneLocation(-350.0f, -2275.0f, -1100.0f);
	const FVector WrongGatePanelLocation(-650.0f, -2275.0f, -1100.0f);

	constexpr TCHAR NodeLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	constexpr TCHAR InstrumentLabel[] = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	constexpr TCHAR CutoffLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad");
	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR ArrayOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");
	constexpr TCHAR HostResearcherLabel[] = TEXT("Host_Neuro_Researcher");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");

	constexpr TCHAR CutoffReviewPrompt[] = TEXT("Research Load Isolated");
	constexpr TCHAR TerminalReviewPrompt[] = TEXT("Signal Trace Complete");
	constexpr TCHAR NodeInspectPrompt[] = TEXT("Follow Neural Signature");
	constexpr TCHAR NodeReviewPrompt[] = TEXT("Neural Signature Located");
	constexpr TCHAR InstrumentInspectPrompt[] = TEXT("Examine neural-change evidence");
	constexpr TCHAR InstrumentReviewPrompt[] = TEXT("Review neural-change evidence");
	constexpr TCHAR ArrayInspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ArrayReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ArrayLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	constexpr TCHAR ObservationLine[] =
		TEXT("The signature continues into the research wing. Epitope wasn\u2019t just recording the damage. They were studying the same change in every subject.");
	constexpr TCHAR EvidenceLine[] =
		TEXT("These patterns match across multiple subjects. Epitope wasn\u2019t documenting isolated changes; they were tracking the same neural adaptation.");

	constexpr float NodeInteractionRange = 175.0f;
	constexpr float InstrumentInteractionRange = 175.0f;
	constexpr float ArrayInteractionRange = 200.0f;
	const FVector NodeLocation(500.0f, -2100.0f, -1100.0f);
	const FVector InstrumentLocation(500.0f, -2520.0f, -1100.0f);
	const FRotator InstrumentRotation(0.0f, 90.0f, 0.0f);
	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);

	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FVector NodePedestalRel(0.0f, 0.0f, 30.0f);
	const FVector NodePedestalScale(0.60f, 0.50f, 0.60f);
	const FVector NodeColumnRel(0.0f, 0.0f, 95.0f);
	const FVector NodeColumnScale(0.30f, 0.30f, 1.00f);
	const FVector NodeHeadRel(0.0f, 0.0f, 155.0f);
	const FVector NodeHeadScale(0.90f, 0.45f, 0.25f);
	const FVector ArrayPedestalRel(0.0f, 0.0f, 40.0f);
	const FVector ArrayPedestalScale(1.2f, 1.2f, 0.8f);
	const FVector ArrayColumnRel(0.0f, 0.0f, 110.0f);
	const FVector ArrayColumnScale(0.35f, 0.35f, 1.4f);
	const FVector ArrayHeadRel(0.0f, 0.0f, 192.5f);
	const FVector ArrayHeadScale(1.6f, 1.6f, 0.25f);

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

	int32 CountJournalId(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		int32 Count = 0;
		if (!Objectives)
		{
			return 0;
		}
		for (const FProjectOrganoidObjective& Entry : Objectives->GetJournalEntries())
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

	class FNeuroRestoreLabPowerFunctional : public IOrganoidPlaytestCase
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
			MapPanel.Reset();
			TransientPanels.Reset();
			TransientInstruments.Reset();
			OwnedHudWidget.Reset();
			DecoyHudWidget.Reset();
			OpeningMissionProbe.Reset();
			OpeningMissionObjectives.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroyTransientActors();
			UnbindOpeningMissionProbe();
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
				UnbindOpeningMissionProbe();
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
		TWeakObjectPtr<AProjectOrganoidPowerPanel> MapPanel;
		TArray<TWeakObjectPtr<AProjectOrganoidPowerPanel>> TransientPanels;
		TArray<TWeakObjectPtr<AProjectOrganoidInspectableInstrument>> TransientInstruments;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> OwnedHudWidget;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> DecoyHudWidget;
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

		UProjectOrganoidObjectiveDataAsset* LoadNeuroGeneticsMission() const
		{
			return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, NeuroGeneticsMissionSoftPath);
		}

		UProjectOrganoidObjectiveDataAsset* LoadRestoreMission() const
		{
			return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RestoreMissionSoftPath);
		}

		void TrackPanel(AProjectOrganoidPowerPanel* Panel)
		{
			if (Panel)
			{
				TransientPanels.Add(Panel);
			}
		}

		void TrackInstrument(AProjectOrganoidInspectableInstrument* Actor)
		{
			if (Actor)
			{
				TransientInstruments.Add(Actor);
			}
		}

		void DestroyTransientActors()
		{
			for (const TWeakObjectPtr<AProjectOrganoidPowerPanel>& Weak : TransientPanels)
			{
				if (AProjectOrganoidPowerPanel* Panel = Weak.Get())
				{
					Panel->Destroy();
				}
			}
			TransientPanels.Reset();
			for (const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& Weak : TransientInstruments)
			{
				if (AProjectOrganoidInspectableInstrument* Actor = Weak.Get())
				{
					Actor->Destroy();
				}
			}
			TransientInstruments.Reset();
			if (UProjectOrganoidHUDWidget* Decoy = DecoyHudWidget.Get())
			{
				Decoy->RemoveFromParent();
			}
			DecoyHudWidget.Reset();
			MapPanel.Reset();
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

		void ApplyBeat7PanelContract(AProjectOrganoidPowerPanel* Panel) const
		{
			if (!Panel)
			{
				return;
			}
			Panel->PowerSector = EProjectOrganoidPowerSector::NeuroGenetics;
			Panel->RestoredState = EProjectOrganoidPowerState::Online;
			Panel->bSingleUse = true;
			Panel->bDiscoverPowerFailureBeforeRestore = true;
			Panel->FailureStatusReport = FText::FromString(ExpectedFailureStatus);
			Panel->DiscoveryObjectiveEventId = FName(DiscoveryEvent);
			Panel->SuccessObjectiveEventId = FName(RestoreEvent);
			Panel->RequiredActiveObjectiveId = FName(RestoreObjectiveId);
			Panel->ActiveRestorePrompt = FText::FromString(ExpectedActiveRestorePrompt);
			Panel->ReviewPrompt = FText::FromString(ExpectedCompletedReviewPrompt);
			Panel->RestoreSuccessNotificationSpeaker = FText::FromString(ExpectedSpeaker);
			Panel->RestoreSuccessNotificationText = FText::FromString(ExpectedRestoreLine);
			Panel->RestoreSuccessNotificationDurationSeconds = ExpectedRestoreDuration;
			Panel->InteractionPrompt = FText::FromString(ExpectedInspectPrompt);
			Panel->RefreshPrompt();
		}

		AProjectOrganoidPowerPanel* SpawnBeat7PanelClone(
			UWorld* World,
			const TCHAR* Label,
			const FVector& Location,
			bool bPreDiscovered) const
		{
			if (!World)
			{
				return nullptr;
			}
			AProjectOrganoidPowerPanel* Panel = World->SpawnActorDeferred<AProjectOrganoidPowerPanel>(
				AProjectOrganoidPowerPanel::StaticClass(),
				FTransform(Location));
			if (!Panel)
			{
				return nullptr;
			}
			Panel->SetActorLabel(Label);
			ApplyBeat7PanelContract(Panel);
			Panel->bHasDiscoveredPowerFailure = bPreDiscovered;
			Panel->bHasBeenEngaged = false;
			Panel->DiscoveryEventFireCount = 0;
			Panel->SuccessEventFireCount = 0;
			Panel->RestoreSuccessNotificationCount = 0;
			Panel->StatusReportCount = 0;
			Panel->FinishSpawning(FTransform(Location));
			ApplyBeat7PanelContract(Panel);
			Panel->RefreshPrompt();
			return Panel;
		}

		AProjectOrganoidPowerPanel* SpawnWrongGatePanel(UWorld* World) const
		{
			AProjectOrganoidPowerPanel* Panel = SpawnBeat7PanelClone(
				World, WrongGatePanelLabel, WrongGatePanelLocation, false);
			if (Panel)
			{
				Panel->RequiredActiveObjectiveId = FName(InvestigationObjectiveId);
				Panel->RefreshPrompt();
			}
			return Panel;
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

		void ConfigureArrayClone(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(ArrayLocation);
			Actor->InteractionRange = ArrayInteractionRange;
			Actor->RequiredActiveObjectiveId = NAME_None;
			Actor->ObjectiveEventId = FName(ArrayEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(ResearchFloorId);
			Actor->InspectionPrompt = FText::FromString(ArrayInspectPrompt);
			Actor->ReviewPrompt = FText::FromString(ArrayReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(ArrayLine);
			Actor->NotificationDurationSeconds = 4.0f;
			ApplyPresentationMesh(Actor->PedestalMesh, CubeMeshPath, ArrayPedestalRel, ArrayPedestalScale);
			ApplyPresentationMesh(Actor->ColumnMesh, CylinderMeshPath, ArrayColumnRel, ArrayColumnScale);
			ApplyPresentationMesh(Actor->ArrayHeadMesh, CylinderMeshPath, ArrayHeadRel, ArrayHeadScale);
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnArrayClone(UWorld* World)
		{
			AProjectOrganoidInspectableInstrument* Actor = World->SpawnActor<AProjectOrganoidInspectableInstrument>(
				AProjectOrganoidInspectableInstrument::StaticClass(),
				ArrayLocation,
				FRotator::ZeroRotator);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(ArrayOrderedLabel);
			ConfigureArrayClone(Actor);
			TrackInstrument(Actor);
			return Actor;
		}

		bool BootstrapThroughRestoreMissionHandoff(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			UProjectOrganoidObjectiveSubsystem* Objectives,
			AProjectOrganoidInspectableInstrument* Cutoff,
			AProjectOrganoidInspectableInstrument* Terminal,
			AProjectOrganoidInspectableInstrument* Node,
			AProjectOrganoidInspectableInstrument* Instrument,
			UProjectOrganoidHUDWidget* HUD)
		{
			Objectives->LoadDefaultMission();
			if (!AssertTrue(
					Record, TEXT("handoff.opening_current"),
					Objectives->GetActiveMissionId() == TEXT("Mission_OpeningFoundation"),
					TEXT("Mission_OpeningFoundation"),
					Objectives->GetActiveMissionId().ToString(),
					TEXT("mission")))
			{
				return false;
			}
			AssertTrue(
				Record, TEXT("handoff.pending_neurogenetics"),
				Objectives->GetPendingNextMissionAssetPath().ToString() == NeuroGeneticsMissionSoftPath,
				NeuroGeneticsMissionSoftPath,
				Objectives->GetPendingNextMissionAssetPath().ToString(),
				TEXT("objectives"));

			AProjectOrganoidInspectableInstrument* OrderedArray = SpawnArrayClone(World);
			if (!AssertTrue(
					Record, TEXT("handoff.array_clone"),
					OrderedArray != nullptr,
					TEXT("spawned"),
					OrderedArray ? TEXT("spawned") : TEXT("null"),
					ArrayOrderedLabel))
			{
				return false;
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
			const bool bArrayInteract = OrderedArray->Interact(Character);
			AssertTrue(Record, TEXT("handoff.array_interact"), bArrayInteract, TEXT("true"), BoolText(bArrayInteract), ArrayOrderedLabel);
			Objectives->TriggerEvent(FName(ReceptionEvent));
			Objectives->TriggerEvent(FName(SecurityEvent));

			AssertTrue(
				Record, TEXT("handoff.neurogenetics_current"),
				Objectives->GetActiveMissionId() == FName(NeuroGeneticsMissionId),
				NeuroGeneticsMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("handoff.opening_completed_once"),
				LiveOpeningProbe && LiveOpeningProbe->OpeningFoundationCompletedCount == 1,
				TEXT("1"),
				FString::FromInt(LiveOpeningProbe ? LiveOpeningProbe->OpeningFoundationCompletedCount : -1),
				TEXT("mission"));
			UnbindOpeningMissionProbe();
			if (OrderedArray)
			{
				OrderedArray->Destroy();
			}

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCutoffOk = Cutoff && Cutoff->Interact(Character);
			AssertTrue(Record, TEXT("handoff.cutoff"), bCutoffOk, TEXT("true"), BoolText(bCutoffOk), CutoffLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bTerminalOk = Terminal && Terminal->Interact(Character);
			AssertTrue(Record, TEXT("handoff.terminal"), bTerminalOk, TEXT("true"), BoolText(bTerminalOk), TerminalLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bNodeOk = Node && Node->Interact(Character);
			AssertTrue(Record, TEXT("handoff.node"), bNodeOk, TEXT("true"), BoolText(bNodeOk), NodeLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bInstrumentOk = Instrument && Instrument->Interact(Character);
			AssertTrue(Record, TEXT("handoff.instrument"), bInstrumentOk, TEXT("true"), BoolText(bInstrumentOk), InstrumentLabel);

			AssertTrue(
				Record, TEXT("handoff.neurogenetics_tasks_completed"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1
					&& CountCompletedId(Objectives, FName(TraceId)) == 1
					&& CountCompletedId(Objectives, FName(FollowId)) == 1
					&& CountCompletedId(Objectives, FName(ExamineId)) == 1,
				TEXT("4/4 completed"),
				FString::Printf(
					TEXT("%d/%d/%d/%d"),
					CountCompletedId(Objectives, FName(IsolateId)),
					CountCompletedId(Objectives, FName(TraceId)),
					CountCompletedId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(ExamineId))),
				NeuroGeneticsMissionId);
			AssertTrue(
				Record, TEXT("handoff.restore_mission_current"),
				Objectives->GetActiveMissionId() == FName(RestoreMissionId),
				RestoreMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("handoff.restore_objective_active"),
				CountActiveId(Objectives, FName(RestoreObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(RestoreObjectiveId))),
				RestoreObjectiveId);

			FProjectOrganoidObjective RestoreObjective;
			Objectives->GetObjective(FName(RestoreObjectiveId), RestoreObjective);
			AssertTrue(
				Record, TEXT("handoff.restore_title"),
				RestoreObjective.Title.ToString() == RestoreObjectiveTitle,
				RestoreObjectiveTitle,
				RestoreObjective.Title.ToString(),
				RestoreObjectiveId);

			return !bAnyAssertFailed;
		}

		void AssertPersistedMissionContracts(FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* NeuroMission = LoadNeuroGeneticsMission();
			AssertTrue(
				Record, TEXT("asset.neurogenetics_loaded"),
				NeuroMission != nullptr,
				TEXT("loaded"),
				NeuroMission ? TEXT("loaded") : TEXT("null"),
				TEXT("DA"));
			if (!NeuroMission)
			{
				return;
			}
			AssertTrue(
				Record, TEXT("asset.neurogenetics_next_restore"),
				NeuroMission->NextMissionAsset.ToSoftObjectPath().ToString() == RestoreMissionSoftPath,
				RestoreMissionSoftPath,
				NeuroMission->NextMissionAsset.ToSoftObjectPath().ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.neurogenetics_package_clean"),
				!PackageIsDirty(NeuroGeneticsMissionPackage),
				TEXT("clean"),
				PackageIsDirty(NeuroGeneticsMissionPackage) ? TEXT("dirty") : TEXT("clean"),
				NeuroGeneticsMissionPackage);
			AssertTrue(
				Record, TEXT("asset.neurogenetics_task_count"),
				NeuroMission->Tasks.Num() == 4,
				TEXT("4"),
				FString::FromInt(NeuroMission->Tasks.Num()),
				TEXT("DA"));

			UProjectOrganoidObjectiveDataAsset* RestoreMission = LoadRestoreMission();
			AssertTrue(
				Record, TEXT("asset.restore_loaded"),
				RestoreMission != nullptr,
				TEXT("loaded"),
				RestoreMission ? TEXT("loaded") : TEXT("null"),
				TEXT("DA"));
			if (!RestoreMission)
			{
				return;
			}
			AssertTrue(
				Record, TEXT("asset.restore_soft_path"),
				FSoftObjectPath(RestoreMission).ToString() == RestoreMissionSoftPath,
				RestoreMissionSoftPath,
				FSoftObjectPath(RestoreMission).ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.restore_package_clean"),
				!PackageIsDirty(RestoreMissionPackage),
				TEXT("clean"),
				PackageIsDirty(RestoreMissionPackage) ? TEXT("dirty") : TEXT("clean"),
				RestoreMissionPackage);
			AssertTrue(
				Record, TEXT("asset.restore_mission_id"),
				RestoreMission->MissionId == FName(RestoreMissionId),
				RestoreMissionId,
				RestoreMission->MissionId.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.restore_mission_title"),
				RestoreMission->MissionTitle.ToString() == RestoreMissionTitle,
				RestoreMissionTitle,
				RestoreMission->MissionTitle.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.restore_mission_description"),
				RestoreMission->MissionDescription.ToString() == RestoreMissionDescription,
				RestoreMissionDescription,
				RestoreMission->MissionDescription.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.restore_next_null"),
				RestoreMission->NextMissionAsset.IsNull(),
				TEXT("null"),
				RestoreMission->NextMissionAsset.IsNull() ? TEXT("null") : RestoreMission->NextMissionAsset.ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("asset.restore_task_count_one"),
				RestoreMission->Tasks.Num() == 1,
				TEXT("1"),
				FString::FromInt(RestoreMission->Tasks.Num()),
				TEXT("DA"));
			if (RestoreMission->Tasks.Num() == 1)
			{
				const FProjectOrganoidMissionTaskDefinition& Task = RestoreMission->Tasks[0];
				AssertTrue(
					Record, TEXT("asset.restore_objective_id"),
					Task.Objective.ObjectiveId == FName(RestoreObjectiveId),
					RestoreObjectiveId,
					Task.Objective.ObjectiveId.ToString(),
					TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.restore_objective_title"),
					Task.Objective.Title.ToString() == RestoreObjectiveTitle,
					RestoreObjectiveTitle,
					Task.Objective.Title.ToString(),
					TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.restore_objective_description"),
					Task.Objective.Description.ToString() == RestoreObjectiveDescription,
					RestoreObjectiveDescription,
					Task.Objective.Description.ToString(),
					TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.restore_auto_activate"),
					Task.bAutoActivate,
					TEXT("true"),
					BoolText(Task.bAutoActivate),
					TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.restore_target_count"),
					Task.Objective.TargetProgress == 1,
					TEXT("1"),
					FString::FromInt(Task.Objective.TargetProgress),
					TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.restore_complete_event"),
					Task.EventTriggers.Num() == 1
						&& Task.EventTriggers[0].EventId == FName(RestoreEvent)
						&& Task.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete,
					TEXT("Event_NeuroPowerRestored/Complete"),
					Task.EventTriggers.Num() == 1
						? Task.EventTriggers[0].EventId.ToString()
						: FString::FromInt(Task.EventTriggers.Num()),
					TEXT("DA"));
			}
		}

		void AssertMapPanelBeat7Contract(FOrganoidPlaytestRecord& Record, AProjectOrganoidPowerPanel* Panel)
		{
			AssertTrue(
				Record, TEXT("map.panel_unique_label"),
				OrganoidPlaytestActions::ActorLabel(Panel).Equals(MapPanelLabel, ESearchCase::CaseSensitive),
				MapPanelLabel,
				OrganoidPlaytestActions::ActorLabel(Panel),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.panel_neuro_package"),
				OrganoidPlaytestActions::NormalizePackage(OrganoidPlaytestActions::ActorPackage(Panel))
					.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				OrganoidPlaytestActions::NormalizePackage(OrganoidPlaytestActions::ActorPackage(Panel)),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.panel_location"),
				Panel->GetActorLocation().Equals(ExpectedPanelLocation, 1.0f),
				ExpectedPanelLocation.ToCompactString(),
				Panel->GetActorLocation().ToCompactString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.discovery_opt_in"),
				Panel->bDiscoverPowerFailureBeforeRestore,
				TEXT("true"),
				BoolText(Panel->bDiscoverPowerFailureBeforeRestore),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.required_restore_objective"),
				Panel->RequiredActiveObjectiveId == FName(RestoreObjectiveId),
				RestoreObjectiveId,
				Panel->RequiredActiveObjectiveId.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.active_restore_prompt"),
				Panel->ActiveRestorePrompt.ToString() == ExpectedActiveRestorePrompt,
				ExpectedActiveRestorePrompt,
				Panel->ActiveRestorePrompt.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.completed_review_prompt"),
				Panel->ReviewPrompt.ToString() == ExpectedCompletedReviewPrompt,
				ExpectedCompletedReviewPrompt,
				Panel->ReviewPrompt.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_event_id"),
				Panel->SuccessObjectiveEventId == FName(RestoreEvent),
				RestoreEvent,
				Panel->SuccessObjectiveEventId.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_speaker"),
				Panel->RestoreSuccessNotificationSpeaker.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Panel->RestoreSuccessNotificationSpeaker.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_line"),
				Panel->RestoreSuccessNotificationText.ToString() == ExpectedRestoreLine,
				ExpectedRestoreLine,
				Panel->RestoreSuccessNotificationText.ToString(),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_duration"),
				FMath::IsNearlyEqual(Panel->RestoreSuccessNotificationDurationSeconds, ExpectedRestoreDuration, 0.01f),
				FString::SanitizeFloat(ExpectedRestoreDuration),
				FString::SanitizeFloat(Panel->RestoreSuccessNotificationDurationSeconds),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.sector_neuro"),
				Panel->PowerSector == EProjectOrganoidPowerSector::NeuroGenetics,
				TEXT("NeuroGenetics"),
				TEXT("NeuroGenetics"),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restored_state_online"),
				Panel->RestoredState == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Panel->RestoredState),
				MapPanelLabel);
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			if (!GEditor)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Editor is not available."));
				return;
			}
			if (GEditor->IsPlaySessionInProgress())
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(NeuroPackage)
				|| PackageIsDirty(NeuroGeneticsMissionPackage) || PackageIsDirty(RestoreMissionPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Required map or mission packages are dirty. Refusing to start."));
				return;
			}

			TArray<FString> DirtyNow;
			CollectDirtyPackageNames(DirtyNow);
			if (DirtyNow.Num() != 0)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					FString::Printf(TEXT("Unexpected dirty packages: %s"), *FString::Join(DirtyNow, TEXT(","))));
				return;
			}

			AssertPersistedMissionContracts(Record);
			if (LoadNeuroGeneticsMission() == nullptr || LoadRestoreMission() == nullptr)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Beat 7 mission assets missing. Run bridge create/configure actions and save before this test."));
				return;
			}
			if (bAnyAssertFailed)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Persisted mission contract mismatch."));
				return;
			}

			DirtyBefore = DirtyNow;
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("StartPie"));
			if (!Owner.RequestStartPie(MapPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			(void)Record;
			WaitSeconds = 0.0f;
			bRequestedNeuroStream = false;
			Stage = EStage::WaitReady;
			Owner.SetStage(TEXT("WaitReady"));
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

			bool bPlayerReady = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bPlayerReady = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling || WaitSeconds > 10.0f;
				}
			}

			const TArray<AActor*> PanelMatches = OrganoidPlaytestActions::FindActorsByLabel(World, MapPanelLabel);
			const bool bPanelReady = PanelMatches.Num() == 1;
			const bool bCutoffReady = CountLabel(World, CutoffLabel) == 1;
			const bool bTerminalReady = CountLabel(World, TerminalLabel) == 1;
			const bool bNodeReady = CountLabel(World, NodeLabel) == 1;
			const bool bInstrumentReady = CountLabel(World, InstrumentLabel) == 1;

			if (Character && bPlayerReady && bPanelReady && bCutoffReady && bTerminalReady && bNodeReady && bInstrumentReady)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}

			if (WaitSeconds > 90.0f)
			{
				FailAndStop(
					Owner,
					Record,
					FString::Printf(
						TEXT("Timed out waiting for PIE player and Neuro Beat 7 actors. panel=%d cutoff=%d terminal=%d node=%d instrument=%d"),
						PanelMatches.Num(),
						CountLabel(World, CutoffLabel),
						CountLabel(World, TerminalLabel),
						CountLabel(World, NodeLabel),
						CountLabel(World, InstrumentLabel)));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			UProjectOrganoidHUDWidget* HUD = ResolveOwnedHud(PC);
			UProjectOrganoidHUDWidget* Decoy = SpawnDecoyHud(PC);

			if (!World || !Character || !Power || !Objectives || !Saves || !HUD)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE world/character/power/objectives/saves/HUD."));
				return;
			}

			const int32 StationCountBefore = CountLabel(World, StationLabel);
			const int32 Host1Before = CountLabel(World, Host1Label);
			const int32 Host2Before = CountLabel(World, Host2Label);
			const int32 Host3Before = CountLabel(World, Host3Label);
			const int32 HostResearcherBefore = CountLabel(World, HostResearcherLabel);
			const int32 GateCountBefore = CountLabel(World, GateLabel);

			AssertTrue(
				Record, TEXT("seed.neuro_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("seed.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));

			AssertPersistedMissionContracts(Record);

			const TArray<AActor*> PanelMatches = OrganoidPlaytestActions::FindActorsByLabel(World, MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.panel_count"),
				PanelMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(PanelMatches.Num()),
				MapPanelLabel);
			AProjectOrganoidPowerPanel* Panel = PanelMatches.Num() == 1
				? Cast<AProjectOrganoidPowerPanel>(PanelMatches[0])
				: nullptr;
			MapPanel = Panel;
			if (!Panel)
			{
				FailAndStop(Owner, Record, TEXT("PowerPanel_NeuroBackup missing or wrong class."));
				return;
			}
			AssertMapPanelBeat7Contract(Record, Panel);

			AProjectOrganoidInspectableInstrument* Cutoff = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel)[0]);
			AProjectOrganoidInspectableInstrument* Terminal = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel)[0]);
			AProjectOrganoidInspectableInstrument* Node = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel)[0]);
			AProjectOrganoidInspectableInstrument* Instrument = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, InstrumentLabel)[0]);
			if (!Cutoff || !Terminal || !Node || !Instrument)
			{
				FailAndStop(Owner, Record, TEXT("Missing Neuro handoff map actors."));
				return;
			}

			// ---- Pre-handoff: map panel discovery does not restore power or complete restore objective ----
			const int32 RestoreHandledPre = Objectives->TriggerEvent(FName(RestoreEvent));
			AssertTrue(
				Record, TEXT("pre.restore_event_unhandled"),
				RestoreHandledPre == 0,
				TEXT("0"),
				FString::FromInt(RestoreHandledPre),
				RestoreEvent);
			const bool bDiscoveryOk = Panel->Interact(Character);
			AssertTrue(Record, TEXT("pre.discovery_interact"), bDiscoveryOk, TEXT("true"), BoolText(bDiscoveryOk), MapPanelLabel);
			AssertTrue(
				Record, TEXT("pre.discovery_no_engagement"),
				Panel->bHasDiscoveredPowerFailure && !Panel->bHasBeenEngaged,
				TEXT("discovered/not engaged"),
				FString::Printf(
					TEXT("discovered=%s engaged=%s"),
					*BoolText(Panel->bHasDiscoveredPowerFailure),
					*BoolText(Panel->bHasBeenEngaged)),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("pre.discovery_neuro_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("pre.discovery_success_event_zero"),
				Panel->SuccessEventFireCount == 0,
				TEXT("0"),
				FString::FromInt(Panel->SuccessEventFireCount),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("pre.discovery_review_prompt"),
				Panel->GetInteractionPrompt().ToString() == ExpectedDiscoveryReviewPrompt
					|| Panel->GetInteractionPrompt().ToString() == ExpectedCompletedReviewPrompt,
				ExpectedDiscoveryReviewPrompt,
				Panel->GetInteractionPrompt().ToString(),
				MapPanelLabel);

			if (!BootstrapThroughRestoreMissionHandoff(
					Owner, Record, World, Character, Objectives, Cutoff, Terminal, Node, Instrument, HUD))
			{
				FailAndStop(Owner, Record, TEXT("Failed real mission handoff to Mission_NeuroPowerRestore."));
				return;
			}

			// ---- Pre-restore: gated restore blocked on map panel until objective active (already active) ----
			AssertTrue(
				Record, TEXT("gate.restore_objective_active_once"),
				CountActiveId(Objectives, FName(RestoreObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(RestoreObjectiveId))),
				RestoreObjectiveId);
			AssertTrue(
				Record, TEXT("gate.map_pre_discovered_active_prompt"),
				Panel->GetInteractionPrompt().ToString() == ExpectedActiveRestorePrompt,
				ExpectedActiveRestorePrompt,
				Panel->GetInteractionPrompt().ToString(),
				MapPanelLabel);

			// ---- Unrelated wrong-gate panel cannot complete restore objective ----
			AProjectOrganoidPowerPanel* WrongGate = SpawnWrongGatePanel(World);
			TrackPanel(WrongGate);
			if (WrongGate)
			{
				const int32 SuccessBeforeWrong = WrongGate->SuccessEventFireCount;
				const bool bWrongInteract = WrongGate->Interact(Character);
				AssertTrue(
					Record, TEXT("unrelated.wrong_gate_interact_ok"),
					bWrongInteract,
					TEXT("true"),
					BoolText(bWrongInteract),
					WrongGatePanelLabel);
				AssertTrue(
					Record, TEXT("unrelated.wrong_gate_no_success_event"),
					WrongGate->SuccessEventFireCount == SuccessBeforeWrong,
					TEXT("0"),
					FString::FromInt(WrongGate->SuccessEventFireCount - SuccessBeforeWrong),
					WrongGatePanelLabel);
				AssertTrue(
					Record, TEXT("unrelated.wrong_gate_no_restore"),
					!WrongGate->bHasBeenEngaged,
					TEXT("false"),
					BoolText(WrongGate->bHasBeenEngaged),
					WrongGatePanelLabel);
				AssertTrue(
					Record, TEXT("unrelated.objective_still_active"),
					CountActiveId(Objectives, FName(RestoreObjectiveId)) == 1
						&& CountCompletedId(Objectives, FName(RestoreObjectiveId)) == 0,
					TEXT("active=1 completed=0"),
					FString::Printf(
						TEXT("active=%d completed=%d"),
						CountActiveId(Objectives, FName(RestoreObjectiveId)),
						CountCompletedId(Objectives, FName(RestoreObjectiveId))),
					RestoreObjectiveId);
				AssertTrue(
					Record, TEXT("unrelated.neuro_still_emergency"),
					Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
					TEXT("Emergency"),
					PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
					TEXT("Power"));
			}

			AProjectOrganoidPowerPanel* UndiscoveredClone =
				SpawnBeat7PanelClone(World, UndiscoveredCloneLabel, UndiscoveredCloneLocation, false);
			TrackPanel(UndiscoveredClone);
			AssertTrue(
				Record, TEXT("undiscovered.clone_spawned"),
				UndiscoveredClone != nullptr,
				TEXT("spawned"),
				UndiscoveredClone ? TEXT("spawned") : TEXT("null"),
				UndiscoveredCloneLabel);
			if (UndiscoveredClone)
			{
				AssertTrue(
					Record, TEXT("undiscovered.prompt_active_restore"),
					!UndiscoveredClone->bHasDiscoveredPowerFailure
						&& UndiscoveredClone->GetInteractionPrompt().ToString() == ExpectedActiveRestorePrompt,
					ExpectedActiveRestorePrompt,
					UndiscoveredClone->GetInteractionPrompt().ToString(),
					UndiscoveredCloneLabel);
			}

			// ---- Pre-discovered map restore: one interact while restore objective Active ----
			const int32 MapSuccessBefore = Panel->SuccessEventFireCount;
			const int32 MapNotifyBefore = Panel->RestoreSuccessNotificationCount;
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bMapRestore = Panel->Interact(Character);
			AssertTrue(Record, TEXT("prediscovered.map_restore_interact"), bMapRestore, TEXT("true"), BoolText(bMapRestore), MapPanelLabel);
			AssertTrue(
				Record, TEXT("prediscovered.map_engaged"),
				Panel->bHasBeenEngaged && Panel->bHasDiscoveredPowerFailure,
				TEXT("engaged+discovered"),
				FString::Printf(
					TEXT("engaged=%s discovered=%s"),
					*BoolText(Panel->bHasBeenEngaged),
					*BoolText(Panel->bHasDiscoveredPowerFailure)),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("prediscovered.success_event_once"),
				Panel->SuccessEventFireCount == MapSuccessBefore + 1,
				TEXT("1"),
				FString::FromInt(Panel->SuccessEventFireCount - MapSuccessBefore),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("prediscovered.nathan_once"),
				Panel->RestoreSuccessNotificationCount == MapNotifyBefore + 1
					&& HUD->GetLastResourceNotification().ToString() == ExpectedRestoreRendered,
				ExpectedRestoreRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("prediscovered.decoy_rejected"),
				Decoy->GetLastResourceNotification().ToString() != ExpectedRestoreRendered,
				TEXT("decoy != owned"),
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("prediscovered.neuro_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("prediscovered.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("prediscovered.objective_completed_once"),
				CountCompletedId(Objectives, FName(RestoreObjectiveId)) == 1
					&& CountActiveId(Objectives, FName(RestoreObjectiveId)) == 0,
				TEXT("completed=1 active=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(RestoreObjectiveId)),
					CountCompletedId(Objectives, FName(RestoreObjectiveId))),
				RestoreObjectiveId);
			AssertTrue(
				Record, TEXT("prediscovered.mission_complete"),
				Objectives->IsMissionComplete(FName(RestoreMissionId)),
				TEXT("true"),
				BoolText(Objectives->IsMissionComplete(FName(RestoreMissionId))),
				RestoreMissionId);
			AssertTrue(
				Record, TEXT("prediscovered.map_review_prompt"),
				Panel->GetInteractionPrompt().ToString() == ExpectedCompletedReviewPrompt,
				ExpectedCompletedReviewPrompt,
				Panel->GetInteractionPrompt().ToString(),
				MapPanelLabel);

			if (UndiscoveredClone)
			{
				AssertTrue(
					Record, TEXT("undiscovered.review_after_mission"),
					UndiscoveredClone->GetInteractionPrompt().ToString() == ExpectedCompletedReviewPrompt,
					ExpectedCompletedReviewPrompt,
					UndiscoveredClone->GetInteractionPrompt().ToString(),
					UndiscoveredCloneLabel);
			}

			const int32 MapSuccessBeforeRepeat = Panel->SuccessEventFireCount;
			const int32 MapNotifyBeforeRepeat = Panel->RestoreSuccessNotificationCount;
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bMapReview = Panel->Interact(Character);
			AssertTrue(Record, TEXT("repeat.map_review_interact"), bMapReview, TEXT("true"), BoolText(bMapReview), MapPanelLabel);
			AssertTrue(
				Record, TEXT("repeat.map_no_event_replay"),
				Panel->SuccessEventFireCount == MapSuccessBeforeRepeat,
				TEXT("0"),
				FString::FromInt(Panel->SuccessEventFireCount - MapSuccessBeforeRepeat),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("repeat.map_no_nathan_replay"),
				Panel->RestoreSuccessNotificationCount == MapNotifyBeforeRepeat,
				TEXT("0"),
				FString::FromInt(Panel->RestoreSuccessNotificationCount - MapNotifyBeforeRepeat),
				MapPanelLabel);

			if (UndiscoveredClone)
			{
				const int32 RepeatEvents = UndiscoveredClone->SuccessEventFireCount;
				const int32 RepeatNotify = UndiscoveredClone->RestoreSuccessNotificationCount;
				HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
				const bool bRepeat = UndiscoveredClone->Interact(Character);
				AssertTrue(Record, TEXT("repeat.clone_interact_ok"), bRepeat, TEXT("true"), BoolText(bRepeat), UndiscoveredCloneLabel);
				AssertTrue(
					Record, TEXT("repeat.clone_no_event_replay"),
					UndiscoveredClone->SuccessEventFireCount == RepeatEvents,
					TEXT("0"),
					FString::FromInt(UndiscoveredClone->SuccessEventFireCount - RepeatEvents),
					UndiscoveredCloneLabel);
				AssertTrue(
					Record, TEXT("repeat.clone_no_notify_replay"),
					UndiscoveredClone->RestoreSuccessNotificationCount == RepeatNotify,
					TEXT("0"),
					FString::FromInt(UndiscoveredClone->RestoreSuccessNotificationCount - RepeatNotify),
					UndiscoveredCloneLabel);
			}

			// ---- Save/load preserves Online/Blackout, completed restore mission, review behavior ----
			UProjectOrganoidSaveGame* SaveGame = NewObject<UProjectOrganoidSaveGame>(GetTransientPackage());
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.restore_completed_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(RestoreObjectiveId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(RestoreObjectiveId))),
				TEXT("save"));

			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.restore_objective_completed"),
				CountCompletedId(Objectives, FName(RestoreObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(RestoreObjectiveId))),
				RestoreObjectiveId);
			AProjectOrganoidPowerPanel* ReloadClone =
				SpawnBeat7PanelClone(World, TEXT("PowerPanel_NeuroBackup_ReloadClone"), ExpectedPanelLocation + FVector(0.0f, 150.0f, 0.0f), true);
			TrackPanel(ReloadClone);
			if (ReloadClone)
			{
				AssertTrue(
					Record, TEXT("saveload.reload_syncs_power"),
					Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
					TEXT("Online"),
					PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
					TEXT("Power"));
				AssertTrue(
					Record, TEXT("saveload.cryo_blackout"),
					Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
					TEXT("Blackout"),
					PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
					TEXT("Power"));
				AssertTrue(
					Record, TEXT("saveload.reload_review_prompt"),
					ReloadClone->GetInteractionPrompt().ToString() == ExpectedCompletedReviewPrompt,
					ExpectedCompletedReviewPrompt,
					ReloadClone->GetInteractionPrompt().ToString(),
					TEXT("reload"));
				const int32 ReloadEvents = ReloadClone->SuccessEventFireCount;
				const bool bReloadInteract = ReloadClone->Interact(Character);
				AssertTrue(Record, TEXT("saveload.reload_interact"), bReloadInteract, TEXT("true"), BoolText(bReloadInteract), TEXT("reload"));
				AssertTrue(
					Record, TEXT("saveload.reload_no_event_replay"),
					ReloadClone->SuccessEventFireCount == ReloadEvents,
					TEXT("0"),
					FString::FromInt(ReloadClone->SuccessEventFireCount - ReloadEvents),
					TEXT("reload"));
			}

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.disk_roundtrip"), bSaved && bLoaded, TEXT("true"), BoolText(bSaved && bLoaded), SaveSlot);
			AssertTrue(
				Record, TEXT("saveload.disk_neuro_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("saveload.disk_cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			Saves->DeleteSave(SaveSlot);

			// ---- Explicit Beat 7 exclusions / keep-list ----
			AssertTrue(
				Record, TEXT("exclude.station_untouched"),
				CountLabel(World, StationLabel) == StationCountBefore,
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, StationLabel)),
				StationLabel);
			AssertTrue(
				Record, TEXT("exclude.hosts_untouched"),
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
				Record, TEXT("exclude.gate_untouched"),
				CountLabel(World, GateLabel) == GateCountBefore,
				TEXT("unchanged"),
				FString::FromInt(CountLabel(World, GateLabel)),
				GateLabel);

			// Restore seed Emergency for durable package assert (PIE-only mutation).
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);

			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(
				Record, TEXT("dirty_unchanged"),
				DirtyAfter == DirtyBefore,
				FString::Join(DirtyBefore, TEXT(",")),
				FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"));
			Record.AddActor(TEXT("dirty_count"), FString::FromInt(DirtyAfter.Num()));
			Stage = EStage::Finalize;
		}
	};

	struct FNeuroRestoreLabPowerAutoRegister
	{
		FNeuroRestoreLabPowerAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroRestoreLabPowerFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroRestoreLabPowerAutoRegister GRegisterNeuroRestoreLabPower;
}
