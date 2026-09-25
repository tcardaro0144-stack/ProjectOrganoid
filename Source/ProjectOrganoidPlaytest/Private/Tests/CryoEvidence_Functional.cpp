#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidDataPad.h"
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

namespace CryoEvidenceFunctional
{
	constexpr TCHAR TestId[] = TEXT("CryoEvidence_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Cryo Evidence Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR EvidenceSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoEvidence.DA_Mission_CryoEvidence");
	constexpr TCHAR EntrySoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoEntry.DA_Mission_CryoEntry");
	constexpr TCHAR AccessSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_CryoAccess.DA_Mission_CryoAccess");
	constexpr TCHAR RevelationSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation.DA_Mission_NeuroRevelation");
	constexpr TCHAR EvidenceMissionId[] = TEXT("Mission_CryoEvidence");
	constexpr TCHAR EntryMissionId[] = TEXT("Mission_CryoEntry");
	constexpr TCHAR EvidenceObjectiveId[] = TEXT("Obj_RecoverCryoEvidence");
	constexpr TCHAR EvidenceEvent[] = TEXT("Event_CryoEvidenceRecovered");
	constexpr TCHAR EntryEvent[] = TEXT("Event_CryoEntered");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidCryoEvidenceTest");
	constexpr TCHAR ExpectedLine[] = TEXT("Lot numbers, consent forms... These weren't specimens. They were staff. Authorization was filed before anyone died.");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Recover Cryo Evidence");
	const TCHAR* PadLabels[] = { TEXT("DataPad_SpecimenManifest"), TEXT("DataPad_ConsentForms"), TEXT("DataPad_SterlingCryoNote") };
	const FVector PadLocations[] = {
		FVector(-2330.f, -2150.f, -2310.f),
		FVector(-400.f, -1150.f, -2310.f),
		FVector(-2425.f, 1025.f, -2310.f)
	};

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
	int32 ObjectiveProgress(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		FProjectOrganoidObjective Objective;
		return Objectives && Objectives->GetObjective(Id, Objective) ? Objective.CurrentProgress : -1;
	}

