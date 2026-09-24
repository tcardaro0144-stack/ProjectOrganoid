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
#include "ProjectOrganoidPowerPanel.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace CryoAccessFunctional
{
	constexpr TCHAR TestId[] = TEXT("CryoAccess_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Cryo Access Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR CryoSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoAccess.DA_Mission_CryoAccess");
	constexpr TCHAR RevelationSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation.DA_Mission_NeuroRevelation");
	constexpr TCHAR CryoMissionId[] = TEXT("Mission_CryoAccess");
	constexpr TCHAR RevelationMissionId[] = TEXT("Mission_NeuroRevelation");
	constexpr TCHAR CryoObjectiveId[] = TEXT("Obj_RestoreCryoPower");
	constexpr TCHAR RevelationObjectiveId[] = TEXT("Obj_ReachNeuroRevelation");
	constexpr TCHAR CryoEvent[] = TEXT("Event_CryoBackupEngaged");
	constexpr TCHAR RevelationEvent[] = TEXT("Event_NeuroRevelationReached");
	constexpr TCHAR PanelLabel[] = TEXT("PowerPanel_CryoBackup");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_FreightAirlock");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidCryoAccessTest");
	constexpr TCHAR ExpectedLine[] = TEXT("Cryo's backup came up. I can go in. I still don't know what they were keeping this cold.");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Engage Cryo Backup");
	const FVector CheckpointLocation(1950.f, 0.f, -2340.f);

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

	class FCryoAccessFunctional : public IOrganoidPlaytestCase
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
			bRequestedCryoStream = false;
			bSecondSession = false;
			bCampaignDone = false;
			bReloadDone = false;
			EditorPanelLocation = FVector::ZeroVector;
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
			Owner.SetStage(TEXT("Preflight"));
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
					bRequestedCryoStream = false;
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
			UProjectOrganoidObjectiveDataAsset* CryoMission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, CryoSoftPath);
			UProjectOrganoidObjectiveDataAsset* Revelation = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RevelationSoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = CryoMission && CryoMission->Tasks.Num() == 1 ? &CryoMission->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.cryo_id"), CryoMission && CryoMission->MissionId == FName(CryoMissionId), CryoMissionId, CryoMission ? CryoMission->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.cryo_title"), CryoMission && CryoMission->MissionTitle.ToString() == TEXT("Open the Cryo Route"), TEXT("Open the Cryo Route"), CryoMission ? CryoMission->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.cryo_description"), CryoMission && CryoMission->MissionDescription.ToString().Contains(TEXT("emergency backup")), TEXT("emergency backup"), CryoMission ? CryoMission->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.cryo_next_null"), CryoMission && CryoMission->NextMissionAsset.IsNull(), TEXT("null"), CryoMission && CryoMission->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), CryoMission && CryoMission->Tasks.Num() == 1, TEXT("1"), CryoMission ? FString::FromInt(CryoMission->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(CryoObjectiveId), CryoObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Restore Cryo Power"), TEXT("Restore Cryo Power"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("Cryo backup")), TEXT("Cryo backup"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(CryoEvent), CryoEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.revelation_next_cryo"), Revelation && Revelation->NextMissionAsset.ToSoftObjectPath().ToString() == CryoSoftPath, CryoSoftPath, Revelation ? Revelation->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Panels = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, PanelLabel) : TArray<AActor*>();
			AProjectOrganoidPowerPanel* Panel = Panels.Num() == 1 ? Cast<AProjectOrganoidPowerPanel>(Panels[0]) : nullptr;
			TArray<AActor*> Checkpoints = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, CheckpointLabel) : TArray<AActor*>();
			AActor* Checkpoint = Checkpoints.Num() == 1 ? Checkpoints[0] : nullptr;
			const FString PanelPackage = Panel && Panel->GetOutermost() ? Panel->GetOutermost()->GetName() : TEXT("missing");
			if (Panel) EditorPanelLocation = Panel->GetActorLocation();
			AssertTrue(Record, TEXT("panel.editor_count"), Panels.Num() == 1, TEXT("1"), FString::FromInt(Panels.Num()), PanelLabel);
			AssertTrue(Record, TEXT("panel.editor_class"), Panel != nullptr, TEXT("PowerPanel"), Panels.Num() == 1 && Panels[0] ? Panels[0]->GetClass()->GetName() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.editor_package"), PanelPackage.Contains(TEXT("SL_Epitope_Cryo")), CryoPackage, PanelPackage, PanelLabel);
			AssertTrue(Record, TEXT("panel.sector"), Panel && Panel->PowerSector == EProjectOrganoidPowerSector::Cryo, TEXT("Cryo"), Panel ? TEXT("other") : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.restored"), Panel && Panel->RestoredState == EProjectOrganoidPowerState::Online, TEXT("Online"), Panel ? PowerText(Panel->RestoredState) : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.required"), Panel && Panel->RequiredActiveObjectiveId == FName(CryoObjectiveId), CryoObjectiveId, Panel ? Panel->RequiredActiveObjectiveId.ToString() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.prompt"), Panel && Panel->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Panel ? Panel->InteractionPrompt.ToString() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.event"), Panel && Panel->SuccessObjectiveEventId == FName(CryoEvent), CryoEvent, Panel ? Panel->SuccessObjectiveEventId.ToString() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.line"), Panel && Panel->RestoreSuccessNotificationText.ToString() == ExpectedLine, ExpectedLine, Panel ? Panel->RestoreSuccessNotificationText.ToString() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.duration"), Panel && FMath::IsNearlyEqual(Panel->RestoreSuccessNotificationDurationSeconds, 6.f), TEXT("6"), Panel ? FString::SanitizeFloat(Panel->RestoreSuccessNotificationDurationSeconds) : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.discovery_gate"), Panel && Panel->bDiscoverPowerFailureBeforeRestore, TEXT("true"), Panel ? BoolText(Panel->bDiscoverPowerFailureBeforeRestore) : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("panel.discovery_event_none"), Panel && Panel->DiscoveryObjectiveEventId.IsNone(), TEXT("None"), Panel ? Panel->DiscoveryObjectiveEventId.ToString() : TEXT("missing"), PanelLabel);
			AssertTrue(Record, TEXT("checkpoint.location"), Checkpoint && Checkpoint->GetActorLocation().Equals(CheckpointLocation, 1.f), TEXT("1950,0,-2340"), Checkpoint ? Checkpoint->GetActorLocation().ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.cryo_clean"), !PackageIsDirty(CryoPackage), TEXT("clean"), PackageIsDirty(CryoPackage) ? TEXT("dirty") : TEXT("clean"), CryoPackage);
			if (bAnyAssertFailed || !CryoMission || !Revelation || !Panel || !Checkpoint)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 13 mission contract missing.") : Record.FailureReason);
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
			bRequestedCryoStream = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedCryoStream)
			{
				const FName CryoName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel3_Cryo);
				if (!CryoName.IsNone())
				{
					Levels->AddStreamRequest(CryoName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedCryoStream = true;
				}
			}
			if (Character && bRequestedCryoStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, PanelLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with PowerPanel_CryoBackup."));
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
			TArray<AActor*> Panels = World ? OrganoidPlaytestActions::FindActorsByLabel(World, PanelLabel) : TArray<AActor*>();
			AProjectOrganoidPowerPanel* Panel = Panels.Num() == 1 ? Cast<AProjectOrganoidPowerPanel>(Panels[0]) : nullptr;
			TArray<AActor*> Checkpoints = World ? OrganoidPlaytestActions::FindActorsByLabel(World, CheckpointLabel) : TArray<AActor*>();
			if (!World || !Character || !Objectives || !Saves || !Power || !Panel || Checkpoints.Num() != 1 || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 13 actors or subsystems missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Emergency);
			const FString PiePackage = Panel->GetOutermost() ? Panel->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("panel.pie_package"), PiePackage.Contains(TEXT("SL_Epitope_Cryo")), CryoPackage, PiePackage, PanelLabel);
			AssertTrue(Record, TEXT("panel.not_moved"), Panel->GetActorLocation().Equals(EditorPanelLocation, 5.f), EditorPanelLocation.ToString(), Panel->GetActorLocation().ToString(), PanelLabel);
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));

			UProjectOrganoidObjectiveDataAsset* Revelation = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RevelationSoftPath);
			const bool bRevelation = Revelation && Objectives->LoadMission(Revelation, false);
			AssertTrue(Record, TEXT("reject.revelation_loaded"), bRevelation && Objectives->GetActiveMissionId() == FName(RevelationMissionId), RevelationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.cryo_not_active"), CountActiveId(Objectives, FName(CryoObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(CryoObjectiveId))), CryoObjectiveId);
			const bool bRejected = Panel->Interact(Character);
			AssertTrue(Record, TEXT("reject.interact"), bRejected, TEXT("true"), BoolText(bRejected), PanelLabel);
			AssertTrue(Record, TEXT("reject.no_credit"), Panel->SuccessEventFireCount == 0 && CountCompletedId(Objectives, FName(CryoObjectiveId)) == 0, TEXT("0"), FString::FromInt(Panel->SuccessEventFireCount), PanelLabel);
			AssertTrue(Record, TEXT("reject.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("reject.not_engaged"), !Panel->bHasBeenEngaged, TEXT("false"), BoolText(Panel->bHasBeenEngaged), PanelLabel);

			const int32 Revealed = Objectives->TriggerEvent(FName(RevelationEvent));
			AssertTrue(Record, TEXT("chain.revelation_event"), Revealed > 0, TEXT(">0"), FString::FromInt(Revealed), RevelationEvent);
			AssertTrue(Record, TEXT("chain.cryo_current"), Objectives->GetActiveMissionId() == FName(CryoMissionId), CryoMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.objective_active"), CountActiveId(Objectives, FName(CryoObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(CryoObjectiveId))), CryoObjectiveId);
			AssertTrue(Record, TEXT("chain.opening_not_complete"), !Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation"))), TEXT("false"), BoolText(Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation")))), TEXT("mission"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bSuccess = Panel->Interact(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.interact"), bSuccess, TEXT("true"), BoolText(bSuccess), PanelLabel);
			AssertTrue(Record, TEXT("success.fire"), Panel->SuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Panel->SuccessEventFireCount), PanelLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(CryoObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(CryoObjectiveId))), CryoObjectiveId);
			AssertTrue(Record, TEXT("success.mission"), Objectives->IsMissionComplete(FName(CryoMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.notification"), Panel->RestoreSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Panel->RestoreSuccessNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 5.0f && Remaining <= 6.0f, TEXT("6"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.no_door"), Checkpoints.Num() == 1 && Checkpoints[0]->GetActorLocation().Equals(CheckpointLocation, 5.f), TEXT("checkpoint"), Checkpoints[0]->GetActorLocation().ToString(), CheckpointLabel);
			AssertTrue(Record, TEXT("success.panel_unmoved"), Panel->GetActorLocation().Equals(EditorPanelLocation, 5.f), EditorPanelLocation.ToString(), Panel->GetActorLocation().ToString(), PanelLabel);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Panel->Interact(Character);
			AssertTrue(Record, TEXT("replay.fire_once"), Panel->SuccessEventFireCount == 1 && Panel->RestoreSuccessNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("fire=%d notes=%d"), Panel->SuccessEventFireCount, Panel->RestoreSuccessNotificationCount), PanelLabel);
			AssertTrue(Record, TEXT("replay.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("replay.line_cleared"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("cleared"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->IsMissionComplete(FName(CryoMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.cryo_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Panels = World ? OrganoidPlaytestActions::FindActorsByLabel(World, PanelLabel) : TArray<AActor*>();
			AProjectOrganoidPowerPanel* Panel = Panels.Num() == 1 ? Cast<AProjectOrganoidPowerPanel>(Panels[0]) : nullptr;
			if (!Objectives || !Saves || !Power || !Panel || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->IsMissionComplete(FName(CryoMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(CryoObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(CryoObjectiveId))), CryoObjectiveId);
			AssertTrue(Record, TEXT("save.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("save.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Panel->Interact(Character);
			AssertTrue(Record, TEXT("save.replay_no_fire"), Panel->SuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Panel->SuccessEventFireCount), PanelLabel);
			AssertTrue(Record, TEXT("save.notification_absent"), Panel->RestoreSuccessNotificationCount == 0, TEXT("0"), FString::FromInt(Panel->RestoreSuccessNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("dirty.reload_cryo"), !PackageIsDirty(CryoPackage), TEXT("clean"), PackageIsDirty(CryoPackage) ? TEXT("dirty") : TEXT("clean"), CryoPackage);
			AssertTrue(Record, TEXT("dirty.reload_admin"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedCryoStream = false;
		bool bSecondSession = false;
		bool bCampaignDone = false;
		bool bReloadDone = false;
		FVector EditorPanelLocation = FVector::ZeroVector;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FCryoAccessFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister GRegister;
}
