#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
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

namespace SyringeKitFunctional
{
	constexpr TCHAR TestId[] = TEXT("SyringeKit_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Syringe Kit Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR SyringeSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_SyringeKit.DA_Mission_SyringeKit");
	constexpr TCHAR ResearchSoftPath[] = TEXT("/Game/Data/Missions/DA_Mission_ResearchStation.DA_Mission_ResearchStation");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR SyringeMissionId[] = TEXT("Mission_SyringeKit");
	constexpr TCHAR SyringeObjectiveId[] = TEXT("Obj_RecoverSyringeKit");
	constexpr TCHAR SyringeEvent[] = TEXT("Event_SyringeKitRecovered");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Recover Syringe Kit");
	constexpr TCHAR ExpectedLine[] = TEXT("More syringes. Each one targets a different system. Movement, vision... Epitope was building a toolkit.");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidSyringeKitTest");
	const FVector StationLocation(800.f, -1600.f, -1100.f);

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	FString PowerText(EProjectOrganoidPowerState State)
	{
		switch (State)
		{
		case EProjectOrganoidPowerState::Online: return TEXT("Online");
		case EProjectOrganoidPowerState::Blackout: return TEXT("Blackout");
		case EProjectOrganoidPowerState::Emergency: return TEXT("Emergency");
		default: return TEXT("Other");
		}
	}

