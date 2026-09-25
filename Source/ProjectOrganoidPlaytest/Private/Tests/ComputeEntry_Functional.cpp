#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace ComputeEntryFunctional
{
	constexpr TCHAR TestId[] = TEXT("ComputeEntry_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Compute Entry Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR EntrySoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeEntry.DA_Mission_ComputeEntry");
	constexpr TCHAR EvidenceSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoEvidence.DA_Mission_CryoEvidence");
	constexpr TCHAR CryoEntrySoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoEntry.DA_Mission_CryoEntry");
	constexpr TCHAR EntryMissionId[] = TEXT("Mission_ComputeEntry");
	constexpr TCHAR EvidenceMissionId[] = TEXT("Mission_CryoEvidence");
	constexpr TCHAR CryoEntryMissionId[] = TEXT("Mission_CryoEntry");
	constexpr TCHAR EntryObjectiveId[] = TEXT("Obj_EnterCompute");
	constexpr TCHAR EvidenceObjectiveId[] = TEXT("Obj_RecoverCryoEvidence");
	constexpr TCHAR EntryEvent[] = TEXT("Event_ComputeEntered");
	constexpr TCHAR EvidenceEvent[] = TEXT("Event_CryoEvidenceRecovered");
	constexpr TCHAR CryoEntryEvent[] = TEXT("Event_CryoEntered");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_InterfaceChamber");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidComputeEntryTest");
	constexpr TCHAR ExpectedLine[] = TEXT("The compute substrate is still running. It's been running the whole lockdown.");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Enter Compute");
	const FVector CheckpointLocation(-2425.f, -1650.f, -3540.f);

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

	class FComputeEntryFunctional : public IOrganoidPlaytestCase
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
			bRequestedComputeStream = false;
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
					bRequestedComputeStream = false;
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
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			UProjectOrganoidObjectiveDataAsset* Evidence = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EvidenceSoftPath);
			UProjectOrganoidObjectiveDataAsset* CryoEntry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, CryoEntrySoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Entry && Entry->Tasks.Num() == 1 ? &Entry->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.entry_id"), Entry && Entry->MissionId == FName(EntryMissionId), EntryMissionId, Entry ? Entry->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_title"), Entry && Entry->MissionTitle.ToString() == TEXT("The Substrate"), TEXT("The Substrate"), Entry ? Entry->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_description"), Entry && Entry->MissionDescription.ToString().Contains(TEXT("wake the interface")), TEXT("wake the interface"), Entry ? Entry->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_next_null"), Entry && Entry->NextMissionAsset.IsNull(), TEXT("null"), Entry && Entry->NextMissionAsset.IsNull() ? TEXT("null") : Entry->NextMissionAsset.ToSoftObjectPath().ToString(), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), Entry && Entry->Tasks.Num() == 1, TEXT("1"), Entry ? FString::FromInt(Entry->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(EntryObjectiveId), EntryObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Enter Compute"), TEXT("Enter Compute"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("compute wing")), TEXT("compute wing"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(EntryEvent), EntryEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.evidence_next_entry"), Evidence && Evidence->NextMissionAsset.ToSoftObjectPath().ToString() == EntrySoftPath, EntrySoftPath, Evidence ? Evidence->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.cryo_entry_next_evidence"), CryoEntry && CryoEntry->NextMissionAsset.ToSoftObjectPath().ToString() == EvidenceSoftPath, EvidenceSoftPath, CryoEntry ? CryoEntry->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Found = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, CheckpointLabel) : TArray<AActor*>();
			AProjectOrganoidCheckpoint* Checkpoint = Found.Num() == 1 ? Cast<AProjectOrganoidCheckpoint>(Found[0]) : nullptr;
			const FString PackageName = Checkpoint && Checkpoint->GetOutermost() ? Checkpoint->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("checkpoint.count"), Found.Num() == 1, TEXT("1"), FString::FromInt(Found.Num()), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.class"), Checkpoint != nullptr, TEXT("Checkpoint"), Found.Num() == 1 && Found[0] ? Found[0]->GetClass()->GetName() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.package"), PackageName.Contains(TEXT("SL_Epitope_Compute")), ComputePackage, PackageName, CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.location"), Checkpoint && Checkpoint->GetActorLocation().Equals(CheckpointLocation, 1.f), TEXT("-2425,-1650,-3540"), Checkpoint ? Checkpoint->GetActorLocation().ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.objective"), Checkpoint && Checkpoint->CampaignRequiredActiveObjectiveId == FName(EntryObjectiveId), EntryObjectiveId, Checkpoint ? Checkpoint->CampaignRequiredActiveObjectiveId.ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.event"), Checkpoint && Checkpoint->CampaignSuccessEventId == FName(EntryEvent), EntryEvent, Checkpoint ? Checkpoint->CampaignSuccessEventId.ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.prompt"), Checkpoint && Checkpoint->CampaignEntryPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Checkpoint ? Checkpoint->CampaignEntryPrompt.ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.speaker"), Checkpoint && Checkpoint->CampaignNotificationSpeaker.ToString() == TEXT("Nathan"), TEXT("Nathan"), Checkpoint ? Checkpoint->CampaignNotificationSpeaker.ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.line"), Checkpoint && Checkpoint->CampaignNotificationText.ToString() == ExpectedLine, ExpectedLine, Checkpoint ? Checkpoint->CampaignNotificationText.ToString() : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.duration"), Checkpoint && FMath::IsNearlyEqual(Checkpoint->CampaignNotificationDurationSeconds, 7.f), TEXT("7"), Checkpoint ? FString::SanitizeFloat(Checkpoint->CampaignNotificationDurationSeconds) : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.save_slot_default"), Checkpoint && Checkpoint->SaveSlotOverride.IsEmpty(), TEXT("empty"), Checkpoint ? Checkpoint->SaveSlotOverride : TEXT("missing"), CheckpointLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.compute_clean"), !PackageIsDirty(ComputePackage), TEXT("clean"), PackageIsDirty(ComputePackage) ? TEXT("dirty") : TEXT("clean"), ComputePackage);
			if (bAnyAssertFailed || !Entry || !Evidence || !CryoEntry || !Checkpoint)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 16 mission contract missing.") : Record.FailureReason);
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
			bRequestedComputeStream = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedComputeStream)
			{
				const FName ComputeName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel4_Compute);
				if (!ComputeName.IsNone())
				{
					Levels->AddStreamRequest(ComputeName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedComputeStream = true;
				}
			}
			if (Character && bRequestedComputeStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, CheckpointLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with Checkpoint_InterfaceChamber."));
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
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, CheckpointLabel) : TArray<AActor*>();
			AProjectOrganoidCheckpoint* Checkpoint = Found.Num() == 1 ? Cast<AProjectOrganoidCheckpoint>(Found[0]) : nullptr;
			if (!World || !Character || !Objectives || !Saves || !Power || !Checkpoint || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 16 actors or subsystems missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Emergency);
			AssertTrue(Record, TEXT("power.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("checkpoint.unmoved"), Checkpoint->GetActorLocation().Equals(CheckpointLocation, 5.f), TEXT("-2425,-1650,-3540"), Checkpoint->GetActorLocation().ToString(), CheckpointLabel);
			AssertTrue(Record, TEXT("checkpoint.prompt_live"), Checkpoint->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Checkpoint->InteractionPrompt.ToString(), CheckpointLabel);

			UProjectOrganoidObjectiveDataAsset* CryoEntry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, CryoEntrySoftPath);
			const bool bCryoEntry = CryoEntry && Objectives->LoadMission(CryoEntry, false);
			AssertTrue(Record, TEXT("reject.cryo_entry_loaded"), bCryoEntry && Objectives->GetActiveMissionId() == FName(CryoEntryMissionId), CryoEntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Objectives->TriggerEvent(FName(CryoEntryEvent));
			AssertTrue(Record, TEXT("reject.evidence_current"), Objectives->GetActiveMissionId() == FName(EvidenceMissionId), EvidenceMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.enter_not_active"), CountActiveId(Objectives, FName(EntryObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(EntryObjectiveId))), EntryObjectiveId);
			Checkpoint->SaveSlotOverride = SaveSlot;
			const bool bRejected = Checkpoint->Interact(Character);
			AssertTrue(Record, TEXT("reject.interact"), bRejected, TEXT("true"), BoolText(bRejected), CheckpointLabel);
			AssertTrue(Record, TEXT("reject.no_credit"), Checkpoint->CampaignEventFireCount == 0, TEXT("0"), FString::FromInt(Checkpoint->CampaignEventFireCount), CheckpointLabel);
			AssertTrue(Record, TEXT("reject.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));

			int32 Recovered = 0;
			for (int32 ReadIndex = 0; ReadIndex < 3; ++ReadIndex)
			{
				Recovered += Objectives->TriggerEvent(FName(EvidenceEvent));
			}
			AssertTrue(Record, TEXT("chain.evidence_event"), Recovered > 0, TEXT(">0"), FString::FromInt(Recovered), EvidenceEvent);
			AssertTrue(Record, TEXT("chain.entry_current"), Objectives->GetActiveMissionId() == FName(EntryMissionId), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.objective_active"), CountActiveId(Objectives, FName(EntryObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(EntryObjectiveId))), EntryObjectiveId);
			AssertTrue(Record, TEXT("chain.evidence_completed"), CountCompletedId(Objectives, FName(EvidenceObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("chain.opening_not_complete"), !Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation"))), TEXT("false"), BoolText(Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation")))), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bSuccess = Checkpoint->Interact(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.interact"), bSuccess, TEXT("true"), BoolText(bSuccess), CheckpointLabel);
			AssertTrue(Record, TEXT("success.fire"), Checkpoint->CampaignEventFireCount == 1, TEXT("1"), FString::FromInt(Checkpoint->CampaignEventFireCount), CheckpointLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(EntryObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EntryObjectiveId))), EntryObjectiveId);
			AssertTrue(Record, TEXT("success.mission"), Objectives->GetActiveMissionId() == FName(EntryMissionId) && Objectives->IsMissionComplete(FName(EntryMissionId)), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.notification"), Checkpoint->CampaignNotificationCount == 1, TEXT("1"), FString::FromInt(Checkpoint->CampaignNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.no_door"), Found.Num() == 1 && Checkpoint->GetActorLocation().Equals(CheckpointLocation, 5.f), TEXT("checkpoint"), Checkpoint->GetActorLocation().ToString(), CheckpointLabel);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Checkpoint->Interact(Character);
			AssertTrue(Record, TEXT("replay.fire_once"), Checkpoint->CampaignEventFireCount == 1 && Checkpoint->CampaignNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("fire=%d notes=%d"), Checkpoint->CampaignEventFireCount, Checkpoint->CampaignNotificationCount), CheckpointLabel);
			AssertTrue(Record, TEXT("replay.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("replay.line_cleared"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("cleared"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->GetActiveMissionId() == FName(EntryMissionId) && Objectives->IsMissionComplete(FName(EntryMissionId)), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.compute_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, CheckpointLabel) : TArray<AActor*>();
			AProjectOrganoidCheckpoint* Checkpoint = Found.Num() == 1 ? Cast<AProjectOrganoidCheckpoint>(Found[0]) : nullptr;
			if (!Objectives || !Saves || !Power || !Checkpoint || !Character || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Blackout);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(EntryMissionId) && Objectives->IsMissionComplete(FName(EntryMissionId)), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(EntryObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EntryObjectiveId))), EntryObjectiveId);
			AssertTrue(Record, TEXT("save.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("save"));
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("save"));
			Checkpoint->SaveSlotOverride = SaveSlot;
			Checkpoint->Interact(Character);
			AssertTrue(Record, TEXT("save.replay_no_fire"), Checkpoint->CampaignEventFireCount == 0, TEXT("0"), FString::FromInt(Checkpoint->CampaignEventFireCount), CheckpointLabel);
			AssertTrue(Record, TEXT("save.unmoved"), Checkpoint->GetActorLocation().Equals(CheckpointLocation, 5.f), TEXT("-2425,-1650,-3540"), Checkpoint->GetActorLocation().ToString(), CheckpointLabel);
			AssertTrue(Record, TEXT("dirty.reload_compute"), !PackageIsDirty(ComputePackage), TEXT("clean"), PackageIsDirty(ComputePackage) ? TEXT("dirty") : TEXT("clean"), ComputePackage);
			AssertTrue(Record, TEXT("dirty.reload_admin"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedComputeStream = false;
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
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FComputeEntryFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterComputeEntryFunctional;
}
