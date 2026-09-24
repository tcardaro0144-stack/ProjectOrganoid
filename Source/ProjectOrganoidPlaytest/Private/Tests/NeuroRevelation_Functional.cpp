#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace NeuroRevelationFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroRevelation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Revelation Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR GeneticsSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR UseSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse.DA_Mission_NeuroNeuralSlowUse");
	constexpr TCHAR ConnectionSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroAdaptationConnection.DA_Mission_NeuroAdaptationConnection");
	constexpr TCHAR RevelationSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation.DA_Mission_NeuroRevelation");
	constexpr TCHAR RevelationPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation");
	constexpr TCHAR ConnectionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroAdaptationConnection");
	constexpr TCHAR RevelationMissionId[] = TEXT("Mission_NeuroRevelation");
	constexpr TCHAR UseMissionId[] = TEXT("Mission_NeuroNeuralSlowUse");
	constexpr TCHAR RevelationObjectiveId[] = TEXT("Obj_ReachNeuroRevelation");
	constexpr TCHAR ConnectObjectiveId[] = TEXT("Obj_ConnectLiveAdaptation");
	constexpr TCHAR FollowObjectiveId[] = TEXT("Obj_FollowNeuralSignature");
	constexpr TCHAR RevelationEvent[] = TEXT("Event_NeuroRevelationReached");
	constexpr TCHAR ConnectEvent[] = TEXT("Event_LiveAdaptationConnected");
	constexpr TCHAR FollowEvent[] = TEXT("Event_NeuralSignatureFollowed");
	constexpr TCHAR NodeLabel[] = TEXT("NeuralSignatureObservationNode_NeuroGenetics");
	constexpr TCHAR TerminalLabel[] = TEXT("NeuralMappingTerminal_NeuroGenetics");
	constexpr TCHAR EvidenceLabel[] = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroRevelationTest");
	constexpr TCHAR ExpectedLine[] = TEXT("These people are not simply infected; the research in this wing has been systematically reorganizing their nervous systems.");
	const FVector NodeLocation(500.f, -2100.f, -1100.f);

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
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
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

	class FNeuroRevelationFunctional : public IOrganoidPlaytestCase
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
			PrimaryFireBefore = 0;
			EquippedBefore.Reset();
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
					bRequestedNeuroStream = false;
					Stage = EStage::StartPie;
					return;
				}
				if (WaitSeconds > 20.f)
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("First PIE session did not end."));
				}
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
				if (Record.FailureReason.IsEmpty())
				{
					Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
				}
			}
			return bPassed;
		}

		void StopIfFailed()
		{
			if (bAnyAssertFailed) Stage = EStage::EndPie;
		}

		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUD ? HUD->GetBoundHUDWidget() : nullptr;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Revelation = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RevelationSoftPath);
			UProjectOrganoidObjectiveDataAsset* Connection = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConnectionSoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Revelation && Revelation->Tasks.Num() == 1 ? &Revelation->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.revelation_id"), Revelation && Revelation->MissionId == FName(RevelationMissionId), RevelationMissionId, Revelation ? Revelation->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.revelation_title"), Revelation && Revelation->MissionTitle.ToString() == TEXT("Read the Neural Pattern"), TEXT("Read the Neural Pattern"), Revelation ? Revelation->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.revelation_description"), Revelation && Revelation->MissionDescription.ToString().Contains(TEXT("systematic reorganization")), TEXT("systematic reorganization"), Revelation ? Revelation->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.revelation_next_null"), Revelation && Revelation->NextMissionAsset.IsNull(), TEXT("null"), Revelation && Revelation->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_task"), Revelation && Revelation->Tasks.Num() == 1, TEXT("1"), Revelation ? FString::FromInt(Revelation->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(RevelationObjectiveId), RevelationObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Reach Neuro Revelation"), TEXT("Reach Neuro Revelation"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("observation node")), TEXT("observation node"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(RevelationEvent), RevelationEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.connection_next_revelation"), Connection && Connection->NextMissionAsset.ToSoftObjectPath().ToString() == RevelationSoftPath, RevelationSoftPath, Connection ? Connection->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Nodes = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, NodeLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Node = Nodes.Num() == 1 ? Cast<AProjectOrganoidInspectableInstrument>(Nodes[0]) : nullptr;
			const FString PackageName = Node && Node->GetOutermost() ? Node->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("node.editor_count"), Nodes.Num() == 1, TEXT("1"), FString::FromInt(Nodes.Num()), NodeLabel);
			AssertTrue(Record, TEXT("node.editor_class"), Node != nullptr, TEXT("InspectableInstrument"), Nodes.Num() == 1 && Nodes[0] ? Nodes[0]->GetClass()->GetName() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.editor_package"), PackageName.Contains(TEXT("SL_Epitope_NeuroGenetics")), NeuroPackage, PackageName, NodeLabel);
			AssertTrue(Record, TEXT("node.editor_location"), Node && Node->GetActorLocation().Equals(NodeLocation, 1.f), TEXT("500,-2100,-1100"), Node ? Node->GetActorLocation().ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.follow_required"), Node && Node->RequiredActiveObjectiveId == FName(FollowObjectiveId), FollowObjectiveId, Node ? Node->RequiredActiveObjectiveId.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.follow_guard"), Node && Node->CompletedObjectiveIdForReplayGuard == FName(FollowObjectiveId), FollowObjectiveId, Node ? Node->CompletedObjectiveIdForReplayGuard.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.follow_event"), Node && Node->ObjectiveEventId == FName(FollowEvent), FollowEvent, Node ? Node->ObjectiveEventId.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.hook_required"), Node && Node->FollowupRequiredActiveObjectiveId == FName(RevelationObjectiveId), RevelationObjectiveId, Node ? Node->FollowupRequiredActiveObjectiveId.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.hook_prerequisite"), Node && Node->FollowupPrerequisiteCompletedObjectiveId == FName(ConnectObjectiveId), ConnectObjectiveId, Node ? Node->FollowupPrerequisiteCompletedObjectiveId.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.hook_event"), Node && Node->FollowupSuccessEventId == FName(RevelationEvent), RevelationEvent, Node ? Node->FollowupSuccessEventId.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.hook_line"), Node && Node->FollowupResponseText.ToString() == ExpectedLine, ExpectedLine, Node ? Node->FollowupResponseText.ToString() : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("node.hook_duration"), Node && FMath::IsNearlyEqual(Node->FollowupNotificationDurationSeconds, 7.f), TEXT("7"), Node ? FString::SanitizeFloat(Node->FollowupNotificationDurationSeconds) : TEXT("missing"), NodeLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.admin_clean"), !PackageIsDirty(AdminPackage), TEXT("clean"), PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"), AdminPackage);
			AssertTrue(Record, TEXT("dirty.neuro_clean"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			AssertTrue(Record, TEXT("dirty.revelation_clean"), !PackageIsDirty(RevelationPackage), TEXT("clean"), PackageIsDirty(RevelationPackage) ? TEXT("dirty") : TEXT("clean"), RevelationPackage);
			AssertTrue(Record, TEXT("dirty.connection_clean"), !PackageIsDirty(ConnectionPackage), TEXT("clean"), PackageIsDirty(ConnectionPackage) ? TEXT("dirty") : TEXT("clean"), ConnectionPackage);
			if (bAnyAssertFailed || !Revelation || !Connection || !Node)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 12 mission contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedNeuroStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Owner, Record, TEXT("PIE did not become ready with the observation node."));
			}
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
			TArray<AActor*> Nodes = World ? OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Node = Nodes.Num() == 1 ? Cast<AProjectOrganoidInspectableInstrument>(Nodes[0]) : nullptr;
			TArray<AActor*> Terminals = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Terminal = Terminals.Num() == 1 ? Cast<AProjectOrganoidInspectableInstrument>(Terminals[0]) : nullptr;
			TArray<AActor*> Evidence = World ? OrganoidPlaytestActions::FindActorsByLabel(World, EvidenceLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Instrument = Evidence.Num() == 1 ? Cast<AProjectOrganoidInspectableInstrument>(Evidence[0]) : nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			if (!World || !Character || !Objectives || !Saves || !Power || !Node || !Terminal || !Instrument || !Adapt || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 12 actors or subsystems missing."));
				return;
			}

			const FString PiePackage = Node->GetOutermost() ? Node->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("node.pie_package"), PiePackage.Contains(TEXT("SL_Epitope_NeuroGenetics")), NeuroPackage, PiePackage, NodeLabel);
			AssertTrue(Record, TEXT("node.pie_location"), Node->GetActorLocation().Equals(NodeLocation, 5.f), TEXT("500,-2100,-1100"), Node->GetActorLocation().ToString(), NodeLabel);
			AssertTrue(Record, TEXT("node.pie_label"), Node->GetActorLabel() == NodeLabel, NodeLabel, Node->GetActorLabel(), NodeLabel);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			EquippedBefore = Adapt->GetEquippedAdaptationPath().ToString();
			AssertTrue(Record, TEXT("hook.pie_event"), Node->FollowupSuccessEventId == FName(RevelationEvent), RevelationEvent, Node->FollowupSuccessEventId.ToString(), NodeLabel);
			AssertTrue(Record, TEXT("evidence.connection_hook_unchanged"), Instrument->FollowupSuccessEventId == FName(ConnectEvent), ConnectEvent, Instrument->FollowupSuccessEventId.ToString(), EvidenceLabel);
			AssertTrue(Record, TEXT("terminal.hook_off"), Terminal->FollowupSuccessEventId.IsNone(), TEXT("None"), Terminal->FollowupSuccessEventId.ToString(), TerminalLabel);

			UProjectOrganoidObjectiveDataAsset* Genetics = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, GeneticsSoftPath);
			UProjectOrganoidObjectiveDataAsset* Use = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, UseSoftPath);
			UProjectOrganoidObjectiveDataAsset* Connection = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConnectionSoftPath);
			UProjectOrganoidObjectiveDataAsset* Revelation = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, RevelationSoftPath);
			const bool bGenetics = Genetics && Objectives->LoadMission(Genetics, false);
			Objectives->TriggerEvent(FName(TEXT("Event_NeuroResearchLoadIsolated")));
			Objectives->TriggerEvent(FName(TEXT("Event_NeuralMappingSignalTraced")));
			Objectives->TriggerEvent(FName(FollowEvent));
			Objectives->TriggerEvent(FName(TEXT("Event_NeuralChangeEvidenceExamined")));
			AssertTrue(Record, TEXT("chain.follow_completed"), bGenetics && CountCompletedId(Objectives, FName(FollowObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(FollowObjectiveId))), FollowObjectiveId);
			AssertTrue(Record, TEXT("chain.opening_not_complete"), !Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation"))), TEXT("false"), BoolText(Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation")))), TEXT("mission"));
			PrimaryFireBefore = Node->ObjectiveEventFireCount;
			Node->Interact(Character);
			AssertTrue(Record, TEXT("reject.before_connection_no_fire"), Node->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Node->FollowupEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("reject.before_connection_no_credit"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			AssertTrue(Record, TEXT("reject.before_connection_follow_only"), Node->ObjectiveEventId == FName(FollowEvent) && Node->ObjectiveEventFireCount == PrimaryFireBefore, FollowEvent, FString::Printf(TEXT("event=%s fire=%d"), *Node->ObjectiveEventId.ToString(), Node->ObjectiveEventFireCount), NodeLabel);

			const bool bConnection = Connection && Objectives->LoadMission(Connection, false);
			AssertTrue(Record, TEXT("reject.connection_loaded"), bConnection && Objectives->GetActiveMissionId() == FName(TEXT("Mission_NeuroAdaptationConnection")), TEXT("Mission_NeuroAdaptationConnection"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("reject.revelation_not_active"), CountActiveId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			Node->Interact(Character);
			AssertTrue(Record, TEXT("reject.before_event_no_fire"), Node->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Node->FollowupEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("reject.before_event_no_credit"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);

			const bool bUse = Use && Objectives->LoadMission(Use, false);
			AssertTrue(Record, TEXT("reject.use_current"), bUse && Objectives->GetActiveMissionId() == FName(UseMissionId), UseMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			const int32 Connected = Objectives->TriggerEvent(FName(ConnectEvent));
			AssertTrue(Record, TEXT("reject.connection_event"), Connected > 0, TEXT(">0"), FString::FromInt(Connected), ConnectEvent);
			AssertTrue(Record, TEXT("reject.connection_completed"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			AssertTrue(Record, TEXT("reject.revelation_still_inactive"), Objectives->GetActiveMissionId() == FName(UseMissionId) && CountActiveId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("inactive"), Objectives->GetActiveMissionId().ToString(), RevelationObjectiveId);
			Node->Interact(Character);
			AssertTrue(Record, TEXT("reject.after_event_no_fire"), Node->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Node->FollowupEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("reject.after_event_no_credit"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			AssertTrue(Record, TEXT("reject.follow_event_unchanged"), Node->ObjectiveEventId == FName(FollowEvent) && Node->ObjectiveEventFireCount == PrimaryFireBefore, FollowEvent, Node->ObjectiveEventId.ToString(), NodeLabel);

			const bool bRevelation = Revelation && Objectives->LoadMission(Revelation, false);
			AssertTrue(Record, TEXT("success.mission_loaded"), bRevelation && Objectives->GetActiveMissionId() == FName(RevelationMissionId), RevelationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.objective_active"), CountActiveId(Objectives, FName(RevelationObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			Terminal->Interact(Character);
			AssertTrue(Record, TEXT("reject.terminal_no_credit"), Node->FollowupEventFireCount == 0 && CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("0"), FString::FromInt(Node->FollowupEventFireCount), TerminalLabel);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bSuccess = Node->Interact(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.interact"), bSuccess, TEXT("true"), BoolText(bSuccess), NodeLabel);
			AssertTrue(Record, TEXT("success.fire"), Node->FollowupEventFireCount == 1, TEXT("1"), FString::FromInt(Node->FollowupEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 1 && CountActiveId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("completed"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			AssertTrue(Record, TEXT("success.mission"), Objectives->IsMissionComplete(FName(RevelationMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.notification"), Node->FollowupNotificationCount == 1, TEXT("1"), FString::FromInt(Node->FollowupNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.follow_untouched"), Node->ObjectiveEventFireCount == PrimaryFireBefore && Node->ObjectiveEventId == FName(FollowEvent), FollowEvent, FString::FromInt(Node->ObjectiveEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("success.no_equipment_change"), Adapt->GetEquippedAdaptationPath().ToString() == EquippedBefore, EquippedBefore, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.opening_not_complete"), !Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation"))), TEXT("false"), BoolText(Objectives->IsMissionComplete(FName(TEXT("Mission_OpeningFoundation")))), TEXT("mission"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Node->Interact(Character);
			AssertTrue(Record, TEXT("replay.fire_once"), Node->FollowupEventFireCount == 1 && Node->FollowupNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("fire=%d notes=%d"), Node->FollowupEventFireCount, Node->FollowupNotificationCount), NodeLabel);
			AssertTrue(Record, TEXT("replay.line_cleared"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("cleared"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->IsMissionComplete(FName(RevelationMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.guard_captured"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed) Stage = EStage::EndSession;
		}

		void TickReload(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Nodes = World ? OrganoidPlaytestActions::FindActorsByLabel(World, NodeLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Node = Nodes.Num() == 1 ? Cast<AProjectOrganoidInspectableInstrument>(Nodes[0]) : nullptr;
			if (!Objectives || !Saves || !Power || !Node || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->IsMissionComplete(FName(RevelationMissionId)), TEXT("complete"), Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 1 && CountActiveId(Objectives, FName(RevelationObjectiveId)) == 0, TEXT("completed"), FString::Printf(TEXT("completed=%d active=%d"), CountCompletedId(Objectives, FName(RevelationObjectiveId)), CountActiveId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("save.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			AssertTrue(Record, TEXT("save.notification_absent"), Node->FollowupNotificationCount == 0, TEXT("0"), FString::FromInt(Node->FollowupNotificationCount), TEXT("HUD"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Node->Interact(Character);
			AssertTrue(Record, TEXT("save.replay_no_fire"), Node->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Node->FollowupEventFireCount), NodeLabel);
			AssertTrue(Record, TEXT("save.replay_no_line"), Node->FollowupNotificationCount == 0, TEXT("0"), FString::FromInt(Node->FollowupNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("save.objective_remains"), CountCompletedId(Objectives, FName(RevelationObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(RevelationObjectiveId))), RevelationObjectiveId);
			AssertTrue(Record, TEXT("dirty.reload_neuro"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.reload_revelation"), !PackageIsDirty(RevelationPackage), TEXT("clean"), PackageIsDirty(RevelationPackage) ? TEXT("dirty") : TEXT("clean"), RevelationPackage);
			AssertTrue(Record, TEXT("dirty.reload_connection"), !PackageIsDirty(ConnectionPackage), TEXT("clean"), PackageIsDirty(ConnectionPackage) ? TEXT("dirty") : TEXT("clean"), ConnectionPackage);
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
		int32 PrimaryFireBefore = 0;
		FString EquippedBefore;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroRevelationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister GRegister;
}