	class FSyringeKitFunctional : public IOrganoidPlaytestCase
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
			bProofDone = false;
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
			if (!Record)
			{
				return;
			}
			switch (Stage)
			{
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie: TickStartPie(Owner, *Record); break;
			case EStage::WaitReady: TickWaitReady(Owner, *Record, DeltaTime); break;
			case EStage::Proof: TickProof(Owner, *Record, DeltaTime); break;
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
					if (PackageIsDirty(NeuroPackage) || PackageIsDirty(MapPackage))
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty())
						{
							Record->FailureReason = TEXT("A package was dirty after the syringe-kit test.");
						}
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, EndPie, WaitStopped };

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
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

		UProjectOrganoidHUDWidget* FindHud(UWorld* World, AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
			UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
			return HUD ? HUD->GetBoundHUDWidget() : nullptr;
		}

		bool ObjectiveProgress(UProjectOrganoidObjectiveSubsystem* Objectives, int32& OutProgress, EProjectOrganoidObjectiveState& OutState) const
		{
			FProjectOrganoidObjective Objective;
			if (!Objectives || !Objectives->GetObjective(FName(SyringeObjectiveId), Objective))
			{
				return false;
			}
			OutProgress = Objective.CurrentProgress;
			OutState = Objective.State;
			return true;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Syringe = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, SyringeSoftPath);
			UProjectOrganoidObjectiveDataAsset* Research = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, ResearchSoftPath);
			UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt* Locomotor = LoadObject<UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt>(nullptr, UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::ContentPath());
			UProjectOrganoidBiologicalAdaptation_OpticalDisrupt* Optical = LoadObject<UProjectOrganoidBiologicalAdaptation_OpticalDisrupt>(nullptr, UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::ContentPath());
			const FProjectOrganoidMissionTaskDefinition* Task = Syringe && Syringe->Tasks.Num() == 1 ? &Syringe->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Syringe && Syringe->MissionId == FName(SyringeMissionId), SyringeMissionId, Syringe ? Syringe->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Syringe && Syringe->MissionTitle.ToString() == TEXT("Syringe Kit"), TEXT("Syringe Kit"), Syringe ? Syringe->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Syringe && Syringe->MissionDescription.ToString().Contains(TEXT("full Epitope syringe kit")), TEXT("full Epitope syringe kit"), Syringe ? Syringe->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_null"), Syringe && Syringe->NextMissionAsset.IsNull(), TEXT("null"), Syringe && Syringe->NextMissionAsset.IsNull() ? TEXT("null") : TEXT("set"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main && Task->Objective.ObjectiveId == FName(SyringeObjectiveId), SyringeObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.auto"), Task && Task->bAutoActivate && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, TEXT("auto"), Task ? TEXT("checked") : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.target_2"), Task && Task->Objective.TargetProgress == 2, TEXT("2"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.event_advance"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(SyringeEvent) && Task->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Advance && Task->EventTriggers[0].ProgressDelta == 1, SyringeEvent, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.research_next"), Research && Research->NextMissionAsset.ToSoftObjectPath().ToString() == SyringeSoftPath, SyringeSoftPath, Research ? Research->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.locomotor"), Locomotor && Locomotor->AdaptationId == TEXT("Adaptation_LocomotorDisrupt") && FMath::IsNearlyEqual(Locomotor->LocomotorSpeedMultiplier, 0.5f) && FMath::IsNearlyEqual(Locomotor->DurationSeconds, 5.f) && FMath::IsNearlyEqual(Locomotor->PECost, 20.f), TEXT("Adaptation_LocomotorDisrupt"), Locomotor ? Locomotor->AdaptationId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.optical"), Optical && Optical->AdaptationId == TEXT("Adaptation_OpticalDisrupt") && FMath::IsNearlyEqual(Optical->DurationSeconds, 5.f) && FMath::IsNearlyEqual(Optical->PECost, 20.f) && FMath::IsNearlyEqual(Optical->MaxTargetRange, 800.f), TEXT("Adaptation_OpticalDisrupt"), Optical ? Optical->AdaptationId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.distinct"), Locomotor && Optical && Locomotor->GetClass() != Optical->GetClass(), TEXT("distinct"), TEXT("classes"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			TArray<AActor*> Found = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, StationLabel) : TArray<AActor*>();
			AProjectOrganoidResearchStation* Station = Found.Num() == 1 ? Cast<AProjectOrganoidResearchStation>(Found[0]) : nullptr;
			AssertTrue(Record, TEXT("station.label"), Found.Num() == 1 && Station, TEXT("1"), FString::FromInt(Found.Num()), StationLabel);
			AssertTrue(Record, TEXT("station.location"), Station && Station->GetActorLocation().Equals(StationLocation, 1.f), TEXT("800,-1600,-1100"), Station ? Station->GetActorLocation().ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.contracts"), Station && Station->CampaignRequiredActiveObjectiveId == TEXT("Obj_EquipNeuralSlow") && Station->RespecRequiredActiveObjectiveId == TEXT("Obj_UseResearchStation") && Station->SyringeRequiredActiveObjectiveId == FName(SyringeObjectiveId), TEXT("3"), Station ? Station->SyringeRequiredActiveObjectiveId.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.prompt_property"), Station && Station->SyringePrompt.ToString() == ExpectedPrompt && Station->InteractionPrompt.ToString() == TEXT("Use Research Station"), ExpectedPrompt, Station ? Station->SyringePrompt.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.line"), Station && Station->SyringeNotificationText.ToString() == ExpectedLine && Station->SyringeNotificationSpeaker.ToString() == TEXT("Nathan") && FMath::IsNearlyEqual(Station->SyringeNotificationDurationSeconds, 7.f), ExpectedLine, Station ? Station->SyringeNotificationText.ToString() : TEXT("missing"), StationLabel);
			AssertTrue(Record, TEXT("station.event"), Station && Station->SyringeSuccessObjectiveEventId == FName(SyringeEvent) && Station->SyringeReplayGuardObjectiveId == FName(SyringeObjectiveId), SyringeEvent, Station ? Station->SyringeSuccessObjectiveEventId.ToString() : TEXT("missing"), StationLabel);
			if (bAnyAssertFailed || !Syringe || !Station || !Locomotor || !Optical)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Syringe kit contract missing.") : Record.FailureReason);
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
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Owner, Record, TEXT("PIE did not become ready with ResearchStation_NeuroGenetics."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			if (bProofDone)
			{
				return;
			}
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
			bProofDone = true;
			RunProof(Owner, Record, World, Character, Widget);
		}

		void RunProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			TArray<AActor*> Found = World ? OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel) : TArray<AActor*>();
			AProjectOrganoidResearchStation* Station = Found.Num() == 1 ? Cast<AProjectOrganoidResearchStation>(Found[0]) : nullptr;
			UProjectOrganoidBiologicalAdaptationData* Locomotor = UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve();
			UProjectOrganoidBiologicalAdaptationData* Optical = UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve();
			if (!Objectives || !Saves || !Power || !Station || !Adapt || !Locomotor || !Optical)
			{
				FailAndStop(Owner, Record, TEXT("Syringe kit actors or subsystems missing."));
				return;
			}

			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
			AssertTrue(Record, TEXT("power.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));

			UProjectOrganoidObjectiveDataAsset* Syringe = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, SyringeSoftPath);
			const bool bLoaded = Syringe && Objectives->LoadMission(Syringe, false);
			int32 Progress = -1;
			EProjectOrganoidObjectiveState State = EProjectOrganoidObjectiveState::Inactive;
			ObjectiveProgress(Objectives, Progress, State);
			AssertTrue(Record, TEXT("mission.active"), bLoaded && Objectives->GetActiveMissionId() == FName(SyringeMissionId), SyringeMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("objective.active"), State == EProjectOrganoidObjectiveState::Active && Progress == 0, TEXT("0"), FString::FromInt(Progress), SyringeObjectiveId);
			AssertTrue(Record, TEXT("objective.target"), Syringe && Syringe->Tasks.Num() == 1 && Syringe->Tasks[0].Objective.TargetProgress == 2, TEXT("2"), TEXT("asset"), SyringeObjectiveId);
			AssertTrue(Record, TEXT("prompt.recover"), Station->GetInteractionPrompt().ToString() == ExpectedPrompt, ExpectedPrompt, Station->GetInteractionPrompt().ToString(), StationLabel);
			AssertTrue(Record, TEXT("adapt.locked"), !Adapt->IsAdaptationUnlocked(Locomotor) && !Adapt->IsAdaptationUnlocked(Optical), TEXT("locked"), TEXT("unlocked"), TEXT("adapt"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bFirst = Station->Interact(Character);
			Station->CloseResearchStationUI();
			ObjectiveProgress(Objectives, Progress, State);
			const FString Line = Widget->GetLastResourceNotification().ToString();
			const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
			AssertTrue(Record, TEXT("first.interact"), bFirst, TEXT("true"), bFirst ? TEXT("true") : TEXT("false"), StationLabel);
			AssertTrue(Record, TEXT("first.progress"), Progress == 1 && State == EProjectOrganoidObjectiveState::Active, TEXT("1"), FString::FromInt(Progress), SyringeObjectiveId);
			AssertTrue(Record, TEXT("first.locomotor"), Adapt->IsAdaptationUnlocked(Locomotor) && !Adapt->IsAdaptationUnlocked(Optical), TEXT("locomotor"), Optical && Adapt->IsAdaptationUnlocked(Optical) ? TEXT("both") : TEXT("locomotor"), TEXT("adapt"));
			AssertTrue(Record, TEXT("first.fire"), Station->SyringeEventFireCount == 1 && Station->SyringeNotificationCount == 1, TEXT("1"), FString::FromInt(Station->SyringeEventFireCount), StationLabel);
			AssertTrue(Record, TEXT("first.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
			AssertTrue(Record, TEXT("first.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
			AssertTrue(Record, TEXT("first.unmoved"), Station->GetActorLocation().Equals(StationLocation, 5.f), TEXT("800,-1600,-1100"), Station->GetActorLocation().ToString(), StationLabel);
			AssertTrue(Record, TEXT("first.intro_kept"), Station->CampaignRequiredActiveObjectiveId == TEXT("Obj_EquipNeuralSlow") && Station->RespecRequiredActiveObjectiveId == TEXT("Obj_UseResearchStation"), TEXT("kept"), Station->RespecRequiredActiveObjectiveId.ToString(), StationLabel);
			AssertTrue(Record, TEXT("first.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("Power"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			const bool bSecond = Station->Interact(Character);
			Station->CloseResearchStationUI();
			ObjectiveProgress(Objectives, Progress, State);
			AssertTrue(Record, TEXT("second.interact"), bSecond, TEXT("true"), bSecond ? TEXT("true") : TEXT("false"), StationLabel);
			AssertTrue(Record, TEXT("second.progress"), Progress == 2 && State == EProjectOrganoidObjectiveState::Completed, TEXT("2"), FString::FromInt(Progress), SyringeObjectiveId);
			AssertTrue(Record, TEXT("second.both"), Adapt->IsAdaptationUnlocked(Locomotor) && Adapt->IsAdaptationUnlocked(Optical), TEXT("both"), TEXT("missing"), TEXT("adapt"));
			AssertTrue(Record, TEXT("second.not_equipped"), Adapt->GetEquippedAdaptation() != Locomotor && Adapt->GetEquippedAdaptation() != Optical, TEXT("unequipped"), Adapt->GetEquippedAdaptation() ? Adapt->GetEquippedAdaptation()->GetName() : TEXT("none"), TEXT("adapt"));
			AssertTrue(Record, TEXT("second.fire"), Station->SyringeEventFireCount == 2 && Station->SyringeNotificationCount == 1, TEXT("2"), FString::Printf(TEXT("fire=%d notes=%d"), Station->SyringeEventFireCount, Station->SyringeNotificationCount), StationLabel);
			AssertTrue(Record, TEXT("second.line_once"), !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("once"), Widget->GetLastResourceNotification().ToString(), TEXT("HUD"));
			AssertTrue(Record, TEXT("second.mission"), Objectives->GetActiveMissionId() == FName(SyringeMissionId) && Objectives->IsMissionComplete(FName(SyringeMissionId)), SyringeMissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			Station->Interact(Character);
			Station->CloseResearchStationUI();
			AssertTrue(Record, TEXT("replay.guard"), Station->SyringeEventFireCount == 2 && Station->SyringeNotificationCount == 1, TEXT("2"), FString::FromInt(Station->SyringeEventFireCount), StationLabel);

			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			Power->SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Blackout);
			const bool bRestored = Saves->LoadPlayerProgress(Character, SaveSlot);
			ObjectiveProgress(Objectives, Progress, State);
			AssertTrue(Record, TEXT("save.wrote"), bSaved && bRestored, TEXT("true"), bSaved && bRestored ? TEXT("true") : TEXT("false"), TEXT("save"));
			AssertTrue(Record, TEXT("save.mission"), Objectives->GetActiveMissionId() == FName(SyringeMissionId) && Progress == 2 && State == EProjectOrganoidObjectiveState::Completed, TEXT("2"), FString::FromInt(Progress), TEXT("save"));
			AssertTrue(Record, TEXT("save.adaptations"), Adapt->IsAdaptationUnlocked(Locomotor) && Adapt->IsAdaptationUnlocked(Optical), TEXT("both"), TEXT("missing"), TEXT("save"));
			AssertTrue(Record, TEXT("save.neuro_online"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics) == EProjectOrganoidPowerState::Online, TEXT("Online"), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics)), TEXT("save"));
			AssertTrue(Record, TEXT("dirty.neuro"), !PackageIsDirty(NeuroPackage), TEXT("clean"), PackageIsDirty(NeuroPackage) ? TEXT("dirty") : TEXT("clean"), NeuroPackage);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bProofDone = false;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FSyringeKitFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterSyringeKitFunctional;
}
