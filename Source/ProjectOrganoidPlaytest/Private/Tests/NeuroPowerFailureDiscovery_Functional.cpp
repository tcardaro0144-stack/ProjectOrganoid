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
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "Components/PointLightComponent.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("NeuroPowerFailureDiscovery_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Power Failure Discovery Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR MapPanelLabel[] = TEXT("PowerPanel_NeuroBackup");

	constexpr TCHAR ExpectedStatus[] = TEXT("PRIMARY FEED OFFLINE — EMERGENCY BACKUP ACTIVE");
	constexpr TCHAR ExpectedInspectPrompt[] = TEXT("Inspect Power Controls");
	constexpr TCHAR ExpectedReviewPrompt[] = TEXT("Review backup power status");
	constexpr TCHAR DiscoveryEvent[] = TEXT("Event_NeuroPowerFailureDiscovered");
	constexpr TCHAR RestoreEvent[] = TEXT("Event_NeuroPowerRestored");
	constexpr TCHAR ObjectiveId[] = TEXT("Obj_InvestigateNeuroPowerFailure");
	constexpr TCHAR ObjectiveTitle[] = TEXT("Investigate the NeuroGenetics power failure");
	const FVector ExpectedPanelLocation(-500.0f, -2275.0f, -1100.0f);

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

	bool LocationMatches(const FVector& Live, const FVector& Expected, float Tol = 1.0f)
	{
		return Live.Equals(Expected, Tol);
	}

	class FNeuroPowerFailureDiscoveryFunctional : public IOrganoidPlaytestCase
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
			RestoreControlPanel.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroySpawnedControl();
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
				DestroySpawnedControl();
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
		TWeakObjectPtr<AProjectOrganoidPowerPanel> MapDiscoveryPanel;
		TWeakObjectPtr<AProjectOrganoidPowerPanel> RestoreControlPanel;

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

		void DestroySpawnedControl()
		{
			if (AProjectOrganoidPowerPanel* Panel = RestoreControlPanel.Get())
			{
				Panel->Destroy();
			}
			RestoreControlPanel.Reset();
			// Never destroy the saved map actor PowerPanel_NeuroBackup.
			MapDiscoveryPanel.Reset();
		}

		AProjectOrganoidPowerPanel* SpawnLegacyRestoreControl(UWorld* World, const FVector& Location)
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

			Panel->SetActorLabel(TEXT("PowerPanel_RestoreDefaultCase"));
			Panel->PowerSector = EProjectOrganoidPowerSector::NeuroGenetics;
			Panel->RestoredState = EProjectOrganoidPowerState::Online;
			Panel->bSingleUse = true;
			Panel->bDiscoverPowerFailureBeforeRestore = false;
			Panel->SuccessObjectiveEventId = FName(RestoreEvent);
			Panel->InteractionPrompt = FText::FromString(TEXT("Engage Backup Power"));
			Panel->FinishSpawning(FTransform(Location));
			return Panel;
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
			const bool bPanelReady = PanelMatches.Num() == 1;

			if (Character && bPlayerReady && bPanelReady)
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
						TEXT("Timed out waiting for PIE player and PowerPanel_NeuroBackup. neuro_loaded=%s neuro_visible=%s panel_count=%d streams=%s"),
						bNeuroLoaded ? TEXT("true") : TEXT("false"),
						bNeuroVisible ? TEXT("true") : TEXT("false"),
						PanelMatches.Num(),
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

			const EProjectOrganoidPowerState NeuroBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics);
			const EProjectOrganoidPowerState CryoBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo);
			AssertTrue(
				Record, TEXT("seed.neuro_emergency"),
				NeuroBefore == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"), PowerStateName(NeuroBefore), TEXT("Power"));
			AssertTrue(
				Record, TEXT("seed.cryo_blackout"),
				CryoBefore == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"), PowerStateName(CryoBefore), TEXT("Power"));

			FProjectOrganoidObjective ObjBefore;
			const bool bHadObj = Objectives->GetObjective(FName(ObjectiveId), ObjBefore);
			AssertTrue(
				Record, TEXT("objective.registered_inactive"),
				bHadObj && ObjBefore.State == EProjectOrganoidObjectiveState::Inactive
					&& ObjBefore.Title.ToString() == ObjectiveTitle,
				TEXT("Inactive titled objective"),
				bHadObj ? UEnum::GetValueAsString(ObjBefore.State) + TEXT(" / ") + ObjBefore.Title.ToString() : TEXT("missing"),
				ObjectiveId);

			{
				int32 NeuroJournalCount = 0;
				for (const FProjectOrganoidObjective& Entry : Objectives->GetJournalEntries())
				{
					if (Entry.ObjectiveId == FName(ObjectiveId))
					{
						++NeuroJournalCount;
					}
				}
				AssertTrue(
					Record, TEXT("objective.journal_hidden_before_discovery"),
					NeuroJournalCount == 0,
					TEXT("0"), FString::FromInt(NeuroJournalCount), ObjectiveId);
			}

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
				FailAndStop(
					Owner, Record,
					TEXT("PowerPanel_NeuroBackup missing or not AProjectOrganoidPowerPanel. Configure map before this test can pass."));
				return;
			}

			const FString LiveLabel = OrganoidPlaytestActions::ActorLabel(Discovery);
			AssertTrue(
				Record, TEXT("map.panel_exact_label"),
				LiveLabel.Equals(MapPanelLabel, ESearchCase::CaseSensitive),
				MapPanelLabel, LiveLabel, MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.panel_native_class"),
				Discovery->GetClass() == AProjectOrganoidPowerPanel::StaticClass(),
				TEXT("ProjectOrganoidPowerPanel"),
				Discovery->GetClass() ? Discovery->GetClass()->GetName() : TEXT("null"),
				MapPanelLabel);

			const FString LivePackage = OrganoidPlaytestActions::NormalizePackage(
				OrganoidPlaytestActions::ActorPackage(Discovery));
			AssertTrue(
				Record, TEXT("map.panel_neuro_package"),
				LivePackage.Equals(NeuroPackage, ESearchCase::CaseSensitive),
				NeuroPackage, LivePackage, MapPanelLabel);

			const FVector LiveLocation = Discovery->GetActorLocation();
			AssertTrue(
				Record, TEXT("map.panel_location"),
				LocationMatches(LiveLocation, ExpectedPanelLocation),
				ExpectedPanelLocation.ToCompactString(),
				LiveLocation.ToCompactString(),
				MapPanelLabel);

			AssertTrue(
				Record, TEXT("map.discovery_opt_in"),
				Discovery->bDiscoverPowerFailureBeforeRestore,
				TEXT("true"), BoolText(Discovery->bDiscoverPowerFailureBeforeRestore), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.inspect_prompt"),
				Discovery->GetInteractionPrompt().ToString() == ExpectedInspectPrompt,
				ExpectedInspectPrompt, Discovery->GetInteractionPrompt().ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.review_prompt"),
				Discovery->ReviewPrompt.ToString() == ExpectedReviewPrompt,
				ExpectedReviewPrompt, Discovery->ReviewPrompt.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.discovery_event_id"),
				Discovery->DiscoveryObjectiveEventId == FName(DiscoveryEvent),
				DiscoveryEvent, Discovery->DiscoveryObjectiveEventId.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.failure_status_text"),
				Discovery->FailureStatusReport.ToString() == ExpectedStatus,
				ExpectedStatus, Discovery->FailureStatusReport.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restored_state_online"),
				Discovery->RestoredState == EProjectOrganoidPowerState::Online,
				TEXT("Online"), PowerStateName(Discovery->RestoredState), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.power_sector_neuro"),
				Discovery->PowerSector == EProjectOrganoidPowerSector::NeuroGenetics,
				TEXT("NeuroGenetics"), TEXT("NeuroGenetics"), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.restore_event_id"),
				Discovery->SuccessObjectiveEventId == FName(RestoreEvent),
				RestoreEvent, Discovery->SuccessObjectiveEventId.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.not_engaged"),
				!Discovery->bHasBeenEngaged,
				TEXT("false"), BoolText(Discovery->bHasBeenEngaged), MapPanelLabel);
			AssertTrue(
				Record, TEXT("map.not_inspected"),
				!Discovery->bHasDiscoveredPowerFailure,
				TEXT("false"), BoolText(Discovery->bHasDiscoveredPowerFailure), MapPanelLabel);

			if (!Discovery->bDiscoverPowerFailureBeforeRestore)
			{
				FailAndStop(
					Owner, Record,
					TEXT("Saved PowerPanel_NeuroBackup is not discovery-configured yet. Run configure_neuro_power_failure_discovery then Neuro-only save."));
				return;
			}

			const FLinearColor LightBefore = Discovery->StatusLight ? Discovery->StatusLight->GetLightColor() : FLinearColor::Black;
			const bool bFirstOk = Discovery->Interact(Character);
			AssertTrue(Record, TEXT("discovery.first_interact_ok"), bFirstOk, TEXT("true"), BoolText(bFirstOk), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.inspected_not_engaged"),
				Discovery->bHasDiscoveredPowerFailure && !Discovery->bHasBeenEngaged,
				TEXT("inspected / not engaged"),
				FString::Printf(
					TEXT("discovered=%s engaged=%s"),
					*BoolText(Discovery->bHasDiscoveredPowerFailure),
					*BoolText(Discovery->bHasBeenEngaged)),
				MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.status_text"),
				Discovery->LastStatusReport.ToString() == ExpectedStatus,
				ExpectedStatus, Discovery->LastStatusReport.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.event_once"),
				Discovery->DiscoveryEventFireCount == 1,
				TEXT("1"), FString::FromInt(Discovery->DiscoveryEventFireCount), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.status_count_1"),
				Discovery->StatusReportCount == 1,
				TEXT("1"), FString::FromInt(Discovery->StatusReportCount), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.prompt_review"),
				Discovery->GetInteractionPrompt().ToString() == ExpectedReviewPrompt,
				ExpectedReviewPrompt, Discovery->GetInteractionPrompt().ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.still_interactable"),
				Discovery->bIsInteractable,
				TEXT("true"), BoolText(Discovery->bIsInteractable), MapPanelLabel);

			const EProjectOrganoidPowerState NeuroAfterFirst = Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics);
			const EProjectOrganoidPowerState CryoAfterFirst = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo);
			AssertTrue(
				Record, TEXT("discovery.neuro_still_emergency"),
				NeuroAfterFirst == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"), PowerStateName(NeuroAfterFirst), TEXT("Power"));
			AssertTrue(
				Record, TEXT("discovery.cryo_still_blackout"),
				CryoAfterFirst == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"), PowerStateName(CryoAfterFirst), TEXT("Power"));

			if (Discovery->StatusLight)
			{
				const FLinearColor LightAfter = Discovery->StatusLight->GetLightColor();
				const bool bStillAmber =
					FMath::IsNearlyEqual(LightAfter.R, LightBefore.R, 0.05f)
					&& FMath::IsNearlyEqual(LightAfter.G, LightBefore.G, 0.05f)
					&& FMath::IsNearlyEqual(LightAfter.B, LightBefore.B, 0.05f);
				AssertTrue(Record, TEXT("discovery.light_not_green"), bStillAmber, TEXT("amber retained"), LightAfter.ToString(), MapPanelLabel);
			}

			FProjectOrganoidObjective ObjAfter;
			const bool bObjActive = Objectives->GetObjective(FName(ObjectiveId), ObjAfter)
				&& ObjAfter.State == EProjectOrganoidObjectiveState::Active;
			AssertTrue(
				Record, TEXT("discovery.objective_activated_once"),
				bObjActive && ObjAfter.Title.ToString() == ObjectiveTitle && ObjAfter.CurrentProgress == 0,
				TEXT("Active progress 0"),
				bObjActive
					? FString::Printf(TEXT("%s progress=%d"), *UEnum::GetValueAsString(ObjAfter.State), ObjAfter.CurrentProgress)
					: TEXT("missing"),
				ObjectiveId);

			{
				int32 NeuroJournalCount = 0;
				for (const FProjectOrganoidObjective& Entry : Objectives->GetJournalEntries())
				{
					if (Entry.ObjectiveId == FName(ObjectiveId))
					{
						++NeuroJournalCount;
					}
				}
				AssertTrue(
					Record, TEXT("discovery.objective_journal_visible_once"),
					NeuroJournalCount == 1,
					TEXT("1"), FString::FromInt(NeuroJournalCount), ObjectiveId);
			}

			if (UProjectOrganoidLogComponent* Logs = Character->GetLogComponent())
			{
				FProjectOrganoidLogEntry Entry;
				const bool bHasLog = Logs->GetEntry(TEXT("Log_NeuroPowerFailureStatus"), Entry)
					&& Entry.Body.ToString() == ExpectedStatus;
				AssertTrue(Record, TEXT("discovery.log_entry"), bHasLog, ExpectedStatus, Entry.Body.ToString(), TEXT("log"));
			}

			const bool bSecondOk = Discovery->Interact(Character);
			AssertTrue(Record, TEXT("discovery.review_interact_ok"), bSecondOk, TEXT("true"), BoolText(bSecondOk), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.event_still_once"),
				Discovery->DiscoveryEventFireCount == 1,
				TEXT("1"), FString::FromInt(Discovery->DiscoveryEventFireCount), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.status_count_2"),
				Discovery->StatusReportCount == 2,
				TEXT("2"), FString::FromInt(Discovery->StatusReportCount), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.review_status_same"),
				Discovery->LastStatusReport.ToString() == ExpectedStatus,
				ExpectedStatus, Discovery->LastStatusReport.ToString(), MapPanelLabel);
			AssertTrue(
				Record, TEXT("discovery.still_not_engaged"),
				!Discovery->bHasBeenEngaged,
				TEXT("false"), BoolText(Discovery->bHasBeenEngaged), MapPanelLabel);

			FProjectOrganoidObjective ObjReview;
			Objectives->GetObjective(FName(ObjectiveId), ObjReview);
			AssertTrue(
				Record, TEXT("discovery.objective_not_duplicated"),
				ObjReview.State == EProjectOrganoidObjectiveState::Active && ObjReview.CurrentProgress == 0,
				TEXT("Active progress 0"),
				FString::Printf(TEXT("%s progress=%d"), *UEnum::GetValueAsString(ObjReview.State), ObjReview.CurrentProgress),
				ObjectiveId);

			{
				int32 NeuroJournalCount = 0;
				for (const FProjectOrganoidObjective& Entry : Objectives->GetJournalEntries())
				{
					if (Entry.ObjectiveId == FName(ObjectiveId))
					{
						++NeuroJournalCount;
					}
				}
				AssertTrue(
					Record, TEXT("review.objective_journal_still_once"),
					NeuroJournalCount == 1,
					TEXT("1"), FString::FromInt(NeuroJournalCount), ObjectiveId);
			}

			AssertTrue(
				Record, TEXT("discovery.neuro_unchanged_after_review"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Emergency,
				TEXT("Emergency"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("discovery.cryo_unchanged_after_review"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("discovery.no_restored_event_path"),
				Discovery->SuccessObjectiveEventId == FName(RestoreEvent) && !Discovery->bHasBeenEngaged,
				TEXT("restore event reserved / not engaged"),
				FString::Printf(TEXT("event=%s engaged=%s"), *Discovery->SuccessObjectiveEventId.ToString(), *BoolText(Discovery->bHasBeenEngaged)),
				MapPanelLabel);

			// Separate spawned default-mode control proves legacy restore-on-interact still works.
			AProjectOrganoidPowerPanel* Restore = SpawnLegacyRestoreControl(World, FVector(-300.0f, -2100.0f, -1100.0f));
			RestoreControlPanel = Restore;
			if (!AssertTrue(Record, TEXT("restore.control_spawned"), Restore != nullptr, TEXT("valid"), Restore ? TEXT("valid") : TEXT("null"), TEXT("restore")))
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn legacy restore control panel."));
				return;
			}

			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			const bool bRestoreOk = Restore->Interact(Character);
			AssertTrue(Record, TEXT("restore.default_interact_ok"), bRestoreOk, TEXT("true"), BoolText(bRestoreOk), TEXT("restore"));
			AssertTrue(
				Record, TEXT("restore.sets_online"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("Online"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)),
				TEXT("restore"));
			AssertTrue(
				Record, TEXT("restore.engaged"),
				Restore->bHasBeenEngaged && !Restore->bIsInteractable,
				TEXT("engaged"),
				BoolText(Restore->bHasBeenEngaged),
				TEXT("restore"));
			AssertTrue(
				Record, TEXT("restore.cryo_untouched"),
				Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout,
				TEXT("Blackout"),
				PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)),
				TEXT("Power"));
			AssertTrue(
				Record, TEXT("map.discovery_untouched_by_restore_control"),
				Discovery->bHasDiscoveredPowerFailure && !Discovery->bHasBeenEngaged
					&& Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online,
				TEXT("map panel discovery-only; sector Online via control"),
				FString::Printf(
					TEXT("discovered=%s engaged=%s neuro=%s"),
					*BoolText(Discovery->bHasDiscoveredPowerFailure),
					*BoolText(Discovery->bHasBeenEngaged),
					*PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics))),
				MapPanelLabel);

			// Restore seed power so durable package assert is the only post-PIE check.
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

	struct FNeuroPowerFailureDiscoveryAutoRegister
	{
		FNeuroPowerFailureDiscoveryAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroPowerFailureDiscoveryFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroPowerFailureDiscoveryAutoRegister GRegisterNeuroPowerFailureDiscovery;
}
