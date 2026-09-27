#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
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
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponData.h"

namespace WeaponRosterFunctional
{
	constexpr TCHAR TestId[] = TEXT("WeaponRoster_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Weapon Roster Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR CryoPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo");
	constexpr TCHAR ComputePackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Compute");
	constexpr TCHAR MissionPath[] = TEXT("/Game/Data/Missions/DA_Mission_WeaponRoster.DA_Mission_WeaponRoster");
	constexpr TCHAR MissionId[] = TEXT("Mission_WeaponRoster");
	constexpr TCHAR ObjectiveId[] = TEXT("Obj_RecoverWeaponRoster");
	constexpr TCHAR EventId[] = TEXT("Event_WeaponRosterRecovered");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Recover Weapon Roster");
	constexpr TCHAR ExpectedLine[] = TEXT("Five more. Each with a different job. Lytic was the emergency — these are the toolkit. No unlimited ammo.");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidWeaponRosterTest");

	const TCHAR* PadLabels[] = {
		TEXT("DataPad_SpecimenManifest"),
		TEXT("DataPad_ConsentForms"),
		TEXT("DataPad_SterlingCryoNote"),
		TEXT("DataPad_AutonomousDecisionLog"),
		TEXT("DataPad_SterlingConfession")
	};
	const TCHAR* WeaponIds[] = {
		TEXT("Weapon_BioStabilizerPistol"),
		TEXT("Weapon_PulseCarbine"),
		TEXT("Weapon_CryoInjector"),
		TEXT("Weapon_DenaturingShotgun"),
		TEXT("Weapon_IncineratorLance")
	};
	const FVector PadLocations[] = {
		FVector(-2330.f, -2150.f, -2310.f),
		FVector(-400.f, -1150.f, -2310.f),
		FVector(-2425.f, 1025.f, -2310.f),
		FVector(0.f, -1650.f, -3510.f),
		FVector(-2425.f, -2275.f, -3510.f)
	};

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

