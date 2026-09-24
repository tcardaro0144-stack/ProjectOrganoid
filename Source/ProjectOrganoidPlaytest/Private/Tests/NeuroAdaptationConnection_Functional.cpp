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
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInspectableInstrument.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace NeuroAdaptationConnectionFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroAdaptationConnection_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Adaptation Connection Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR GeneticsSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroGenetics.DA_Mission_NeuroGenetics");
	constexpr TCHAR IntroSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro.DA_Mission_NeuroResearchStationIntro");
	constexpr TCHAR UseSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse.DA_Mission_NeuroNeuralSlowUse");
	constexpr TCHAR ConnectionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroAdaptationConnection.DA_Mission_NeuroAdaptationConnection");
	constexpr TCHAR RevelationSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroRevelation.DA_Mission_NeuroRevelation");
	constexpr TCHAR RevelationMissionId[] = TEXT("Mission_NeuroRevelation");
	constexpr TCHAR ConnectionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroAdaptationConnection");
	constexpr TCHAR UsePackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroNeuralSlowUse");
	constexpr TCHAR ConnectionMissionId[] = TEXT("Mission_NeuroAdaptationConnection");
	constexpr TCHAR UseMissionId[] = TEXT("Mission_NeuroNeuralSlowUse");
	constexpr TCHAR ConnectObjectiveId[] = TEXT("Obj_ConnectLiveAdaptation");
	constexpr TCHAR ApplyObjectiveId[] = TEXT("Obj_ApplyNeuralSlow");
	constexpr TCHAR ExamineObjectiveId[] = TEXT("Obj_ExamineNeuralChangeEvidence");
	constexpr TCHAR ConnectEvent[] = TEXT("Event_LiveAdaptationConnected");
	constexpr TCHAR ApplyEvent[] = TEXT("Event_NeuralSlowApplied");
	constexpr TCHAR EquipEvent[] = TEXT("Event_NeuralSlowEquipped");
	constexpr TCHAR ExamineEvent[] = TEXT("Event_NeuralChangeEvidenceExamined");
	constexpr TCHAR InstrumentLabel[] = TEXT("NeuralChangeEvidenceInstrument_NeuroGenetics");
	constexpr TCHAR ArrayLabel[] = TEXT("NeuralMappingArray_NeuroGenetics");
	constexpr TCHAR HostLabel[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroAdaptationConnectionTest");
	constexpr TCHAR ExpectedLine[] =
		TEXT("The live Host slowed the same way these records describe. Epitope was adapting nervous systems, not only recording them.");
	const FVector InstrumentLocation(500.f, -2520.f, -1100.f);

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

	class FNeuroAdaptationConnectionFunctional : public IOrganoidPlaytestCase
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
			EquippedBefore.Reset();
			ExamineFireBefore = 0;
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
						if (Record->FailureReason.IsEmpty())
						{
							Record->FailureReason = TEXT("A package was dirty after the test.");
						}
					}
					Owner.CompleteActive(
						bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass,
						Record->FailureReason);
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

		void ApplyFollowupContract(AProjectOrganoidInspectableInstrument* Instrument) const
		{
			if (!Instrument) return;
			Instrument->FollowupRequiredActiveObjectiveId = FName(ConnectObjectiveId);
			Instrument->FollowupPrerequisiteCompletedObjectiveId = FName(ApplyObjectiveId);
			Instrument->FollowupSuccessEventId = FName(ConnectEvent);
			Instrument->FollowupSpeakerLabel = FText::FromString(TEXT("Nathan"));
			Instrument->FollowupResponseText = FText::FromString(ExpectedLine);
			Instrument->FollowupNotificationDurationSeconds = 7.0f;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Connection = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConnectionSoftPath);
			UProjectOrganoidObjectiveDataAsset* Use = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, UseSoftPath);
			UProjectOrganoidObjectiveDataAsset* Intro = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroSoftPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Connection && Connection->Tasks.Num() == 1 ? &Connection->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.connection_id"), Connection && Connection->MissionId == FName(ConnectionMissionId), ConnectionMissionId, Connection ? Connection->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.connection_title"), Connection && Connection->MissionTitle.ToString() == TEXT("Connect Live Adaptation"), TEXT("Connect Live Adaptation"), Connection ? Connection->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.connection_description"), Connection && Connection->MissionDescription.ToString().Contains(TEXT("live Host")), TEXT("live Host"), Connection ? Connection->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.connection_next_revelation"), Connection && Connection->NextMissionAsset.ToSoftObjectPath().ToString() == RevelationSoftPath, RevelationSoftPath, Connection ? Connection->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.connection_one_task"), Connection && Connection->Tasks.Num() == 1, TEXT("1"), Connection ? FString::FromInt(Connection->Tasks.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_id"), Task && Task->Objective.ObjectiveId == FName(ConnectObjectiveId), ConnectObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_title"), Task && Task->Objective.Title.ToString() == TEXT("Connect Live Adaptation to Evidence"), TEXT("Connect Live Adaptation to Evidence"), Task ? Task->Objective.Title.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_description"), Task && Task->Objective.Description.ToString().Contains(TEXT("neural evidence instrument")), TEXT("neural evidence instrument"), Task ? Task->Objective.Description.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), Task ? TEXT("other") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_target"), Task && Task->Objective.TargetProgress == 1, TEXT("1"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_auto"), Task && Task->bAutoActivate, TEXT("true"), Task ? BoolText(Task->bAutoActivate) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_no_prereq"), Task && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), Task ? FString::FromInt(Task->Objective.PrerequisiteObjectiveIds.Num()) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.task_event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(ConnectEvent), ConnectEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.intro_next_use"), Intro && Intro->NextMissionAsset.ToSoftObjectPath().ToString() == UseSoftPath, UseSoftPath, Intro ? Intro->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.use_next_connection"), Use && Use->NextMissionAsset.ToSoftObjectPath().ToString() == ConnectionSoftPath, ConnectionSoftPath, Use ? Use->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Authored = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, InstrumentLabel) : TArray<AActor*>();
			AActor* AuthoredActor = Authored.Num() == 1 ? Authored[0] : nullptr;
			AProjectOrganoidInspectableInstrument* AuthoredInstrument = Cast<AProjectOrganoidInspectableInstrument>(AuthoredActor);
			const FString PackageName = AuthoredActor && AuthoredActor->GetOutermost() ? AuthoredActor->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("instrument.editor_count"), Authored.Num() == 1, TEXT("1"), FString::FromInt(Authored.Num()), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.editor_class"), AuthoredInstrument != nullptr, TEXT("InspectableInstrument"), AuthoredActor ? AuthoredActor->GetClass()->GetName() : TEXT("missing"), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.editor_package"), PackageName.Contains(TEXT("SL_Epitope_NeuroGenetics")), NeuroPackage, PackageName, InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.editor_location"), AuthoredActor && AuthoredActor->GetActorLocation().Equals(InstrumentLocation, 1.f), TEXT("500,-2520,-1100"), AuthoredActor ? AuthoredActor->GetActorLocation().ToString() : TEXT("missing"), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.examine_required"), AuthoredInstrument && AuthoredInstrument->RequiredActiveObjectiveId == FName(ExamineObjectiveId), ExamineObjectiveId, AuthoredInstrument ? AuthoredInstrument->RequiredActiveObjectiveId.ToString() : TEXT("missing"), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.examine_guard"), AuthoredInstrument && AuthoredInstrument->CompletedObjectiveIdForReplayGuard == FName(ExamineObjectiveId), ExamineObjectiveId, AuthoredInstrument ? AuthoredInstrument->CompletedObjectiveIdForReplayGuard.ToString() : TEXT("missing"), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.examine_event"), AuthoredInstrument && AuthoredInstrument->ObjectiveEventId == FName(ExamineEvent), ExamineEvent, AuthoredInstrument ? AuthoredInstrument->ObjectiveEventId.ToString() : TEXT("missing"), InstrumentLabel);
			AssertTrue(Record, TEXT("dirty.root_clean"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.neuro_clean"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			AssertTrue(Record, TEXT("dirty.connection_clean"), !PackageIsDirty(ConnectionPackage), TEXT("clean"), PackageIsDirty(ConnectionPackage) ? TEXT("dirty") : TEXT("clean"), ConnectionPackage);
			AssertTrue(Record, TEXT("dirty.use_clean"), !PackageIsDirty(UsePackage), TEXT("clean"), PackageIsDirty(UsePackage) ? TEXT("dirty") : TEXT("clean"), UsePackage);
			if (bAnyAssertFailed || !Connection || !Use || !Intro || !AuthoredInstrument)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Beat 11 mission contract missing.") : Record.FailureReason);
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
			if (Character && bRequestedNeuroStream && World && OrganoidPlaytestActions::FindActorsByLabel(World, InstrumentLabel).Num() == 1)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Owner, Record, TEXT("PIE did not become ready with the neural evidence instrument."));
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
				if (WaitSeconds > 10.f)
				{
					FailAndStop(Owner, Record, TEXT("HUD was not ready."));
				}
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

		void TickCampaign(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			UProjectOrganoidHUDWidget* Widget)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>()
				: nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>()
				: nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Instruments = World ? OrganoidPlaytestActions::FindActorsByLabel(World, InstrumentLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Instrument = Instruments.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(Instruments[0])
				: nullptr;
			TArray<AActor*> Arrays = World ? OrganoidPlaytestActions::FindActorsByLabel(World, ArrayLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Array = Arrays.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(Arrays[0])
				: nullptr;
			TArray<AActor*> Hosts = World ? OrganoidPlaytestActions::FindActorsByLabel(World, HostLabel) : TArray<AActor*>();
			AProjectOrganoidHostBase* Host = Hosts.Num() == 1 ? Cast<AProjectOrganoidHostBase>(Hosts[0]) : nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			if (!World || !Character || !Objectives || !Saves || !Power || !Instrument || !Array || !Host || !Adapt || !Widget)
			{
				FailAndStop(Owner, Record, TEXT("Beat 11 actors or subsystems missing."));
				return;
			}

			const FString PiePackage = Instrument->GetOutermost() ? Instrument->GetOutermost()->GetName() : TEXT("missing");
			AssertTrue(Record, TEXT("instrument.pie_package"), PiePackage.Contains(TEXT("SL_Epitope_NeuroGenetics")), NeuroPackage, PiePackage, InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.pie_location"), Instrument->GetActorLocation().Equals(InstrumentLocation, 5.f), TEXT("500,-2520,-1100"), Instrument->GetActorLocation().ToString(), InstrumentLabel);
			AssertTrue(Record, TEXT("instrument.pie_label"), Instrument->GetActorLabel() == InstrumentLabel, InstrumentLabel, Instrument->GetActorLabel(), InstrumentLabel);

			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("power.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			EquippedBefore = Adapt->GetEquippedAdaptationPath().ToString();

			Instrument->FollowupRequiredActiveObjectiveId = NAME_None;
			Instrument->FollowupPrerequisiteCompletedObjectiveId = NAME_None;
			Instrument->FollowupSuccessEventId = NAME_None;
			const bool bOrdinary = Instrument->Interact(Character);
			AssertTrue(Record, TEXT("ordinary.unconfigured_no_fire"), Instrument->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupEventFireCount), InstrumentLabel);
			AssertTrue(Record, TEXT("ordinary.unconfigured_no_credit"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			(void)bOrdinary;

			AssertTrue(Record, TEXT("ordinary.array_unconfigured"), Array->FollowupSuccessEventId.IsNone(), TEXT("None"), Array->FollowupSuccessEventId.ToString(), ArrayLabel);
			AssertTrue(Record, TEXT("ordinary.host_not_instrument"), !Host->IsA(AProjectOrganoidInspectableInstrument::StaticClass()), TEXT("host"), Host->GetClass()->GetName(), HostLabel);
			Array->Interact(Character);
			AssertTrue(Record, TEXT("ordinary.array_no_connection"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 0 && Instrument->FollowupEventFireCount == 0 && Array->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupEventFireCount), ArrayLabel);
			ApplyFollowupContract(Instrument);
			AssertTrue(Record, TEXT("hook.required"), Instrument->FollowupRequiredActiveObjectiveId == FName(ConnectObjectiveId), ConnectObjectiveId, Instrument->FollowupRequiredActiveObjectiveId.ToString(), InstrumentLabel);
			AssertTrue(Record, TEXT("hook.prerequisite"), Instrument->FollowupPrerequisiteCompletedObjectiveId == FName(ApplyObjectiveId), ApplyObjectiveId, Instrument->FollowupPrerequisiteCompletedObjectiveId.ToString(), InstrumentLabel);
			AssertTrue(Record, TEXT("hook.event"), Instrument->FollowupSuccessEventId == FName(ConnectEvent), ConnectEvent, Instrument->FollowupSuccessEventId.ToString(), InstrumentLabel);
			AssertTrue(Record, TEXT("hook.speaker"), Instrument->FollowupSpeakerLabel.ToString() == TEXT("Nathan"), TEXT("Nathan"), Instrument->FollowupSpeakerLabel.ToString(), InstrumentLabel);
			AssertTrue(Record, TEXT("hook.duration"), FMath::IsNearlyEqual(Instrument->FollowupNotificationDurationSeconds, 7.f), TEXT("7"), FString::SanitizeFloat(Instrument->FollowupNotificationDurationSeconds), InstrumentLabel);
			AssertTrue(Record, TEXT("hook.line"), Instrument->FollowupResponseText.ToString() == ExpectedLine, ExpectedLine, Instrument->FollowupResponseText.ToString(), InstrumentLabel);

			UProjectOrganoidObjectiveDataAsset* Genetics = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, GeneticsSoftPath);
			UProjectOrganoidObjectiveDataAsset* Intro = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroSoftPath);
			UProjectOrganoidObjectiveDataAsset* Use = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, UseSoftPath);
			UProjectOrganoidObjectiveDataAsset* Connection = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ConnectionSoftPath);
			const bool bGenetics = Genetics && Objectives->LoadMission(Genetics, false);
			Objectives->TriggerEvent(FName(TEXT("Event_NeuroResearchLoadIsolated")));
			Objectives->TriggerEvent(FName(TEXT("Event_NeuralMappingSignalTraced")));
			Objectives->TriggerEvent(FName(TEXT("Event_NeuralSignatureFollowed")));
			Objectives->TriggerEvent(FName(ExamineEvent));
			AssertTrue(Record, TEXT("chain.examine_completed"), bGenetics && CountCompletedId(Objectives, FName(ExamineObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ExamineObjectiveId))), ExamineObjectiveId);

			const bool bIntro = Intro && Objectives->LoadMission(Intro, false);
			const int32 EquipHandled = Objectives->TriggerEvent(FName(EquipEvent));
			AssertTrue(Record, TEXT("chain.intro_loaded"), bIntro, TEXT("true"), BoolText(bIntro), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.equip_handled"), EquipHandled > 0, TEXT(">0"), FString::FromInt(EquipHandled), EquipEvent);
			AssertTrue(Record, TEXT("chain.use_current"), Objectives->GetActiveMissionId() == FName(UseMissionId), UseMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.apply_active"), CountActiveId(Objectives, FName(ApplyObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(ApplyObjectiveId))), ApplyObjectiveId);
			AssertTrue(Record, TEXT("reject.before_slow_not_active"), CountActiveId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountActiveId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			Instrument->Interact(Character);
			AssertTrue(Record, TEXT("reject.before_slow_no_fire"), Instrument->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupEventFireCount), InstrumentLabel);
			AssertTrue(Record, TEXT("reject.before_slow_no_credit"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);

			const bool bEarlyConnection = Connection && Objectives->LoadMission(Connection, false);
			AssertTrue(Record, TEXT("reject.connection_forced_active"), bEarlyConnection && CountActiveId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("active"), Objectives->GetActiveMissionId().ToString(), ConnectObjectiveId);
			AssertTrue(Record, TEXT("reject.prerequisite_open"), CountCompletedId(Objectives, FName(ApplyObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(ApplyObjectiveId))), ApplyObjectiveId);
			Instrument->Interact(Character);
			AssertTrue(Record, TEXT("reject.without_event_no_fire"), Instrument->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupEventFireCount), InstrumentLabel);
			AssertTrue(Record, TEXT("reject.without_event_no_credit"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);

			const bool bUseReloaded = Use && Objectives->LoadMission(Use, false);
			if (!bUseReloaded || Objectives->GetActiveMissionId() != FName(UseMissionId))
			{
				FailAndStop(Owner, Record, TEXT("Neural Slow use mission did not reload."));
				return;
			}
			ExamineFireBefore = Instrument->ObjectiveEventFireCount;
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const int32 Applied = Objectives->TriggerEvent(FName(ApplyEvent));
			AssertTrue(Record, TEXT("chain.slow_event"), Applied > 0, TEXT(">0"), FString::FromInt(Applied), ApplyEvent);
			AssertTrue(Record, TEXT("chain.apply_completed"), CountCompletedId(Objectives, FName(ApplyObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ApplyObjectiveId))), ApplyObjectiveId);
			AssertTrue(Record, TEXT("chain.connection_current"), Objectives->GetActiveMissionId() == FName(ConnectionMissionId), ConnectionMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("chain.connect_active"), CountActiveId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);

			const bool bSuccess = Instrument->Interact(Character);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("success.interact"), bSuccess, TEXT("true"), BoolText(bSuccess), InstrumentLabel);
			AssertTrue(Record, TEXT("success.fire"), Instrument->FollowupEventFireCount == 1, TEXT("1"), FString::FromInt(Instrument->FollowupEventFireCount), InstrumentLabel);
			AssertTrue(Record, TEXT("success.objective"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1 && CountActiveId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("completed"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			AssertTrue(Record, TEXT("success.revelation_current"), Objectives->GetActiveMissionId() == FName(RevelationMissionId), RevelationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("success.notification"), Instrument->FollowupNotificationCount == 1, TEXT("1"), FString::FromInt(Instrument->FollowupNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("success.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("success.examine_untouched"), Instrument->ObjectiveEventFireCount == ExamineFireBefore, FString::FromInt(ExamineFireBefore), FString::FromInt(Instrument->ObjectiveEventFireCount), ExamineEvent);
			AssertTrue(Record, TEXT("success.no_equipment_change"), Adapt->GetEquippedAdaptationPath().ToString() == EquippedBefore, EquippedBefore, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
			AssertTrue(Record, TEXT("success.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("success.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Instrument->Interact(Character);
			AssertTrue(Record, TEXT("replay.fire_once"), Instrument->FollowupEventFireCount == 1 && Instrument->FollowupNotificationCount == 1, TEXT("1"), FString::Printf(TEXT("fire=%d notes=%d"), Instrument->FollowupEventFireCount, Instrument->FollowupNotificationCount), InstrumentLabel);
			AssertTrue(Record, TEXT("ordinary.host_no_credit"), Host->AdaptationCampaignEventFireCount == 0 && CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("0"), FString::FromInt(Host->AdaptationCampaignEventFireCount), HostLabel);

			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission_captured"), bSaved && Objectives->GetActiveMissionId() == FName(RevelationMissionId), RevelationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.guard_captured"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), TEXT("save"));
			StopIfFailed();
			if (!bAnyAssertFailed)
			{
				Stage = EStage::EndSession;
			}
		}

		void TickReload(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			UProjectOrganoidHUDWidget* Widget)
		{
			bReloadDone = true;
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>()
				: nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance()
				? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>()
				: nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			TArray<AActor*> Instruments = World ? OrganoidPlaytestActions::FindActorsByLabel(World, InstrumentLabel) : TArray<AActor*>();
			AProjectOrganoidInspectableInstrument* Instrument = Instruments.Num() == 1
				? Cast<AProjectOrganoidInspectableInstrument>(Instruments[0])
				: nullptr;
			if (!Objectives || !Saves || !Power || !Instrument || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Reload actors missing."));
				return;
			}

			ApplyFollowupContract(Instrument);
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("save.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(RevelationMissionId), RevelationMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("save"));
			AssertTrue(Record, TEXT("save.objective"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1 && CountActiveId(Objectives, FName(ConnectObjectiveId)) == 0, TEXT("completed"), FString::Printf(TEXT("completed=%d active=%d"), CountCompletedId(Objectives, FName(ConnectObjectiveId)), CountActiveId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
			AssertTrue(Record, TEXT("save.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Instrument->Interact(Character);
			AssertTrue(Record, TEXT("save.replay_no_fire"), Instrument->FollowupEventFireCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupEventFireCount), InstrumentLabel);
			AssertTrue(Record, TEXT("save.replay_no_line"), Instrument->FollowupNotificationCount == 0, TEXT("0"), FString::FromInt(Instrument->FollowupNotificationCount), TEXT("HUD"));
			AssertTrue(Record, TEXT("save.objective_remains"), CountCompletedId(Objectives, FName(ConnectObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(ConnectObjectiveId))), ConnectObjectiveId);
			AssertTrue(Record, TEXT("dirty.reload_neuro"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			AssertTrue(Record, TEXT("dirty.reload_root"), !PackageIsDirty(MapPackage), TEXT("clean"), PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"), MapPackage);
			AssertTrue(Record, TEXT("dirty.reload_connection"), !PackageIsDirty(ConnectionPackage), TEXT("clean"), PackageIsDirty(ConnectionPackage) ? TEXT("dirty") : TEXT("clean"), ConnectionPackage);
			AssertTrue(Record, TEXT("dirty.reload_use"), !PackageIsDirty(UsePackage), TEXT("clean"), PackageIsDirty(UsePackage) ? TEXT("dirty") : TEXT("clean"), UsePackage);
			StopIfFailed();
			if (!bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
			}
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bSecondSession = false;
		bool bCampaignDone = false;
		bool bReloadDone = false;
		int32 ExamineFireBefore = 0;
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
				return MakeShared<FNeuroAdaptationConnectionFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister GRegister;
}
