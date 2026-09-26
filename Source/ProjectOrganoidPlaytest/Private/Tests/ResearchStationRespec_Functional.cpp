#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace ResearchStationRespecFunctional
{
	constexpr TCHAR TestId[] = TEXT("ResearchStationRespec_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Research Station Respec Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR ResearchSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ResearchStation.DA_Mission_ResearchStation");
	constexpr TCHAR SyringeSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_SyringeKit.DA_Mission_SyringeKit");
	constexpr TCHAR SyringeMissionId[] = TEXT("Mission_SyringeKit");
	constexpr TCHAR ConclusionSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion.DA_Mission_TheConclusion");
	constexpr TCHAR HandoverSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeHandover.DA_Mission_ComputeHandover");
	constexpr TCHAR ResearchMissionId[] = TEXT("Mission_ResearchStation");
	constexpr TCHAR ConclusionMissionId[] = TEXT("Mission_TheConclusion");
	constexpr TCHAR ResearchObjectiveId[] = TEXT("Obj_UseResearchStation");
	constexpr TCHAR ConclusionObjectiveId[] = TEXT("Obj_ReachControlSpine");
	constexpr TCHAR ResearchEvent[] = TEXT("Event_ResearchStationUsed");
	constexpr TCHAR ConclusionEvent[] = TEXT("Event_ReactorControlUsed");
	constexpr TCHAR IntroObjectiveId[] = TEXT("Obj_EquipNeuralSlow");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidResearchStationRespecTest");
	constexpr TCHAR ExpectedLine[] = TEXT("Free respec, no penalty. This is where I rethink the build. No currency, no shop — just reconfiguration.");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Use Research Station");
	const FVector StationLocation(800.f, -1600.f, -1100.f);

	FString BoolText(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }
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
	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path)) return Package->IsDirty();
		return false;
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

	class FResearchStationRespecFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }
		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			bSecondSession = false;
			bCampaignDone = false;
			bReloadDone = false;
			Owner.SetStage(TEXT("Preflight"));
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
		}
		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
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
				if (WaitSeconds > 20.f) Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("First PIE session did not end."));
				break;
			case EStage::EndPie:
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
		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty()) Record.FailureReason = Reason;
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			(void)Owner;
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
		void StopIfFailed() { if (bAnyAssertFailed) Stage = EStage::EndPie; }
		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUD ? HUD->GetBoundHUDWidget() : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Research = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ResearchSoftPath);
			UProjectOrganoidObjectiveDataAsset* Conclusion = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConclusionSoftPath);
			UProjectOrganoidObjectiveDataAsset* Handover = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, HandoverSoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Research && Research->Tasks.Num() == 1 ? &Research->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.research_id"), Research && Research->MissionId == FName(ResearchMissionId), ResearchMissionId, Research ? Research->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.research_title"), Research && Research->MissionTitle.ToString() == TEXT("Research Station"), TEXT("Research Station"), Research ? Research->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.research_description"), Research && Research->MissionDescription.ToString().Contains(TEXT("rethink your build")), TEXT("rethink your build"), Research ? Research->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.research_next_syringe"), Research && Research->NextMissionAsset.ToSoftObjectPath().ToString() == SyringeSoftPath, SyringeSoftPath, Research ? Research->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), Research && Research->Tasks.Num() == 1, TEXT("1"), Research ? FString::FromInt(Research->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(ResearchObjectiveId), ResearchObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Use Research Station"), TEXT("Use Research Station"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("Research Station")), TEXT("Research Station"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(ResearchEvent), ResearchEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.conclusion_next_research"), Conclusion && Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() == ResearchSoftPath, ResearchSoftPath, Conclusion ? Conclusion->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.handover_next_conclusion"), Handover && Handover->NextMissionAsset.ToSoftObjectPath().ToString() == ConclusionSoftPath, ConclusionSoftPath, Handover ? Handover->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Found = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, StationLabel) : TArray<AActor*>();
			AProjectOrganoidResearchStation* Station = Found.Num() == 1 ? Cast<AProjectOrganoidResearchStation>(Found[0]) : nullptr;
			const FString PackageName = Station && Station->GetOutermost() ? Station->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("station.count"), Found.Num() == 1, TEXT("1"), FString::FromInt(Found.Num()), StationLabel);
			AssertTrue(Record, TEXT("station.class"), Station != nullptr, TEXT("ProjectOrganoidResearchStation"), Found.Num() == 1 && Found[0] ? Found[0]->GetClass()->GetName() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.package"), PackageName.Contains(TEXT("SL_Epitope_NeuroGenetics")), NeuroPackage, PackageName, StationLabel);
			AssertTrue(Record, TEXT("station.location"), Station && Station->GetActorLocation().Equals(StationLocation, 1.f), TEXT("800,-1600,-1100"), Station ? Station->GetActorLocation().ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.prompt"), Station && Station->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Station ? Station->InteractionPrompt.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.objective"), Station && Station->RespecRequiredActiveObjectiveId == FName(ResearchObjectiveId), ResearchObjectiveId, Station ? Station->RespecRequiredActiveObjectiveId.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.event"), Station && Station->RespecSuccessObjectiveEventId == FName(ResearchEvent), ResearchEvent, Station ? Station->RespecSuccessObjectiveEventId.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.replay_guard"), Station && Station->RespecReplayGuardObjectiveId == FName(ResearchObjectiveId), ResearchObjectiveId, Station ? Station->RespecReplayGuardObjectiveId.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.speaker"), Station && Station->RespecNotificationSpeaker.ToString() == TEXT("Nathan"), TEXT("Nathan"), Station ? Station->RespecNotificationSpeaker.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.line"), Station && Station->RespecNotificationText.ToString() == ExpectedLine, ExpectedLine, Station ? Station->RespecNotificationText.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.duration"), Station && FMath::IsNearlyEqual(Station->RespecNotificationDurationSeconds, 7.f), TEXT("7"), Station ? FString::SanitizeFloat(Station->RespecNotificationDurationSeconds) : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.intro_kept"), Station && Station->CampaignRequiredActiveObjectiveId == FName(IntroObjectiveId), IntroObjectiveId, Station ? Station->CampaignRequiredActiveObjectiveId.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.zero_cost"), Station && Station->GetConfigurationSOTCost() == 0, TEXT("0"), Station ? FString::FromInt(Station->GetConfigurationSOTCost()) : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.neuro_clean"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			if (bAnyAssertFailed || !Research || !Conclusion || !Handover || !Station)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 19 mission contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedNeuroStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with ResearchStation_NeuroGenetics."));
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidHUDWidget* Widget = FindHud(World, Character);
			if (!Widget)
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds > 10.f) FailAndStop(Owner, Record, TEXT("HUD was not ready."));
				return;
			}
			if (bSecondSession)
			{
				if (!bReloadDone) TickReload(Owner, Record, World, Character, Widget);
				return;
			}
			if (!bCampaignDone)
			{
				bCampaignDone = true;
				TickCampaign(Owner, Record, World, Character, Widget);
			}
		}

		void TickCampaign(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel) : TArray<AActor*>();
			AProjectOrganoidResearchStation* Station = Found.Num() == 1 ? Cast<AProjectOrganoidResearchStation>(Found[0]) : nullptr;
			if (!World || !Character || !Objectives || !Saves || !Power || !Station || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 19 actors or subsystems missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Emergency);
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("station.unmoved"), Station->GetActorLocation().Equals(StationLocation, 5.f), TEXT("800,-1600,-1100"), Station->GetActorLocation().ToString(), StationLabel);
			AssertTrue(Record, TEXT("station.prompt_live"), Station->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Station->InteractionPrompt.ToString(), StationLabel);
			AssertTrue(Record, TEXT("station.idle_open"), Station->CanInteract(Character) && !Station->IsLockedByEncounter(), TEXT("true"), BoolText(Station->CanInteract(Character)), StationLabel);

			UProjectOrganoidObjectiveDataAsset* Conclusion = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConclusionSoftPath);
			const bool bConclusionLoaded = Conclusion && Objectives->LoadMission(Conclusion, false);
			AssertTrue(Record, TEXT("reject.conclusion_loaded"), bConclusionLoaded && Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.use_not_active"), CountActiveId(Objectives, FName(ResearchObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(ResearchObjectiveId))), ResearchObjectiveId);
			const bool bEarlyInteract = Station->Interact(Character);
			Station->CloseResearchStationUI();
			AssertTrue(Record, TEXT("reject.interact"), bEarlyInteract, TEXT("true"), BoolText(bEarlyInteract), StationLabel);
			AssertTrue(Record, TEXT("reject.no_credit"), Station->RespecEventFireCount == 0 && Station->RespecNotificationCount == 0, TEXT("0"), FString::FromInt(Station->RespecEventFireCount), StationLabel);
			AssertTrue(Record, TEXT("reject.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));

			Objectives->TriggerEvent(FName(ConclusionEvent));
			AssertTrue(Record, TEXT("chain.research_current"), Objectives->GetActiveMissionId() == FName(ResearchMissionId), ResearchMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.objective_active"), CountActiveId(Objectives, FName(ResearchObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(ResearchObjectiveId))), ResearchObjectiveId);
			AssertTrue(Record, TEXT("chain.spine_completed"), CountCompletedId(Objectives, FName(ConclusionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConclusionObjectiveId))), ConclusionObjectiveId);
			AssertTrue(Record, TEXT("chain.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bUsed = Station->Interact(Character);
			Station->CloseResearchStationUI();
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.interact"), bUsed, TEXT("true"), BoolText(bUsed), StationLabel);
			AssertTrue(Record, TEXT("success.fire"), Station->RespecEventFireCount == 1, TEXT("1"), FString::FromInt(Station->RespecEventFireCount), StationLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(ResearchObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ResearchObjectiveId))), ResearchObjectiveId);
			AssertTrue(Record, TEXT("success.mission"), Objectives->GetActiveMissionId() == FName(SyringeMissionId) && CountCompletedId(Objectives, FName(ResearchObjectiveId)) == 1, SyringeMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.notification"), Station->RespecNotificationCount == 1, TEXT("1"), FString::FromInt(Station->RespecNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.unmoved"), Found.Num() == 1 && Station->GetActorLocation().Equals(StationLocation, 5.f), TEXT("800,-1600,-1100"), Station->GetActorLocation().ToString(), StationLabel);
			AssertTrue(Record, TEXT("success.intro_kept"), Station->CampaignRequiredActiveObjectiveId == FName(IntroObjectiveId), IntroObjectiveId, Station->CampaignRequiredActiveObjectiveId.ToString(), StationLabel);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Station->Interact(Character);
			Station->CloseResearchStationUI();
			AssertTrue(Record, TEXT("replay.fire_once"), Station->RespecEventFireCount == 1 && Station->RespecNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("fire=%d notes=%d"), Station->RespecEventFireCount, Station->RespecNotificationCount), StationLabel);
			AssertTrue(Record, TEXT("replay.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("replay.line_cleared"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("cleared"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->GetActiveMissionId() == FName(SyringeMissionId), SyringeMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.neuro_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel) : TArray<AActor*>();
			AProjectOrganoidResearchStation* Station = Found.Num() == 1 ? Cast<AProjectOrganoidResearchStation>(Found[0]) : nullptr;
			if (!Objectives || !Saves || !Power || !Station || !Character || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Blackout);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(SyringeMissionId), SyringeMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(ResearchObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ResearchObjectiveId))), ResearchObjectiveId);
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("save"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Station->Interact(Character);
			Station->CloseResearchStationUI();
			AssertTrue(Record, TEXT("save.replay_no_line"), Station->RespecNotificationCount == 0 && Station->RespecEventFireCount == 0, TEXT("0"), FString::FromInt(Station->RespecNotificationCount), StationLabel);
			AssertTrue(Record, TEXT("save.unmoved"), Station->GetActorLocation().Equals(StationLocation, 5.f), TEXT("800,-1600,-1100"), Station->GetActorLocation().ToString(), StationLabel);
			AssertTrue(Record, TEXT("dirty.reload_neuro"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			AssertTrue(Record, TEXT("dirty.reload_admin"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bSecondSession = false;
		bool bCampaignDone = false;
		bool bReloadDone = false;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FResearchStationRespecFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterResearchStationRespecFunctional;
}