	class FWeaponRosterFunctional : public IOrganoidPlaytestCase
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
			bRequestedStreams = false;
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
					if (PackageIsDirty(CryoPackage) || PackageIsDirty(ComputePackage) || PackageIsDirty(MapPackage))
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty())
						{
							Record->FailureReason = TEXT("A package was dirty after the weapon-roster test.");
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

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			const FProjectOrganoidMissionTaskDefinition* Task = Mission && Mission->Tasks.Num() == 1 ? &Mission->Tasks[0] : nullptr;
			AssertTrue(Record, TEXT("asset.id"), Mission && Mission->MissionId == FName(MissionId), MissionId, Mission ? Mission->MissionId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Mission && Mission->MissionTitle.ToString() == TEXT("Weapon Roster"), TEXT("Weapon Roster"), Mission ? Mission->MissionTitle.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.description"), Mission && Mission->MissionDescription.ToString().Contains(TEXT("armory reveals the remaining weapons")), TEXT("armory"), Mission ? Mission->MissionDescription.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.next_pursuer_intro"), Mission && Mission->NextMissionAsset.ToSoftObjectPath().ToString() == TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro.DA_Mission_PursuerIntro"), TEXT("/Game/Data/Missions/DA_Mission_PursuerIntro.DA_Mission_PursuerIntro"), Mission ? Mission->NextMissionAsset.ToSoftObjectPath().ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.one_main"), Task && Task->Objective.Type == EProjectOrganoidObjectiveType::Main && Task->Objective.ObjectiveId == FName(ObjectiveId) && Task->Objective.PrerequisiteObjectiveIds.Num() == 0, ObjectiveId, Task ? Task->Objective.ObjectiveId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.auto_target"), Task && Task->bAutoActivate && Task->Objective.TargetProgress == 5, TEXT("5"), Task ? FString::FromInt(Task->Objective.TargetProgress) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.event"), Task && Task->EventTriggers.Num() == 1 && Task->EventTriggers[0].EventId == FName(EventId) && Task->EventTriggers[0].Action == EProjectOrganoidObjectiveEventAction::Advance && Task->EventTriggers[0].ProgressDelta == 1, EventId, Task && Task->EventTriggers.Num() == 1 ? Task->EventTriggers[0].EventId.ToString() : TEXT("missing"), TEXT("DA"));

			UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
			bool bPadsOk = EditorWorld != nullptr;
			for (int32 Index = 0; Index < 5; ++Index)
			{
				TArray<AActor*> Found = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, PadLabels[Index]) : TArray<AActor*>();
				AProjectOrganoidDataPad* Pad = Found.Num() == 1 ? Cast<AProjectOrganoidDataPad>(Found[0]) : nullptr;
				const bool bThis = Pad && Pad->GetActorLocation().Equals(PadLocations[Index], 1.f) && Pad->WeaponRosterWeaponId == FName(WeaponIds[Index]);
				AssertTrue(Record, FString::Printf(TEXT("pad.%d"), Index), bThis, WeaponIds[Index], Pad ? Pad->WeaponRosterWeaponId.ToString() : TEXT("missing"), PadLabels[Index]);
				bPadsOk = bPadsOk && bThis;
			}
			TArray<AActor*> CryoFound = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, PadLabels[0]) : TArray<AActor*>();
			AProjectOrganoidDataPad* CryoPad = CryoFound.Num() == 1 ? Cast<AProjectOrganoidDataPad>(CryoFound[0]) : nullptr;
			TArray<AActor*> ConfessionFound = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, PadLabels[4]) : TArray<AActor*>();
			AProjectOrganoidDataPad* Confession = ConfessionFound.Num() == 1 ? Cast<AProjectOrganoidDataPad>(ConfessionFound[0]) : nullptr;
			AssertTrue(Record, TEXT("cryo.kept"), CryoPad && CryoPad->InteractionPrompt.ToString() == TEXT("Recover Cryo Evidence") && CryoPad->ObjectiveEventId == TEXT("Event_CryoEvidenceRecovered"), TEXT("kept"), CryoPad ? CryoPad->InteractionPrompt.ToString() : TEXT("missing"), PadLabels[0]);
			AssertTrue(Record, TEXT("confession.kept"), Confession && Confession->ObjectiveEventId == TEXT("Event_SterlingConfessionRead") && Confession->InteractionPrompt.ToString() == TEXT("Read Sterling's Confession"), TEXT("kept"), Confession ? Confession->ObjectiveEventId.ToString() : TEXT("missing"), PadLabels[4]);

			bool bWeaponsDistinct = true;
			FString SeenEffects;
			for (const TCHAR* Id : WeaponIds)
			{
				UProjectOrganoidWeaponData* Data = UProjectOrganoidWeaponData::ResolveById(FName(Id));
				const bool bThis = Data && Data->WeaponId == FName(Id) && Data->MagazineCapacity > 0 && Data->MagazineCapacity < 12 && Data->Effect != EProjectOrganoidWeaponRosterEffect::None;
				AssertTrue(Record, FString::Printf(TEXT("weapon.%s"), Id), bThis, Id, Data ? Data->WeaponId.ToString() : TEXT("missing"), TEXT("DA"));
				const FString EffectName = Data ? UEnum::GetValueAsString(Data->Effect) : TEXT("missing");
				bWeaponsDistinct = bWeaponsDistinct && bThis && !SeenEffects.Contains(EffectName);
				SeenEffects += EffectName + TEXT(";");
			}
			AssertTrue(Record, TEXT("weapons.distinct"), bWeaponsDistinct && !SeenEffects.Contains(TEXT("Lytic")) && !SeenEffects.Contains(TEXT("Arc")), TEXT("five jobs"), SeenEffects, TEXT("DA"));
			if (bAnyAssertFailed || !Mission || !bPadsOk)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Weapon roster contract missing.") : Record.FailureReason);
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
			bRequestedStreams = false;
			Stage = EStage::WaitReady;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
			if (Levels && Character && !bRequestedStreams)
			{
				Levels->AddStreamRequest(Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel3_Cryo), Character);
				Levels->AddStreamRequest(Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel4_Compute), Character);
				Levels->ReconcileStreamingNow();
				bRequestedStreams = true;
			}
			bool bPads = Character && bRequestedStreams && World;
			for (const TCHAR* Label : PadLabels)
			{
				bPads = bPads && OrganoidPlaytestActions::FindActorsByLabel(World, Label).Num() == 1;
			}
			if (bPads)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Owner, Record, TEXT("PIE did not become ready with the five roster datapads."));
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

		void OpenGate(UProjectOrganoidObjectiveSubsystem* Objectives, AProjectOrganoidDataPad* Pad)
		{
			if (!Objectives || !Pad || Pad->RequiredObjectiveIdForInteraction.IsNone())
			{
				return;
			}
			FProjectOrganoidObjective Existing;
			if (!Objectives->GetObjective(Pad->RequiredObjectiveIdForInteraction, Existing))
			{
				FProjectOrganoidObjective Gate;
				Gate.ObjectiveId = Pad->RequiredObjectiveIdForInteraction;
				Gate.State = EProjectOrganoidObjectiveState::Active;
				Gate.TargetProgress = 1;
				Objectives->RegisterObjective(Gate);
			}
			Objectives->CompleteObjective(Pad->RequiredObjectiveIdForInteraction);
		}

		void RunProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, UWorld* World, AProjectOrganoidCharacter* Character, UProjectOrganoidHUDWidget* Widget)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			UProjectOrganoidSaveSubsystem* Saves = World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			UProjectOrganoidWeaponComponent* Weapons = Character ? Character->GetWeaponComponent() : nullptr;
			if (!Objectives || !Saves || !Power || !Weapons)
			{
				FailAndStop(Owner, Record, TEXT("Weapon roster subsystems missing."));
				return;
			}

			const EProjectOrganoidPowerState CryoBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo);
			const EProjectOrganoidPowerState ComputeBefore = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute);
			AssertTrue(Record, TEXT("power.before"), CryoBefore == Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) && ComputeBefore == Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute), PowerText(CryoBefore), PowerText(ComputeBefore), TEXT("Power"));

			AProjectOrganoidDataPad* Pads[5] = {};
			for (int32 Index = 0; Index < 5; ++Index)
			{
				TArray<AActor*> Found = OrganoidPlaytestActions::FindActorsByLabel(World, PadLabels[Index]);
				Pads[Index] = Found.Num() == 1 ? Cast<AProjectOrganoidDataPad>(Found[0]) : nullptr;
			}
			UProjectOrganoidObjectiveDataAsset* Mission = LoadObject<UProjectOrganoidObjectiveDataAsset>(nullptr, MissionPath);
			const bool bLoaded = Mission && Objectives->LoadMission(Mission, false);
			FProjectOrganoidObjective Objective;
			const bool bObjective = Objectives->GetObjective(FName(ObjectiveId), Objective);
			AssertTrue(Record, TEXT("mission.active"), bLoaded && Objectives->GetActiveMissionId() == FName(MissionId), MissionId, Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			AssertTrue(Record, TEXT("objective.active"), bObjective && Objective.State == EProjectOrganoidObjectiveState::Active && Objective.CurrentProgress == 0 && Objective.TargetProgress == 5, TEXT("0/5"), bObjective ? FString::FromInt(Objective.CurrentProgress) : TEXT("missing"), ObjectiveId);
			AssertTrue(Record, TEXT("prompt.recover"), Pads[0] && Pads[0]->GetInteractionPrompt().ToString() == ExpectedPrompt, ExpectedPrompt, Pads[0] ? Pads[0]->GetInteractionPrompt().ToString() : TEXT("missing"), PadLabels[0]);

			Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			for (int32 Index = 0; Index < 5; ++Index)
			{
				OpenGate(Objectives, Pads[Index]);
				const FVector Before = Pads[Index] ? Pads[Index]->GetActorLocation() : FVector::ZeroVector;
				const bool bInteract = Pads[Index] && Pads[Index]->Interact(Character);
				Objectives->GetObjective(FName(ObjectiveId), Objective);
				AssertTrue(Record, FString::Printf(TEXT("read.%d"), Index), bInteract && Objective.CurrentProgress == Index + 1, FString::FromInt(Index + 1), FString::FromInt(Objective.CurrentProgress), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("unlock.%d"), Index), Weapons->IsWeaponRosterUnlocked(FName(WeaponIds[Index])), WeaponIds[Index], TEXT("locked"), PadLabels[Index]);
				AssertTrue(Record, FString::Printf(TEXT("unmoved.%d"), Index), Pads[Index] && Pads[Index]->GetActorLocation().Equals(Before, 1.f), TEXT("unmoved"), Pads[Index] ? Pads[Index]->GetActorLocation().ToString() : TEXT("missing"), PadLabels[Index]);
				if (Index == 0)
				{
					const FString Line = Widget->GetLastResourceNotification().ToString();
					const float Remaining = Widget->GetTransientNotificationSecondsRemaining();
					AssertTrue(Record, TEXT("nathan.line"), Line.Contains(ExpectedLine) && Line.StartsWith(TEXT("Nathan:")), ExpectedLine, Line, TEXT("HUD"));
					AssertTrue(Record, TEXT("nathan.duration"), Remaining > 6.0f && Remaining <= 7.0f, TEXT("7"), FString::SanitizeFloat(Remaining), TEXT("HUD"));
					AssertTrue(Record, TEXT("nathan.once_pad"), Pads[0]->WeaponRosterNotificationCount == 1 && Pads[0]->WeaponRosterCreditCount == 1, TEXT("1"), FString::FromInt(Pads[0]->WeaponRosterNotificationCount), PadLabels[0]);
				}
				Widget->ShowTransientNotification(FText::GetEmpty(), FText::FromString(TEXT("clear")), 0.0f);
			}

			Objectives->GetObjective(FName(ObjectiveId), Objective);
			AssertTrue(Record, TEXT("objective.complete"), Objective.State == EProjectOrganoidObjectiveState::Completed && Objective.CurrentProgress == 5, TEXT("5"), FString::FromInt(Objective.CurrentProgress), ObjectiveId);
			AssertTrue(Record, TEXT("mission.advances"), Objectives->GetActiveMissionId() == FName(TEXT("Mission_PursuerIntro")), TEXT("Mission_PursuerIntro"), Objectives->GetActiveMissionId().ToString(), TEXT("mission"));
			int32 Notes = 0;
			for (AProjectOrganoidDataPad* Pad : Pads)
			{
				Notes += Pad ? Pad->WeaponRosterNotificationCount : 0;
			}
			AssertTrue(Record, TEXT("nathan.global_once"), Notes == 1, TEXT("1"), FString::FromInt(Notes), TEXT("HUD"));
			Pads[0]->Interact(Character);
			AssertTrue(Record, TEXT("replay.guard"), Pads[0]->WeaponRosterCreditCount == 1 && !Widget->GetLastResourceNotification().ToString().Contains(ExpectedLine), TEXT("once"), FString::FromInt(Pads[0]->WeaponRosterCreditCount), PadLabels[0]);
			AssertTrue(Record, TEXT("power.after"), Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo) == CryoBefore && Power->GetSectorPowerState(EProjectOrganoidPowerSector::Compute) == ComputeBefore, PowerText(CryoBefore), PowerText(Power->GetSectorPowerState(EProjectOrganoidPowerSector::Cryo)), TEXT("Power"));

			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			Weapons->ApplySavedWeaponRoster(TArray<FSoftObjectPath>(), FSoftObjectPath());
			const bool bRestored = Saves->LoadPlayerProgress(Character, SaveSlot);
			bool bAll = bSaved && bRestored;
			for (const TCHAR* Id : WeaponIds)
			{
				bAll = bAll && Character->GetWeaponComponent() && Character->GetWeaponComponent()->IsWeaponRosterUnlocked(FName(Id));
			}
			AssertTrue(Record, TEXT("save.roster"), bAll, TEXT("five"), bAll ? TEXT("five") : TEXT("missing"), TEXT("save"));
			AssertTrue(Record, TEXT("dirty.maps"), !PackageIsDirty(CryoPackage) && !PackageIsDirty(ComputePackage), TEXT("clean"), TEXT("checked"), TEXT("maps"));
			Stage = EStage::EndPie;
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedStreams = false;
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
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FWeaponRosterFunctional>(); };
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};
	static FRegister RegisterWeaponRosterFunctional;
}
