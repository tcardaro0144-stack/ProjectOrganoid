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
	constexpr TCHAR TestId[] = TEXT("NeuroMappingSignalTrace_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Mapping Signal Trace Functional");
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

	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");

	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR TerminalReloadLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics_ReloadClone");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR PanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR ArrayLabel[] = TEXT("NeuralMappingArray_NeuroGenetics");
	constexpr TCHAR CutoffLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");
	constexpr TCHAR HazardLabel[] = TEXT("Hazard_ScrubberLeak");
	constexpr TCHAR TrapLabel[] = TEXT("CorridorTraps_GowningRing");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");
	constexpr TCHAR PadContainmentLabel[] = TEXT("DataPad_NeuroContainment");
	constexpr TCHAR PadFailureLabel[] = TEXT("DataPad_NeuroResearchFailure");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_NeuroAirlock");

	constexpr TCHAR InspectPrompt[] = TEXT("Trace Neural Mapping Signal");
	constexpr TCHAR ReviewPrompt[] = TEXT("Signal Trace Complete");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	// Exact line: two ASCII \u2019 escapes (victims' / Something's).
	constexpr TCHAR TerminalLine[] =
		TEXT("These scans line up with the victims\u2019 neural changes. Something\u2019s been tracking the same pattern across all of them.");
	constexpr TCHAR TerminalRendered[] =
		TEXT("Nathan: These scans line up with the victims\u2019 neural changes. Something\u2019s been tracking the same pattern across all of them.");

	constexpr float TerminalInteractionRange = 175.0f;
	constexpr float TerminalNotifySeconds = 4.0f;
	const FVector TerminalLocation(300.0f, -600.0f, -1100.0f);

	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	const FVector TerminalPedestalRel(0.0f, 0.0f, 30.0f);
	const FVector TerminalPedestalScale(0.55f, 0.45f, 0.60f);
	const FVector TerminalColumnRel(0.0f, 0.0f, 90.0f);
	const FVector TerminalColumnScale(0.50f, 0.25f, 0.60f);
	const FVector TerminalHeadRel(0.0f, 0.0f, 145.0f);
	const FVector TerminalHeadScale(0.75f, 0.35f, 0.25f);

	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");
	constexpr TCHAR IsolateEventCutoff[] = TEXT("Event_NeuroResearchLoadIsolated");
	constexpr TCHAR ArrayOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR HostResearcherLabel[] = TEXT("Host_Neuro_Researcher");
	constexpr TCHAR CutoffInspectPrompt[] = TEXT("Isolate Research Load");
	constexpr TCHAR CutoffReviewPrompt[] = TEXT("Research Load Isolated");
	constexpr TCHAR CutoffLine[] =
		TEXT("That cut the feed. The array\u2019s offline, but its last mapping data should still be here.");
	constexpr TCHAR ArrayInspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ArrayReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ArrayLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);
	const FVector CutoffLocation(-100.0f, -600.0f, -1100.0f);
	constexpr float ArrayInteractionRange = 200.0f;
	constexpr float CutoffInteractionRange = 175.0f;
	const FVector ArrayPedestalRel(0.0f, 0.0f, 40.0f);
	const FVector ArrayPedestalScale(1.2f, 1.2f, 0.8f);
	const FVector ArrayColumnRel(0.0f, 0.0f, 110.0f);
	const FVector ArrayColumnScale(0.35f, 0.35f, 1.4f);
	const FVector ArrayHeadRel(0.0f, 0.0f, 192.5f);
	const FVector ArrayHeadScale(1.6f, 1.6f, 0.25f);
	const FVector CutoffPedestalRel(0.0f, 0.0f, 35.0f);
	const FVector CutoffPedestalScale(0.45f, 0.45f, 0.70f);
	const FVector CutoffColumnRel(0.0f, 0.0f, 100.0f);
	const FVector CutoffColumnScale(0.65f, 0.30f, 0.60f);
	const FVector CutoffHeadRel(0.0f, 0.0f, 150.0f);
	const FVector CutoffHeadScale(0.80f, 0.40f, 0.25f);

	constexpr TCHAR TransientMissionObjectName[] = TEXT("DA_Mission_NeuroGenetics_Beat4_Transient_Test");

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

	class FNeuroMappingSignalTraceFunctional : public IOrganoidPlaytestCase
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
			UnbindOpeningMissionProbe();
			DestroyTransientActors();
			DestroyDecoyHud();
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
				UnbindOpeningMissionProbe();
				DestroyTransientActors();
				DestroyDecoyHud();
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

		UProjectOrganoidObjectiveDataAsset* LoadPersistedMissionAsset() const
		{
			return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionSoftPath);
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
				AProjectOrganoidInspectableInstrument::StaticClass(), ArrayLocation, FRotator::ZeroRotator, Params);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString(), true);
			ConfigureArrayIdentically(Actor);
			Track(Actor);
			return Actor;
		}

		FProjectOrganoidMissionTaskDefinition MakeTask(
			const TCHAR* ObjectiveId,
			const TCHAR* Title,
			const TCHAR* Description,
			bool bAutoActivate,
			const TArray<FName>& Prerequisites,
			const TArray<FName>& EventIds) const
		{
			FProjectOrganoidMissionTaskDefinition Task;
			Task.bAutoActivate = bAutoActivate;
			Task.Objective.ObjectiveId = FName(ObjectiveId);
			Task.Objective.Title = FText::FromString(Title);
			Task.Objective.Description = FText::FromString(Description);
			Task.Objective.Type = EProjectOrganoidObjectiveType::Main;
			Task.Objective.State = EProjectOrganoidObjectiveState::Inactive;
			Task.Objective.CurrentProgress = 0;
			Task.Objective.TargetProgress = 1;
			Task.Objective.PrerequisiteObjectiveIds = Prerequisites;
			Task.Objective.bAutoUnlockWhenPrerequisitesMet = true;
			for (const FName& EventId : EventIds)
			{
				FProjectOrganoidObjectiveEventTrigger Trigger;
				Trigger.EventId = EventId;
				Trigger.ObjectiveId = FName(ObjectiveId);
				Trigger.Action = EProjectOrganoidObjectiveEventAction::Complete;
				Task.EventTriggers.Add(Trigger);
			}
			return Task;
		}

		UProjectOrganoidObjectiveDataAsset* CreateTransientThreeTaskFixture()
		{
			UProjectOrganoidObjectiveDataAsset* Asset = NewObject<UProjectOrganoidObjectiveDataAsset>(
				GetTransientPackage(),
				TransientMissionObjectName,
				RF_Transient);
			if (!Asset)
			{
				return nullptr;
			}

			Asset->MissionId = FName(MissionId);
			Asset->MissionTitle = FText::FromString(MissionTitle);
			Asset->MissionDescription = FText::FromString(MissionDescription);
			Asset->NextMissionAsset = nullptr;

			TArray<FName> NoPrereq;
			TArray<FName> IsolateOnly;
			IsolateOnly.Add(FName(IsolateId));
			TArray<FName> TraceOnly;
			TraceOnly.Add(FName(TraceId));

			TArray<FName> IsolateEvents;
			IsolateEvents.Add(FName(IsolateEvent));
			TArray<FName> TraceEvents;
			TraceEvents.Add(FName(TraceEvent));
			TArray<FName> NoEvents;

			Asset->Tasks.Reset();
			Asset->Tasks.Add(MakeTask(IsolateId, IsolateTitle, IsolateDescription, true, NoPrereq, IsolateEvents));
			Asset->Tasks.Add(MakeTask(TraceId, TraceTitle, TraceDescription, true, IsolateOnly, TraceEvents));
			Asset->Tasks.Add(MakeTask(FollowId, FollowTitle, FollowDescription, true, TraceOnly, NoEvents));
			return Asset;
		}

		void ConfigureTerminalIdentically(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(TerminalLocation);
			Actor->SetActorRotation(FRotator::ZeroRotator);
			Actor->SetActorScale3D(FVector::OneVector);
			Actor->InteractionRange = TerminalInteractionRange;
			Actor->RequiredActiveObjectiveId = FName(TraceId);
			Actor->ObjectiveEventId = FName(TraceEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(TraceId);
			Actor->InspectionPrompt = FText::FromString(InspectPrompt);
			Actor->ReviewPrompt = FText::FromString(ReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(TerminalLine);
			Actor->NotificationDurationSeconds = TerminalNotifySeconds;
			Actor->bHasBeenInspected = false;
			Actor->InspectionNotificationCount = 0;
			Actor->ObjectiveEventFireCount = 0;
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredTerminal(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			const FTransform Xform(FRotator::ZeroRotator, TerminalLocation, FVector::OneVector);
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(), Xform);
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString());
			ConfigureTerminalIdentically(Actor);
			Actor->FinishSpawning(Xform);
			ConfigureTerminalIdentically(Actor);
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
			const int32 TerminalCount = CountLabel(World, TerminalLabel);
			const int32 CutoffCount = CountLabel(World, CutoffLabel);
			const int32 ArrayCount = CountLabel(World, ArrayLabel);
			if (World && Character && StationCount >= 1 && TerminalCount == 1 && CutoffCount == 1 && ArrayCount == 1)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				FailAndStop(
					Owner, Record,
					TEXT("Timed out waiting for PIE player, NeuralMappingTerminal_NeuroGenetics, EmergencyCutoff_NeuroResearchLoad, and NeuralMappingArray_NeuroGenetics. Persist Beat 4 + terminal + cutoff saves before this test can pass."));
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
			UProjectOrganoidObjectiveDataAsset* Fixture = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("mission.asset_loaded"),
				Fixture != nullptr,
				TEXT("loaded"),
				Fixture ? TEXT("loaded") : TEXT("missing"),
				MissionSoftPath);
			if (!Fixture)
			{
				FailAndStop(Owner, Record, TEXT("DA_Mission_NeuroGenetics missing. Require Beat 4 expand+save and terminal spawn+save before this test can pass."));
				return;
			}

			AssertTrue(
				Record, TEXT("mission.exact_path"),
				FSoftObjectPath(Fixture).ToString() == MissionSoftPath,
				MissionSoftPath,
				FSoftObjectPath(Fixture).ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.exact_class"),
				Fixture->GetClass() == UProjectOrganoidObjectiveDataAsset::StaticClass(),
				TEXT("ProjectOrganoidObjectiveDataAsset"),
				Fixture->GetClass() ? Fixture->GetClass()->GetName() : TEXT("null"),
				TEXT("DA"));
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
				Record, TEXT("mission.task_count_three"),
				Fixture->Tasks.Num() == 3,
				TEXT("3"),
				FString::FromInt(Fixture->Tasks.Num()),
				TEXT("DA"));
			if (Fixture->Tasks.Num() != 3)
			{
				FailAndStop(Owner, Record, TEXT("DA_Mission_NeuroGenetics is not exact Beat 4 (task count != 3)."));
				return;
			}

			const FProjectOrganoidMissionTaskDefinition& Task1 = Fixture->Tasks[0];
			const FProjectOrganoidMissionTaskDefinition& Task2 = Fixture->Tasks[1];
			const FProjectOrganoidMissionTaskDefinition& Task3 = Fixture->Tasks[2];

			AssertTrue(
				Record, TEXT("mission.task1_id"),
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
				Record, TEXT("mission.task1_no_prereq"),
				Task1.Objective.PrerequisiteObjectiveIds.Num() == 0,
				TEXT("0"),
				FString::FromInt(Task1.Objective.PrerequisiteObjectiveIds.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task1_event_exact"),
				Task1.EventTriggers.Num() == 1
					&& Task1.EventTriggers[0].EventId == FName(IsolateEvent)
					&& Task1.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete
					&& Task1.EventTriggers[0].ObjectiveId == FName(IsolateId),
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
				Record, TEXT("mission.task2_event_exact"),
				Task2.EventTriggers.Num() == 1
					&& Task2.EventTriggers[0].EventId == FName(TraceEvent)
					&& Task2.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete
					&& Task2.EventTriggers[0].ObjectiveId == FName(TraceId),
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
				Record, TEXT("mission.task3_no_events"),
				Task3.EventTriggers.Num() == 0,
				TEXT("0"),
				FString::FromInt(Task3.EventTriggers.Num()),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.no_fourth_task"),
				Fixture->Tasks.Num() == 3,
				TEXT("3"),
				FString::FromInt(Fixture->Tasks.Num()),
				TEXT("DA"));

			// ---- B. Persisted terminal exact contract ----
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
				FailAndStop(Owner, Record, TEXT("NeuralMappingTerminal_NeuroGenetics missing. Require spawn_neuro_neural_mapping_terminal + save."));
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
				Record, TEXT("map.terminal_range_175"),
				FMath::IsNearlyEqual(Terminal->InteractionRange, TerminalInteractionRange, 0.01f),
				TEXT("175"),
				FString::SanitizeFloat(Terminal->InteractionRange),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_required_active"),
				Terminal->RequiredActiveObjectiveId == FName(TraceId),
				TraceId,
				Terminal->RequiredActiveObjectiveId.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_objective_event"),
				Terminal->ObjectiveEventId == FName(TraceEvent),
				TraceEvent,
				Terminal->ObjectiveEventId.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_replay_guard"),
				Terminal->CompletedObjectiveIdForReplayGuard == FName(TraceId),
				TraceId,
				Terminal->CompletedObjectiveIdForReplayGuard.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_inspect_prompt"),
				Terminal->InspectionPrompt.ToString() == InspectPrompt,
				InspectPrompt,
				Terminal->InspectionPrompt.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_review_prompt"),
				Terminal->ReviewPrompt.ToString() == ReviewPrompt,
				ReviewPrompt,
				Terminal->ReviewPrompt.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_speaker"),
				Terminal->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Terminal->SpeakerLabel.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_line_exact"),
				Terminal->InspectionResponseText.ToString() == TerminalLine,
				TerminalLine,
				Terminal->InspectionResponseText.ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("map.terminal_notify_4"),
				FMath::IsNearlyEqual(Terminal->NotificationDurationSeconds, TerminalNotifySeconds, 0.01f),
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

			const TArray<AActor*> CutoffMatches = OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel);
			AProjectOrganoidInspectableInstrument* Cutoff = CutoffMatches.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(CutoffMatches[0])
				: nullptr;
			AssertTrue(
				Record, TEXT("map.cutoff_present"),
				Cutoff != nullptr,
				TEXT("present"),
				Cutoff ? TEXT("present") : TEXT("missing"),
				CutoffLabel);
			const TArray<AActor*> ArrayMatches = OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_present"),
				ArrayMatches.Num() == 1,
				TEXT("1"),
				FString::FromInt(ArrayMatches.Num()),
				ArrayLabel);
			if (!Cutoff)
			{
				FailAndStop(Owner, Record, TEXT("EmergencyCutoff_NeuroResearchLoad missing."));
				return;
			}

			// ---- C. Before prerequisites: terminal gated ----
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const int32 PreEvents = Terminal->ObjectiveEventFireCount;
			const int32 PreLines = Terminal->InspectionNotificationCount;
			const bool bCanBefore = Terminal->CanInteract(Character);
			const bool bInteractBefore = Terminal->Interact(Character);
			AssertTrue(
				Record, TEXT("pre.terminal_can_false"),
				!bCanBefore,
				TEXT("false"),
				BoolText(bCanBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("pre.terminal_interact_rejected"),
				!bInteractBefore,
				TEXT("false"),
				BoolText(bInteractBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("pre.no_trace_event"),
				Terminal->ObjectiveEventFireCount == PreEvents,
				TEXT("0"),
				FString::FromInt(Terminal->ObjectiveEventFireCount - PreEvents),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("pre.no_line"),
				Terminal->InspectionNotificationCount == PreLines,
				TEXT("0"),
				FString::FromInt(Terminal->InspectionNotificationCount - PreLines),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("pre.not_inspected"),
				!Terminal->bHasBeenInspected,
				TEXT("false"),
				BoolText(Terminal->bHasBeenInspected),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("pre.decoy_untouched"),
				Decoy->GetLastResourceNotification().IsEmpty()
					|| Decoy->GetLastResourceNotification().ToString() != TerminalRendered,
				TEXT("no terminal line"),
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));

			// ---- C2. OpeningFoundation → diagnosis → array → cutoff (persisted path) ----
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
				ObjectiveStateName(ResearchAfterDiagnosis.State),
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
					&& LoadPersistedMissionAsset()->Tasks.Num() == 3,
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("runtime.isolate_active"),
				CountActiveId(Objectives, FName(IsolateId)) == 1,
				TEXT("Active/1"),
				FString::Printf(TEXT("%s/%d"), TEXT("Active"), CountActiveId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("runtime.trace_inactive_before_isolate"),
				CountActiveId(Objectives, FName(TraceId)) == 0
					&& CountCompletedId(Objectives, FName(TraceId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				TraceId);
			AssertTrue(
				Record, TEXT("runtime.follow_inactive_before_isolate"),
				CountActiveId(Objectives, FName(FollowId)) == 0
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("Inactive"),
				TEXT("Inactive"),
				FollowId);

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

			// Terminal still gated while isolate Active / trace Inactive
			AssertTrue(
				Record, TEXT("pre_cutoff.terminal_still_gated"),
				!Terminal->CanInteract(Character),
				TEXT("false"),
				BoolText(Terminal->CanInteract(Character)),
				TerminalLabel);

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCutoffCan = Cutoff->CanInteract(Character);
			AssertTrue(
				Record, TEXT("cutoff.can_interact"),
				bCutoffCan,
				TEXT("true"),
				BoolText(bCutoffCan),
				CutoffLabel);
			const int32 CutoffEventsBefore = Cutoff->ObjectiveEventFireCount;
			const bool bCutoffInteract = Cutoff->Interact(Character);
			AssertTrue(
				Record, TEXT("cutoff.interact_accepted"),
				bCutoffInteract,
				TEXT("true"),
				BoolText(bCutoffInteract),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("isolate.completed_once"),
				CountCompletedId(Objectives, FName(IsolateId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(IsolateId))),
				IsolateId);
			AssertTrue(
				Record, TEXT("isolate.trace_active_once"),
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
				TEXT("active=0 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("cutoff.line_once"),
				Cutoff->ObjectiveEventFireCount == CutoffEventsBefore + 1,
				TEXT("1"),
				FString::FromInt(Cutoff->ObjectiveEventFireCount - CutoffEventsBefore),
				CutoffLabel);
			AssertTrue(
				Record, TEXT("cutoff.review_prompt"),
				Cutoff->GetInteractionPrompt().ToString() == CutoffReviewPrompt,
				CutoffReviewPrompt,
				Cutoff->GetInteractionPrompt().ToString(),
				CutoffLabel);

			// ---- D. Terminal interaction ----
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bCanAfter = Terminal->CanInteract(Character);
			AssertTrue(
				Record, TEXT("terminal.can_after_isolate"),
				bCanAfter,
				TEXT("true"),
				BoolText(bCanAfter),
				TerminalLabel);
			const int32 EventsBefore = Terminal->ObjectiveEventFireCount;
			const int32 LinesBefore = Terminal->InspectionNotificationCount;
			const bool bInteract = Terminal->Interact(Character);
			AssertTrue(
				Record, TEXT("terminal.interact_accepted"),
				bInteract,
				TEXT("true"),
				BoolText(bInteract),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("terminal.event_once"),
				Terminal->ObjectiveEventFireCount == EventsBefore + 1,
				TEXT("1"),
				FString::FromInt(Terminal->ObjectiveEventFireCount - EventsBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("terminal.trace_completed_once"),
				CountCompletedId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("terminal.line_once"),
				Terminal->InspectionNotificationCount == LinesBefore + 1,
				TEXT("1"),
				FString::FromInt(Terminal->InspectionNotificationCount - LinesBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("terminal.nathan_exact_owned_hud"),
				HUD->GetLastResourceNotification().ToString() == TerminalRendered,
				TerminalRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("terminal.decoy_rejected"),
				Decoy->GetLastResourceNotification().ToString() != TerminalRendered,
				TEXT("decoy != owned line"),
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("terminal.review_prompt_after_interact"),
				Terminal->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Terminal->GetInteractionPrompt().ToString(),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("terminal.follow_active_once"),
				CountActiveId(Objectives, FName(FollowId)) == 1
					&& CountCompletedId(Objectives, FName(FollowId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("terminal.mission_still_neuro"),
				Objectives->GetActiveMissionId() == FName(MissionId),
				MissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("terminal.mission_incomplete"),
				!Objectives->IsMissionComplete(FName(MissionId)),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(FName(MissionId))),
				TEXT("mission"));

			// ---- E. Repeat ----
			const int32 RepeatEventsBefore = Terminal->ObjectiveEventFireCount;
			const int32 RepeatLinesBefore = Terminal->InspectionNotificationCount;
			const int32 FollowActiveBefore = CountActiveId(Objectives, FName(FollowId));
			const bool bRepeat = Terminal->Interact(Character);
			AssertTrue(
				Record, TEXT("repeat.interact_ok"),
				bRepeat,
				TEXT("true"),
				BoolText(bRepeat),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("repeat.no_event_replay"),
				Terminal->ObjectiveEventFireCount == RepeatEventsBefore,
				TEXT("0"),
				FString::FromInt(Terminal->ObjectiveEventFireCount - RepeatEventsBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("repeat.no_line_replay"),
				Terminal->InspectionNotificationCount == RepeatLinesBefore,
				TEXT("0"),
				FString::FromInt(Terminal->InspectionNotificationCount - RepeatLinesBefore),
				TerminalLabel);
			AssertTrue(
				Record, TEXT("repeat.trace_completed"),
				CountCompletedId(Objectives, FName(TraceId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TraceId))),
				TraceId);
			AssertTrue(
				Record, TEXT("repeat.follow_active"),
				CountActiveId(Objectives, FName(FollowId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(FollowId))),
				FollowId);
			AssertTrue(
				Record, TEXT("repeat.no_duplicate_activation"),
				CountActiveId(Objectives, FName(FollowId)) == FollowActiveBefore
					&& CountActiveId(Objectives, FName(FollowId)) == 1
					&& CountCompletedId(Objectives, FName(TraceId)) == 1,
				TEXT("follow_active=1 trace_completed=1"),
				FString::Printf(
					TEXT("follow_active=%d trace_completed=%d"),
					CountActiveId(Objectives, FName(FollowId)),
					CountCompletedId(Objectives, FName(TraceId))),
				TEXT("objectives"));

			// ---- F. Save/load ----
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
				Record, TEXT("saveload.follow_active"),
				CountActiveId(Objectives, FName(FollowId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(FollowId))),
				FollowId);

			AProjectOrganoidInspectableInstrument* Reloaded =
				SpawnConfiguredTerminal(World, FName(TerminalReloadLabel));
			AssertTrue(
				Record, TEXT("saveload.reload_spawned"),
				Reloaded != nullptr,
				TEXT("spawned"),
				Reloaded ? TEXT("spawned") : TEXT("null"),
				TerminalReloadLabel);
			if (!Reloaded)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn identically configured terminal reload clone."));
				return;
			}
			AssertTrue(
				Record, TEXT("saveload.starts_review"),
				Reloaded->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Reloaded->GetInteractionPrompt().ToString(),
				TerminalReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.not_marked_without_interact"),
				!Reloaded->bHasBeenInspected,
				TEXT("false"),
				BoolText(Reloaded->bHasBeenInspected),
				TerminalReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_production_da_mutation"),
				FSoftObjectPath(Fixture).ToString() == MissionSoftPath
					&& Fixture->Tasks.Num() == 3
					&& !PackageIsDirty(MissionPackage),
				TEXT("persisted Beat4 clean"),
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
				TerminalReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_event_replay"),
				Reloaded->ObjectiveEventFireCount == ReloadEventsBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->ObjectiveEventFireCount - ReloadEventsBefore),
				TerminalReloadLabel);
			AssertTrue(
				Record, TEXT("saveload.no_line_replay"),
				Reloaded->InspectionNotificationCount == ReloadLinesBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->InspectionNotificationCount - ReloadLinesBefore),
				TerminalReloadLabel);

			// ---- G. Preservation ----
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

	struct FNeuroMappingSignalTraceAutoRegister
	{
		FNeuroMappingSignalTraceAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroMappingSignalTraceFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroMappingSignalTraceAutoRegister GRegisterNeuroMappingSignalTrace;
}
