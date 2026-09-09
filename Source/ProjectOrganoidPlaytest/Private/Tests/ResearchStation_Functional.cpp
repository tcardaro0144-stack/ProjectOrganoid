#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerController.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidInteractionComponent.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("ResearchStation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Research Station Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidResearchStationTest");
	constexpr TCHAR SotPath[] = TEXT("/Game/Data/Items/DA_Item_SOT.DA_Item_SOT");
	constexpr TCHAR PistolAmmoPath[] = TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo");

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

	class FResearchStationFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Content;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroyStation();
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
				DestroyStation();
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
			Content,
			SpawnStation,
			OpenClose,
			HostStates,
			AudioLinger,
			NamedSource,
			UnlockInstallRemove,
			SaveLoad,
			CheckpointSetup,
			CheckpointChange,
			PlayerDeath,
			WaitRestart,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Content;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		int32 SotAtUnlock = 0;
		int32 CheckpointSot = 0;
		bool bCheckpointHadBarrel = false;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> ActivatedCheckpoint;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host;
		TWeakObjectPtr<AProjectOrganoidResearchStation> Station;

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			DestroyStation();
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

		void DestroyStation()
		{
			if (AProjectOrganoidResearchStation* Alive = Station.Get())
			{
				Alive->CloseResearchStationUI();
				Alive->Destroy();
			}
			Station.Reset();
		}

		AProjectOrganoidWeapon* GetWeapon(AProjectOrganoidCharacter* Character) const
		{
			return Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
		}

		UProjectOrganoidWeaponModComponent* GetModComp(AProjectOrganoidCharacter* Character) const
		{
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			return Weapon ? Weapon->GetWeaponModComponent() : nullptr;
		}

		UProjectOrganoidWeaponModData* ResolveBarrel() const
		{
			return UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve();
		}

		int32 ReserveOf(AProjectOrganoidCharacter* Character) const
		{
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			if (!Weapon || !Inventory)
			{
				return 0;
			}
			return Inventory->CountAmmoOfType(Weapon->AmmoType);
		}

		int32 CountSot(AProjectOrganoidCharacter* Character) const
		{
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			return Inventory ? Inventory->CountItemsOfType(EProjectOrganoidItemType::SOT) : 0;
		}

		bool GrantSot(AProjectOrganoidCharacter* Character, int32 Quantity)
		{
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			UProjectOrganoidItemData* Sot = LoadObject<UProjectOrganoidItemData>(nullptr, SotPath);
			if (!Inventory || !Sot || Quantity <= 0)
			{
				return false;
			}
			FGuid Id;
			return Inventory->TryAddItem(Sot, Id, Quantity);
		}

		bool SetReserveExact(AProjectOrganoidCharacter* Character, int32 Quantity)
		{
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			if (!Inventory || !Weapon)
			{
				return false;
			}
			const int32 Current = Inventory->CountAmmoOfType(Weapon->AmmoType);
			if (Current > Quantity)
			{
				return Inventory->ConsumeAmmoOfType(Weapon->AmmoType, Current - Quantity);
			}
			if (Current < Quantity)
			{
				UProjectOrganoidItemData* Ammo = LoadObject<UProjectOrganoidItemData>(nullptr, PistolAmmoPath);
				if (!Ammo)
				{
					Ammo = UProjectOrganoidItemData::CreateTransientPistolAmmo(Character);
				}
				if (!Ammo)
				{
					return false;
				}
				FGuid Id;
				return Inventory->TryAddItem(Ammo, Id, Quantity - Current);
			}
			return true;
		}

		AProjectOrganoidHostBase* FindHost(UWorld* World) const
		{
			return Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, Host1Label));
		}

		AProjectOrganoidHostAIController* GetHostAI(AProjectOrganoidHostBase* InHost) const
		{
			return InHost ? Cast<AProjectOrganoidHostAIController>(InHost->GetController()) : nullptr;
		}

		AProjectOrganoidCheckpoint* FindAnyCheckpoint(UWorld* World) const
		{
			if (!World)
			{
				return nullptr;
			}
			for (TActorIterator<AProjectOrganoidCheckpoint> It(World); It; ++It)
			{
				return *It;
			}
			return nullptr;
		}

		bool IsCursorGameplay(AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			return PC && !PC->bShowMouseCursor;
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope or Admin is dirty. Refusing to start."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
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
			UProjectOrganoidLevelManagerSubsystem* Levels = World
				? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>()
				: nullptr;
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

			AProjectOrganoidCheckpoint* Checkpoint = FindAnyCheckpoint(World);
			AProjectOrganoidHostBase* FoundHost = FindHost(World);
			if (World && Character && GetWeapon(Character) && Checkpoint && FoundHost && GetHostAI(FoundHost))
			{
				Player = Character;
				ActivatedCheckpoint = Checkpoint;
				Host = FoundHost;
				WaitSeconds = 0.0f;
				Proof = EProof::Content;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 90.0f)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE player, weapon, checkpoint, and Host_Neuro_1 (world=%s pawn=%s weapon=%s checkpoint=%s host=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						GetWeapon(Character) ? TEXT("yes") : TEXT("no"),
						Checkpoint ? TEXT("yes") : TEXT("no"),
						FoundHost ? TEXT("yes") : TEXT("no")));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			AProjectOrganoidHostBase* HostActor = Host.Get();
			AProjectOrganoidHostAIController* HostAI = GetHostAI(HostActor);
			if (!World || !Character || !Weapon || !HostActor || !HostAI)
			{
				FailAndStop(Owner, Record, TEXT("PIE player, weapon, or Host vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Content:
			{
				UProjectOrganoidWeaponModData* Barrel = ResolveBarrel();
				AssertTrue(Record, TEXT("stabilized_barrel_exists"), Barrel != nullptr, TEXT("present"), Barrel ? Barrel->ModId.ToString() : TEXT("none"), TEXT("mod"));
				AssertTrue(Record, TEXT("stabilized_barrel_slot"), Barrel && Barrel->Slot == EProjectOrganoidWeaponModSlot::Barrel, TEXT("Barrel"), Barrel ? TEXT("ok") : TEXT("none"), TEXT("mod"));
				AssertTrue(Record, TEXT("stabilized_barrel_no_mag_change"), Barrel && FMath::IsNearlyEqual(Barrel->FireRateMultiplier, 1.0f) && FMath::IsNearlyEqual(Barrel->PenetrationMultiplier, 1.0f), TEXT("ammo-neutral"), TEXT("ok"), TEXT("mod"));
				AssertTrue(Record, TEXT("provisional_damage_1_05"), Barrel && FMath::IsNearlyEqual(Barrel->DamageMultiplier, 1.05f), TEXT("1.05"), Barrel ? FString::SanitizeFloat(Barrel->DamageMultiplier) : TEXT("none"), TEXT("PROVISIONAL DESIGN TUNING"));
				Proof = EProof::SpawnStation;
				break;
			}
			case EProof::SpawnStation:
			{
				const FVector SpawnLoc = Character->GetActorLocation() + Character->GetActorForwardVector() * 120.0f;
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				AProjectOrganoidResearchStation* Spawned = World->SpawnActor<AProjectOrganoidResearchStation>(
					AProjectOrganoidResearchStation::StaticClass(),
					SpawnLoc,
					(-Character->GetActorForwardVector()).Rotation(),
					Params);
				Station = Spawned;
				AssertTrue(Record, TEXT("station_spawned_transient"), Spawned != nullptr, TEXT("spawned"), Spawned ? Spawned->GetName() : TEXT("none"), TEXT("station"));
				if (!Spawned)
				{
					FailAndStop(Owner, Record, TEXT("Failed to spawn transient Research Station."));
					return;
				}
				OrganoidPlaytestActions::FaceActor(Character, Spawned);
				Proof = EProof::OpenClose;
				WaitSeconds = 0.0f;
				break;
			}
			case EProof::OpenClose:
			{
				WaitSeconds += DeltaTime;
				AProjectOrganoidResearchStation* Alive = Station.Get();
				if (!Alive)
				{
					FailAndStop(Owner, Record, TEXT("Transient station vanished."));
					return;
				}
				if (WaitSeconds < 0.2f)
				{
					break;
				}

				UProjectOrganoidInteractionComponent* Interaction = Character->GetInteractionComponent();
				const float HealthBefore = Character->GetHealth();
				AProjectOrganoidInteractable* Focus = Interaction ? Interaction->GetFocusedInteractable() : nullptr;
				AssertTrue(Record, TEXT("station_focus"), Focus == Alive, TEXT("ResearchStation"), Focus ? Focus->GetName() : TEXT("none"), TEXT("scan"));
				AssertTrue(Record, TEXT("station_prompt"), Alive->GetInteractionPrompt().ToString().Contains(TEXT("Research Station")), TEXT("Use Research Station"), Alive->GetInteractionPrompt().ToString(), TEXT("prompt"));
				AssertTrue(Record, TEXT("can_interact_idle"), Alive->CanInteract(Character), TEXT("true"), Alive->CanInteract(Character) ? TEXT("true") : TEXT("false"), TEXT("Idle"));

				const bool bInteracted = Interaction && Interaction->TryInteract();
				AssertTrue(Record, TEXT("open_outside_combat"), bInteracted && Alive->IsStationUIOpen(), TEXT("open"), bInteracted ? TEXT("open") : TEXT("failed"), TEXT("ui"));
				APlayerController* PC = Cast<APlayerController>(Character->GetController());
				AssertTrue(Record, TEXT("game_and_ui_input"), PC && PC->bShowMouseCursor, TEXT("cursor"), (PC && PC->bShowMouseCursor) ? TEXT("cursor") : TEXT("hidden"), TEXT("input"));
				AssertTrue(Record, TEXT("no_heal_on_interact"), FMath::IsNearlyEqual(Character->GetHealth(), HealthBefore), FString::SanitizeFloat(HealthBefore), FString::SanitizeFloat(Character->GetHealth()), TEXT("vitals"));

				if (UProjectOrganoidResearchStationWidget* Widget = Alive->GetActiveStationWidget())
				{
					Widget->CloseStationUI();
				}
				else
				{
					Alive->CloseResearchStationUI();
				}
				AssertTrue(Record, TEXT("close_hides_ui"), !Alive->IsStationUIOpen(), TEXT("closed"), Alive->IsStationUIOpen() ? TEXT("open") : TEXT("closed"), TEXT("ui"));
				AssertTrue(Record, TEXT("close_restores_gameplay_input"), IsCursorGameplay(Character), TEXT("gameplay"), IsCursorGameplay(Character) ? TEXT("gameplay") : TEXT("ui"), TEXT("input"));
				Proof = EProof::HostStates;
				break;
			}
			case EProof::HostStates:
			{
				AProjectOrganoidResearchStation* Alive = Station.Get();
				UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>();
				if (!Alive || !Presence)
				{
					FailAndStop(Owner, Record, TEXT("Station or encounter presence missing."));
					return;
				}

				auto CheckState = [&](EProjectOrganoidHostCombatState State, bool bExpectLock, const TCHAR* Id)
				{
					HostAI->ApplyCombatState(State);
					const bool bLocked = Alive->IsLockedByEncounter() || !Alive->CanInteract(Character);
					const bool bPresence = Presence->IsEncounterActive();
					const bool bOk = bExpectLock ? (bLocked && bPresence) : (!bLocked && !Presence->DoesCombatStateLockStations(State));
					AssertTrue(
						Record, Id, bOk,
						bExpectLock ? TEXT("locked") : TEXT("allowed"),
						bLocked ? TEXT("locked") : TEXT("allowed"),
						StateName(State));
				};

				CheckState(EProjectOrganoidHostCombatState::Idle, false, TEXT("idle_allows_station"));
				CheckState(EProjectOrganoidHostCombatState::Investigate, false, TEXT("investigate_allows_station"));
				CheckState(EProjectOrganoidHostCombatState::Search, false, TEXT("search_allows_station"));
				CheckState(EProjectOrganoidHostCombatState::Return, false, TEXT("return_allows_station"));
				CheckState(EProjectOrganoidHostCombatState::Dead, false, TEXT("dead_allows_station"));
				CheckState(EProjectOrganoidHostCombatState::Pursue, true, TEXT("pursue_locks_station"));
				CheckState(EProjectOrganoidHostCombatState::Attack, true, TEXT("attack_locks_station"));
				HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
				AssertTrue(Record, TEXT("idle_after_lock_unlocks"), Alive->CanInteract(Character) && !Presence->IsEncounterActive(), TEXT("allowed"), Presence->IsEncounterActive() ? TEXT("locked") : TEXT("allowed"), TEXT("Idle"));
				Proof = EProof::AudioLinger;
				break;
			}
			case EProof::AudioLinger:
			{
				AProjectOrganoidResearchStation* Alive = Station.Get();
				UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>();
				UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>();
				if (!Alive || !Ambience || !Presence)
				{
					FailAndStop(Owner, Record, TEXT("Audio or presence subsystem missing."));
					return;
				}
				HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
				bool bInjected = false;
				if (UFunction* Notify = Ambience->FindFunction(FName(TEXT("NotifyCombatStimulus"))))
				{
					struct FNotifyCombatStimulusParams
					{
						float Intensity = 1.0f;
					} Params;
					Ambience->ProcessEvent(Notify, &Params);
					bInjected = true;
				}
				AssertTrue(Record, TEXT("audio_combat_linger_injected"), bInjected, TEXT("true"), bInjected ? TEXT("true") : TEXT("false"), TEXT("audio"));
				AssertTrue(Record, TEXT("audio_combat_linger_active"), Ambience->IsInCombat(), TEXT("true"), Ambience->IsInCombat() ? TEXT("true") : TEXT("false"), TEXT("audio"));
				AssertTrue(Record, TEXT("audio_linger_does_not_lock_station"), Alive->CanInteract(Character) && !Presence->IsEncounterActive(), TEXT("allowed"), Presence->IsEncounterActive() ? TEXT("locked") : TEXT("allowed"), TEXT("presence"));
				Proof = EProof::NamedSource;
				break;
			}
			case EProof::NamedSource:
			{
				AProjectOrganoidResearchStation* Alive = Station.Get();
				UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>();
				if (!Alive || !Presence)
				{
					FailAndStop(Owner, Record, TEXT("Named-source presence missing."));
					return;
				}
				const FName BossSource(TEXT("Playtest.SpecialEncounter"));
				Presence->SetEncounterSourceActive(BossSource, true);
				AssertTrue(Record, TEXT("named_source_locks_without_host_class"), !Alive->CanInteract(Character) && Presence->IsEncounterActive(), TEXT("locked"), Alive->CanInteract(Character) ? TEXT("allowed") : TEXT("locked"), TEXT("boss-api"));
				Presence->SetEncounterSourceActive(BossSource, false);
				AssertTrue(Record, TEXT("named_source_clears"), Alive->CanInteract(Character) && !Presence->IsEncounterActive(), TEXT("allowed"), Presence->IsEncounterActive() ? TEXT("locked") : TEXT("allowed"), TEXT("boss-api"));
				Proof = EProof::UnlockInstallRemove;
				break;
			}
			case EProof::UnlockInstallRemove:
			{
				AProjectOrganoidResearchStation* Alive = Station.Get();
				UProjectOrganoidWeaponModData* Barrel = ResolveBarrel();
				UProjectOrganoidWeaponModComponent* ModComp = GetModComp(Character);
				if (!Alive || !Barrel || !ModComp)
				{
					FailAndStop(Owner, Record, TEXT("Station, barrel, or mod component missing."));
					return;
				}

				if (CountSot(Character) < 3)
				{
					GrantSot(Character, 3);
				}
				SotAtUnlock = CountSot(Character);
				Weapon->SetCurrentMagazine(7);
				if (!SetReserveExact(Character, 9))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for remount ammo proof."));
					return;
				}
				const int32 MagBefore = Weapon->GetCurrentMagazine();
				const int32 ReserveBefore = ReserveOf(Character);
				const int32 CapBefore = Weapon->MagazineCapacity;
				const float BaseDamage = Weapon->Damage;
				const float BaseFireRate = Weapon->GetEffectiveFireRate();

				AssertTrue(Record, TEXT("not_owned_before_unlock"), !Character->IsWeaponModUnlocked(Barrel), TEXT("locked"), TEXT("owned"), TEXT("ownership"));
				AssertTrue(Record, TEXT("unlock_succeeds"), Character->UnlockWeaponMod(Barrel), TEXT("true"), TEXT("false"), TEXT("ownership"));
				AssertTrue(Record, TEXT("stabilized_barrel_owned"), Character->IsWeaponModUnlocked(Barrel), TEXT("true"), TEXT("false"), TEXT("ownership"));

				const bool bOpened = Alive->Interact(Character);
				UProjectOrganoidResearchStationWidget* Widget = Alive->GetActiveStationWidget();
				AssertTrue(Record, TEXT("ui_open_for_remount"), bOpened && Widget != nullptr, TEXT("open"), Widget ? TEXT("open") : TEXT("none"), TEXT("ui"));
				AssertTrue(Record, TEXT("ui_shows_unlocked"), Widget && Widget->GetStatusText().ToString().Contains(TEXT("Stabilized Barrel")), TEXT("Stabilized Barrel"), Widget ? Widget->GetStatusText().ToString() : TEXT("none"), TEXT("ui"));

				const bool bInstalled = Widget && Widget->InstallUnlockedMod(Barrel);
				AssertTrue(Record, TEXT("install_succeeds"), bInstalled, TEXT("true"), bInstalled ? TEXT("true") : TEXT("false"), TEXT("loadout"));
				AssertTrue(Record, TEXT("installed_visible"), ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == Barrel, TEXT("Stabilized Barrel"), ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) ? TEXT("installed") : TEXT("empty"), TEXT("loadout"));
				AssertTrue(
					Record, TEXT("install_affects_weapon"),
					FMath::IsNearlyEqual(Weapon->GetEffectiveDamage(), BaseDamage * 1.05f, 0.01f),
					FString::SanitizeFloat(BaseDamage * 1.05f),
					FString::SanitizeFloat(Weapon->GetEffectiveDamage()),
					TEXT("WeaponModComponent"));
				AssertTrue(Record, TEXT("install_zero_sot"), CountSot(Character) == SotAtUnlock, FString::FromInt(SotAtUnlock), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				AssertTrue(Record, TEXT("install_mag_unchanged"), Weapon->GetCurrentMagazine() == MagBefore && Weapon->MagazineCapacity == CapBefore, FString::FromInt(MagBefore), FString::FromInt(Weapon->GetCurrentMagazine()), TEXT("magazine"));
				AssertTrue(Record, TEXT("install_reserve_unchanged"), ReserveOf(Character) == ReserveBefore, FString::FromInt(ReserveBefore), FString::FromInt(ReserveOf(Character)), TEXT("ammo"));
				AssertTrue(Record, TEXT("install_fire_rate_unchanged"), FMath::IsNearlyEqual(Weapon->GetEffectiveFireRate(), BaseFireRate), FString::SanitizeFloat(BaseFireRate), FString::SanitizeFloat(Weapon->GetEffectiveFireRate()), TEXT("reload-arch"));

				const bool bRemoved = Widget && Widget->RemoveInstalledMod();
				AssertTrue(Record, TEXT("remove_succeeds"), bRemoved && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == nullptr, TEXT("removed"), bRemoved ? TEXT("removed") : TEXT("failed"), TEXT("loadout"));
				AssertTrue(Record, TEXT("remove_keeps_ownership"), Character->IsWeaponModUnlocked(Barrel), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
				AssertTrue(Record, TEXT("remove_zero_sot"), CountSot(Character) == SotAtUnlock, FString::FromInt(SotAtUnlock), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				AssertTrue(Record, TEXT("remove_restores_base_damage"), FMath::IsNearlyEqual(Weapon->GetEffectiveDamage(), BaseDamage, 0.01f), FString::SanitizeFloat(BaseDamage), FString::SanitizeFloat(Weapon->GetEffectiveDamage()), TEXT("WeaponModComponent"));
				AssertTrue(Record, TEXT("remove_mag_unchanged"), Weapon->GetCurrentMagazine() == MagBefore && ReserveOf(Character) == ReserveBefore, FString::FromInt(MagBefore), FString::FromInt(Weapon->GetCurrentMagazine()), TEXT("magazine"));

				AssertTrue(Record, TEXT("reinstall_for_save"), Widget && Widget->InstallUnlockedMod(Barrel), TEXT("true"), TEXT("false"), TEXT("loadout"));
				if (Widget)
				{
					Widget->CloseStationUI();
				}
				Proof = EProof::SaveLoad;
				break;
			}
			case EProof::SaveLoad:
			{
				UProjectOrganoidWeaponModData* Barrel = ResolveBarrel();
				UGameInstance* GI = Character->GetGameInstance();
				UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
				UProjectOrganoidWeaponModComponent* ModComp = GetModComp(Character);
				if (!Saves || !Barrel || !ModComp)
				{
					FailAndStop(Owner, Record, TEXT("Save subsystem missing."));
					return;
				}
				Saves->DeleteSave(SaveSlot);
				const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
				AssertTrue(Record, TEXT("saveload_wrote"), bSaved, TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), TEXT("save"));

				ModComp->RemoveModFromSlot(EProjectOrganoidWeaponModSlot::Barrel);
				Character->ApplyUnlockedWeaponMods(TArray<FSoftObjectPath>());
				AssertTrue(Record, TEXT("runtime_cleared_before_load"), !Character->IsWeaponModUnlocked(Barrel) && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == nullptr, TEXT("cleared"), TEXT("still-set"), TEXT("runtime"));

				const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				Weapon = GetWeapon(Character);
				ModComp = GetModComp(Character);
				AssertTrue(Record, TEXT("saveload_restored"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), TEXT("save"));
				AssertTrue(Record, TEXT("saveload_ownership"), Character->IsWeaponModUnlocked(Barrel), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
				AssertTrue(Record, TEXT("saveload_installed"), ModComp && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) != nullptr, TEXT("installed"), TEXT("empty"), TEXT("loadout"));
				Saves->DeleteSave(SaveSlot);
				Proof = EProof::CheckpointSetup;
				break;
			}
			case EProof::CheckpointSetup:
			{
				AProjectOrganoidCheckpoint* Checkpoint = ActivatedCheckpoint.Get();
				if (!Checkpoint)
				{
					FailAndStop(Owner, Record, TEXT("No checkpoint actor available."));
					return;
				}
				UProjectOrganoidWeaponModData* Barrel = ResolveBarrel();
				if (!Character->IsWeaponModUnlocked(Barrel))
				{
					Character->UnlockWeaponMod(Barrel);
				}
				if (UProjectOrganoidWeaponModComponent* ModComp = GetModComp(Character))
				{
					ModComp->InstallMod(Barrel, true);
				}
				CheckpointSot = CountSot(Character);
				bCheckpointHadBarrel = true;
				const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
				AssertTrue(Record, TEXT("checkpoint_saved_config"), bSaved && Character->HasActivatedCheckpoint(), TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), OrganoidPlaytestActions::ActorLabel(Checkpoint));
				Proof = EProof::CheckpointChange;
				break;
			}
			case EProof::CheckpointChange:
			{
				if (UProjectOrganoidWeaponModComponent* ModComp = GetModComp(Character))
				{
					ModComp->RemoveModFromSlot(EProjectOrganoidWeaponModSlot::Barrel);
				}
				GrantSot(Character, 2);
				AssertTrue(Record, TEXT("config_diverged_after_checkpoint"), GetModComp(Character) && GetModComp(Character)->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == nullptr, TEXT("removed"), TEXT("installed"), TEXT("loadout"));
				AssertTrue(Record, TEXT("sot_diverged_after_checkpoint"), CountSot(Character) != CheckpointSot, TEXT("changed"), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				Proof = EProof::PlayerDeath;
				break;
			}
			case EProof::PlayerDeath:
			{
				Character->ApplyHealthDelta(-Character->GetMaxHealth(), EProjectOrganoidHealthDeltaSource::Generic);
				AssertTrue(Record, TEXT("player_enters_death"), Character->IsPlayerDead(), TEXT("true"), Character->IsPlayerDead() ? TEXT("true") : TEXT("false"), TEXT("player"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitRestart;
				break;
			}
			case EProof::WaitRestart:
			{
				WaitSeconds += DeltaTime;
				Weapon = GetWeapon(Character);
				UProjectOrganoidWeaponModData* Barrel = ResolveBarrel();
				if (!Character->IsPlayerDead() && Character->GetHealth() > 0.0f && Weapon && Barrel)
				{
					UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent();
					AssertTrue(Record, TEXT("death_restores_ownership"), Character->IsWeaponModUnlocked(Barrel), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
					AssertTrue(Record, TEXT("death_restores_installed"), ModComp && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) != nullptr, TEXT("installed"), TEXT("empty"), TEXT("checkpoint"));
					AssertTrue(Record, TEXT("death_restores_sot"), CountSot(Character) == CheckpointSot, FString::FromInt(CheckpointSot), FString::FromInt(CountSot(Character)), TEXT("SOT"));
					AssertTrue(Record, TEXT("no_sot_duplication"), CountSot(Character) == CheckpointSot, FString::FromInt(CheckpointSot), FString::FromInt(CountSot(Character)), TEXT("SOT"));
					Proof = EProof::Done;
					break;
				}
				if (WaitSeconds > 4.0f)
				{
					FailAndStop(Owner, Record, TEXT("Death restart did not restore checkpoint Research Station configuration."));
				}
				break;
			}
			case EProof::Done:
				DestroyStation();
				Stage = EStage::EndPie;
				break;
			}

			if (bAnyAssertFailed && Stage == EStage::Proof)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
			}
		}

		FString StateName(EProjectOrganoidHostCombatState State) const
		{
			switch (State)
			{
			case EProjectOrganoidHostCombatState::Investigate: return TEXT("Investigate");
			case EProjectOrganoidHostCombatState::Pursue: return TEXT("Pursue");
			case EProjectOrganoidHostCombatState::Attack: return TEXT("Attack");
			case EProjectOrganoidHostCombatState::Search: return TEXT("Search");
			case EProjectOrganoidHostCombatState::Return: return TEXT("Return");
			case EProjectOrganoidHostCombatState::Dead: return TEXT("Dead");
			default: return TEXT("Idle");
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
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Stage = EStage::Finalize;
		}
	};

	struct FResearchStationAutoRegister
	{
		FResearchStationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FResearchStationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FResearchStationAutoRegister GRegisterResearchStation;
}
