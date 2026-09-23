#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveDataAsset.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"

namespace NeuroResearchStationIntroFunctional
{
	constexpr TCHAR TestId[] = TEXT("NeuroResearchStationIntro_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Research Station Intro Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR TargetingMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroTargetingWhy");
	constexpr TCHAR TargetingMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroTargetingWhy.DA_Mission_NeuroTargetingWhy");
	constexpr TCHAR IntroMissionPackage[] = TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro");
	constexpr TCHAR IntroMissionSoftPath[] =
		TEXT("/Game/Data/Missions/DA_Mission_NeuroResearchStationIntro.DA_Mission_NeuroResearchStationIntro");
	constexpr TCHAR AdaptationPath[] =
		TEXT("/Game/Data/Adaptations/DA_Adaptation_NeuralSlow.DA_Adaptation_NeuralSlow");

	constexpr TCHAR TargetingMissionId[] = TEXT("Mission_NeuroTargetingWhy");
	constexpr TCHAR IntroMissionId[] = TEXT("Mission_NeuroResearchStationIntro");
	constexpr TCHAR IntroMissionTitle[] = TEXT("Use the Research Station");
	constexpr TCHAR IntroMissionDescription[] =
		TEXT("Mount the neural adaptation at the NeuroGenetics Research Station.");
	constexpr TCHAR IntroObjectiveId[] = TEXT("Obj_EquipNeuralSlow");
	constexpr TCHAR IntroObjectiveTitle[] = TEXT("Equip Neural Slow");
	constexpr TCHAR IntroObjectiveDescription[] = TEXT("Open the Research Station and equip Neural Slow.");
	constexpr TCHAR IntroEvent[] = TEXT("Event_NeuralSlowEquipped");
	constexpr TCHAR TargetingEvent[] = TEXT("Event_NeuroLocomotorTargetDemonstrated");

	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR ResearcherLabel[] = TEXT("Host_Neuro_Researcher");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroResearchStationIntroTest");
	constexpr TCHAR ExpectedSpeaker[] = TEXT("Nathan");
	constexpr TCHAR ExpectedLine[] =
		TEXT("Neural Slow is mounted. Research Stations can swap unlocked adaptations without spending SOT.");
	constexpr float ExpectedDuration = 7.0f;
	const FVector StationStand(720.0f, -1600.0f, -1100.0f);
	const FVector AwayFromStation(0.0f, 0.0f, 200.0f);

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

	FString BoolText(bool bValue) { return bValue ? TEXT("true") : TEXT("false"); }

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

	int32 CountLabel(UWorld* World, const TCHAR* Label)
	{
		return World ? OrganoidPlaytestActions::FindActorsByLabel(World, Label).Num() : 0;
	}

	class FNeuroResearchStationIntroFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::PreActive;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			Host1Before = 0;
			Host2Before = 0;
			Host3Before = 0;
			ResearcherBefore = 0;
			DirtyBefore.Reset();
			WrongStation.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroyWrongStation();
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
				TickProof(Owner, *Record, DeltaTime);
				break;
			case EStage::EndPie:
				DestroyWrongStation();
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

		enum class EProof : uint8
		{
			PreActive,
			Handoff,
			CombatBlock,
			CombatClear,
			UnlockAndNegatives,
			Equip,
			Replay,
			SaveLoad,
			MigrationAway,
			MigrationVisit,
			SideEffects
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::PreActive;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		int32 Host1Before = 0;
		int32 Host2Before = 0;
		int32 Host3Before = 0;
		int32 ResearcherBefore = 0;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidResearchStation> WrongStation;

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

		void DestroyWrongStation()
		{
			if (AProjectOrganoidResearchStation* Station = WrongStation.Get())
			{
				Station->Destroy();
			}
			WrongStation.Reset();
		}

		void StandAtStation(AProjectOrganoidCharacter* Character, AProjectOrganoidResearchStation* Station) const
		{
			if (!Character || !Station)
			{
				return;
			}
			const float StandZ = Station->GetActorLocation().Z;
			OrganoidPlaytestActions::TeleportNear(Character, FVector(StationStand.X, StationStand.Y, StandZ), 0.0f, StandZ);
		}

