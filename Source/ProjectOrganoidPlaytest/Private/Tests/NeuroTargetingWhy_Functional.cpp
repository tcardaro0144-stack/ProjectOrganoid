#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/StrongObjectPtr.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"

#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AISense_Hearing.h"
#include "ProjectOrganoidPerceptionTypes.h"

namespace NeuroTargetingWhyFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroTargetingWhy_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Targeting Why Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR NeuroGeneticsMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	constexpr TCHAR NeuroGeneticsMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR RestoreMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore");
	constexpr TCHAR RestoreMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroPowerRestore.DA_Mission_NeuroPowerRestore");
	constexpr TCHAR TargetingMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroTargetingWhy");
	constexpr TCHAR TargetingMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroTargetingWhy.DA_Mission_NeuroTargetingWhy");

	constexpr TCHAR NeuroGeneticsMissionId[] = TEXT("Mission_NeuroGenetics");
	constexpr TCHAR RestoreMissionId[] = TEXT("Mission_NeuroPowerRestore");
	constexpr TCHAR TargetingMissionId[] = TEXT("Mission_NeuroTargetingWhy");
	constexpr TCHAR TargetingMissionTitle[] = TEXT("Target the Nervous System");
	constexpr TCHAR TargetingObjectiveId[] = TEXT("Obj_ImpairHostLocomotorNerves");
	constexpr TCHAR TargetingObjectiveTitle[] = TEXT("Impair the Host’s locomotor nerves");
	constexpr TCHAR TargetingEvent[] = TEXT("Event_NeuroLocomotorTargetDemonstrated");
	constexpr TCHAR RestoreObjectiveId[] = TEXT("Obj_RestoreNeuroLabPower");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");

	constexpr TCHAR IsolateId[] = TEXT("Obj_IsolateNeuroResearchLoad");
	constexpr TCHAR TraceId[] = TEXT("Obj_TraceNeuralMappingSignal");
	constexpr TCHAR FollowId[] = TEXT("Obj_FollowNeuralSignature");
	constexpr TCHAR ExamineId[] = TEXT("Obj_ExamineNeuralChangeEvidence");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");

	constexpr TCHAR MapPanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR CutoffLabel[] = TEXT("EmergencyCutoff_NeuroResearchLoad");
	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR NodeLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	constexpr TCHAR InstrumentLabel[] = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	constexpr TCHAR ArrayOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");
	constexpr TCHAR ResearcherLabel[] = TEXT("Host_Neuro_Researcher");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroTargetingWhyTest");

	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	constexpr TCHAR ExpectedLine[] =
		TEXT("That matches the evidence. Target the locomotor nerves, and the whole body slows with them.");
	constexpr float ExpectedDuration = 7.0f;

	constexpr TCHAR ArrayInspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ArrayReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ArrayLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	constexpr float ArrayInteractionRange = 200.0f;
	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);
	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
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

	FString BoolText(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }

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

	class FNeuroTargetingWhyFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			ProofPhase = EProofPhase::PreHandoff;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			bViabilityMoveRequested = false;
			ViabilityStartLocation = FVector::ZeroVector;
			ViabilityStandProjected = FVector::ZeroVector;
			DirtyBefore.Reset();
			TransientInstruments.Reset();
			OwnedHudWidget.Reset();
			ViabilityResearcher.Reset();
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
				TickProof(Owner, *Record, DeltaTime);
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

		enum class EProofPhase : uint8
		{
			PreHandoff,
			EncounterViabilitySetup,
			EncounterViabilityWait,
			LessonExercise
		};

		EStage Stage = EStage::Preflight;
		EProofPhase ProofPhase = EProofPhase::PreHandoff;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bViabilityMoveRequested = false;
		FVector ViabilityStartLocation = FVector::ZeroVector;
		FVector ViabilityStandProjected = FVector::ZeroVector;
		FVector ViabilityInvestigateCandidate = FVector::ZeroVector;
		FVector ViabilityInvestigateProjected = FVector::ZeroVector;
		float ViabilityInvestigatePathLength = 0.0f;
		float ViabilityInitialGap = 0.0f;
		int32 ViabilityInvestigateCandidateIndex = -1;
		TArray<FString> DirtyBefore;
		TArray<TWeakObjectPtr<AProjectOrganoidInspectableInstrument>> TransientInstruments;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> OwnedHudWidget;
		TWeakObjectPtr<AProjectOrganoidHostBase> ViabilityResearcher;
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

		void DestroyTransientActors()
		{
			for (const TWeakObjectPtr<AProjectOrganoidInspectableInstrument>& Weak : TransientInstruments)
			{
				if (AProjectOrganoidInspectableInstrument* Actor = Weak.Get())
				{
					Actor->Destroy();
				}
			}
			TransientInstruments.Reset();
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

		void TrackInstrument(AProjectOrganoidInspectableInstrument* Actor)
		{
			if (Actor)
			{
				TransientInstruments.Add(Actor);
			}
		}

		UProjectOrganoidHUDWidget* ResolveOwnedHud(APlayerController* PC)
		{
			if (UProjectOrganoidHUDWidget* Existing = OwnedHudWidget.Get())
			{
				return Existing;
			}
			if (!PC || !PC->GetWorld())
			{
				return nullptr;
			}
			if (AProjectOrganoidGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AProjectOrganoidGameMode>())
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
			TrackInstrument(Actor);
			return Actor;
		}

		void ApproachHost(AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* Host) const
		{
			if (!Character || !Host)
			{
				return;
			}
			const FVector Stand = Host->GetActorLocation() + Host->GetActorForwardVector() * 140.0f;
			OrganoidPlaytestActions::TeleportNear(Character, Stand, 0.0f, Stand.Z);
		}

		void FireTacticalWeakPoint(
			AProjectOrganoidCharacter* Character,
			AProjectOrganoidHostBase* Host,
			EProjectOrganoidWeakPointType WeakPoint,
			bool bTactical,
			float Damage)
		{
			if (!Character || !Host)
			{
				return;
			}
			if (bTactical && !Character->IsTacticalModeActive())
			{
				Character->ToggleTacticalMode();
			}
			if (!bTactical && Character->IsTacticalModeActive())
			{
				Character->ToggleTacticalMode();
			}

			USphereComponent* Hitbox = nullptr;
			switch (WeakPoint)
			{
			case EProjectOrganoidWeakPointType::LocomotorNerves:
				Hitbox = Host->LocomotorNervesHitbox;
				break;
			case EProjectOrganoidWeakPointType::OpticalNodes:
				Hitbox = Host->OpticalNodesHitbox;
				break;
			case EProjectOrganoidWeakPointType::OrganoidCore:
				Hitbox = Host->BioCoreHitbox;
				break;
			default:
				break;
			}

			UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent();
			AProjectOrganoidWeapon* Weapon = WeaponComp ? WeaponComp->GetEquippedWeapon() : nullptr;
			if (!Weapon || !Hitbox)
			{
				FProjectOrganoidBallisticHit Direct;
				Direct.WeakPoint = WeakPoint;
				Direct.bTacticalModeHit = bTactical;
				Direct.FinalDamage = Damage;
				Direct.bTriggeredDismemberment =
					bTactical && WeakPoint == EProjectOrganoidWeakPointType::LocomotorNerves && Damage >= 20.0f;
				Host->ApplyResolvedOrganoidHit(Direct, Character);
				return;
			}

			FHitResult Hit;
			Hit.HitObjectHandle = FActorInstanceHandle(Host);
			Hit.Component = Hitbox;
			Hit.Location = Hitbox->GetComponentLocation();
			Hit.ImpactPoint = Hit.Location;
			Hit.BoneName = Hitbox->ComponentTags.Num() > 0 ? Hitbox->ComponentTags[0] : NAME_None;
			Hit.bBlockingHit = true;
			Hit.Distance = 100.0f;

			const FProjectOrganoidBallisticHit Resolved = Weapon->ProcessBallisticHit(Hit, Damage, bTactical);
			Host->ApplyResolvedOrganoidHit(Resolved, Character);
		}

		void AssertPersistedMissionContracts(FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* RestoreDA =
				LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RestoreMissionSoftPath);
			UProjectOrganoidObjectiveDataAsset* TargetingDA =
				LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, TargetingMissionSoftPath);

			AssertTrue(Record, TEXT("asset.restore_present"), RestoreDA != nullptr, TEXT("present"), RestoreDA ? TEXT("present") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.targeting_present"), TargetingDA != nullptr, TEXT("present"), TargetingDA ? TEXT("present") : TEXT("missing"), TEXT("DA"));
			if (RestoreDA)
			{
				AssertTrue(
					Record, TEXT("asset.restore_next_targeting_why"),
					RestoreDA->NextMissionAsset.ToSoftObjectPath().ToString() == TargetingMissionSoftPath,
					TargetingMissionSoftPath,
					RestoreDA->NextMissionAsset.IsNull()
						? TEXT("null")
						: RestoreDA->NextMissionAsset.ToSoftObjectPath().ToString(),
					TEXT("DA"));
			}
			if (!TargetingDA)
			{
				return;
			}
			AssertTrue(Record, TEXT("asset.targeting_mission_id"), TargetingDA->MissionId == FName(TargetingMissionId), TargetingMissionId, TargetingDA->MissionId.ToString(), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.targeting_title"), TargetingDA->MissionTitle.ToString() == TargetingMissionTitle, TargetingMissionTitle, TargetingDA->MissionTitle.ToString(), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.targeting_next_null"), TargetingDA->NextMissionAsset.IsNull(), TEXT("null"), TargetingDA->NextMissionAsset.IsNull() ? TEXT("null") : TargetingDA->NextMissionAsset.ToString(), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.targeting_task_count"), TargetingDA->Tasks.Num() == 1, TEXT("1"), FString::FromInt(TargetingDA->Tasks.Num()), TEXT("DA"));
			if (TargetingDA->Tasks.Num() == 1)
			{
				const FProjectOrganoidMissionTaskDefinition& Task = TargetingDA->Tasks[0];
				AssertTrue(Record, TEXT("asset.targeting_objective_id"), Task.Objective.ObjectiveId == FName(TargetingObjectiveId), TargetingObjectiveId, Task.Objective.ObjectiveId.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.targeting_objective_title"), Task.Objective.Title.ToString() == TargetingObjectiveTitle, TargetingObjectiveTitle, Task.Objective.Title.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.targeting_auto_activate"), Task.bAutoActivate, TEXT("true"), BoolText(Task.bAutoActivate), TEXT("DA"));
				AssertTrue(
					Record, TEXT("asset.targeting_complete_event"),
					Task.EventTriggers.Num() == 1
						&& Task.EventTriggers[0].EventId == FName(TargetingEvent)
						&& Task.EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete,
					TEXT("Event_NeuroLocomotorTargetDemonstrated/Complete"),
					Task.EventTriggers.Num() == 1 ? Task.EventTriggers[0].EventId.ToString() : FString::FromInt(Task.EventTriggers.Num()),
					TEXT("DA"));
			}
		}

		bool BootstrapThroughTargetingMissionHandoff(
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			UProjectOrganoidObjectiveSubsystem* Objectives,
			AProjectOrganoidPowerPanel* Panel,
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
			UnbindOpeningMissionProbe();
			if (OrderedArray)
			{
				OrderedArray->Destroy();
			}

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			AssertTrue(Record, TEXT("handoff.cutoff"), Cutoff && Cutoff->Interact(Character), TEXT("true"), TEXT("checked"), CutoffLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			AssertTrue(Record, TEXT("handoff.terminal"), Terminal && Terminal->Interact(Character), TEXT("true"), TEXT("checked"), TerminalLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			AssertTrue(Record, TEXT("handoff.node"), Node && Node->Interact(Character), TEXT("true"), TEXT("checked"), NodeLabel);
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			AssertTrue(Record, TEXT("handoff.instrument"), Instrument && Instrument->Interact(Character), TEXT("true"), TEXT("checked"), InstrumentLabel);

			AssertTrue(
				Record, TEXT("handoff.restore_current"),
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

			if (Panel && !Panel->bHasDiscoveredPowerFailure)
			{
				Panel->Interact(Character);
			}
			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bRestore = Panel && Panel->Interact(Character);
			AssertTrue(Record, TEXT("handoff.panel_restore"), bRestore, TEXT("true"), BoolText(bRestore), MapPanelLabel);

			AssertTrue(
				Record, TEXT("handoff.targeting_current"),
				Objectives->GetActiveMissionId() == FName(TargetingMissionId),
				TargetingMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("handoff.targeting_objective_active"),
				CountActiveId(Objectives, FName(TargetingObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(TargetingObjectiveId))),
				TargetingObjectiveId);

			return !bAnyAssertFailed;
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
				|| PackageIsDirty(NeuroGeneticsMissionPackage) || PackageIsDirty(RestoreMissionPackage)
				|| PackageIsDirty(TargetingMissionPackage))
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
			if (LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, TargetingMissionSoftPath) == nullptr
				|| LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RestoreMissionSoftPath) == nullptr)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Beat 8 mission assets missing. Run bridge create/configure/save before this test."));
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

			const bool bActorsReady = CountLabel(World, MapPanelLabel) == 1
				&& CountLabel(World, CutoffLabel) == 1
				&& CountLabel(World, TerminalLabel) == 1
				&& CountLabel(World, NodeLabel) == 1
				&& CountLabel(World, InstrumentLabel) == 1
				&& CountLabel(World, ResearcherLabel) == 1;

			if (Character && bPlayerReady && bActorsReady)
			{
				Stage = EStage::Proof;
				ProofPhase = EProofPhase::PreHandoff;
				WaitSeconds = 0.0f;
				bViabilityMoveRequested = false;
				Owner.SetStage(TEXT("Proof"));
				return;
			}

			if (WaitSeconds > 90.0f)
			{
				FailAndStop(Owner, Record, TEXT("Timed out waiting for PIE player and Neuro Beat 8 actors."));
			}
		}

		bool TryFindLocalEncounterStand(
			UWorld* World,
			AProjectOrganoidHostBase* Researcher,
			FVector& OutStandWorld,
			FVector& OutStandProjected,
			FString& OutDetail) const
		{
			OutStandWorld = FVector::ZeroVector;
			OutStandProjected = FVector::ZeroVector;
			OutDetail.Reset();
			if (!World || !Researcher)
			{
				OutDetail = TEXT("missing world/researcher");
				return false;
			}

			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
			if (!NavSys || !NavData)
			{
				OutDetail = TEXT("nav system/data missing");
				return false;
			}

			FNavLocation HostProjected;
			if (!NavSys->ProjectPointToNavigation(
					Researcher->GetActorLocation(), HostProjected, FVector(300.0f, 300.0f, 500.0f)))
			{
				OutDetail = TEXT("researcher not projectable");
				return false;
			}

			const float Range = FMath::Max(50.0f, Researcher->ProximityActivationRange);
			const FVector Forward = Researcher->GetActorForwardVector().GetSafeNormal2D();
			const FVector Right = Researcher->GetActorRightVector().GetSafeNormal2D();
			const float Distances[] = {160.0f, 180.0f, 140.0f, 120.0f};
			const FVector Dirs[] = {
				Forward,
				-Forward,
				Right,
				-Right,
				(Forward + Right).GetSafeNormal2D(),
				(Forward - Right).GetSafeNormal2D(),
				(-Forward + Right).GetSafeNormal2D(),
				(-Forward - Right).GetSafeNormal2D(),
			};

			for (const float Dist : Distances)
			{
				if (Dist > Range)
				{
					continue;
				}
				for (const FVector& Dir : Dirs)
				{
					if (Dir.IsNearlyZero())
					{
						continue;
					}
					const FVector Candidate = Researcher->GetActorLocation() + Dir * Dist;
					FNavLocation CandidateProjected;
					if (!NavSys->ProjectPointToNavigation(Candidate, CandidateProjected, FVector(200.0f, 200.0f, 300.0f)))
					{
						continue;
					}
					const float PlanarFromHost = FVector::Dist2D(CandidateProjected.Location, HostProjected.Location);
					if (PlanarFromHost > Range + 25.0f || PlanarFromHost < 115.0f)
					{
						continue;
					}

					const FPathFindingQuery Query(
						NavSys, *NavData, HostProjected.Location, CandidateProjected.Location);
					const FPathFindingResult PathResult = NavSys->FindPathSync(Query);
					if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid() || PathResult.IsPartial())
					{
						continue;
					}

					OutStandWorld = Candidate;
					OutStandProjected = CandidateProjected.Location;
					OutDetail = FString::Printf(
						TEXT("stand=(%.1f,%.1f,%.1f) projected=(%.1f,%.1f,%.1f) planar=%.1f path_len=%.1f"),
						Candidate.X,
						Candidate.Y,
						Candidate.Z,
						CandidateProjected.Location.X,
						CandidateProjected.Location.Y,
						CandidateProjected.Location.Z,
						PlanarFromHost,
						PathResult.Path->GetLength());
					return true;
				}
			}

			OutDetail = TEXT("no local activation-range stand with successful Researcher path");
			return false;
		}

		/** V1D: post-activation Investigate destination with path_len >= 180 (test geometry only). */
		bool TryFindLocalInvestigateDestination(
			UWorld* World,
			AProjectOrganoidHostBase* Researcher,
			FVector& OutCandidateWorld,
			FVector& OutCandidateProjected,
			float& OutPathLength,
			int32& OutCandidateIndex,
			FString& OutDetail) const
		{
			OutCandidateWorld = FVector::ZeroVector;
			OutCandidateProjected = FVector::ZeroVector;
			OutPathLength = 0.0f;
			OutCandidateIndex = -1;
			OutDetail.Reset();
			if (!World || !Researcher)
			{
				OutDetail = TEXT("missing world/researcher");
				return false;
			}

			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
			if (!NavSys || !NavData)
			{
				OutDetail = TEXT("nav system/data missing");
				return false;
			}

			FNavLocation HostProjected;
			if (!NavSys->ProjectPointToNavigation(
					Researcher->GetActorLocation(), HostProjected, FVector(300.0f, 300.0f, 500.0f)))
			{
				OutDetail = TEXT("researcher not projectable");
				return false;
			}

			// Fixed ordered offsets: enough planar distance for 80 accept + 40 slack + 35 move + margin.
			constexpr float MinPathLength = 180.0f;
			constexpr float MaxCombatPlanar = 450.0f;
			const float Distances[] = {220.0f, 250.0f, 280.0f, 320.0f, 360.0f, 400.0f};
			const FVector Forward = Researcher->GetActorForwardVector().GetSafeNormal2D();
			const FVector Right = Researcher->GetActorRightVector().GetSafeNormal2D();
			const FVector Dirs[] = {
				Forward,
				-Forward,
				Right,
				-Right,
				(Forward + Right).GetSafeNormal2D(),
				(Forward - Right).GetSafeNormal2D(),
				(-Forward + Right).GetSafeNormal2D(),
				(-Forward - Right).GetSafeNormal2D(),
			};

			int32 CandidateIndex = 0;
			for (const float Dist : Distances)
			{
				for (const FVector& Dir : Dirs)
				{
					++CandidateIndex;
					if (Dir.IsNearlyZero())
					{
						continue;
					}
					const FVector Candidate = Researcher->GetActorLocation() + Dir * Dist;
					FNavLocation CandidateProjected;
					if (!NavSys->ProjectPointToNavigation(Candidate, CandidateProjected, FVector(200.0f, 200.0f, 300.0f)))
					{
						continue;
					}
					const float PlanarFromHost = FVector::Dist2D(CandidateProjected.Location, HostProjected.Location);
					if (PlanarFromHost < MinPathLength || PlanarFromHost > MaxCombatPlanar)
					{
						continue;
					}

					const FPathFindingQuery Query(
						NavSys, *NavData, HostProjected.Location, CandidateProjected.Location);
					const FPathFindingResult PathResult = NavSys->FindPathSync(Query);
					if (!PathResult.IsSuccessful() || !PathResult.Path.IsValid() || PathResult.IsPartial())
					{
						continue;
					}

					const float PathLen = PathResult.Path->GetLength();
					if (PathLen < MinPathLength)
					{
						continue;
					}

					OutCandidateWorld = Candidate;
					OutCandidateProjected = CandidateProjected.Location;
					OutPathLength = PathLen;
					OutCandidateIndex = CandidateIndex;
					OutDetail = FString::Printf(
						TEXT("idx=%d candidate=(%.1f,%.1f,%.1f) projected=(%.1f,%.1f,%.1f) planar=%.1f path_len=%.1f"),
						CandidateIndex,
						Candidate.X,
						Candidate.Y,
						Candidate.Z,
						CandidateProjected.Location.X,
						CandidateProjected.Location.Y,
						CandidateProjected.Location.Z,
						PlanarFromHost,
						PathLen);
					return true;
				}
			}

			OutDetail = TEXT("no local combat-space Investigate dest with path_len>=180 on existing navigation");
			return false;
		}

		static FString MovementModeName(EMovementMode Mode)
		{
			switch (Mode)
			{
			case MOVE_None: return TEXT("None");
			case MOVE_Walking: return TEXT("Walking");
			case MOVE_NavWalking: return TEXT("NavWalking");
			case MOVE_Falling: return TEXT("Falling");
			case MOVE_Swimming: return TEXT("Swimming");
			case MOVE_Flying: return TEXT("Flying");
			case MOVE_Custom: return TEXT("Custom");
			default: return TEXT("Unknown");
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			switch (ProofPhase)
			{
			case EProofPhase::PreHandoff:
				TickProofPreHandoff(Owner, Record);
				break;
			case EProofPhase::EncounterViabilitySetup:
				TickEncounterViabilitySetup(Owner, Record);
				break;
			case EProofPhase::EncounterViabilityWait:
				TickEncounterViabilityWait(Owner, Record, DeltaTime);
				break;
			case EProofPhase::LessonExercise:
				TickProofLessonExercise(Owner, Record);
				break;
			default:
				break;
			}
		}

		void TickProofPreHandoff(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			UProjectOrganoidHUDWidget* HUD = ResolveOwnedHud(PC);

			if (!World || !Character || !Power || !Objectives || !Saves || !HUD)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE world/character/power/objectives/saves/HUD."));
				return;
			}

			AssertTrue(Record, TEXT("seed.pe_available"), Character->GetPEEnergy() > 0.0f, TEXT(">0"), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
			UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent();
			AProjectOrganoidWeapon* Weapon = WeaponComp ? WeaponComp->GetEquippedWeapon() : nullptr;
			AssertTrue(Record, TEXT("seed.weapon_equipped"), Weapon != nullptr, TEXT("present"), Weapon ? TEXT("present") : TEXT("missing"), TEXT("Weapon"));
			AssertTrue(Record, TEXT("seed.magazine"), Weapon && Weapon->GetCurrentMagazine() > 0, TEXT(">0"), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("0"), TEXT("Weapon"));

			AProjectOrganoidHostBase* Researcher = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindActorsByLabel(World, ResearcherLabel)[0]);
			AProjectOrganoidHostBase* Host1 = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindActorsByLabel(World, Host1Label)[0]);
			AProjectOrganoidPowerPanel* Panel = Cast<AProjectOrganoidPowerPanel>(
				OrganoidPlaytestActions::FindActorsByLabel(World, MapPanelLabel)[0]);
			AProjectOrganoidInspectableInstrument* Cutoff = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, CutoffLabel)[0]);
			AProjectOrganoidInspectableInstrument* Terminal = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel)[0]);
			AProjectOrganoidInspectableInstrument* Node = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel)[0]);
			AProjectOrganoidInspectableInstrument* Instrument = Cast<AProjectOrganoidInspectableInstrument>(
				OrganoidPlaytestActions::FindActorsByLabel(World, InstrumentLabel)[0]);

			if (!Researcher || !Panel || !Cutoff || !Terminal || !Node || !Instrument)
			{
				FailAndStop(Owner, Record, TEXT("Missing Researcher or Beat 7 handoff actors."));
				return;
			}

			AssertTrue(Record, TEXT("map.researcher_requires_activation"), Researcher->bRequiresEncounterActivation, TEXT("true"), BoolText(Researcher->bRequiresEncounterActivation), ResearcherLabel);
			AssertTrue(Record, TEXT("map.researcher_lesson_objective"), Researcher->RequiredActiveObjectiveId == FName(TargetingObjectiveId), TargetingObjectiveId, Researcher->RequiredActiveObjectiveId.ToString(), ResearcherLabel);
			AssertTrue(Record, TEXT("map.researcher_lesson_wp"), Researcher->RequiredLessonWeakPoint == EProjectOrganoidWeakPointType::LocomotorNerves, TEXT("LocomotorNerves"), TEXT("checked"), ResearcherLabel);
			AssertTrue(Record, TEXT("map.researcher_lesson_event"), Researcher->LessonSuccessObjectiveEventId == FName(TargetingEvent), TargetingEvent, Researcher->LessonSuccessObjectiveEventId.ToString(), ResearcherLabel);
			AssertTrue(Record, TEXT("map.host1_no_lesson"), Host1 && Host1->RequiredActiveObjectiveId.IsNone(), TEXT("None"), Host1 ? Host1->RequiredActiveObjectiveId.ToString() : TEXT("missing"), Host1Label);

			ApproachHost(Character, Researcher);
			AssertTrue(Record, TEXT("pre.researcher_dormant"), !Researcher->IsEncounterActivated(), TEXT("dormant"), BoolText(Researcher->IsEncounterActivated()), ResearcherLabel);
			Researcher->TryActivateEncounterFromProximity(Character);
			AssertTrue(Record, TEXT("pre.proximity_rejected"), !Researcher->IsEncounterActivated(), TEXT("dormant"), BoolText(Researcher->IsEncounterActivated()), ResearcherLabel);
			// Attack before Active must not award credit and must not permanently destroy the
			// LocomotorNerves lesson target (tactical locomotor would DestroyWeakPoint).
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::OpticalNodes, true, 4.0f);
			AssertTrue(Record, TEXT("pre.tactical_hit_no_credit"), Researcher->LessonSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			AssertTrue(Record, TEXT("pre.no_activation"), !Researcher->IsEncounterActivated(), TEXT("dormant"), BoolText(Researcher->IsEncounterActivated()), ResearcherLabel);
			AssertTrue(
				Record, TEXT("pre.no_targeting_mission"),
				Objectives->GetActiveMissionId() != FName(TargetingMissionId),
				TEXT("not TargetingWhy"),
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));

			if (!BootstrapThroughTargetingMissionHandoff(
					Record, World, Character, Objectives, Panel, Cutoff, Terminal, Node, Instrument, HUD))
			{
				FailAndStop(Owner, Record, TEXT("Failed handoff through PowerRestore into Mission_NeuroTargetingWhy."));
				return;
			}

			AssertTrue(
				Record, TEXT("power.neuro_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("power.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));

			// V1B: prior FindPathSync destination was ApproachHost forward*140 (encounter stand),
			// not reception/power panel. Local encounter viability runs next with proper path query.
			{
				const FVector FailedStand = Researcher->GetActorLocation() + Researcher->GetActorForwardVector() * 140.0f;
				Record.AddActor(
					TEXT("v1b_failed_destination_audit"),
					FString::Printf(
						TEXT("stand=(%.1f,%.1f,%.1f) planar=140 activation_range=%.1f class=encounter_stand not_reception not_power_panel"),
						FailedStand.X,
						FailedStand.Y,
						FailedStand.Z,
						Researcher->ProximityActivationRange));
			}

			ViabilityResearcher = Researcher;
			ProofPhase = EProofPhase::EncounterViabilitySetup;
			Owner.SetStage(TEXT("EncounterViabilitySetup"));
		}

		void TickEncounterViabilitySetup(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			AProjectOrganoidHostBase* Researcher = ViabilityResearcher.Get();
			if (!Researcher && World)
			{
				const TArray<AActor*> Matches = OrganoidPlaytestActions::FindActorsByLabel(World, ResearcherLabel);
				Researcher = Matches.Num() == 1 ? Cast<AProjectOrganoidHostBase>(Matches[0]) : nullptr;
				ViabilityResearcher = Researcher;
			}
			if (!World || !Character || !Researcher)
			{
				FailAndStop(Owner, Record, TEXT("Beat 8 V1B blocker: missing PIE world/player/Host_Neuro_Researcher."));
				return;
			}

			AProjectOrganoidHostAIController* HostAI = Cast<AProjectOrganoidHostAIController>(Researcher->GetController());
			if (!AssertTrue(
					Record, TEXT("viability.researcher_ai_controller"),
					HostAI != nullptr,
					TEXT("AProjectOrganoidHostAIController"),
					Researcher->GetController() ? Researcher->GetController()->GetClass()->GetName() : TEXT("none"),
					ResearcherLabel))
			{
				FailAndStop(Owner, Record, TEXT("Beat 8 V1B blocker: Host_Neuro_Researcher has no valid Host AI controller."));
				return;
			}

			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
			FNavLocation HostProjected;
			const bool bHostOnNav = NavSys && NavSys->ProjectPointToNavigation(
				Researcher->GetActorLocation(), HostProjected, FVector(300.0f, 300.0f, 500.0f));
			if (!AssertTrue(
					Record, TEXT("viability.researcher_nav_project"),
					bHostOnNav,
					TEXT("projectable"),
					bHostOnNav ? TEXT("projectable") : TEXT("no nav"),
					ResearcherLabel))
			{
				FailAndStop(
					Owner,
					Record,
					TEXT("Beat 8 V1B blocker: Host_Neuro_Researcher location does not project onto existing navigation."));
				return;
			}

			FVector StandWorld;
			FVector StandProjected;
			FString StandDetail;
			const bool bLocalPath = TryFindLocalEncounterStand(World, Researcher, StandWorld, StandProjected, StandDetail);
			AssertTrue(
				Record, TEXT("viability.local_encounter_path"),
				bLocalPath,
				TEXT("local path ok"),
				StandDetail,
				ResearcherLabel);
			if (!bLocalPath)
			{
				FailAndStop(
					Owner,
					Record,
					TEXT("Beat 8 V1B blocker: no safe local encounter path within Host_Neuro_Researcher activation range on existing navigation."));
				return;
			}

			OrganoidPlaytestActions::TeleportNear(Character, StandProjected, 0.0f, StandWorld.Z);
			OrganoidPlaytestActions::FaceActor(Character, Researcher);
			Researcher->TryActivateEncounterFromProximity(Character);
			if (!AssertTrue(
					Record, TEXT("viability.encounter_activated"),
					Researcher->IsEncounterActivated(),
					TEXT("active"),
					BoolText(Researcher->IsEncounterActivated()),
					ResearcherLabel))
			{
				FailAndStop(
					Owner,
					Record,
					TEXT("Beat 8 V1D blocker: Host_Neuro_Researcher failed to activate at local encounter stand."));
				return;
			}

			FVector InvestigateWorld;
			FVector InvestigateProjected;
			float InvestigatePathLen = 0.0f;
			int32 InvestigateIdx = -1;
			FString InvestigateDetail;
			const bool bInvestigateDest = TryFindLocalInvestigateDestination(
				World,
				Researcher,
				InvestigateWorld,
				InvestigateProjected,
				InvestigatePathLen,
				InvestigateIdx,
				InvestigateDetail);
			AssertTrue(
				Record, TEXT("viability.investigate_destination"),
				bInvestigateDest,
				TEXT("path_len>=180 local combat dest"),
				InvestigateDetail,
				ResearcherLabel);
			if (!bInvestigateDest)
			{
				FailAndStop(
					Owner,
					Record,
					TEXT("Beat 8 V1D blocker: no local Investigate destination with path_len>=180 on existing navigation."));
				return;
			}

			// Player stands at the Investigate destination (accessible combat space). Activation stand already used.
			OrganoidPlaytestActions::TeleportNear(Character, InvestigateProjected, 0.0f, InvestigateWorld.Z);
			OrganoidPlaytestActions::FaceActor(Character, Researcher);

			if (UCharacterMovementComponent* HostMove = Researcher->GetCharacterMovement())
			{
				HostMove->SetMovementMode(MOVE_Walking);
			}

			AProjectOrganoidHostAIController* MovingAI = Cast<AProjectOrganoidHostAIController>(Researcher->GetController());
			ViabilityStartLocation = Researcher->GetActorLocation();
			ViabilityStandProjected = StandProjected;
			ViabilityInvestigateCandidate = InvestigateWorld;
			ViabilityInvestigateProjected = InvestigateProjected;
			ViabilityInvestigatePathLength = InvestigatePathLen;
			ViabilityInvestigateCandidateIndex = InvestigateIdx;
			ViabilityInitialGap = FVector::Dist2D(ViabilityStartLocation, InvestigateProjected);

			Record.AddActor(
				TEXT("v1d_investigate_destination"),
				FString::Printf(
					TEXT("%s initial_gap=%.1f"),
					*InvestigateDetail,
					ViabilityInitialGap));

			if (MovingAI)
			{
				MovingAI->RequestInvestigateAt(InvestigateProjected);
			}
			UAISense_Hearing::ReportNoiseEvent(
				World,
				InvestigateProjected,
				2.5f,
				Character,
				3500.0f,
				ProjectOrganoidNoiseTags::Gunfire);

			WaitSeconds = 0.0f;
			bViabilityMoveRequested = true;
			ProofPhase = EProofPhase::EncounterViabilityWait;
			Owner.SetStage(TEXT("EncounterViabilityWait"));
		}

		void TickEncounterViabilityWait(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			AProjectOrganoidHostBase* Researcher = ViabilityResearcher.Get();
			if (!Researcher || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Beat 8 V1D blocker: Host_Neuro_Researcher or player vanished during move wait."));
				return;
			}

			const float MovedFromStart = FVector::Dist2D(Researcher->GetActorLocation(), ViabilityStartLocation);
			const float GapNow = FVector::Dist2D(Researcher->GetActorLocation(), ViabilityInvestigateProjected);
			const float GapDelta = ViabilityInitialGap - GapNow;
			const EProjectOrganoidHostCombatState CombatState = Researcher->GetCombatState();
			const bool bCombatEngaged = CombatState == EProjectOrganoidHostCombatState::Pursue
				|| CombatState == EProjectOrganoidHostCombatState::Attack
				|| CombatState == EProjectOrganoidHostCombatState::Investigate;
			const EMovementMode MoveMode = Researcher->GetCharacterMovement()
				? static_cast<EMovementMode>(Researcher->GetCharacterMovement()->MovementMode.GetValue())
				: MOVE_None;
			const FString MoveModeText = MovementModeName(MoveMode);
			const bool bClosedGap = GapDelta >= 35.0f;
			const bool bMoved = MovedFromStart >= 35.0f;

			const FString Evidence = FString::Printf(
				TEXT("idx=%d candidate=(%.1f,%.1f,%.1f) projected=(%.1f,%.1f,%.1f) path_len=%.1f initial_gap=%.1f final_gap=%.1f moved=%.1f gap_delta=%.1f mode=%s combat=%d"),
				ViabilityInvestigateCandidateIndex,
				ViabilityInvestigateCandidate.X,
				ViabilityInvestigateCandidate.Y,
				ViabilityInvestigateCandidate.Z,
				ViabilityInvestigateProjected.X,
				ViabilityInvestigateProjected.Y,
				ViabilityInvestigateProjected.Z,
				ViabilityInvestigatePathLength,
				ViabilityInitialGap,
				GapNow,
				MovedFromStart,
				GapDelta,
				*MoveModeText,
				static_cast<int32>(CombatState));

			if (bMoved || bClosedGap)
			{
				AssertTrue(
					Record, TEXT("viability.researcher_observable_move"),
					true,
					TEXT("moved|closed-gap"),
					Evidence,
					ResearcherLabel);
				ProofPhase = EProofPhase::LessonExercise;
				Owner.SetStage(TEXT("LessonExercise"));
				return;
			}

			if (WaitSeconds > 10.0f)
			{
				AssertTrue(
					Record, TEXT("viability.researcher_observable_move"),
					false,
					TEXT("moved|closed-gap"),
					FString::Printf(TEXT("%s engaged=%s"), *Evidence, bCombatEngaged ? TEXT("true") : TEXT("false")),
					ResearcherLabel);
				FailAndStop(
					Owner,
					Record,
					TEXT("Beat 8 V1D blocker: Host_Neuro_Researcher produced no observable encounter movement toward the Investigate destination."));
			}
		}

		void TickProofLessonExercise(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			UProjectOrganoidHUDWidget* HUD = ResolveOwnedHud(PC);
			AProjectOrganoidHostBase* Researcher = ViabilityResearcher.Get();
			if (!Researcher && World)
			{
				const TArray<AActor*> Matches = OrganoidPlaytestActions::FindActorsByLabel(World, ResearcherLabel);
				Researcher = Matches.Num() == 1 ? Cast<AProjectOrganoidHostBase>(Matches[0]) : nullptr;
				ViabilityResearcher = Researcher;
			}
			AProjectOrganoidHostBase* Host1 = World
				? Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindActorsByLabel(World, Host1Label)[0])
				: nullptr;
			if (!World || !Character || !Power || !Objectives || !Saves || !HUD || !Researcher)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE context for Beat 8 lesson exercise."));
				return;
			}

			const int32 StationBefore = CountLabel(World, StationLabel);
			const int32 Host1Before = CountLabel(World, Host1Label);
			const int32 Host2Before = CountLabel(World, Host2Label);
			const int32 Host3Before = CountLabel(World, Host3Label);

			AssertTrue(Record, TEXT("active.proximity_activates"), Researcher->IsEncounterActivated(), TEXT("active"), BoolText(Researcher->IsEncounterActivated()), ResearcherLabel);

			const int32 EventBeforeWrong = Researcher->LessonSuccessEventFireCount;
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::LocomotorNerves, false, 4.0f);
			AssertTrue(Record, TEXT("reject.nontactical"), Researcher->LessonSuccessEventFireCount == EventBeforeWrong, TEXT("unchanged"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::OpticalNodes, true, 4.0f);
			AssertTrue(Record, TEXT("reject.optical"), Researcher->LessonSuccessEventFireCount == EventBeforeWrong, TEXT("unchanged"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::None, true, 4.0f);
			AssertTrue(Record, TEXT("reject.body_no_effect"), Researcher->LessonSuccessEventFireCount == EventBeforeWrong, TEXT("unchanged"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			if (Host1)
			{
				const int32 Host1Events = Host1->LessonSuccessEventFireCount;
				FireTacticalWeakPoint(Character, Host1, EProjectOrganoidWeakPointType::LocomotorNerves, true, 4.0f);
				AssertTrue(Record, TEXT("reject.host1"), Host1->LessonSuccessEventFireCount == Host1Events, TEXT("0"), FString::FromInt(Host1->LessonSuccessEventFireCount), Host1Label);
			}

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::LocomotorNerves, true, 25.0f);
			AssertTrue(Record, TEXT("valid.event_once"), Researcher->LessonSuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			AssertTrue(
				Record, TEXT("valid.impairment"),
				Researcher->bIsStaggered || Researcher->bLocomotorNervesDestroyed || Researcher->bIsDismembered,
				TEXT("impaired"),
				TEXT("checked"),
				ResearcherLabel);
			AssertTrue(Record, TEXT("valid.notification_count"), Researcher->LessonSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Researcher->LessonSuccessNotificationCount), ResearcherLabel);
			AssertTrue(
				Record, TEXT("valid.nathan_speaker"),
				Researcher->LessonSuccessNotificationSpeaker.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Researcher->LessonSuccessNotificationSpeaker.ToString(),
				ResearcherLabel);
			AssertTrue(
				Record, TEXT("valid.nathan_line"),
				Researcher->LessonSuccessNotificationText.ToString() == ExpectedLine,
				ExpectedLine,
				Researcher->LessonSuccessNotificationText.ToString(),
				ResearcherLabel);
			AssertTrue(
				Record, TEXT("valid.duration"),
				FMath::IsNearlyEqual(Researcher->LessonSuccessNotificationDurationSeconds, ExpectedDuration),
				TEXT("7.0"),
				FString::SanitizeFloat(Researcher->LessonSuccessNotificationDurationSeconds),
				ResearcherLabel);
			AssertTrue(
				Record, TEXT("valid.objective_completed"),
				CountCompletedId(Objectives, FName(TargetingObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TargetingObjectiveId))),
				TargetingObjectiveId);
			AssertTrue(
				Record, TEXT("valid.mission_complete"),
				Objectives->IsMissionComplete(FName(TargetingMissionId)),
				TEXT("true"),
				BoolText(Objectives->IsMissionComplete(FName(TargetingMissionId))),
				TargetingMissionId);

			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::LocomotorNerves, true, 25.0f);
			AssertTrue(Record, TEXT("replay.event_still_one"), Researcher->LessonSuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Researcher->LessonSuccessEventFireCount), ResearcherLabel);
			AssertTrue(Record, TEXT("replay.notify_still_one"), Researcher->LessonSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Researcher->LessonSuccessNotificationCount), ResearcherLabel);

			AssertTrue(Record, TEXT("side.station_unchanged"), CountLabel(World, StationLabel) == StationBefore, FString::FromInt(StationBefore), FString::FromInt(CountLabel(World, StationLabel)), StationLabel);
			AssertTrue(
				Record, TEXT("side.hosts_present"),
				CountLabel(World, Host1Label) == Host1Before
					&& CountLabel(World, Host2Label) == Host2Before
					&& CountLabel(World, Host3Label) == Host3Before,
				TEXT("unchanged"),
				TEXT("ok"),
				TEXT("Hosts"));
			AssertTrue(
				Record, TEXT("side.neuro_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("side.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));

			UProjectOrganoidSaveGame* SaveGame = NewObject<UProjectOrganoidSaveGame>(GetTransientPackage());
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.targeting_completed_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(TargetingObjectiveId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(TargetingObjectiveId))),
				TEXT("save"));

			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.objective_completed"),
				CountCompletedId(Objectives, FName(TargetingObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountCompletedId(Objectives, FName(TargetingObjectiveId))),
				TargetingObjectiveId);

			ApproachHost(Character, Researcher);
			const bool bWasActivated = Researcher->IsEncounterActivated();
			const bool bRearmed = Researcher->TryActivateEncounterFromProximity(Character);
			AssertTrue(
				Record, TEXT("saveload.no_rearm"),
				!bRearmed || bWasActivated,
				TEXT("no new activation"),
				bRearmed ? TEXT("rearmed") : TEXT("held"),
				ResearcherLabel);
			const int32 BeforeReloadHit = Researcher->LessonSuccessEventFireCount;
			FireTacticalWeakPoint(Character, Researcher, EProjectOrganoidWeakPointType::LocomotorNerves, true, 25.0f);
			AssertTrue(
				Record, TEXT("saveload.no_event_replay"),
				Researcher->LessonSuccessEventFireCount == BeforeReloadHit
					|| Researcher->LessonSuccessEventFireCount == 1,
				TEXT("no replay"),
				FString::FromInt(Researcher->LessonSuccessEventFireCount),
				ResearcherLabel);

			AssertTrue(
				Record, TEXT("saveload.pre_disk_neuro_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online
					|| Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Online|Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
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

	struct FNeuroTargetingWhyAutoRegister
	{
		FNeuroTargetingWhyAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroTargetingWhyFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroTargetingWhyAutoRegister GRegisterNeuroTargetingWhy;
}
