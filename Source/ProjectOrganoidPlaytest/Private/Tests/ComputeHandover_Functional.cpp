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
#include "ProjectOrganoidTerminal.h"

namespace ComputeHandoverFunctional
{
	constexpr TCHAR TestId[] = TEXT("ComputeHandover_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Compute Handover Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR HandoverSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeHandover.DA_Mission_ComputeHandover");
	constexpr TCHAR ConclusionSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_TheConclusion.DA_Mission_TheConclusion");
	constexpr TCHAR EntrySoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ComputeEntry.DA_Mission_ComputeEntry");
	constexpr TCHAR HandoverMissionId[] = TEXT("Mission_ComputeHandover");
	constexpr TCHAR ConclusionMissionId[] = TEXT("Mission_TheConclusion");
	constexpr TCHAR EntryMissionId[] = TEXT("Mission_ComputeEntry");
	constexpr TCHAR HackObjectiveId[] = TEXT("Obj_HackComputeCore");
	constexpr TCHAR ConfessionObjectiveId[] = TEXT("Obj_ReadSterlingConfession");
	constexpr TCHAR EntryObjectiveId[] = TEXT("Obj_EnterCompute");
	constexpr TCHAR HackEvent[] = TEXT("Event_ComputeCoreHacked");
	constexpr TCHAR ConfessionEvent[] = TEXT("Event_SterlingConfessionRead");
	constexpr TCHAR EntryEvent[] = TEXT("Event_ComputeEntered");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidComputeHandoverTest");
	constexpr TCHAR HackLine[] = TEXT("It's been running the lockdown the whole time. It didn't lose control.");
	constexpr TCHAR ConfessionLine[] = TEXT("He didn't lose control. He handed it over.");
	constexpr TCHAR HackPrompt[] = TEXT("Hack Compute Core");
	constexpr TCHAR ConfessionPrompt[] = TEXT("Read Sterling's Confession");
	constexpr TCHAR PadLabel[] = TEXT("DataPad_SterlingConfession");
	const TCHAR* TerminalLabels[] = { TEXT("Terminal_CoreInterface_1"), TEXT("Terminal_CoreInterface_2"), TEXT("Terminal_CoreInterface_3") };
	const FVector TerminalLocations[] = {
		FVector(-1760.f, -2150.f, -3500.f),
		FVector(-1760.f, -1650.f, -3500.f),
		FVector(-1760.f, -1150.f, -3500.f)
	};
	const FVector PadLocation(-2425.f, -2275.f, -3510.f);

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
	int32 ObjectiveProgress(UProjectOrganoidObjectiveSubsystem* Objectives, FName Id)
	{
		FProjectOrganoidObjective Objective;
		return Objectives && Objectives->GetObjective(Id, Objective) ? Objective.CurrentProgress : -1;
	}