		UProjectOrganoidHUDWidget* ResolveHud(AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			if (!PC || !PC->GetWorld())
			{
				return nullptr;
			}
			AProjectOrganoidGameMode* GameMode = PC->GetWorld()->GetAuthGameMode<AProjectOrganoidGameMode>();
			UProjectOrganoidGameplayHUDController* Controller = GameMode ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return Controller ? Controller->GetBoundHUDWidget() : nullptr;
		}

		bool AdaptationPathEquals(const UProjectOrganoidBiologicalAdaptationData* Adaptation, const TCHAR* Path) const
		{
			return Adaptation && FSoftObjectPath(Adaptation).ToString() == Path;
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(NeuroPackage)
				|| PackageIsDirty(TargetingMissionPackage) || PackageIsDirty(IntroMissionPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Required map or mission packages are dirty. Refusing to start."));
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

			UProjectOrganoidObjectiveDataAsset* TargetingDA =
				LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, TargetingMissionSoftPath);
			UProjectOrganoidObjectiveDataAsset* IntroDA =
				LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroMissionSoftPath);
			AssertTrue(Record, TEXT("asset.targeting_present"), TargetingDA != nullptr, TEXT("present"), TargetingDA ? TEXT("present") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.intro_present"), IntroDA != nullptr, TEXT("present"), IntroDA ? TEXT("present") : TEXT("missing"), TEXT("DA"));
			if (TargetingDA)
			{
				AssertTrue(
					Record, TEXT("asset.targeting_next_intro"),
					TargetingDA->NextMissionAsset.ToSoftObjectPath().ToString() == IntroMissionSoftPath,
					IntroMissionSoftPath,
					TargetingDA->NextMissionAsset.IsNull() ? TEXT("null") : TargetingDA->NextMissionAsset.ToSoftObjectPath().ToString(),
					TEXT("DA"));
			}
			if (IntroDA)
			{
				AssertTrue(Record, TEXT("asset.intro_id"), IntroDA->MissionId == FName(IntroMissionId), IntroMissionId, IntroDA->MissionId.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.intro_title"), IntroDA->MissionTitle.ToString() == IntroMissionTitle, IntroMissionTitle, IntroDA->MissionTitle.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.intro_description"), IntroDA->MissionDescription.ToString() == IntroMissionDescription, IntroMissionDescription, IntroDA->MissionDescription.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.intro_next_null"), IntroDA->NextMissionAsset.IsNull(), TEXT("null"), IntroDA->NextMissionAsset.IsNull() ? TEXT("null") : IntroDA->NextMissionAsset.ToString(), TEXT("DA"));
				AssertTrue(Record, TEXT("asset.intro_task_count"), IntroDA->Tasks.Num() == 1, TEXT("1"), FString::FromInt(IntroDA->Tasks.Num()), TEXT("DA"));
				if (IntroDA->Tasks.Num() == 1)
				{
					const FProjectOrganoidMissionTaskDefinition& Task = IntroDA->Tasks[0];
					AssertTrue(Record, TEXT("asset.intro_objective_id"), Task.Objective.ObjectiveId == FName(IntroObjectiveId), IntroObjectiveId, Task.Objective.ObjectiveId.ToString(), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_objective_title"), Task.Objective.Title.ToString() == IntroObjectiveTitle, IntroObjectiveTitle, Task.Objective.Title.ToString(), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_objective_description"), Task.Objective.Description.ToString() == IntroObjectiveDescription, IntroObjectiveDescription, Task.Objective.Description.ToString(), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_main"), Task.Objective.Type == EProjectOrganoidObjectiveType::Main, TEXT("Main"), TEXT("checked"), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_target"), Task.Objective.TargetProgress == 1, TEXT("1"), FString::FromInt(Task.Objective.TargetProgress), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_autoactivate"), Task.bAutoActivate, TEXT("true"), BoolText(Task.bAutoActivate), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_no_prereq"), Task.Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("0"), FString::FromInt(Task.Objective.PrerequisiteObjectiveIds.Num()), TEXT("DA"));
					AssertTrue(Record, TEXT("asset.intro_one_event"), Task.EventTriggers.Num() == 1, TEXT("1"), FString::FromInt(Task.EventTriggers.Num()), TEXT("DA"));
					if (Task.EventTriggers.Num() == 1)
					{
						AssertTrue(Record, TEXT("asset.intro_event"), Task.EventTriggers[0].EventId == FName(IntroEvent), IntroEvent, Task.EventTriggers[0].EventId.ToString(), TEXT("DA"));
					}
				}
			}
			if (bAnyAssertFailed || !TargetingDA || !IntroDA)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Beat 9 mission contract missing or mismatched."));
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

			bool bPlayerReady = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bPlayerReady = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling || WaitSeconds > 10.0f;
				}
			}

			const bool bActorsReady = CountLabel(World, StationLabel) == 1 && CountLabel(World, ResearcherLabel) == 1;
			if (Character && bPlayerReady && bActorsReady)
			{
				Stage = EStage::Proof;
				Proof = EProof::PreActive;
				WaitSeconds = 0.0f;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 90.0f)
			{
				FailAndStop(Owner, Record, TEXT("Timed out waiting for PIE player, station, and Researcher."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
			UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance ? GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = GameInstance ? GameInstance->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			AActor* StationActor = World ? OrganoidPlaytestActions::FindUniqueByLabel(World, StationLabel) : nullptr;
			AProjectOrganoidResearchStation* Station = Cast<AProjectOrganoidResearchStation>(StationActor);
			AProjectOrganoidHostBase* Researcher = Cast<AProjectOrganoidHostBase>(
				World ? OrganoidPlaytestActions::FindUniqueByLabel(World, ResearcherLabel) : nullptr);
			AProjectOrganoidHostAIController* HostAI = Researcher
				? Cast<AProjectOrganoidHostAIController>(Researcher->GetController())
				: nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			UProjectOrganoidBiologicalAdaptationData* NeuralSlow = UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve();
			UProjectOrganoidEncounterPresenceSubsystem* Presence = World ? World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>() : nullptr;

			if (!World || !Character || !Objectives || !Power || !Saves || !Station || !Researcher || !HostAI || !Adapt || !NeuralSlow || !Presence)
			{
				FailAndStop(Owner, Record, TEXT("PIE station, Researcher, adaptations, or subsystems missing."));
				return;
			}

			switch (Proof)
			{
			case EProof::PreActive:
			{
				Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
				Power->SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
				Host1Before = CountLabel(World, Host1Label);
				Host2Before = CountLabel(World, Host2Label);
				Host3Before = CountLabel(World, Host3Label);
				ResearcherBefore = CountLabel(World, ResearcherLabel);

				AssertTrue(Record, TEXT("station.class"), Station->GetClass() == AProjectOrganoidResearchStation::StaticClass(), TEXT("AProjectOrganoidResearchStation"), Station->GetClass()->GetName(), StationLabel);
				AssertTrue(Record, TEXT("station.prompt"), Station->GetInteractionPrompt().ToString() == TEXT("Use Research Station"), TEXT("Use Research Station"), Station->GetInteractionPrompt().ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.objective"), Station->CampaignRequiredActiveObjectiveId == FName(IntroObjectiveId), IntroObjectiveId, Station->CampaignRequiredActiveObjectiveId.ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.unlock_path"), Station->CampaignUnlockAdaptation.ToSoftObjectPath().ToString() == AdaptationPath, AdaptationPath, Station->CampaignUnlockAdaptation.ToSoftObjectPath().ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.credit_path"), Station->CampaignCreditAdaptation.ToSoftObjectPath().ToString() == AdaptationPath, AdaptationPath, Station->CampaignCreditAdaptation.ToSoftObjectPath().ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.event"), Station->CampaignSuccessObjectiveEventId == FName(IntroEvent), IntroEvent, Station->CampaignSuccessObjectiveEventId.ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.replay_guard"), Station->CampaignReplayGuardObjectiveId == FName(IntroObjectiveId), IntroObjectiveId, Station->CampaignReplayGuardObjectiveId.ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.speaker"), Station->CampaignSuccessNotificationSpeaker.ToString() == ExpectedSpeaker, ExpectedSpeaker, Station->CampaignSuccessNotificationSpeaker.ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.line"), Station->CampaignSuccessNotificationText.ToString() == ExpectedLine, ExpectedLine, Station->CampaignSuccessNotificationText.ToString(), StationLabel);
				AssertTrue(Record, TEXT("station.duration"), FMath::IsNearlyEqual(Station->CampaignSuccessNotificationDurationSeconds, ExpectedDuration), TEXT("7.0"), FString::SanitizeFloat(Station->CampaignSuccessNotificationDurationSeconds), StationLabel);
				AssertTrue(Record, TEXT("pre.not_current"), Objectives->GetActiveMissionId() != FName(IntroMissionId), TEXT("not intro"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));

				StandAtStation(Character, Station);
				const bool bOpened = Station->Interact(Character);
				AssertTrue(Record, TEXT("pre.open"), bOpened && Station->IsStationUIOpen(), TEXT("open"), bOpened ? TEXT("open") : TEXT("failed"), StationLabel);
				AssertTrue(Record, TEXT("pre.no_unlock"), !Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("locked"), Adapt->IsAdaptationUnlocked(NeuralSlow) ? TEXT("unlocked") : TEXT("locked"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("pre.no_event"), Station->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				AssertTrue(Record, TEXT("pre.no_credit"), CountCompletedId(Objectives, FName(IntroObjectiveId)) == 0, TEXT("0"), FString::FromInt(CountCompletedId(Objectives, FName(IntroObjectiveId))), IntroObjectiveId);
				if (UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget())
				{
					Widget->CloseStationUI();
				}
				Proof = EProof::Handoff;
				break;
			}
			case EProof::Handoff:
			{
				UProjectOrganoidObjectiveDataAsset* TargetingDA =
					LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, TargetingMissionSoftPath);
				if (!TargetingDA || !Objectives->LoadMission(TargetingDA, false))
				{
					FailAndStop(Owner, Record, TEXT("Failed to load Targeting Why for the real NextMission handoff."));
					return;
				}
				Objectives->TriggerEvent(FName(TargetingEvent));
				AssertTrue(Record, TEXT("handoff.intro_current"), Objectives->GetActiveMissionId() == FName(IntroMissionId), IntroMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
				AssertTrue(Record, TEXT("handoff.objective_active"), CountActiveId(Objectives, FName(IntroObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(IntroObjectiveId))), IntroObjectiveId);
				AssertTrue(Record, TEXT("handoff.no_unlock_yet"), !Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("locked"), Adapt->IsAdaptationUnlocked(NeuralSlow) ? TEXT("unlocked") : TEXT("locked"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("handoff.no_event"), Station->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				Proof = EProof::CombatBlock;
				break;
			}
			case EProof::CombatBlock:
			{
				Researcher->ActivateEncounter();
				HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Pursue);
				StandAtStation(Character, Station);
				const bool bBlocked = !Station->CanInteract(Character) && Station->IsLockedByEncounter() && Presence->IsEncounterActive();
				AssertTrue(Record, TEXT("combat.pursue_blocks"), bBlocked, TEXT("blocked"), bBlocked ? TEXT("blocked") : TEXT("allowed"), StationLabel);
				const bool bInteracted = Station->Interact(Character);
				AssertTrue(Record, TEXT("combat.interact_rejected"), !bInteracted && !Station->IsStationUIOpen(), TEXT("rejected"), bInteracted ? TEXT("opened") : TEXT("rejected"), StationLabel);
				AssertTrue(Record, TEXT("combat.no_unlock"), !Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("locked"), Adapt->IsAdaptationUnlocked(NeuralSlow) ? TEXT("unlocked") : TEXT("locked"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("combat.no_event"), Station->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				WaitSeconds = 0.0f;
				Proof = EProof::CombatClear;
				break;
			}
			case EProof::CombatClear:
			{
				WaitSeconds += DeltaTime;
				const EProjectOrganoidHostCombatState State = HostAI->GetCombatState();
				const bool bCleared = !Presence->IsEncounterActive()
					&& !Presence->DoesCombatStateLockStations(State)
					&& Station->CanInteract(Character);
				if (!bCleared && WaitSeconds < 3.0f)
				{
					break;
				}
				AssertTrue(Record, TEXT("combat.cleared_naturally"), bCleared, TEXT("Search or non-locking"), HostAI->GetCombatState() == EProjectOrganoidHostCombatState::Pursue ? TEXT("still Pursue") : TEXT("cleared"), ResearcherLabel);
				if (!bCleared)
				{
					FailAndStop(Owner, Record, TEXT("Researcher stayed in a station-locking state. Station remains blocked."));
					return;
				}
				Proof = EProof::UnlockAndNegatives;
				break;
			}
			case EProof::UnlockAndNegatives:
			{
				AProjectOrganoidResearchStation* Other = World->SpawnActor<AProjectOrganoidResearchStation>(
					AProjectOrganoidResearchStation::StaticClass(), Character->GetActorLocation(), FRotator::ZeroRotator);
				WrongStation = Other;
				if (!Other)
				{
					FailAndStop(Owner, Record, TEXT("Failed to spawn an unconfigured station."));
					return;
				}
				Other->SetActorLabel(TEXT("ResearchStation_Wrong_Beat9"));
				const bool bWrong = Other->Interact(Character);
				AssertTrue(Record, TEXT("negative.wrong_station_opens"), bWrong, TEXT("open"), bWrong ? TEXT("open") : TEXT("failed"), TEXT("wrong station"));
				AssertTrue(Record, TEXT("negative.wrong_station_no_unlock"), !Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("locked"), Adapt->IsAdaptationUnlocked(NeuralSlow) ? TEXT("unlocked") : TEXT("locked"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("negative.wrong_station_no_event"), Station->CampaignSuccessEventFireCount == 0 && Other->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				if (UProjectOrganoidResearchStationWidget* WrongWidget = Other->GetActiveStationWidget())
				{
					WrongWidget->CloseStationUI();
				}
				DestroyWrongStation();

				AssertTrue(Record, TEXT("negative.equip_before_unlock"), !Station->TryEquipUnlockedAdaptation(Character, NeuralSlow), TEXT("false"), TEXT("checked"), TEXT("adaptation"));
				Adapt->UnlockAdaptation(NeuralSlow);
				AssertTrue(Record, TEXT("setup.already_unlocked"), Adapt->IsAdaptationUnlocked(NeuralSlow) && Adapt->GetUnlockedAdaptationPaths().Num() == 1, TEXT("1"), FString::FromInt(Adapt->GetUnlockedAdaptationPaths().Num()), TEXT("adaptation"));
				AssertTrue(Record, TEXT("setup.not_equipped"), Adapt->GetEquippedAdaptation() == nullptr, TEXT("none"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("none"), TEXT("adaptation"));

				StandAtStation(Character, Station);
				AssertTrue(
					Record, TEXT("active.in_range"),
					FVector::Dist(Character->GetActorLocation(), Station->GetActorLocation()) <= Station->InteractionRange,
					FString::SanitizeFloat(Station->InteractionRange),
					FString::SanitizeFloat(FVector::Dist(Character->GetActorLocation(), Station->GetActorLocation())),
					StationLabel);
				const bool bOpened = Station->Interact(Character);
				UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget();
				AssertTrue(Record, TEXT("active.open"), bOpened && Station->IsStationUIOpen() && Widget, TEXT("open"), bOpened ? TEXT("open") : TEXT("failed"), StationLabel);
				AssertTrue(Record, TEXT("active.unlock_once"), Adapt->GetUnlockedAdaptationPaths().Num() == 1 && Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("1"), FString::FromInt(Adapt->GetUnlockedAdaptationPaths().Num()), TEXT("adaptation"));
				AssertTrue(Record, TEXT("active.no_event_on_open"), Station->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				AssertTrue(Record, TEXT("active.objective_still_active"), CountActiveId(Objectives, FName(IntroObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountActiveId(Objectives, FName(IntroObjectiveId))), IntroObjectiveId);

				UProjectOrganoidWeaponModData* Barrel = UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve();
				const bool bBarrelUnlockedBefore = Character->IsWeaponModUnlocked(Barrel);
				const bool bBarrelInstalled = Widget && Widget->InstallUnlockedMod(Barrel);
				const bool bRemoved = Widget && Widget->RemoveInstalledMod();
				const bool bUnequipped = Widget && Widget->UnequipAdaptation();
				UProjectOrganoidBiologicalAdaptation_NeuralSlow* OtherAdapt =
					NewObject<UProjectOrganoidBiologicalAdaptation_NeuralSlow>(GetTransientPackage(), TEXT("Beat9OtherAdaptation"));
				Adapt->UnlockAdaptation(OtherAdapt);
				const bool bOtherEquipped = Widget && Widget->EquipUnlockedAdaptation(OtherAdapt);
				AssertTrue(Record, TEXT("negative.barrel_no_credit"), !bBarrelInstalled && Station->CampaignSuccessEventFireCount == 0, TEXT("no credit"), bBarrelInstalled ? TEXT("installed") : TEXT("rejected"), TEXT("barrel"));
				AssertTrue(Record, TEXT("negative.remove_no_credit"), Station->CampaignSuccessEventFireCount == 0, TEXT("0"), FString::FromInt(Station->CampaignSuccessEventFireCount), TEXT("remove"));
				AssertTrue(Record, TEXT("negative.unequip_no_credit"), bUnequipped && Station->CampaignSuccessEventFireCount == 0, TEXT("no credit"), FString::FromInt(Station->CampaignSuccessEventFireCount), TEXT("unequip"));
				AssertTrue(Record, TEXT("negative.other_adaptation_no_credit"), bOtherEquipped && Station->CampaignSuccessEventFireCount == 0 && !AdaptationPathEquals(Adapt->GetEquippedAdaptation(), AdaptationPath), TEXT("no credit"), AdaptationPathEquals(Adapt->GetEquippedAdaptation(), AdaptationPath) ? TEXT("neural") : TEXT("other"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("negative.barrel_unchanged"), Character->IsWeaponModUnlocked(Barrel) == bBarrelUnlockedBefore, TEXT("unchanged"), Character->IsWeaponModUnlocked(Barrel) ? TEXT("unlocked") : TEXT("locked"), TEXT("barrel"));
				(void)bRemoved;
				if (Widget)
				{
					Widget->UnequipAdaptation();
				}
				TArray<FSoftObjectPath> OnlyNeural;
				OnlyNeural.Add(FSoftObjectPath(NeuralSlow));
				Adapt->ApplyUnlockedAdaptations(OnlyNeural);
				Adapt->UnequipAdaptation();
				AssertTrue(Record, TEXT("negative.cleared_other"), Adapt->GetEquippedAdaptation() == nullptr && Adapt->GetUnlockedAdaptationPaths().Num() == 1 && Station->CampaignSuccessEventFireCount == 0, TEXT("only Neural Slow"), FString::FromInt(Adapt->GetUnlockedAdaptationPaths().Num()), TEXT("adaptation"));
				Proof = EProof::Equip;
				break;
			}
			case EProof::Equip:
			{
				UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget();
				if (!Widget)
				{
					FailAndStop(Owner, Record, TEXT("Station widget closed before Equip Neural Slow."));
					return;
				}
				const int32 SotBefore = Character->GetInventoryComponent()
					? Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT)
					: -1;
				const float PEBefore = Character->GetPEEnergy();
				const bool bFailed = Widget->EquipUnlockedAdaptation(nullptr);
				AssertTrue(Record, TEXT("equip.failed_attempt"), !bFailed && Station->CampaignSuccessEventFireCount == 0, TEXT("rejected"), bFailed ? TEXT("accepted") : TEXT("rejected"), TEXT("widget"));
				const bool bEquipped = Widget->EquipUnlockedAdaptation(NeuralSlow);
				UProjectOrganoidHUDWidget* HUD = ResolveHud(Character);
				AssertTrue(Record, TEXT("equip.action"), bEquipped, TEXT("true"), BoolText(bEquipped), TEXT("widget"));
				AssertTrue(Record, TEXT("equip.neural_slow"), AdaptationPathEquals(Adapt->GetEquippedAdaptation(), AdaptationPath), AdaptationPath, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
				AssertTrue(Record, TEXT("equip.event_once"), Station->CampaignSuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				AssertTrue(Record, TEXT("equip.notify_once"), Station->CampaignSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessNotificationCount), StationLabel);
				AssertTrue(Record, TEXT("equip.nathan_line"), HUD && HUD->GetLastResourceNotification().ToString().Contains(ExpectedLine), ExpectedLine, HUD ? HUD->GetLastResourceNotification().ToString() : TEXT("no hud"), TEXT("HUD"));
				AssertTrue(Record, TEXT("equip.objective_completed"), CountCompletedId(Objectives, FName(IntroObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(IntroObjectiveId))), IntroObjectiveId);
				AssertTrue(Record, TEXT("equip.mission_complete"), Objectives->IsMissionComplete(FName(IntroMissionId)), TEXT("true"), BoolText(Objectives->IsMissionComplete(FName(IntroMissionId))), IntroMissionId);
				AssertTrue(Record, TEXT("equip.sot_unchanged"), Character->GetInventoryComponent() && Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT) == SotBefore, FString::FromInt(SotBefore), Character->GetInventoryComponent() ? FString::FromInt(Character->GetInventoryComponent()->CountItemsOfType(EProjectOrganoidItemType::SOT)) : TEXT("none"), TEXT("SOT"));
				AssertTrue(Record, TEXT("equip.pe_unchanged"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
				AssertTrue(Record, TEXT("equip.only_neural_slow"), Adapt->GetUnlockedAdaptationPaths().Num() == 1 && Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("1"), FString::FromInt(Adapt->GetUnlockedAdaptationPaths().Num()), TEXT("adaptation"));
				Proof = EProof::Replay;
				break;
			}
			case EProof::Replay:
			{
				UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget();
				if (Widget)
				{
					Widget->EquipUnlockedAdaptation(NeuralSlow);
					Widget->CloseStationUI();
				}
				const bool bReopened = Station->Interact(Character);
				Widget = Station->GetActiveStationWidget();
				if (Widget)
				{
					Widget->UnequipAdaptation();
					Widget->EquipUnlockedAdaptation(NeuralSlow);
					Widget->CloseStationUI();
				}
				AssertTrue(Record, TEXT("replay.reopen"), bReopened, TEXT("open"), BoolText(bReopened), StationLabel);
				AssertTrue(Record, TEXT("replay.event_still_one"), Station->CampaignSuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				AssertTrue(Record, TEXT("replay.notify_still_one"), Station->CampaignSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessNotificationCount), StationLabel);
				AssertTrue(Record, TEXT("replay.still_equipped"), AdaptationPathEquals(Adapt->GetEquippedAdaptation(), AdaptationPath), AdaptationPath, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
				Proof = EProof::SaveLoad;
				break;
			}
			case EProof::SaveLoad:
			{
				Saves->DeleteSave(SaveSlot);
				const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
				Adapt->UnequipAdaptation();
				const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				AssertTrue(Record, TEXT("saveload.saved"), bSaved && bLoaded, TEXT("true"), BoolText(bSaved && bLoaded), TEXT("save"));
				AssertTrue(Record, TEXT("saveload.unlocked"), Adapt->IsAdaptationUnlocked(NeuralSlow), TEXT("unlocked"), Adapt->IsAdaptationUnlocked(NeuralSlow) ? TEXT("unlocked") : TEXT("locked"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("saveload.equipped"), AdaptationPathEquals(Adapt->GetEquippedAdaptation(), AdaptationPath), AdaptationPath, Adapt->GetEquippedAdaptationPath().ToString(), TEXT("adaptation"));
				AssertTrue(Record, TEXT("saveload.mission_complete"), CountCompletedId(Objectives, FName(IntroObjectiveId)) == 1, TEXT("1"), FString::FromInt(CountCompletedId(Objectives, FName(IntroObjectiveId))), IntroObjectiveId);
				AssertTrue(Record, TEXT("saveload.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
				AssertTrue(Record, TEXT("saveload.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
				const bool bAgain = Station->Interact(Character);
				if (UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget())
				{
					Widget->EquipUnlockedAdaptation(NeuralSlow);
					Widget->CloseStationUI();
				}
				AssertTrue(Record, TEXT("saveload.no_replay"), bAgain && Station->CampaignSuccessEventFireCount == 1 && Station->CampaignSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				Saves->DeleteSave(SaveSlot);

				UProjectOrganoidObjectiveDataAsset* IntroDA = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, IntroMissionSoftPath);
				if (!IntroDA || !Objectives->LoadMission(IntroDA, false))
				{
					FailAndStop(Owner, Record, TEXT("Failed to reload the intro mission for the already-equipped migration."));
					return;
				}
				Station->CampaignSuccessEventFireCount = 0;
				Station->CampaignSuccessNotificationCount = 0;
				Adapt->UnlockAdaptation(NeuralSlow);
				Adapt->EquipAdaptation(NeuralSlow);
				AssertTrue(Record, TEXT("migration.objective_active_before_visit"), CountActiveId(Objectives, FName(IntroObjectiveId)) == 1 && Station->CampaignSuccessEventFireCount == 0, TEXT("active, no event"), FString::FromInt(Station->CampaignSuccessEventFireCount), IntroObjectiveId);
				OrganoidPlaytestActions::TeleportNear(Character, AwayFromStation, 0.0f, AwayFromStation.Z);
				Saves->DeleteSave(SaveSlot);
				const bool bAwaySaved = Saves->SavePlayerProgress(Character, SaveSlot);
				const bool bAwayLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				AssertTrue(Record, TEXT("migration.load_away_does_not_complete"), bAwaySaved && bAwayLoaded && CountActiveId(Objectives, FName(IntroObjectiveId)) == 1 && CountCompletedId(Objectives, FName(IntroObjectiveId)) == 0 && Station->CampaignSuccessEventFireCount == 0, TEXT("still active"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
				Proof = EProof::MigrationAway;
				break;
			}
			case EProof::MigrationAway:
			{
				StandAtStation(Character, Station);
				const bool bVisited = Station->Interact(Character);
				if (UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget())
				{
					Widget->CloseStationUI();
				}
				AssertTrue(Record, TEXT("migration.visit_reconciles_once"), bVisited && Station->CampaignSuccessEventFireCount == 1 && Station->CampaignSuccessNotificationCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				AssertTrue(Record, TEXT("migration.mission_complete"), Objectives->IsMissionComplete(FName(IntroMissionId)), TEXT("true"), BoolText(Objectives->IsMissionComplete(FName(IntroMissionId))), IntroMissionId);
				const bool bAgain = Station->Interact(Character);
				if (UProjectOrganoidResearchStationWidget* Widget = Station->GetActiveStationWidget())
				{
					Widget->EquipUnlockedAdaptation(NeuralSlow);
					Widget->CloseStationUI();
				}
				AssertTrue(Record, TEXT("migration.no_second_credit"), bAgain && Station->CampaignSuccessEventFireCount == 1, TEXT("1"), FString::FromInt(Station->CampaignSuccessEventFireCount), StationLabel);
				Saves->DeleteSave(SaveSlot);
				Proof = EProof::MigrationVisit;
				break;
			}
			case EProof::MigrationVisit:
				Proof = EProof::SideEffects;
				break;
			case EProof::SideEffects:
			{
				AssertTrue(Record, TEXT("side.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));
				AssertTrue(Record, TEXT("side.cryo_blackout"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == EProjectOrganoidPowerState::Blackout, TEXT("Blackout"), PowerStateName(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));
				AssertTrue(Record, TEXT("side.hosts"), CountLabel(World, Host1Label) == Host1Before && CountLabel(World, Host2Label) == Host2Before && CountLabel(World, Host3Label) == Host3Before && CountLabel(World, ResearcherLabel) == ResearcherBefore, TEXT("unchanged"), TEXT("checked"), TEXT("Hosts"));
				AssertTrue(Record, TEXT("side.no_extra_station"), CountLabel(World, StationLabel) == 1 && !WrongStation.IsValid(), TEXT("1"), FString::FromInt(CountLabel(World, StationLabel)), StationLabel);
				if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
				{
					AssertTrue(Record, TEXT("side.view_target"), PC->GetViewTarget() == Character, TEXT("player"), PC->GetViewTarget() ? PC->GetViewTarget()->GetName() : TEXT("none"), TEXT("camera"));
				}
				Stage = EStage::EndPie;
				Owner.SetStage(TEXT("EndPie"));
				break;
			}
			}
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
			(void)Owner;
		}
	};

	struct FNeuroResearchStationIntroAutoRegister
	{
		FNeuroResearchStationIntroAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroResearchStationIntroFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroResearchStationIntroAutoRegister GRegisterNeuroResearchStationIntro;
}
