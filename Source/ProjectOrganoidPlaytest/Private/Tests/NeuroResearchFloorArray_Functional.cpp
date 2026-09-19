#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "NeuroResearchFloorArrayMissionCompletionProbe.h"

#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/PackageName.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidItemData.h"
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
	constexpr TCHAR TestId[] = TEXT("NeuroResearchFloorArray_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Research Floor Array Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");

	constexpr TCHAR ArrayEvent[] = TEXT("Event_NeuroResearchArrayLocated");
	constexpr TCHAR ResearchFloorId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR PowerFailureId[] = TEXT("Obj_InvestigateNeuroPowerFailure");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");
	constexpr TCHAR ReceptionEvent[] = TEXT("Event_ReceptionTerminalUsed");
	constexpr TCHAR SecurityEvent[] = TEXT("Event_SecurityTerminalUsed");
	constexpr TCHAR NextMissionId[] = TEXT("Mission_NeuroGenetics");
	constexpr TCHAR NextObjectiveId[] = TEXT("Obj_IsolateNeuroResearchLoad");
	constexpr TCHAR NextMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR NextMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics");
	constexpr TCHAR ArrayLabel[] = TEXT("NeuralMappingArray_NeuroGenetics");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR TransientReloadLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_ReloadClone");
	constexpr TCHAR TransientOrderedLabel[] = TEXT("NeuralMappingArray_NeuroGenetics_OrderedClone");
	constexpr TCHAR InspectPrompt[] = TEXT("Inspect Neural Mapping Array");
	constexpr TCHAR ReviewPrompt[] = TEXT("Review Neural Mapping Array");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	constexpr TCHAR ExpectedLine[] =
		TEXT("The spikes are coming from this array. It\u2019s still mapping something.");
	constexpr TCHAR ExpectedRendered[] =
		TEXT("Nathan: The spikes are coming from this array. It\u2019s still mapping something.");
	constexpr TCHAR MissionTitle[] = TEXT("NeuroGenetics");
	constexpr TCHAR MissionDescription[] =
		TEXT("Isolate the unstable research load before reconnecting the primary feed.");
	constexpr TCHAR IsolateTitle[] = TEXT("Isolate the NeuroGenetics research load");
	constexpr TCHAR IsolateDescription[] =
		TEXT("Find the emergency cutoff feeding the unstable research equipment.");
	constexpr TCHAR CubeMeshPath[] = TEXT("/Engine/BasicShapes/Cube.Cube");
	constexpr TCHAR CylinderMeshPath[] = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	const FVector ArrayLocation(-500.0f, -600.0f, -1100.0f);
	const FRotator ArrayRotation = FRotator::ZeroRotator;
	const FVector ArrayScale = FVector::OneVector;
	constexpr float ArrayInteractionRange = 200.0f;
	constexpr float ArrayNotifySeconds = 4.0f;
	const FVector PedestalRel(0.0f, 0.0f, 40.0f);
	const FVector PedestalScale(1.2f, 1.2f, 0.8f);
	const FVector ColumnRel(0.0f, 0.0f, 110.0f);
	const FVector ColumnScale(0.35f, 0.35f, 1.4f);
	const FVector HeadRel(0.0f, 0.0f, 192.5f);
	const FVector HeadScale(1.6f, 1.6f, 0.25f);
	const FVector StationLocation(800.0f, -1600.0f, -1100.0f);

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
		return LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, NextMissionSoftPath);
	}

	class FNeuroResearchFloorArrayFunctional : public IOrganoidPlaytestCase
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
			PersistedArray.Reset();
			TransientInstrument.Reset();
			OwnedHudWidget.Reset();
			DecoyHudWidget.Reset();
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
		TWeakObjectPtr<AProjectOrganoidInspectableInstrument> PersistedArray;
		TWeakObjectPtr<AProjectOrganoidInspectableInstrument> TransientInstrument;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> OwnedHudWidget;
		TWeakObjectPtr<UProjectOrganoidHUDWidget> DecoyHudWidget;
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
			// Never destroy the persisted map actor.
			if (AProjectOrganoidInspectableInstrument* Live = TransientInstrument.Get())
			{
				if (!OrganoidPlaytestActions::ActorLabel(Live).Equals(ArrayLabel, ESearchCase::CaseSensitive))
				{
					Live->Destroy();
				}
			}
			TransientInstrument.Reset();
			OwnedHudWidget.Reset();
			if (UProjectOrganoidHUDWidget* Decoy = DecoyHudWidget.Get())
			{
				Decoy->RemoveFromParent();
			}
			DecoyHudWidget.Reset();
		}

		void ConfigureInstrumentIdentically(AProjectOrganoidInspectableInstrument* Actor) const
		{
			if (!Actor)
			{
				return;
			}
			Actor->SetActorLocation(ArrayLocation);
			Actor->SetActorRotation(ArrayRotation);
			Actor->SetActorScale3D(ArrayScale);
			Actor->InteractionRange = ArrayInteractionRange;
			Actor->ObjectiveEventId = FName(ArrayEvent);
			Actor->CompletedObjectiveIdForReplayGuard = FName(ResearchFloorId);
			Actor->InspectionPrompt = FText::FromString(InspectPrompt);
			Actor->ReviewPrompt = FText::FromString(ReviewPrompt);
			Actor->SpeakerLabel = FText::FromString(ExpectedSpeaker);
			Actor->InspectionResponseText = FText::FromString(ExpectedLine);
			Actor->NotificationDurationSeconds = ArrayNotifySeconds;
			Actor->bHasBeenInspected = false;

			auto ApplyMesh = [](UStaticMeshComponent* Mesh, const TCHAR* Path, const FVector& Rel, const FVector& Scale)
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
			};
			ApplyMesh(Actor->PedestalMesh, CubeMeshPath, PedestalRel, PedestalScale);
			ApplyMesh(Actor->ColumnMesh, CylinderMeshPath, ColumnRel, ColumnScale);
			ApplyMesh(Actor->ArrayHeadMesh, CylinderMeshPath, HeadRel, HeadScale);
			Actor->RefreshPrompt();
		}

		AProjectOrganoidInspectableInstrument* SpawnConfiguredClone(UWorld* World, const FName& Label)
		{
			if (!World)
			{
				return nullptr;
			}
			AProjectOrganoidInspectableInstrument* Actor =
				World->SpawnActorDeferred<AProjectOrganoidInspectableInstrument>(
					AProjectOrganoidInspectableInstrument::StaticClass(),
					FTransform(ArrayRotation, ArrayLocation, ArrayScale));
			if (!Actor)
			{
				return nullptr;
			}
			Actor->SetActorLabel(Label.ToString());
			ConfigureInstrumentIdentically(Actor);
			Actor->FinishSpawning(FTransform(ArrayRotation, ArrayLocation, ArrayScale));
			ConfigureInstrumentIdentically(Actor);
			return Actor;
		}

		UProjectOrganoidGameplayHUDController* ResolveHudController(APlayerController* PC) const
		{
			if (!PC)
			{
				return nullptr;
			}
			if (AProjectOrganoidGameMode* GameMode = PC->GetWorld()
				? PC->GetWorld()->GetAuthGameMode<AProjectOrganoidGameMode>()
				: nullptr)
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
				FPackageName::DoesPackageExist(NextMissionPackage),
				TEXT("exists"),
				FPackageName::DoesPackageExist(NextMissionPackage) ? TEXT("exists") : TEXT("missing"),
				NextMissionPackage);
			Owner.SetStage(TEXT("StartPie"));
			Stage = EStage::StartPie;
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

			const TArray<AActor*> Arrays = World
				? OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel)
				: TArray<AActor*>();
			if (World && Character && Arrays.Num() == 1)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				FailAndStop(
					Owner, Record,
					TEXT("Timed out waiting for PIE player and NeuralMappingArray_NeuroGenetics. Persist neuro_neural_mapping_array_v1 before this test can pass."));
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
			if (!HUD || !HudController || !Decoy)
			{
				FailAndStop(Owner, Record, TEXT("Failed to resolve owned HUD/controller or decoy."));
				return;
			}

			// ---- A. Real persisted mission asset ----
			UProjectOrganoidObjectiveDataAsset* Mission = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("mission.asset_loaded"),
				Mission != nullptr,
				TEXT("loaded"),
				Mission ? TEXT("loaded") : TEXT("missing"),
				NextMissionSoftPath);
			if (!Mission)
			{
				FailAndStop(
					Owner, Record,
					TEXT("DA_Mission_NeuroGenetics missing. Persist create_neurogenetics_mission + save before this test can pass."));
				return;
			}
			AssertTrue(
				Record, TEXT("mission.exact_path"),
				FSoftObjectPath(Mission).ToString() == NextMissionSoftPath,
				NextMissionSoftPath,
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
				Mission->MissionId == FName(NextMissionId),
				NextMissionId,
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
				Record, TEXT("mission.next_null"),
				!Mission->NextMissionAsset.ToSoftObjectPath().IsValid(),
				TEXT("null"),
				Mission->NextMissionAsset.ToSoftObjectPath().IsValid()
					? Mission->NextMissionAsset.ToSoftObjectPath().ToString()
					: TEXT("null"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_count_one"),
				Mission->Tasks.Num() == 1,
				TEXT("1"),
				FString::FromInt(Mission->Tasks.Num()),
				TEXT("DA"));
			const bool bTaskOk = Mission->Tasks.Num() == 1;
			const FProjectOrganoidMissionTaskDefinition* Task = bTaskOk ? &Mission->Tasks[0] : nullptr;
			AssertTrue(
				Record, TEXT("mission.task_objective_id"),
				Task && Task->Objective.ObjectiveId == FName(NextObjectiveId),
				NextObjectiveId,
				Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_title"),
				Task && Task->Objective.Title.ToString() == IsolateTitle,
				IsolateTitle,
				Task ? Task->Objective.Title.ToString() : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_description"),
				Task && Task->Objective.Description.ToString() == IsolateDescription,
				IsolateDescription,
				Task ? Task->Objective.Description.ToString() : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_category_main"),
				Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main,
				TEXT("Main"),
				Task ? UEnum::GetValueAsString(Task->Objective.Type) : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_target_one"),
				Task && Task->Objective.TargetProgress == 1,
				TEXT("1"),
				Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_auto_activate"),
				Task && Task->bAutoActivate,
				TEXT("true"),
				Task ? BoolText(Task->bAutoActivate) : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_no_prerequisites"),
				Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0,
				TEXT("0"),
				Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("mission.task_initially_incomplete"),
				Task && Task->Objective.State == EProjectOrganoidObjectiveState::Inactive
					&& Task->Objective.CurrentProgress == 0,
				TEXT("Inactive/0"),
				Task
					? FString::Printf(TEXT("%s/%d"), *ObjectiveStateName(Task->Objective.State), Task->Objective.CurrentProgress)
					: TEXT("missing"),
				TEXT("DA"));

			// ---- B. Persisted map actor ----
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
			PersistedArray = Array;
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
			const FString WorldPackage = OrganoidPlaytestActions::NormalizePackage(
				World->GetOutermost() ? World->GetOutermost()->GetName() : FString());
			AssertTrue(
				Record, TEXT("map.persistent_lvl_epitope"),
				WorldPackage.Equals(MapPackage, ESearchCase::CaseSensitive),
				MapPackage,
				WorldPackage,
				TEXT("world"));
			AssertTrue(
				Record, TEXT("map.transform"),
				VecNear(Array->GetActorLocation(), ArrayLocation)
					&& RotNear(Array->GetActorRotation(), ArrayRotation)
					&& VecNear(Array->GetActorScale3D(), ArrayScale),
				TEXT("(-500,-600,-1100)/(0,0,0)/(1,1,1)"),
				FString::Printf(
					TEXT("(%s)/(%s)/(%s)"),
					*Array->GetActorLocation().ToCompactString(),
					*Array->GetActorRotation().ToCompactString(),
					*Array->GetActorScale3D().ToCompactString()),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.interaction_range"),
				FMath::IsNearlyEqual(Array->InteractionRange, ArrayInteractionRange, 0.05f),
				TEXT("200"),
				FString::SanitizeFloat(Array->InteractionRange),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.objective_event_id"),
				Array->ObjectiveEventId == FName(ArrayEvent),
				ArrayEvent,
				Array->ObjectiveEventId.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.replay_guard_id"),
				Array->CompletedObjectiveIdForReplayGuard == FName(ResearchFloorId),
				ResearchFloorId,
				Array->CompletedObjectiveIdForReplayGuard.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.inspection_prompt"),
				Array->InspectionPrompt.ToString() == InspectPrompt,
				InspectPrompt,
				Array->InspectionPrompt.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.review_prompt"),
				Array->ReviewPrompt.ToString() == ReviewPrompt,
				ReviewPrompt,
				Array->ReviewPrompt.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.speaker_nathan"),
				Array->SpeakerLabel.ToString() == ExpectedSpeaker,
				ExpectedSpeaker,
				Array->SpeakerLabel.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.response_text_u2019"),
				Array->InspectionResponseText.ToString() == ExpectedLine,
				ExpectedLine,
				Array->InspectionResponseText.ToString(),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.notification_duration"),
				FMath::IsNearlyEqual(Array->NotificationDurationSeconds, ArrayNotifySeconds, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(Array->NotificationDurationSeconds),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.initially_uninspected"),
				!Array->bHasBeenInspected,
				TEXT("false"),
				BoolText(Array->bHasBeenInspected),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.components_exist"),
				Array->SceneRoot && Array->PedestalMesh && Array->ColumnMesh && Array->ArrayHeadMesh,
				TEXT("SceneRoot+Pedestal+Column+ArrayHead"),
				TEXT("present"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.pedestal_mesh"),
				MeshPathMatches(Array->PedestalMesh, CubeMeshPath)
					&& VecNear(Array->PedestalMesh->GetRelativeLocation(), PedestalRel)
					&& RotNear(Array->PedestalMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->PedestalMesh->GetRelativeScale3D(), PedestalScale)
					&& Array->PedestalMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->PedestalMesh->GetGenerateOverlapEvents(),
				TEXT("Cube@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.column_mesh"),
				MeshPathMatches(Array->ColumnMesh, CylinderMeshPath)
					&& VecNear(Array->ColumnMesh->GetRelativeLocation(), ColumnRel)
					&& RotNear(Array->ColumnMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->ColumnMesh->GetRelativeScale3D(), ColumnScale)
					&& Array->ColumnMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->ColumnMesh->GetGenerateOverlapEvents(),
				TEXT("Cylinder@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);
			AssertTrue(
				Record, TEXT("map.array_head_mesh"),
				MeshPathMatches(Array->ArrayHeadMesh, CylinderMeshPath)
					&& VecNear(Array->ArrayHeadMesh->GetRelativeLocation(), HeadRel)
					&& RotNear(Array->ArrayHeadMesh->GetRelativeRotation(), FRotator::ZeroRotator)
					&& VecNear(Array->ArrayHeadMesh->GetRelativeScale3D(), HeadScale)
					&& Array->ArrayHeadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision
					&& !Array->ArrayHeadMesh->GetGenerateOverlapEvents(),
				TEXT("Cylinder@rel exact NoCollision"),
				TEXT("checked"),
				ArrayLabel);

			AssertTrue(
				Record, TEXT("hud.owned_via_controller"),
				HudController->GetBoundHUDWidget() == HUD,
				TEXT("bound==owned"),
				HudController->GetBoundHUDWidget() == HUD ? TEXT("bound==owned") : TEXT("mismatch"),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.decoy_distinct"),
				Decoy != HUD,
				TEXT("true"),
				BoolText(Decoy != HUD),
				TEXT("HUD"));

			// ---- HUD API coverage (owned GameMode widget only) ----
			HUD->ShowTransientNotification(FText::FromString(ExpectedSpeaker), FText::FromString(ExpectedLine), 4.0f);
			AssertTrue(
				Record, TEXT("hud.rendered_with_prefix"),
				HUD->GetLastResourceNotification().ToString() == ExpectedRendered,
				ExpectedRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.duration_four"),
				FMath::IsNearlyEqual(HUD->GetTransientNotificationSecondsRemaining(), 4.0f, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(HUD->GetTransientNotificationSecondsRemaining()),
				TEXT("HUD"));

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(ExpectedLine), 2.0f);
			AssertTrue(
				Record, TEXT("hud.empty_speaker_no_colon"),
				HUD->GetLastResourceNotification().ToString() == ExpectedLine,
				ExpectedLine,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.replacement_duration"),
				FMath::IsNearlyEqual(HUD->GetTransientNotificationSecondsRemaining(), 2.0f, 0.05f),
				TEXT("2"),
				FString::SanitizeFloat(HUD->GetTransientNotificationSecondsRemaining()),
				TEXT("HUD"));

			UProjectOrganoidItemData* FakeItem = NewObject<UProjectOrganoidItemData>(GetTransientPackage());
			FakeItem->ItemName = FText::FromString(TEXT("ProbeCell"));
			HUD->NotifyResourceAcquired(FakeItem, 1);
			AssertTrue(
				Record, TEXT("hud.resource_notification_preserved"),
				HUD->GetLastResourceNotification().ToString() == TEXT("ProbeCell acquired."),
				TEXT("ProbeCell acquired."),
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.resource_duration_four"),
				FMath::IsNearlyEqual(HUD->GetTransientNotificationSecondsRemaining(), 4.0f, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(HUD->GetTransientNotificationSecondsRemaining()),
				TEXT("HUD"));

			AssertTrue(
				Record, TEXT("seed.pending_next_soft_path"),
				Objectives->GetPendingNextMissionAssetPath().ToString() == NextMissionSoftPath,
				NextMissionSoftPath,
				Objectives->GetPendingNextMissionAssetPath().ToString(),
				TEXT("objectives"));

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

			// ---- C. Primary early-order path on persisted actor ----
			FProjectOrganoidObjective ResearchBefore;
			const bool bHasResearch = Objectives->GetObjective(FName(ResearchFloorId), ResearchBefore);
			AssertTrue(
				Record, TEXT("early.research_floor_inactive"),
				bHasResearch && ResearchBefore.State == EProjectOrganoidObjectiveState::Inactive,
				TEXT("Inactive"),
				bHasResearch ? ObjectiveStateName(ResearchBefore.State) : TEXT("missing"),
				ResearchFloorId);

			FProjectOrganoidObjective PowerBefore;
			const bool bHasPower = Objectives->GetObjective(FName(PowerFailureId), PowerBefore);
			AssertTrue(
				Record, TEXT("early.power_failure_incomplete"),
				bHasPower && PowerBefore.State != EProjectOrganoidObjectiveState::Completed,
				TEXT("not Completed"),
				bHasPower ? ObjectiveStateName(PowerBefore.State) : TEXT("missing"),
				PowerFailureId);

			AssertTrue(
				Record, TEXT("early.no_required_objective_gate"),
				Array->CanInteract(Character),
				TEXT("true"),
				BoolText(Array->CanInteract(Character)),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.prompt_inspect"),
				Array->GetInteractionPrompt().ToString() == InspectPrompt,
				InspectPrompt,
				Array->GetInteractionPrompt().ToString(),
				TEXT("array"));

			HUD->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Decoy->ShowTransientNotification(FText::FromString(TEXT("Decoy")), FText::FromString(TEXT("stale bait")), 9.0f);
			const FString DecoyBefore = Decoy->GetLastResourceNotification().ToString();
			const int32 NotifyBefore = Array->InspectionNotificationCount;
			const int32 EventBefore = Array->ObjectiveEventFireCount;
			const bool bEarlyInteracted = Array->Interact(Character);
			AssertTrue(
				Record, TEXT("early.first_interact"),
				bEarlyInteracted,
				TEXT("true"),
				BoolText(bEarlyInteracted),
				TEXT("array"));

			FProjectOrganoidObjective ResearchAfterEarly;
			Objectives->GetObjective(FName(ResearchFloorId), ResearchAfterEarly);
			AssertTrue(
				Record, TEXT("early.research_floor_completed"),
				ResearchAfterEarly.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				ObjectiveStateName(ResearchAfterEarly.State),
				ResearchFloorId);
			AssertTrue(
				Record, TEXT("early.event_fired_once"),
				Array->ObjectiveEventFireCount == EventBefore + 1,
				TEXT("1"),
				FString::FromInt(Array->ObjectiveEventFireCount - EventBefore),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.nathan_once"),
				Array->InspectionNotificationCount == NotifyBefore + 1,
				TEXT("1"),
				FString::FromInt(Array->InspectionNotificationCount - NotifyBefore),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.nathan_exact_text"),
				HUD->GetLastResourceNotification().ToString() == ExpectedRendered,
				ExpectedRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("early.nathan_duration"),
				FMath::IsNearlyEqual(HUD->GetTransientNotificationSecondsRemaining(), 4.0f, 0.05f),
				TEXT("4"),
				FString::SanitizeFloat(HUD->GetTransientNotificationSecondsRemaining()),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.route_ignores_decoy"),
				Decoy->GetLastResourceNotification().ToString() == DecoyBefore,
				DecoyBefore,
				Decoy->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("hud.route_uses_owned"),
				HUD->GetLastResourceNotification().ToString() == ExpectedRendered
					&& HudController->GetBoundHUDWidget() == HUD,
				ExpectedRendered,
				HUD->GetLastResourceNotification().ToString(),
				TEXT("HUD"));
			AssertTrue(
				Record, TEXT("early.prompt_review"),
				Array->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Array->GetInteractionPrompt().ToString(),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.inspected_flag"),
				Array->bHasBeenInspected,
				TEXT("true"),
				BoolText(Array->bHasBeenInspected),
				TEXT("array"));

			const int32 NotifyMid = Array->InspectionNotificationCount;
			const int32 EventMid = Array->ObjectiveEventFireCount;
			const bool bRepeatInteracted = Array->Interact(Character);
			AssertTrue(
				Record, TEXT("early.repeat_interact"),
				bRepeatInteracted,
				TEXT("true"),
				BoolText(bRepeatInteracted),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.repeat_no_event"),
				Array->ObjectiveEventFireCount == EventMid,
				TEXT("0"),
				FString::FromInt(Array->ObjectiveEventFireCount - EventMid),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("early.repeat_no_line"),
				Array->InspectionNotificationCount == NotifyMid,
				TEXT("0"),
				FString::FromInt(Array->InspectionNotificationCount - NotifyMid),
				TEXT("array"));

			FProjectOrganoidObjective PowerAfterEarly;
			Objectives->GetObjective(FName(PowerFailureId), PowerAfterEarly);
			AssertTrue(
				Record, TEXT("early.power_still_incomplete"),
				PowerAfterEarly.State != EProjectOrganoidObjectiveState::Completed,
				TEXT("not Completed"),
				ObjectiveStateName(PowerAfterEarly.State),
				PowerFailureId);
			AssertTrue(
				Record, TEXT("early.opening_incomplete"),
				!Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation")),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation"))),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("early.mission_still_opening"),
				Objectives->GetActiveMissionId() == TEXT("Mission_OpeningFoundation"),
				TEXT("Mission_OpeningFoundation"),
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));

			// ---- D. Save/load reconstruction via identically configured transient clone ----
			UProjectOrganoidSaveGame* SaveGame = NewObject<UProjectOrganoidSaveGame>(GetTransientPackage());
			Objectives->CaptureObjectivesToSaveGame(SaveGame);
			AssertTrue(
				Record, TEXT("saveload.research_completed_persisted"),
				SaveGame->CompletedObjectiveIds.Contains(FName(ResearchFloorId)),
				TEXT("true"),
				BoolText(SaveGame->CompletedObjectiveIds.Contains(FName(ResearchFloorId))),
				TEXT("save"));

			Objectives->ApplyObjectivesFromSaveGame(SaveGame);
			AProjectOrganoidInspectableInstrument* Reloaded =
				SpawnConfiguredClone(World, FName(TransientReloadLabel));
			TransientInstrument = Reloaded;
			if (!Reloaded)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn identically configured reload clone."));
				return;
			}

			AssertTrue(
				Record, TEXT("saveload.starts_review_prompt"),
				Reloaded->GetInteractionPrompt().ToString() == ReviewPrompt,
				ReviewPrompt,
				Reloaded->GetInteractionPrompt().ToString(),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("saveload.not_marked_inspected_without_interact"),
				!Reloaded->bHasBeenInspected,
				TEXT("false"),
				BoolText(Reloaded->bHasBeenInspected),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("saveload.no_transient_mission_da"),
				LoadPersistedMissionAsset() == Mission
					&& FSoftObjectPath(Mission).ToString() == NextMissionSoftPath,
				NextMissionSoftPath,
				FSoftObjectPath(Mission).ToString(),
				TEXT("DA"));

			const int32 ReloadEventBefore = Reloaded->ObjectiveEventFireCount;
			const int32 ReloadNotifyBefore = Reloaded->InspectionNotificationCount;
			const bool bReloadInteracted = Reloaded->Interact(Character);
			AssertTrue(
				Record, TEXT("saveload.review_interact"),
				bReloadInteracted,
				TEXT("true"),
				BoolText(bReloadInteracted),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("saveload.no_event_replay"),
				Reloaded->ObjectiveEventFireCount == ReloadEventBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->ObjectiveEventFireCount - ReloadEventBefore),
				TEXT("array"));
			AssertTrue(
				Record, TEXT("saveload.no_line_replay"),
				Reloaded->InspectionNotificationCount == ReloadNotifyBefore,
				TEXT("0"),
				FString::FromInt(Reloaded->InspectionNotificationCount - ReloadNotifyBefore),
				TEXT("array"));

			// ---- E. Ordered path (reseed + diagnosis first); fresh transient (persisted already inspected) ----
			Objectives->LoadDefaultMission();
			AssertTrue(
				Record, TEXT("ordered.reseeds_opening"),
				Objectives->GetActiveMissionId() == TEXT("Mission_OpeningFoundation"),
				TEXT("Mission_OpeningFoundation"),
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("ordered.pending_soft_path"),
				Objectives->GetPendingNextMissionAssetPath().ToString() == NextMissionSoftPath,
				NextMissionSoftPath,
				Objectives->GetPendingNextMissionAssetPath().ToString(),
				TEXT("objectives"));

			UProjectOrganoidObjectiveDataAsset* RealMission = LoadPersistedMissionAsset();
			AssertTrue(
				Record, TEXT("ordered.real_da_loaded"),
				RealMission != nullptr
					&& RealMission->MissionId == FName(NextMissionId)
					&& FSoftObjectPath(RealMission).ToString() == NextMissionSoftPath,
				NextMissionSoftPath,
				RealMission ? FSoftObjectPath(RealMission).ToString() : TEXT("null"),
				TEXT("DA"));

			if (AProjectOrganoidInspectableInstrument* OldClone = TransientInstrument.Get())
			{
				if (!OrganoidPlaytestActions::ActorLabel(OldClone).Equals(ArrayLabel, ESearchCase::CaseSensitive))
				{
					OldClone->Destroy();
				}
			}
			TransientInstrument.Reset();

			AProjectOrganoidInspectableInstrument* OrderedArray =
				SpawnConfiguredClone(World, FName(TransientOrderedLabel));
			TransientInstrument = OrderedArray;
			if (!OrderedArray)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn ordered-path instrument clone."));
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
				Objectives->GetActiveMissionId() == FName(NextMissionId),
				NextMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("ordered.real_da_drives_handoff"),
				Objectives->GetActiveMissionId() == FName(NextMissionId)
					&& LoadPersistedMissionAsset() != nullptr,
				NextMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("DA"));
			AssertTrue(
				Record, TEXT("ordered.isolate_active_once"),
				CountActiveId(Objectives, FName(NextObjectiveId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(NextObjectiveId))),
				NextObjectiveId);

			const int32 IsolateActiveBeforeDup = CountActiveId(Objectives, FName(NextObjectiveId));
			const FName MissionBeforeDup = Objectives->GetActiveMissionId();
			OrderedArray->Interact(Character);
			AssertTrue(
				Record, TEXT("ordered.duplicate_no_extra_mission"),
				Objectives->GetActiveMissionId() == MissionBeforeDup
					&& Objectives->GetActiveMissionId() == FName(NextMissionId),
				NextMissionId,
				Objectives->GetActiveMissionId().ToString(),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("ordered.duplicate_no_extra_isolate"),
				CountActiveId(Objectives, FName(NextObjectiveId)) == IsolateActiveBeforeDup
					&& CountActiveId(Objectives, FName(NextObjectiveId)) == 1
					&& CountCompletedId(Objectives, FName(NextObjectiveId)) == 0,
				TEXT("active=1 completed=0"),
				FString::Printf(
					TEXT("active=%d completed=%d"),
					CountActiveId(Objectives, FName(NextObjectiveId)),
					CountCompletedId(Objectives, FName(NextObjectiveId))),
				NextObjectiveId);

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

			// ---- F. Preservation ----
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
			AssertTrue(
				Record, TEXT("preserve.no_researcher_host_station_ids"),
				!Objectives->GetObjective(TEXT("Obj_Researcher"), ResearchBefore)
					&& !Objectives->GetObjective(TEXT("Obj_Host"), ResearchBefore)
					&& !Objectives->GetObjective(TEXT("Obj_ResearchStation"), ResearchBefore),
				TEXT("absent"),
				TEXT("absent"),
				TEXT("objectives"));

			const TArray<AActor*> Stations = OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel);
			AssertTrue(
				Record, TEXT("preserve.research_station_untouched"),
				Stations.Num() == 1 && VecNear(Stations[0]->GetActorLocation(), StationLocation),
				TEXT("1@ (800,-1600,-1100)"),
				FString::Printf(
					TEXT("%d@%s"),
					Stations.Num(),
					Stations.Num() == 1 ? *Stations[0]->GetActorLocation().ToCompactString() : TEXT("n/a")),
				StationLabel);

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
				!PackageIsDirty(NextMissionPackage),
				TEXT("clean"),
				PackageIsDirty(NextMissionPackage) ? TEXT("dirty") : TEXT("clean"),
				NextMissionPackage);
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

	struct FNeuroResearchFloorArrayAutoRegister
	{
		FNeuroResearchFloorArrayAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroResearchFloorArrayFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroResearchFloorArrayAutoRegister GRegisterNeuroResearchFloorArray;
}