	class FCryoEvidenceFunctional : public IOrganoidPlaytestCase
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
			case EStage::StartPie:
				if (!Owner.RequestStartPie(MapPackage)) { Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed.")); return; }
				WaitSeconds = 0.f;
				bRequestedCryoStream = false;
				Stage = EStage::WaitReady;
				break;
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
					if (bAnyAssertFailed) { Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record->FailureReason); return; }
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
					TArray<UPackage*> WorldDirty;
					TArray<UPackage*> ContentDirty;
					FEditorFileUtils::GetDirtyWorldPackages(WorldDirty);
					FEditorFileUtils::GetDirtyContentPackages(ContentDirty);
					if (UGameplayStatics::DoesSaveGameExist(SaveSlot, 0) || WorldDirty.Num() + ContentDirty.Num() > 0)
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty()) Record->FailureReason = TEXT("Cleanup left a save or a dirty package.");
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
		AProjectOrganoidDataPad* FindPad(UWorld* World, const TCHAR* Label) const
		{
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, Label) : TArray<AActor*>();
			return Found.Num() == 1 ? Cast<AProjectOrganoidDataPad>(Found[0]) : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Evidence = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EvidenceSoftPath);
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			UProjectOrganoidObjectiveDataAsset* Access = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, AccessSoftPath);
			UProjectOrganoidObjectiveDataAsset* Revelation = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RevelationSoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Evidence && Evidence->Tasks.Num() == 1 ? &Evidence->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Evidence && Evidence->MissionId == FName(EvidenceMissionId), EvidenceMissionId, Evidence ? Evidence->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Evidence && Evidence->MissionTitle.ToString() == TEXT("Lot Numbers"), TEXT("Lot Numbers"), Evidence ? Evidence->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Evidence && Evidence->MissionDescription.ToString() == TEXT("The cryo manifests don't match the specimen logs. Recover the remaining facility documents downstairs."), TEXT("locked description"), Evidence ? Evidence->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_null"), Evidence && Evidence->NextMissionAsset.IsNull(), TEXT("null"), Evidence && Evidence->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), Evidence && Evidence->Tasks.Num() == 1, TEXT("1"), Evidence ? FString::FromInt(Evidence->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(EvidenceObjectiveId), EvidenceObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 3, TEXT("3"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Recover Cryo Evidence"), TEXT("Recover Cryo Evidence"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString() == TEXT("Recover the remaining Cryo documents."), TEXT("Recover the remaining Cryo documents."), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(EvidenceEvent) && Task->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Advance && Task->EventTriggers[0].ProgressDelta == 1, EvidenceEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_next"), Entry && Entry->NextMissionAsset.ToSoftObjectPath().ToString() == EvidenceSoftPath, EvidenceSoftPath, Entry ? Entry->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.access_next_entry"), Access && Access->NextMissionAsset.ToSoftObjectPath().ToString() == EntrySoftPath, EntrySoftPath, Access ? Access->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.revelation_next_access"), Revelation && Revelation->NextMissionAsset.ToSoftObjectPath().ToString() == AccessSoftPath, AccessSoftPath, Revelation ? Revelation->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			bool bPadsOk = true;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				AProjectOrganoidDataPad* Pad = FindPad(EditorWorld, PadLabels[Index]);
				const FString PackageName = Pad && Pad->GetOutermost() ? Pad->GetOutermost()->GetName() : TEXT("missing");
				bPadsOk = bPadsOk && Pad != nullptr;
				AssertTrue(Record, FString::Printf(TEXT("pad%d.class"), Index), Pad != nullptr, TEXT("DataPad"), Pad ? Pad->GetClass()->GetName() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.package"), Index), PackageName.Contains(TEXT("SL_Epitope_Cryo")), CryoPackage, PackageName, PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.location"), Index), Pad && Pad->GetActorLocation().Equals(PadLocations[Index], 1.f), PadLocations[Index].ToString(), Pad ? Pad->GetActorLocation().ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.objective"), Index), Pad && Pad->RequiredObjectiveIdForInteraction == FName(EvidenceObjectiveId), EvidenceObjectiveId, Pad ? Pad->RequiredObjectiveIdForInteraction.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.event"), Index), Pad && Pad->ObjectiveEventId == FName(EvidenceEvent), EvidenceEvent, Pad ? Pad->ObjectiveEventId.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.prompt"), Index), Pad && Pad->InteractionPrompt.ToString() == ExpectedPrompt, ExpectedPrompt, Pad ? Pad->InteractionPrompt.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.speaker"), Index), Pad && Pad->CompletionNotificationSpeaker.ToString() == TEXT("Nathan"), TEXT("Nathan"), Pad ? Pad->CompletionNotificationSpeaker.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.line"), Index), Pad && Pad->CompletionNotificationText.ToString() == ExpectedLine, ExpectedLine, Pad ? Pad->CompletionNotificationText.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.log_kept"), Index), Pad && !Pad->LogEntry.EntryId.IsNone() && !Pad->LogEntry.Body.IsEmpty(), TEXT("existing log"), Pad ? Pad->LogEntry.EntryId.ToString() : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.duration"), Index), Pad && FMath::IsNearlyEqual(Pad->CompletionNotificationDurationSeconds, 7.f), TEXT("7"), Pad ? FString::SanitizeFloat(Pad->CompletionNotificationDurationSeconds) : TEXT("missing"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad%d.generic_off"), Index), Pad && !Pad->bBroadcastGenericDataPadEvent, TEXT("false"), Pad ? BoolText(Pad->bBroadcastGenericDataPadEvent) : TEXT("missing"), PadLabels[Index]);
			}
			AssertTrue(Record, TEXT("dirty.cryo_clean"), !PackageIsDirty(CryoPackage), TEXT("clean"), PackageIsDirty(CryoPackage) ? TEXT("dirty") : TEXT("clean"), CryoPackage);
			AssertTrue(Record, TEXT("dirty.neuro_clean"), !PackageIsDirty(TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics")), TEXT("clean"), PackageIsDirty(TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics")) ? TEXT("dirty") : TEXT("clean"), TEXT("Neuro"));
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			if (bAnyAssertFailed || !Evidence || !Entry || !bPadsOk)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 15 mission contract missing.") : Record.FailureReason);
				return;
			}
			Stage = EStage::StartPie;
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
			if (Character && bRequestedCryoStream && FindPad(World, PadLabels[0]) && FindPad(World, PadLabels[1]) && FindPad(World, PadLabels[2]))
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with the Cryo evidence datapads."));
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
			AProjectOrganoidDataPad* Pads[3] = { FindPad(World, PadLabels[0]), FindPad(World, PadLabels[1]), FindPad(World, PadLabels[2]) };
			if (!Objectives || !Saves || !Power || !Pads[0] || !Pads[1] || !Pads[2] || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Beat 15 actors or subsystems missing."));
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
			const bool bRejected = Pads[0]->Interact(Character);
			AssertTrue(Record, TEXT("reject.interact"), !bRejected && !Pads[0]->bHasBeenRead, TEXT("false"), BoolText(bRejected), PadLabels[0]);
			AssertTrue(Record, TEXT("reject.consent"), !Pads[1]->Interact(Character) && !Pads[1]->bHasBeenRead, TEXT("false"), BoolText(Pads[1]->bHasBeenRead), PadLabels[1]);
			AssertTrue(Record, TEXT("reject.note"), !Pads[2]->Interact(Character) && !Pads[2]->bHasBeenRead, TEXT("false"), BoolText(Pads[2]->bHasBeenRead), PadLabels[2]);
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			const bool bLoaded = Entry && Objectives->LoadMission(Entry, false);
			AssertTrue(Record, TEXT("chain.entry_loaded"), bLoaded && Objectives->GetActiveMissionId() == FName(EntryMissionId), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			Objectives->TriggerEvent(FName(EntryEvent));
			FProjectOrganoidObjective EvidenceObjective;
			const bool bEvidenceObjective = Objectives->GetObjective(FName(EvidenceObjectiveId), EvidenceObjective);
			AssertTrue(Record, TEXT("chain.evidence_current"), Objectives->GetActiveMissionId() == FName(EvidenceMissionId), EvidenceMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.active"), bEvidenceObjective && EvidenceObjective.State == EProjectOrganoidObjectiveState::Active, TEXT("Active"), bEvidenceObjective ? TEXT("found") : TEXT("missing"), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("chain.target"), ObjectiveProgress(Objectives, FName(EvidenceObjectiveId)) == 0, TEXT("0"), FString::FromInt(ObjectiveProgress(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			Pads[0]->Interact(Character);
			AssertTrue(Record, TEXT("read1.progress"), ObjectiveProgress(Objectives, FName(EvidenceObjectiveId)) == 1, TEXT("1"), FString::FromInt(ObjectiveProgress(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("read1.not_complete"), CountCompletedId(Objectives, FName(EvidenceObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("read1.read"), Pads[0]->bHasBeenRead && !Pads[1]->bHasBeenRead, TEXT("first only"), BoolText(Pads[0]->bHasBeenRead), PadLabels[0]);
			AssertTrue(Record, TEXT("read1.no_line"), Pads[0]->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Pads[0]->CompletionNotificationCount), PadLabels[0]);
			AssertTrue(Record, TEXT("read1.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			Pads[1]->Interact(Character);
			AssertTrue(Record, TEXT("read2.progress"), ObjectiveProgress(Objectives, FName(EvidenceObjectiveId)) == 2, TEXT("2"), FString::FromInt(ObjectiveProgress(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("read2.not_complete"), CountCompletedId(Objectives, FName(EvidenceObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("read2.read"), Pads[1]->bHasBeenRead, TEXT("true"), BoolText(Pads[1]->bHasBeenRead), PadLabels[1]);
			AssertTrue(Record, TEXT("read2.no_line"), Pads[1]->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Pads[1]->CompletionNotificationCount), PadLabels[1]);
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Pads[2]->Interact(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("read3.progress"), CountCompletedId(Objectives, FName(EvidenceObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("read3.mission"), Objectives->IsMissionComplete(FName(EvidenceMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("read3.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("read3.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("read3.once"), Pads[0]->CompletionNotificationCount + Pads[1]->CompletionNotificationCount + Pads[2]->CompletionNotificationCount == 1, TEXT("1"), FString::FromInt(Pads[2]->CompletionNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("read3.all_read"), Pads[0]->bHasBeenRead && Pads[1]->bHasBeenRead && Pads[2]->bHasBeenRead, TEXT("true"), BoolText(Pads[2]->bHasBeenRead), PadLabels[2]);
			AssertTrue(Record, TEXT("read3.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			AssertTrue(Record, TEXT("read3.unmoved"), Pads[0]->GetActorLocation().Equals(PadLocations[0], 5.f) && Pads[1]->GetActorLocation().Equals(PadLocations[1], 5.f) && Pads[2]->GetActorLocation().Equals(PadLocations[2], 5.f), TEXT("unmoved"), Pads[2]->GetActorLocation().ToString(), PadLabels[2]);
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Pads[2]->Interact(Character);
			AssertTrue(Record, TEXT("replay.once"), Pads[2]->CompletionNotificationCount == 1 && !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("1"), FString::FromInt(Pads[2]->CompletionNotificationCount), PadLabels[2]);
			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved && Objectives->IsMissionComplete(FName(EvidenceMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.cryo_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			(void)Widget;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			AProjectOrganoidDataPad* Pad = FindPad(World, PadLabels[2]);
			if (!Objectives || !Saves || !Power || !Pad || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Emergency);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->IsMissionComplete(FName(EvidenceMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(EvidenceObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EvidenceObjectiveId))), EvidenceObjectiveId);
			AssertTrue(Record, TEXT("save.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("save"));
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("save"));
			AssertTrue(Record, TEXT("save.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("save"));
			AssertTrue(Record, TEXT("save.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("save"));
			Pad->Interact(Character);
			AssertTrue(Record, TEXT("save.replay_no_line"), Pad->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Pad->CompletionNotificationCount), PadLabels[2]);
			AssertTrue(Record, TEXT("dirty.reload_cryo"), !PackageIsDirty(CryoPackage), TEXT("clean"), PackageIsDirty(CryoPackage) ? TEXT("dirty") : TEXT("clean"), CryoPackage);
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
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FCryoEvidenceFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterCryoEvidenceFunctional;
}