	class FComputeHandoverFunctional : public IOrganoidPlaytestCase
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
			case EStage::StartPie:
				if (!Owner.RequestStartPie(MapPackage)) { Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed.")); return; }
				WaitSeconds = 0.f;
				bRequestedComputeStream = false;
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
		AProjectOrganoidTerminal* FindTerminal(UWorld* World, const TCHAR* Label) const
		{
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, Label) : TArray<AActor*>();
			return Found.Num() == 1 ? Cast<AProjectOrganoidTerminal>(Found[0]) : nullptr;
		}
		AProjectOrganoidDataPad* FindPad(UWorld* World) const
		{
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, PadLabel) : TArray<AActor*>();
			return Found.Num() == 1 ? Cast<AProjectOrganoidDataPad>(Found[0]) : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Handover = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, HandoverSoftPath);
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			const FProjectOrganoidMissionTaskDefinition* Hack = Handover && Handover->Tasks.Num() == 2 ? &Handover->Tasks[0] : nullptr;
			const FProjectOrganoidMissionTaskDefinition* Confession = Handover && Handover->Tasks.Num() == 2 ? &Handover->Tasks[1] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Handover && Handover->MissionId == FName(HandoverMissionId), HandoverMissionId, Handover ? Handover->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Handover && Handover->MissionTitle.ToString() == TEXT("The Handover"), TEXT("The Handover"), Handover ? Handover->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Handover && Handover->MissionDescription.ToString().Contains(TEXT("recover Sterling's confession")), TEXT("confession"), Handover ? Handover->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_conclusion"), Handover && Handover->NextMissionAsset.ToSoftObjectPath().ToString() == ConclusionSoftPath, ConclusionSoftPath, Handover ? Handover->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.two_tasks"), Handover && Handover->Tasks.Num() == 2, TEXT("2"), Handover ? FString::FromInt(Handover->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_id"), Hack && Hack->Objective.ObjectiveId == FName(HackObjectiveId), HackObjectiveId, Hack ? Hack->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_main"), Hack && Hack->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Hack ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_target"), Hack && Hack->Objective.TargetProgress == 3, TEXT("3"), Hack ? FString::FromInt(Hack->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_auto"), Hack && Hack->bAutoActivate, TEXT("true"), Hack ? BoolText(Hack->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_no_prereq"), Hack && Hack->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Hack ? FString::FromInt(Hack->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.hack_event"), Hack && Hack->EventTriggers.Num() == 1 && Hack->EventTriggers[0].EventId == FName(HackEvent) && Hack->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Advance && Hack->EventTriggers[0].ProgressDelta == 1, HackEvent, Hack && Hack->EventTriggers.Num() == 1 ? Hack->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.confession_id"), Confession && Confession->Objective.ObjectiveId == FName(ConfessionObjectiveId), ConfessionObjectiveId, Confession ? Confession->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.confession_main"), Confession && Confession->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Confession ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.confession_target"), Confession && Confession->Objective.TargetProgress == 1, TEXT("1"), Confession ? FString::FromInt(Confession->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.confession_gated"), Confession && !Confession->bAutoActivate && Confession->Objective.PrerequisiteObjectiveIds.Num() == 1 && Confession->Objective.PrerequisiteObjectiveIds[0] == FName(HackObjectiveId), HackObjectiveId, Confession ? FString::FromInt(Confession->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.confession_event"), Confession && Confession->EventTriggers.Num() == 1 && Confession->EventTriggers[0].EventId == FName(ConfessionEvent) && Confession->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Complete, ConfessionEvent, Confession && Confession->EventTriggers.Num() == 1 ? Confession->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.entry_next"), Entry && Entry->NextMissionAsset.ToSoftObjectPath().ToString() == HandoverSoftPath, HandoverSoftPath, Entry ? Entry->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			bool bActorsOk = true;
			for (int32 Index = 0; Index < 3; ++Index)
			{
				AProjectOrganoidTerminal* Terminal = FindTerminal(EditorWorld, TerminalLabels[Index]);
				const FString PackageName = Terminal && Terminal->GetOutermost() ? Terminal->GetOutermost()->GetName() : TEXT("missing");
				bActorsOk = bActorsOk && Terminal != nullptr;
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.class"), Index), Terminal != nullptr, TEXT("Terminal"), Terminal ? Terminal->GetClass()->GetName() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.package"), Index), PackageName.Contains(TEXT("SL_Epitope_Compute")), ComputePackage, PackageName, TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.location"), Index), Terminal && Terminal->GetActorLocation().Equals(TerminalLocations[Index], 1.f), TerminalLocations[Index].ToString(), Terminal ? Terminal->GetActorLocation().ToString() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.objective"), Index), Terminal && Terminal->RequiredActiveObjectiveId == FName(HackObjectiveId), HackObjectiveId, Terminal ? Terminal->RequiredActiveObjectiveId.ToString() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.event"), Index), Terminal && Terminal->SuccessObjectiveEventId == FName(HackEvent), HackEvent, Terminal ? Terminal->SuccessObjectiveEventId.ToString() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.prompt"), Index), Terminal && Terminal->InteractionPrompt.ToString() == HackPrompt && Terminal->CampaignHackPrompt.ToString() == HackPrompt, HackPrompt, Terminal ? Terminal->InteractionPrompt.ToString() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.line"), Index), Terminal && Terminal->CompletionNotificationSpeaker.ToString() == TEXT("Nathan") && Terminal->CompletionNotificationText.ToString() == HackLine, HackLine, Terminal ? Terminal->CompletionNotificationText.ToString() : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.duration"), Index), Terminal && FMath::IsNearlyEqual(Terminal->CompletionNotificationDurationSeconds, 7.f), TEXT("7"), Terminal ? FString::SanitizeFloat(Terminal->CompletionNotificationDurationSeconds) : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.power_change_off"), Index), Terminal && !Terminal->bApplyPowerChangeOnSuccess && Terminal->PowerSector == EProjectOrganoidPowerSector::Compute, TEXT("Compute"), Terminal ? TEXT("other") : TEXT("missing"), TerminalLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("terminal%d.no_unlock"), Index), Terminal && Terminal->LinkedDoorLock.IsNull() && Terminal->LinkedSecurityGate.IsNull(), TEXT("null"), TEXT("link"), TerminalLabels[Index]);
			}
			AProjectOrganoidDataPad* Pad = FindPad(EditorWorld);
			const FString PadPackage = Pad && Pad->GetOutermost() ? Pad->GetOutermost()->GetName() : TEXT("missing");
			bActorsOk = bActorsOk && Pad != nullptr;
			AssertTrue(Record, TEXT("pad.class"), Pad != nullptr, TEXT("DataPad"), Pad ? Pad->GetClass()->GetName() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.package"), PadPackage.Contains(TEXT("SL_Epitope_Compute")), ComputePackage, PadPackage, PadLabel);
			AssertTrue(Record, TEXT("pad.location"), Pad && Pad->GetActorLocation().Equals(PadLocation, 1.f), PadLocation.ToString(), Pad ? Pad->GetActorLocation().ToString() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.objective"), Pad && Pad->RequiredObjectiveIdForInteraction == FName(ConfessionObjectiveId), ConfessionObjectiveId, Pad ? Pad->RequiredObjectiveIdForInteraction.ToString() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.event"), Pad && Pad->ObjectiveEventId == FName(ConfessionEvent), ConfessionEvent, Pad ? Pad->ObjectiveEventId.ToString() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.prompt"), Pad && Pad->InteractionPrompt.ToString() == ConfessionPrompt, ConfessionPrompt, Pad ? Pad->InteractionPrompt.ToString() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.line"), Pad && Pad->CompletionNotificationSpeaker.ToString() == TEXT("Nathan") && Pad->CompletionNotificationText.ToString() == ConfessionLine, ConfessionLine, Pad ? Pad->CompletionNotificationText.ToString() : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.duration"), Pad && FMath::IsNearlyEqual(Pad->CompletionNotificationDurationSeconds, 7.f), TEXT("7"), Pad ? FString::SanitizeFloat(Pad->CompletionNotificationDurationSeconds) : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("pad.generic_off"), Pad && !Pad->bBroadcastGenericDataPadEvent, TEXT("false"), Pad ? BoolText(Pad->bBroadcastGenericDataPadEvent) : TEXT("missing"), PadLabel);
			AssertTrue(Record, TEXT("dirty.compute_clean"), !PackageIsDirty(ComputePackage), TEXT("clean"), PackageIsDirty(ComputePackage) ? TEXT("dirty") : TEXT("clean"), ComputePackage);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			if (bAnyAssertFailed || !Handover || !Entry || !bActorsOk)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 17 mission contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedComputeStream && FindTerminal(World, TerminalLabels[0]) && FindTerminal(World, TerminalLabels[1]) && FindTerminal(World, TerminalLabels[2]) && FindPad(World))
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f) FailAndStop(Owner, Record, TEXT("PIE did not become ready with the Compute handover actors."));
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
				if (!bReloadDone) TickReload(Owner, Record, World, Character);
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
			AProjectOrganoidTerminal* Terminals[3] = { FindTerminal(World, TerminalLabels[0]), FindTerminal(World, TerminalLabels[1]), FindTerminal(World, TerminalLabels[2]) };
			AProjectOrganoidDataPad* Pad = FindPad(World);
			if (!Objectives || !Saves || !Power || !Terminals[0] || !Terminals[1] || !Terminals[2] || !Pad || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Beat 17 actors or subsystems missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Emergency);
			AssertTrue(Record, TEXT("power.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.cryo_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.admin_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.facility_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.reactor_emergency"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor) == EProjectOrganoidPowerState::Emergency, TEXT("Emergency"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Reactor)), TEXT("Power"));
			UProjectOrganoidObjectiveDataAsset* Entry = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, EntrySoftPath);
			const bool bEntryLoaded = Entry && Objectives->LoadMission(Entry, false);
			AssertTrue(Record, TEXT("reject.entry_loaded"), bEntryLoaded && Objectives->GetActiveMissionId() == FName(EntryMissionId), EntryMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.terminal"), !Terminals[0]->CanInteract_Implementation(Character) && !Terminals[1]->CanInteract_Implementation(Character) && !Terminals[2]->CanInteract_Implementation(Character), TEXT("false"), BoolText(Terminals[0]->CanInteract_Implementation(Character)), TerminalLabels[0]);
			AssertTrue(Record, TEXT("reject.pad"), !Pad->CanInteract_Implementation(Character) && !Pad->bHasBeenRead, TEXT("false"), BoolText(Pad->bHasBeenRead), PadLabel);
			Objectives->TriggerEvent(FName(EntryEvent));
			AssertTrue(Record, TEXT("chain.handover_current"), Objectives->GetActiveMissionId() == FName(HandoverMissionId), HandoverMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.hack_active"), CountActiveId(Objectives, FName(HackObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("chain.confession_inactive"), CountActiveId(Objectives, FName(ConfessionObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(ConfessionObjectiveId))), ConfessionObjectiveId);
			AssertTrue(Record, TEXT("chain.entry_completed"), CountCompletedId(Objectives, FName(EntryObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(EntryObjectiveId))), EntryObjectiveId);
			AssertTrue(Record, TEXT("chain.prompt"), Terminals[0]->InteractionPrompt.ToString() == HackPrompt, HackPrompt, Terminals[0]->InteractionPrompt.ToString(), TerminalLabels[0]);
			Terminals[0]->ApplyHackRewards(Character);
			AssertTrue(Record, TEXT("hack1.progress"), ObjectiveProgress(Objectives, FName(HackObjectiveId)) == 1, TEXT("1"), FString::FromInt(ObjectiveProgress(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("hack1.not_complete"), CountCompletedId(Objectives, FName(HackObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("hack1.no_line"), Terminals[0]->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Terminals[0]->CompletionNotificationCount), TerminalLabels[0]);
			Terminals[1]->ApplyHackRewards(Character);
			AssertTrue(Record, TEXT("hack2.progress"), ObjectiveProgress(Objectives, FName(HackObjectiveId)) == 2, TEXT("2"), FString::FromInt(ObjectiveProgress(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("hack2.not_complete"), CountCompletedId(Objectives, FName(HackObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("hack2.no_line"), Terminals[1]->CompletionNotificationCount == 0, TEXT("0"), FString::FromInt(Terminals[1]->CompletionNotificationCount), TerminalLabels[1]);
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Terminals[2]->ApplyHackRewards(Character);
			const FString HackShown = Widget->GetLastResourceNotification().ToString();
			const float HackRemaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("hack3.complete"), CountCompletedId(Objectives, FName(HackObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("hack3.confession_active"), CountActiveId(Objectives, FName(ConfessionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(ConfessionObjectiveId))), ConfessionObjectiveId);
			AssertTrue(Record, TEXT("hack3.line"), HackShown.Contains(HackLine) && HackShown.StartsWith(TEXT("Nathan:")), HackLine, HackShown, TEXT("HUD"));
			AssertTrue(Record, TEXT("hack3.duration"), HackRemaining > 6.0f && HackRemaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(HackRemaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("hack3.once"), Terminals[0]->CompletionNotificationCount + Terminals[1]->CompletionNotificationCount + Terminals[2]->CompletionNotificationCount == 1, TEXT("1"), FString::FromInt(Terminals[2]->CompletionNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("hack3.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bRead = Pad->Interact(Character);
			const FString ConfessionShown = Widget->GetLastResourceNotification().ToString();
			const float ConfessionRemaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("read.interact"), bRead, TEXT("true"), BoolText(bRead), PadLabel);
			AssertTrue(Record, TEXT("read.complete"), CountCompletedId(Objectives, FName(ConfessionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConfessionObjectiveId))), ConfessionObjectiveId);
			AssertTrue(Record, TEXT("read.mission"), Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("read.line"), ConfessionShown.Contains(ConfessionLine) && ConfessionShown.StartsWith(TEXT("Nathan:")), ConfessionLine, ConfessionShown, TEXT("HUD"));
			AssertTrue(Record, TEXT("read.duration"), ConfessionRemaining > 6.0f && ConfessionRemaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(ConfessionRemaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("read.once"), Pad->CompletionNotificationCount == 1, TEXT("1"), FString::FromInt(Pad->CompletionNotificationCount), PadLabel);
			AssertTrue(Record, TEXT("read.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("Power"));
			AssertTrue(Record, TEXT("read.unmoved"), Terminals[0]->GetActorLocation().Equals(TerminalLocations[0], 5.f) && Terminals[2]->GetActorLocation().Equals(TerminalLocations[2], 5.f) && Pad->GetActorLocation().Equals(PadLocation, 5.f), TEXT("unmoved"), Pad->GetActorLocation().ToString(), PadLabel);
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Pad->Interact(Character);
			Terminals[2]->ApplyHackRewards(Character);
			AssertTrue(Record, TEXT("replay.once"), Pad->CompletionNotificationCount == 1 && Terminals[2]->CompletionNotificationCount == 1 && !Widget->GetLastResourceNotification().ToString().Contains(ConfessionLine), TEXT("1"), FString::FromInt(Pad->CompletionNotificationCount), PadLabel);
			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved && Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.compute_captured"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			if (!Objectives || !Saves || !Power || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Blackout);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(ConclusionMissionId), ConclusionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.hack"), CountCompletedId(Objectives, FName(HackObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(HackObjectiveId))), HackObjectiveId);
			AssertTrue(Record, TEXT("save.confession"), CountCompletedId(Objectives, FName(ConfessionObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConfessionObjectiveId))), ConfessionObjectiveId);
			AssertTrue(Record, TEXT("save.compute_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute)), TEXT("save"));
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
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FComputeHandoverFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterComputeHandoverFunctional;
}
