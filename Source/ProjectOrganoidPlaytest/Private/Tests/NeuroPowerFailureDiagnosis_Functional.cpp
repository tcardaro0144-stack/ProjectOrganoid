#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/PackageName.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidDataPad.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("NeuroPowerFailureDiagnosis_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Power Failure Diagnosis Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MapPanelLabel[] = TEXT("PowerPanel_NeuroBackup");
	constexpr TCHAR MapPadLabel[] = TEXT("DataPad_NeuroPowerDiagnostics");
	constexpr TCHAR ReloadedPadLabel[] = TEXT("DataPad_NeuroPowerDiagnostics_Reloaded_Test");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroPowerDiagnosisTest");

	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR DiagnosisEvent[] = TEXT("Event_NeuroPowerFailureDiagnosed");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");
	constexpr TCHAR InvestigationId[] = TEXT("Obj_InvestigateNeuroPowerFailure");
	constexpr TCHAR InvestigationTitle[] = TEXT("Investigate the NeuroGenetics power failure");
	constexpr TCHAR FollowUpId[] = TEXT("Obj_InvestigateNeuroResearchFloor");
	constexpr TCHAR FollowUpTitle[] = TEXT("Investigate the NeuroGenetics research floor");
	constexpr TCHAR InspectPrompt[] = TEXT("Inspect Feed Diagnostics");
	constexpr TCHAR ExpectedEntryId[] = TEXT("Log_NeuroPrimaryFeedDiagnostic");
	constexpr TCHAR ExpectedLogTitle[] = TEXT("NEUROGENETICS FEED DIAGNOSTIC");
	constexpr TCHAR ExpectedLogAuthor[] = TEXT("FACILITIES CONTROL");
	constexpr TCHAR ExpectedLogCategory[] = TEXT("Systems");
	constexpr TCHAR ExpectedLogBody[] =
		TEXT("PRIMARY FEED: ISOLATED\nEMERGENCY BACKUP: ACTIVE\nPRIMARY RECONNECT: INHIBITED\nFAULT HISTORY: REPEATING LOAD SPIKES \u2014 RESEARCH FLOOR");
	constexpr TCHAR ExpectedCubeMesh[] = TEXT("/Engine/BasicShapes/Cube.Cube");

	const FVector ExpectedPanelLocation(-500.0f, -2275.0f, -1100.0f);
	const FVector ExpectedPadLocation(-500.0f, -2775.0f, -1110.0f);
	const FRotator ExpectedPadRotation(0.0f, 180.0f, 0.0f);
	const FVector ExpectedPadMeshRelScale(0.8f, 0.2f, 1.6f);

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

	class FNeuroPowerFailureDiagnosisFunctional : public IOrganoidPlaytestCase
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
			MapDiscoveryPanel.Reset();
			MapDiagnosticPad.Reset();
			TestPad.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroySpawnedPad();
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
			case EStage::SaveLoad:
				TickSaveLoad(Owner, *Record);
				break;
			case EStage::EndPie:
				DestroySpawnedPad();
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
			SaveLoad,
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
		TWeakObjectPtr<AProjectOrganoidPowerPanel> MapDiscoveryPanel;
		TWeakObjectPtr<AProjectOrganoidDataPad> MapDiagnosticPad;
		TWeakObjectPtr<AProjectOrganoidDataPad> TestPad;

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

		void DestroySpawnedPad()
		{
			// Only destroy the transient reload fixture. Never destroy map DataPad_NeuroPowerDiagnostics.
			if (AProjectOrganoidDataPad* Pad = TestPad.Get())
			{
				if (!OrganoidPlaytestActions::ActorLabel(Pad).Equals(MapPadLabel, ESearchCase::CaseSensitive))
				{
					Pad->Destroy();
				}
			}
			TestPad.Reset();
			MapDiscoveryPanel.Reset();
			MapDiagnosticPad.Reset();
		}

		AProjectOrganoidDataPad* SpawnReloadPad(UWorld* World)
		{
			if (!World)
			{
				return nullptr;
			}

			const FTransform SpawnXf(ExpectedPadRotation, ExpectedPadLocation);
			AProjectOrganoidDataPad* Pad = World->SpawnActorDeferred<AProjectOrganoidDataPad>(
				AProjectOrganoidDataPad::StaticClass(),
				SpawnXf);
			if (!Pad)
			{
				return nullptr;
			}

			Pad->SetActorLabel(ReloadedPadLabel);
			Pad->RequiredObjectiveIdForInteraction = FName(InvestigationId);
			Pad->ObjectiveEventId = FName(DiagnosisEvent);
			Pad->bBroadcastGenericDataPadEvent = false;
			Pad->InteractionPrompt = FText::FromString(InspectPrompt);
			Pad->InteractionRange = 200.0f;
			Pad->LogEntry.EntryId = FName(ExpectedEntryId);
			Pad->LogEntry.Title = FText::FromString(ExpectedLogTitle);
			Pad->LogEntry.Body = FText::FromString(ExpectedLogBody);
			Pad->LogEntry.Author = FText::FromString(ExpectedLogAuthor);
			Pad->LogEntry.Category = FName(ExpectedLogCategory);
			Pad->bHasBeenRead = false;
			Pad->FinishSpawning(SpawnXf);
			return Pad;
		}

		void DestroyTransientPadOnly()
		{
			if (AProjectOrganoidDataPad* Pad = TestPad.Get())
			{
				if (!OrganoidPlaytestActions::ActorLabel(Pad).Equals(MapPadLabel, ESearchCase::CaseSensitive))
				{
					Pad->Destroy();
				}
			}
			TestPad.Reset();
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(NeuroPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, Admin, or Neuro is dirty. Refusing to start."));
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

		void CollectStreamingDiagnostics(
			UWorld* World,
			bool& bOutNeuroLoaded,
			bool& bOutNeuroVisible,
			FString& OutShortNames) const
		{
			bOutNeuroLoaded = false;
			bOutNeuroVisible = false;
			TArray<FString> ShortNames;
			if (!World)
			{
				OutShortNames = TEXT("(no world)");
				return;
			}
			for (ULevelStreaming* Streaming : World->GetStreamingLevels())
			{
				if (!Streaming)
				{
					continue;
				}
				const FString PackageName = Streaming->GetWorldAssetPackageFName().ToString();
				ShortNames.Add(FPackageName::GetShortName(PackageName));
				if (PackageName.Contains(TEXT("SL_Epitope_NeuroGenetics")))
				{
					bOutNeuroLoaded = Streaming->IsLevelLoaded();
					bOutNeuroVisible = Streaming->IsLevelVisible();
				}
			}
			ShortNames.Sort();
			OutShortNames = ShortNames.Num() > 0 ? FString::Join(ShortNames, TEXT(",")) : TEXT("(none)");
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
			const TArray<AActor*> PadMatches = OrganoidPlaytestActions::FindActorsByLabel(World, MapPadLabel);
			const bool bPanelReady = PanelMatches.Num() == 1;
			const bool bPadReady = PadMatches.Num() == 1;

			if (Character && bPlayerReady && bPanelReady && bPadReady)
			{
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}

			if (WaitSeconds > 90.0f)
			{
				bool bNeuroLoaded = false;
				bool bNeuroVisible = false;
				FString StreamShortNames;
				CollectStreamingDiagnostics(World, bNeuroLoaded, bNeuroVisible, StreamShortNames);
				FailAndStop(
					Owner,
					Record,
					FString::Printf(
						TEXT("Timed out waiting for PIE player, PowerPanel_NeuroBackup, and DataPad_NeuroPowerDiagnostics. neuro_loaded=%s neuro_visible=%s panel_count=%d pad_count=%d streams=%s"),
						bNeuroLoaded ? TEXT("true") : TEXT("false"),
						bNeuroVisible ? TEXT("true") : TEXT("false"),
						PanelMatches.Num(),
						PadMatches.Num(),
						*StreamShortNames));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			if (!World || !Character || !Power || !Objectives)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE world/character/power/objectives."));
				return;
			}

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

			FProjectOrganoidObjective InvestigationBefore;
			const bool bHadInvestigation = Objectives->GetObjective(FName(InvestigationId), InvestigationBefore);
			AssertTrue(
				Record, TEXT("before.investigation_inactive"),
				bHadInvestigation && InvestigationBefore.State == EProjectOrganoidObjectiveState::Inactive
					&& InvestigationBefore.Title.ToString() == InvestigationTitle,
				TEXT("Inactive"),
				bHadInvestigation ? UEnum::GetValueAsString(InvestigationBefore.State) : TEXT("missing"),
				InvestigationId);

			FProjectOrganoidObjective FollowUpBefore;
			const bool bHadFollowUp = Objectives->GetObjective(FName(FollowUpId), FollowUpBefore);
			AssertTrue(
				Record, TEXT("before.followup_inactive"),
				bHadFollowUp && FollowUpBefore.State == EProjectOrganoidObjectiveState::Inactive
					&& FollowUpBefore.Title.ToString() == FollowUpTitle,
				TEXT("Inactive"),
				bHadFollowUp ? UEnum::GetValueAsString(FollowUpBefore.State) : TEXT("missing"),
				FollowUpId);
			AssertTrue(
				Record, TEXT("before.followup_journal_hidden"),
				CountJournalId(Objectives, FName(FollowUpId)) == 0,
				TEXT("0"),
				FString::FromInt(CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("before.followup_not_active"),
				CountActiveId(Objectives, FName(FollowUpId)) == 0,
				TEXT("0"),
				FString::FromInt(CountActiveId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("before.investigation_journal_hidden"),
				CountJournalId(Objectives, FName(InvestigationId)) == 0,
				TEXT("0"),
				FString::FromInt(CountJournalId(Objectives, FName(InvestigationId))),
				InvestigationId);

			const TArray<AActor*> LabelMatches = OrganoidPlaytestActions::FindActorsByLabel(World, MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.panel_unique"),
				LabelMatches.Num() == 1,
				TEXT("1"), FString::FromInt(LabelMatches.Num()), MapPanelLabel);
			AProjectOrganoidPowerPanel* Discovery = LabelMatches.Num() == 1
				? Cast<AProjectOrganoidPowerPanel>(LabelMatches[0])
				: nullptr;
			MapDiscoveryPanel = Discovery;
			if (!Discovery)
			{
				FailAndStop(Owner, Record, TEXT("PowerPanel_NeuroBackup missing or wrong class."));
				return;
			}

			AssertTrue(
				Record, TEXT("map.discovery_opt_in"),
				Discovery->bDiscoverPowerFailureBeforeRestore,
				TEXT("true"), BoolText(Discovery->bDiscoverPowerFailureBeforeRestore), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.discovery_event_id"),
				Discovery->DiscoveryObjectiveEventId == FName(DiscoveryEvent),
				DiscoveryEvent, Discovery->DiscoveryObjectiveEventId.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_event_reserved"),
				Discovery->SuccessObjectiveEventId == FName(RestoreEvent) && !Discovery->bHasBeenEngaged,
				TEXT("Event_NeuroPowerRestored reserved / not engaged"),
				FString::Printf(
					TEXT("event=%s engaged=%s"),
					*Discovery->SuccessObjectiveEventId.ToString(),
					*BoolText(Discovery->bHasBeenEngaged)),
				MapPanelLabel);

			const TArray<AActor*> PadMatches = OrganoidPlaytestActions::FindActorsByLabel(World, MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_unique"),
				PadMatches.Num() == 1,
				TEXT("1"), FString::FromInt(PadMatches.Num()), MapPadLabel);
			AProjectOrganoidDataPad* Pad = PadMatches.Num() == 1
				? Cast<AProjectOrganoidDataPad>(PadMatches[0])
				: nullptr;
			MapDiagnosticPad = Pad;
			if (!Pad)
			{
				FailAndStop(
					Owner, Record,
					TEXT("DataPad_NeuroPowerDiagnostics missing or wrong class. Persist neuro_power_diagnostic_v1 actor before this test can pass."));
				return;
			}

			AssertTrue(
				Record, TEXT("map.pad_exact_label"),
				OrganoidPlaytestActions::ActorLabel(Pad).Equals(MapPadLabel, ESearchCase::CaseSensitive),
				MapPadLabel, OrganoidPlaytestActions::ActorLabel(Pad), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_native_class"),
				Pad->GetClass() == AProjectOrganoidDataPad::StaticClass(),
				TEXT("ProjectOrganoidDataPad"),
				Pad->GetClass() ? Pad->GetClass()->GetName() : TEXT("null"),
				MapPadLabel);
			const FString PadPackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Pad));
			AssertTrue(
				Record, TEXT("map.pad_neuro_package"),
				PadPackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage, PadPackage, MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_location"),
				Pad->GetActorLocation().Equals(ExpectedPadLocation, 1.0f),
				ExpectedPadLocation.ToCompactString(),
				Pad->GetActorLocation().ToCompactString(),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_rotation"),
				FMath::IsNearlyEqual(Pad->GetActorRotation().Pitch, ExpectedPadRotation.Pitch, 0.5f)
					&& FMath::IsNearlyEqual(Pad->GetActorRotation().Yaw, ExpectedPadRotation.Yaw, 0.5f)
					&& FMath::IsNearlyEqual(Pad->GetActorRotation().Roll, ExpectedPadRotation.Roll, 0.5f),
				TEXT("yaw 180"),
				Pad->GetActorRotation().ToCompactString(),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_scale"),
				Pad->GetActorScale3D().Equals(FVector::OneVector, 0.01f),
				TEXT("(1,1,1)"),
				Pad->GetActorScale3D().ToCompactString(),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_interaction_range"),
				FMath::IsNearlyEqual(Pad->InteractionRange, 200.0f, 0.01f),
				TEXT("200"),
				FString::SanitizeFloat(Pad->InteractionRange),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_interactable"),
				Pad->bIsInteractable,
				TEXT("true"), BoolText(Pad->bIsInteractable), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_unread"),
				!Pad->bHasBeenRead,
				TEXT("false"), BoolText(Pad->bHasBeenRead), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_required_objective"),
				Pad->RequiredObjectiveIdForInteraction == FName(InvestigationId),
				InvestigationId, Pad->RequiredObjectiveIdForInteraction.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_diagnosis_event"),
				Pad->ObjectiveEventId == FName(DiagnosisEvent),
				DiagnosisEvent, Pad->ObjectiveEventId.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_no_generic_event"),
				!Pad->bBroadcastGenericDataPadEvent,
				TEXT("false"), BoolText(Pad->bBroadcastGenericDataPadEvent), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_inspect_prompt"),
				Pad->GetInteractionPrompt().ToString() == InspectPrompt,
				InspectPrompt, Pad->GetInteractionPrompt().ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_entry_id"),
				Pad->LogEntry.EntryId == FName(ExpectedEntryId),
				ExpectedEntryId, Pad->LogEntry.EntryId.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_log_title"),
				Pad->LogEntry.Title.ToString() == ExpectedLogTitle,
				ExpectedLogTitle, Pad->LogEntry.Title.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_log_author"),
				Pad->LogEntry.Author.ToString() == ExpectedLogAuthor,
				ExpectedLogAuthor, Pad->LogEntry.Author.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_log_category"),
				Pad->LogEntry.Category == FName(ExpectedLogCategory),
				ExpectedLogCategory, Pad->LogEntry.Category.ToString(), MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_log_body"),
				Pad->LogEntry.Body.ToString() == ExpectedLogBody,
				TEXT("4-line Systems diagnostic body"),
				Pad->LogEntry.Body.ToString(),
				MapPadLabel);

			UStaticMeshComponent* PadMesh = Pad->PadMesh.Get();
			AssertTrue(
				Record, TEXT("map.pad_mesh_present"),
				PadMesh != nullptr,
				TEXT("PadMesh"),
				PadMesh ? TEXT("PadMesh") : TEXT("null"),
				MapPadLabel);
			const FString MeshPath = (PadMesh && PadMesh->GetStaticMesh())
				? PadMesh->GetStaticMesh()->GetPathName()
				: TEXT("null");
			AssertTrue(
				Record, TEXT("map.pad_mesh_cube_blockout"),
				MeshPath.Contains(TEXT("/Engine/BasicShapes/Cube")),
				ExpectedCubeMesh,
				MeshPath,
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_mesh_rel_xform"),
				PadMesh
					&& PadMesh->GetRelativeLocation().IsNearlyZero(0.01f)
					&& PadMesh->GetRelativeRotation().IsNearlyZero(0.5f)
					&& PadMesh->GetRelativeScale3D().Equals(ExpectedPadMeshRelScale, 0.01f),
				TEXT("rel (0,0,0)/(0,0,0)/(0.8,0.2,1.6)"),
				PadMesh ? PadMesh->GetRelativeScale3D().ToCompactString() : TEXT("null"),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("map.pad_mesh_no_collision"),
				PadMesh && PadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision,
				TEXT("NoCollision"),
				(PadMesh && PadMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
					? TEXT("NoCollision")
					: TEXT("other"),
				MapPadLabel);

			const bool bGateBlocked = !Pad->CanInteract(Character);
			AssertTrue(
				Record, TEXT("gate.blocked_while_inactive"),
				bGateBlocked,
				TEXT("false CanInteract"),
				BoolText(!bGateBlocked),
				MapPadLabel);
			const bool bBlockedInteract = Pad->Interact(Character);
			AssertTrue(
				Record, TEXT("gate.interact_fails_while_inactive"),
				!bBlockedInteract && !Pad->bHasBeenRead,
				TEXT("false / unread"),
				FString::Printf(TEXT("ok=%s read=%s"), *BoolText(bBlockedInteract), *BoolText(Pad->bHasBeenRead)),
				MapPadLabel);

			const bool bDiscoveryOk = Discovery->Interact(Character);
			AssertTrue(Record, TEXT("discovery.interact_ok"), bDiscoveryOk, TEXT("true"), BoolText(bDiscoveryOk), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.inspected_not_engaged"),
				Discovery->bHasDiscoveredPowerFailure && !Discovery->bHasBeenEngaged,
				TEXT("inspected / not engaged"),
				FString::Printf(
					TEXT("discovered=%s engaged=%s"),
					*BoolText(Discovery->bHasDiscoveredPowerFailure),
					*BoolText(Discovery->bHasBeenEngaged)),
				MapPanelLabel);

			FProjectOrganoidObjective InvestigationActive;
			AssertTrue(
				Record, TEXT("discovery.investigation_active"),
				Objectives->GetObjective(FName(InvestigationId), InvestigationActive)
					&& InvestigationActive.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				UEnum::GetValueAsString(InvestigationActive.State),
				InvestigationId);
			AssertTrue(
				Record, TEXT("discovery.investigation_journal_once"),
				CountJournalId(Objectives, FName(InvestigationId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(InvestigationId))),
				InvestigationId);

			FProjectOrganoidObjective FollowUpAfterDiscovery;
			Objectives->GetObjective(FName(FollowUpId), FollowUpAfterDiscovery);
			AssertTrue(
				Record, TEXT("discovery.followup_still_inactive"),
				FollowUpAfterDiscovery.State == EProjectOrganoidObjectiveState::Inactive,
				TEXT("Inactive"),
				UEnum::GetValueAsString(FollowUpAfterDiscovery.State),
				FollowUpId);
			AssertTrue(
				Record, TEXT("discovery.followup_journal_hidden"),
				CountJournalId(Objectives, FName(FollowUpId)) == 0,
				TEXT("0"),
				FString::FromInt(CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);

			AssertTrue(
				Record, TEXT("gate.open_when_active"),
				Pad->CanInteract(Character),
				TEXT("true"),
				BoolText(Pad->CanInteract(Character)),
				MapPadLabel);

			AssertTrue(
				Record, TEXT("discovery.neuro_still_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("discovery.cryo_still_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));

			const bool bFirstDiagnosis = Pad->Interact(Character);
			AssertTrue(Record, TEXT("diagnosis.first_interact_ok"), bFirstDiagnosis, TEXT("true"), BoolText(bFirstDiagnosis), MapPadLabel);
			AssertTrue(
				Record, TEXT("diagnosis.pad_marked_read"),
				Pad->bHasBeenRead,
				TEXT("true"), BoolText(Pad->bHasBeenRead), MapPadLabel);

			FProjectOrganoidObjective InvestigationDone;
			AssertTrue(
				Record, TEXT("diagnosis.investigation_completed"),
				Objectives->GetObjective(FName(InvestigationId), InvestigationDone)
					&& InvestigationDone.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				UEnum::GetValueAsString(InvestigationDone.State),
				InvestigationId);
			AssertTrue(
				Record, TEXT("diagnosis.investigation_journal_once"),
				CountJournalId(Objectives, FName(InvestigationId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(InvestigationId))),
				InvestigationId);

			FProjectOrganoidObjective FollowUpActive;
			AssertTrue(
				Record, TEXT("diagnosis.followup_activated"),
				Objectives->GetObjective(FName(FollowUpId), FollowUpActive)
					&& FollowUpActive.State == EProjectOrganoidObjectiveState::Active
					&& FollowUpActive.Title.ToString() == FollowUpTitle,
				TEXT("Active"),
				UEnum::GetValueAsString(FollowUpActive.State),
				FollowUpId);
			AssertTrue(
				Record, TEXT("diagnosis.followup_journal_once"),
				CountJournalId(Objectives, FName(FollowUpId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("diagnosis.followup_is_active_main"),
				Objectives->GetActiveObjectives().ContainsByPredicate(
					[](const FProjectOrganoidObjective& Obj)
					{
						return Obj.ObjectiveId == FName(FollowUpId)
							&& Obj.Type == EProjectOrganoidObjectiveType::Main
							&& Obj.State == EProjectOrganoidObjectiveState::Active;
					}),
				TEXT("follow-up Active Main"),
				TEXT("present"),
				FollowUpId);

			AssertTrue(
				Record, TEXT("diagnosis.mission_not_complete"),
				!Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation")),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation"))),
				TEXT("mission"));

			AssertTrue(
				Record, TEXT("diagnosis.neuro_unchanged"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("diagnosis.cryo_unchanged"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("diagnosis.panel_still_not_engaged"),
				!Discovery->bHasBeenEngaged,
				TEXT("false"),
				BoolText(Discovery->bHasBeenEngaged),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("gate.still_open_when_completed"),
				Pad->CanInteract(Character),
				TEXT("true"),
				BoolText(Pad->CanInteract(Character)),
				MapPadLabel);

			const bool bRepeatDiagnosis = Pad->Interact(Character);
			AssertTrue(Record, TEXT("repeat.interact_ok"), bRepeatDiagnosis, TEXT("true"), BoolText(bRepeatDiagnosis), MapPadLabel);

			FProjectOrganoidObjective InvestigationRepeat;
			Objectives->GetObjective(FName(InvestigationId), InvestigationRepeat);
			AssertTrue(
				Record, TEXT("repeat.investigation_still_completed"),
				InvestigationRepeat.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				UEnum::GetValueAsString(InvestigationRepeat.State),
				InvestigationId);

			FProjectOrganoidObjective FollowUpRepeat;
			Objectives->GetObjective(FName(FollowUpId), FollowUpRepeat);
			AssertTrue(
				Record, TEXT("repeat.followup_still_active"),
				FollowUpRepeat.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				UEnum::GetValueAsString(FollowUpRepeat.State),
				FollowUpId);
			AssertTrue(
				Record, TEXT("repeat.followup_journal_still_once"),
				CountJournalId(Objectives, FName(FollowUpId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("repeat.investigation_journal_still_once"),
				CountJournalId(Objectives, FName(InvestigationId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(InvestigationId))),
				InvestigationId);
			AssertTrue(
				Record, TEXT("repeat.mission_still_incomplete"),
				!Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation")),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation"))),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("repeat.neuro_still_emergency"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("repeat.cryo_blackout"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("repeat.no_restore_event_path"),
				Discovery->SuccessObjectiveEventId == FName(RestoreEvent) && !Discovery->bHasBeenEngaged,
				TEXT("restore reserved / not engaged"),
				FString::Printf(
					TEXT("event=%s engaged=%s"),
					*Discovery->SuccessObjectiveEventId.ToString(),
					*BoolText(Discovery->bHasBeenEngaged)),
				MapPanelLabel);

			Stage = EStage::SaveLoad;
			Owner.SetStage(TEXT("SaveLoad"));
		}

		void TickSaveLoad(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			if (!World || !Character || !Power || !Objectives || !Saves)
			{
				FailAndStop(Owner, Record, TEXT("Missing PIE world/character/power/objectives/saves for SaveLoad."));
				return;
			}

			FProjectOrganoidObjective InvestigationPreSave;
			FProjectOrganoidObjective FollowUpPreSave;
			const bool bReadyToSave =
				Objectives->GetObjective(FName(InvestigationId), InvestigationPreSave)
				&& InvestigationPreSave.State == EProjectOrganoidObjectiveState::Completed
				&& Objectives->GetObjective(FName(FollowUpId), FollowUpPreSave)
				&& FollowUpPreSave.State == EProjectOrganoidObjectiveState::Active
				&& Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency
				&& Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout;
			if (!bReadyToSave)
			{
				FailAndStop(Owner, Record, TEXT("SaveLoad preconditions failed (Completed/Active/Emergency/Blackout)."));
				return;
			}

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));

			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			if (!bSaved || !bLoaded)
			{
				FailAndStop(Owner, Record, TEXT("Save or load failed."));
				return;
			}

			FProjectOrganoidObjective InvestigationLoaded;
			AssertTrue(
				Record, TEXT("saveload.investigation_completed"),
				Objectives->GetObjective(FName(InvestigationId), InvestigationLoaded)
					&& InvestigationLoaded.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				UEnum::GetValueAsString(InvestigationLoaded.State),
				InvestigationId);

			FProjectOrganoidObjective FollowUpLoaded;
			AssertTrue(
				Record, TEXT("saveload.followup_active"),
				Objectives->GetObjective(FName(FollowUpId), FollowUpLoaded)
					&& FollowUpLoaded.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				UEnum::GetValueAsString(FollowUpLoaded.State),
				FollowUpId);
			AssertTrue(
				Record, TEXT("saveload.followup_active_once"),
				CountActiveId(Objectives, FName(FollowUpId)) == 1,
				TEXT("1"),
				FString::FromInt(CountActiveId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("saveload.followup_journal_once"),
				CountJournalId(Objectives, FName(FollowUpId)) == 1,
				TEXT("1"),
				FString::FromInt(CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);

			DestroyTransientPadOnly();
			AProjectOrganoidDataPad* ReloadedPad = SpawnReloadPad(World);
			TestPad = ReloadedPad;
			if (!AssertTrue(
				Record, TEXT("saveload.reloaded_pad_spawned"),
				ReloadedPad != nullptr,
				TEXT("valid"),
				ReloadedPad ? TEXT("valid") : TEXT("null"),
				ReloadedPadLabel))
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn DataPad_NeuroPowerDiagnostics_Reloaded_Test."));
				return;
			}

			AssertTrue(
				Record, TEXT("saveload.gate_allows_completed"),
				ReloadedPad->CanInteract(Character) && !ReloadedPad->bHasBeenRead,
				TEXT("CanInteract unread"),
				FString::Printf(
					TEXT("can=%s read=%s"),
					*BoolText(ReloadedPad->CanInteract(Character)),
					*BoolText(ReloadedPad->bHasBeenRead)),
				ReloadedPadLabel);

			const bool bReloadedInteract = ReloadedPad->Interact(Character);
			AssertTrue(
				Record, TEXT("saveload.reloaded_pad_interact"),
				bReloadedInteract,
				TEXT("true"),
				BoolText(bReloadedInteract),
				ReloadedPadLabel);
			AssertTrue(
				Record, TEXT("saveload.reloaded_pad_read"),
				ReloadedPad->bHasBeenRead,
				TEXT("true"),
				BoolText(ReloadedPad->bHasBeenRead),
				ReloadedPadLabel);

			FProjectOrganoidObjective InvestigationAfterRefire;
			AssertTrue(
				Record, TEXT("saveload.investigation_still_completed"),
				Objectives->GetObjective(FName(InvestigationId), InvestigationAfterRefire)
					&& InvestigationAfterRefire.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"),
				UEnum::GetValueAsString(InvestigationAfterRefire.State),
				InvestigationId);

			FProjectOrganoidObjective FollowUpAfterRefire;
			AssertTrue(
				Record, TEXT("saveload.followup_still_active"),
				Objectives->GetObjective(FName(FollowUpId), FollowUpAfterRefire)
					&& FollowUpAfterRefire.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				UEnum::GetValueAsString(FollowUpAfterRefire.State),
				FollowUpId);
			AssertTrue(
				Record, TEXT("saveload.followup_still_unique"),
				CountActiveId(Objectives, FName(FollowUpId)) == 1
					&& CountJournalId(Objectives, FName(FollowUpId)) == 1,
				TEXT("active=1 journal=1"),
				FString::Printf(
					TEXT("active=%d journal=%d"),
					CountActiveId(Objectives, FName(FollowUpId)),
					CountJournalId(Objectives, FName(FollowUpId))),
				FollowUpId);
			AssertTrue(
				Record, TEXT("saveload.diagnosis_refire_idempotent"),
				InvestigationAfterRefire.State == EProjectOrganoidObjectiveState::Completed
					&& FollowUpAfterRefire.State == EProjectOrganoidObjectiveState::Active
					&& CountActiveId(Objectives, FName(FollowUpId)) == 1
					&& CountJournalId(Objectives, FName(FollowUpId)) == 1,
				TEXT("Completed/Active unique"),
				TEXT("idempotent"),
				DiagnosisEvent);
			AssertTrue(
				Record, TEXT("saveload.mission_incomplete"),
				!Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation")),
				TEXT("false"),
				BoolText(Objectives->IsMissionComplete(TEXT("Mission_OpeningFoundation"))),
				TEXT("mission"));
			AssertTrue(
				Record, TEXT("saveload.power_preserved"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency
					&& Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Emergency+Blackout"),
				FString::Printf(
					TEXT("neuro=%s cryo=%s"),
					*PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
					*PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo))),
				TEXT("Power"));

			Saves->DeleteSave(SaveSlot);

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

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			const TArray<AActor*> EditorPads = OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, MapPadLabel);
			AssertTrue(
				Record, TEXT("durable.map_pad_unique"),
				EditorPads.Num() == 1,
				TEXT("1"),
				FString::FromInt(EditorPads.Num()),
				MapPadLabel);
			AProjectOrganoidDataPad* EditorPad = EditorPads.Num() == 1
				? Cast<AProjectOrganoidDataPad>(EditorPads[0])
				: nullptr;
			AssertTrue(
				Record, TEXT("durable.map_pad_class"),
				EditorPad && EditorPad->GetClass() == AProjectOrganoidDataPad::StaticClass(),
				TEXT("ProjectOrganoidDataPad"),
				EditorPad && EditorPad->GetClass() ? EditorPad->GetClass()->GetName() : TEXT("null"),
				MapPadLabel);
			AssertTrue(
				Record, TEXT("durable.map_pad_neuro_package"),
				EditorPad
					&& OrganoidPlaytestActions::NormalizePackage(OrganoidPlaytestActions::ActorPackage(EditorPad))
						.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage,
				EditorPad
					? OrganoidPlaytestActions::NormalizePackage(OrganoidPlaytestActions::ActorPackage(EditorPad))
					: TEXT("missing"),
				MapPadLabel);

			Record.AddActor(TEXT("dirty_count"), FString::FromInt(DirtyAfter.Num()));
			Stage = EStage::Finalize;
		}
	};

	struct FNeuroPowerFailureDiagnosisAutoRegister
	{
		FNeuroPowerFailureDiagnosisAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroPowerFailureDiagnosisFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroPowerFailureDiagnosisAutoRegister GRegisterNeuroPowerFailureDiagnosis;
}
