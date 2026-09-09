#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHostCombatTypes.h"
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
	constexpr TCHAR TestId[] = TEXT("BiologicalAdaptation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Biological Adaptation Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidBiologicalAdaptationTest");
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

	class FBiologicalAdaptationFunctional : public IOrganoidPlaytestCase
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
			RejectPhase = 0;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			ActivationRealTime = 0.0;
			HostBaselineSpeed = 0.0f;
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
			Ownership,
			SpawnStation,
			StationEquip,
			UnequippedActivate,
			Rejects,
			ValidActivate,
			WaitDuration,
			TacticalCooldown,
			WaitTacticalCooldown,
			BarrelStillWorks,
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
		int32 RejectPhase = 0;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		double ActivationRealTime = 0.0;
		float HostBaselineSpeed = 0.0f;
		float HostHealthBefore = 0.0f;
		int32 SotBeforeConfig = 0;
		int32 MagBefore = 0;
		int32 ReserveBefore = 0;
		int32 GunfireBefore = 0;
		int32 CheckpointSot = 0;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host;
		TWeakObjectPtr<AProjectOrganoidResearchStation> Station;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> ActivatedCheckpoint;

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

		UProjectOrganoidBiologicalAdaptationComponent* GetAdapt(AProjectOrganoidCharacter* Character) const
		{
			return Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
		}

		UProjectOrganoidBiologicalAdaptationData* ResolveNeural() const
		{
			return UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve();
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

		void SetCameraLagEnabled(AProjectOrganoidCharacter* Character, bool bEnabled)
		{
			if (USpringArmComponent* Boom = Character ? Character->GetCameraBoom() : nullptr)
			{
				Boom->bEnableCameraLag = bEnabled;
			}
		}

		void AimAtSky(AProjectOrganoidCharacter* Character)
		{
			if (!Character)
			{
				return;
			}
			if (AController* Controller = Character->GetController())
			{
				const FRotator Rot = Controller->GetControlRotation();
				const FRotator LookDown(-85.0f, Rot.Yaw, 0.0f);
				Controller->SetControlRotation(LookDown);
				Character->SetActorRotation(FRotator(0.0f, Rot.Yaw, 0.0f));
			}
		}

		void PlaceNearHost(AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* InHost, float Distance)
		{
			if (!Character || !InHost)
			{
				return;
			}
			const FVector HostLoc = InHost->GetActorLocation();
			const FVector Offset = FVector(Distance, 0.0f, 0.0f);
			Character->SetActorLocation(HostLoc + Offset, false, nullptr, ETeleportType::TeleportPhysics);
			OrganoidPlaytestActions::FaceActor(Character, InHost);
		}

		void FillPE(AProjectOrganoidCharacter* Character, float Target)
		{
			if (!Character)
			{
				return;
			}
			Character->ApplyPEEnergyDelta(-Character->GetMaxPEEnergy());
			Character->ApplyPEEnergyDelta(Target);
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
			if (World && Character && GetWeapon(Character) && GetAdapt(Character) && Checkpoint && FoundHost && GetHostAI(FoundHost))
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
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE player, adaptation component, checkpoint, and Host_Neuro_1."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = GetAdapt(Character);
			AProjectOrganoidHostBase* HostActor = Host.Get();
			UProjectOrganoidBiologicalAdaptationData* Neural = ResolveNeural();
			if (!World || !Character || !Weapon || !Adapt || !HostActor || !Neural)
			{
				FailAndStop(Owner, Record, TEXT("PIE player, adaptation, weapon, or Host vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Content:
			{
				AssertTrue(Record, TEXT("neural_slow_exists"), Neural != nullptr, TEXT("present"), Neural ? Neural->AdaptationId.ToString() : TEXT("none"), TEXT("adaptation"));
				AssertTrue(Record, TEXT("neural_slow_id"), Neural->AdaptationId == TEXT("NeuralSlow"), TEXT("NeuralSlow"), Neural->AdaptationId.ToString(), TEXT("adaptation"));
				AssertTrue(Record, TEXT("neural_slow_pe_20"), FMath::IsNearlyEqual(Neural->PECost, 20.0f), TEXT("20"), FString::SanitizeFloat(Neural->PECost), TEXT("PROVISIONAL DESIGN TUNING"));
				AssertTrue(Record, TEXT("neural_slow_cooldown_8"), FMath::IsNearlyEqual(Neural->CooldownSeconds, 8.0f), TEXT("8"), FString::SanitizeFloat(Neural->CooldownSeconds), TEXT("PROVISIONAL DESIGN TUNING"));
				AssertTrue(Record, TEXT("neural_slow_range_800"), FMath::IsNearlyEqual(Neural->MaxTargetRange, 800.0f), TEXT("800"), FString::SanitizeFloat(Neural->MaxTargetRange), TEXT("PROVISIONAL DESIGN TUNING"));
				AssertTrue(Record, TEXT("neural_slow_target_count_1"), Neural->TargetCount == 1, TEXT("1"), FString::FromInt(Neural->TargetCount), TEXT("adaptation"));
				if (UProjectOrganoidBiologicalAdaptation_NeuralSlow* Slow = Cast<UProjectOrganoidBiologicalAdaptation_NeuralSlow>(Neural))
				{
					AssertTrue(Record, TEXT("neural_slow_duration_4"), FMath::IsNearlyEqual(Slow->DurationSeconds, 4.0f), TEXT("4"), FString::SanitizeFloat(Slow->DurationSeconds), TEXT("PROVISIONAL DESIGN TUNING"));
					AssertTrue(Record, TEXT("neural_slow_speed_0_6"), FMath::IsNearlyEqual(Slow->LocomotorSpeedMultiplier, 0.6f), TEXT("0.6"), FString::SanitizeFloat(Slow->LocomotorSpeedMultiplier), TEXT("PROVISIONAL DESIGN TUNING"));
				}
				Proof = EProof::Ownership;
				break;
			}
			case EProof::Ownership:
			{
				AssertTrue(Record, TEXT("not_owned_before_unlock"), !Adapt->IsAdaptationUnlocked(Neural), TEXT("locked"), TEXT("owned"), TEXT("ownership"));
				AssertTrue(Record, TEXT("unlock_succeeds"), Adapt->UnlockAdaptation(Neural), TEXT("true"), TEXT("false"), TEXT("ownership"));
				AssertTrue(Record, TEXT("neural_slow_owned"), Adapt->IsAdaptationUnlocked(Neural), TEXT("true"), TEXT("false"), TEXT("ownership"));
				AssertTrue(Record, TEXT("unlock_does_not_equip"), Adapt->GetEquippedAdaptation() == nullptr, TEXT("empty"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("loadout"));
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
				if (CountSot(Character) < 2)
				{
					GrantSot(Character, 2);
				}
				SotBeforeConfig = CountSot(Character);
				OrganoidPlaytestActions::FaceActor(Character, Spawned);
				WaitSeconds = 0.0f;
				Proof = EProof::StationEquip;
				break;
			}
			case EProof::StationEquip:
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

				UProjectOrganoidResearchStationWidget* Widget = Alive->OpenResearchStationUI(Character);
				AssertTrue(Record, TEXT("station_ui_opens"), Widget && Alive->IsStationUIOpen(), TEXT("open"), Alive->IsStationUIOpen() ? TEXT("open") : TEXT("closed"), TEXT("ui"));
				const FString Status = Widget ? Widget->GetStatusText().ToString() : FString();
				AssertTrue(Record, TEXT("station_shows_neural_slow_if_owned"), Status.Contains(TEXT("Neural Slow")), TEXT("Neural Slow"), Status, TEXT("ui"));
				AssertTrue(Record, TEXT("equip_zero_sot"), Widget && Widget->EquipUnlockedAdaptation(Neural) && Adapt->GetEquippedAdaptation() == Neural, TEXT("equipped"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("loadout"));
				AssertTrue(Record, TEXT("equip_sot_unchanged"), CountSot(Character) == SotBeforeConfig, FString::FromInt(SotBeforeConfig), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				AssertTrue(Record, TEXT("unequip_zero_sot"), Widget && Widget->UnequipAdaptation() && Adapt->GetEquippedAdaptation() == nullptr, TEXT("empty"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("loadout"));
				AssertTrue(Record, TEXT("unequip_keeps_ownership"), Adapt->IsAdaptationUnlocked(Neural), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
				AssertTrue(Record, TEXT("unequip_sot_unchanged"), CountSot(Character) == SotBeforeConfig, FString::FromInt(SotBeforeConfig), FString::FromInt(CountSot(Character)), TEXT("SOT"));

				UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>();
				AProjectOrganoidHostAIController* HostAI = GetHostAI(HostActor);
				if (Presence && HostAI)
				{
					HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Pursue);
					AssertTrue(Record, TEXT("pursue_locks_adaptation_config"), !Alive->TryEquipUnlockedAdaptation(Character, Neural), TEXT("locked"), TEXT("equipped"), TEXT("Pursue"));
					HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Attack);
					AssertTrue(Record, TEXT("attack_locks_adaptation_config"), !Alive->TryUnequipAdaptation(Character), TEXT("locked"), TEXT("changed"), TEXT("Attack"));
					HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
					AssertTrue(Record, TEXT("idle_allows_adaptation_config"), Alive->TryEquipUnlockedAdaptation(Character, Neural) && Adapt->GetEquippedAdaptation() == Neural, TEXT("equipped"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("Idle"));
				}

				if (Widget)
				{
					Widget->UnequipAdaptation();
					Widget->CloseStationUI();
				}
				Proof = EProof::UnequippedActivate;
				break;
			}
			case EProof::UnequippedActivate:
			{
				Adapt->UnequipAdaptation();
				FillPE(Character, 100.0f);
				const float PEBefore = Character->GetPEEnergy();
				PlaceNearHost(Character, HostActor, 250.0f);
				const bool bActivated = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("unequipped_ability_does_nothing"), !bActivated && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::Unequipped, TEXT("Unequipped"), TEXT("activated"), TEXT("input"));
				AssertTrue(Record, TEXT("unequipped_consumes_zero_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
				Adapt->EquipAdaptation(Neural);
				Proof = EProof::Rejects;
				break;
			}
			case EProof::Rejects:
			{
				// GetPlayerViewPoint uses the camera cache. Same-tick teleport/aim still
				// sees the previous Host-facing view, so settle the boom before asserting.
				if (RejectPhase == 0)
				{
					SetCameraLagEnabled(Character, false);
					FillPE(Character, 100.0f);
					Character->SetActorLocation(FVector(5000.0f, -900.0f, 220.0f), false, nullptr, ETeleportType::TeleportPhysics);
					AimAtSky(Character);
					WaitSeconds = 0.0f;
					RejectPhase = 1;
					break;
				}
				if (RejectPhase == 1)
				{
					WaitSeconds += DeltaTime;
					if (WaitSeconds < 0.35f)
					{
						AimAtSky(Character);
						break;
					}

					float PEBefore = Character->GetPEEnergy();
					bool bActivated = Adapt->TryActivateEquipped();
					const EProjectOrganoidBiologicalAdaptationFailReason NoTargetReason = Adapt->GetLastFailReason();
					const TCHAR* ReasonName = TEXT("Other");
					switch (NoTargetReason)
					{
					case EProjectOrganoidBiologicalAdaptationFailReason::NoTarget: ReasonName = TEXT("NoTarget"); break;
					case EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange: ReasonName = TEXT("OutOfRange"); break;
					case EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget: ReasonName = TEXT("InvalidTarget"); break;
					case EProjectOrganoidBiologicalAdaptationFailReason::Unequipped: ReasonName = TEXT("Unequipped"); break;
					case EProjectOrganoidBiologicalAdaptationFailReason::Cooldown: ReasonName = TEXT("Cooldown"); break;
					case EProjectOrganoidBiologicalAdaptationFailReason::InsufficientPE: ReasonName = TEXT("InsufficientPE"); break;
					default: break;
					}
					AssertTrue(
						Record, TEXT("no_target_rejected"),
						!bActivated && NoTargetReason == EProjectOrganoidBiologicalAdaptationFailReason::NoTarget,
						TEXT("NoTarget"),
						bActivated ? TEXT("activated") : ReasonName,
						TEXT("target"));
					AssertTrue(Record, TEXT("no_target_zero_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));

					PlaceNearHost(Character, HostActor, 1200.0f);
					WaitSeconds = 0.0f;
					RejectPhase = 2;
					break;
				}

				WaitSeconds += DeltaTime;
				if (WaitSeconds < 0.25f)
				{
					break;
				}

				float PEBefore = Character->GetPEEnergy();
				bool bActivated = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("out_of_range_rejected"), !bActivated && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::OutOfRange, TEXT("OutOfRange"), bActivated ? TEXT("activated") : TEXT("rejected"), TEXT("target"));
				AssertTrue(Record, TEXT("out_of_range_zero_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));

				PlaceNearHost(Character, HostActor, 250.0f);
				HostActor->bIsDead = true;
				PEBefore = Character->GetPEEnergy();
				bActivated = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("dead_target_rejected"), !bActivated && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::InvalidTarget, TEXT("InvalidTarget"), bActivated ? TEXT("activated") : TEXT("rejected"), TEXT("target"));
				AssertTrue(Record, TEXT("dead_target_zero_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
				HostActor->bIsDead = false;

				FillPE(Character, 10.0f);
				PlaceNearHost(Character, HostActor, 250.0f);
				PEBefore = Character->GetPEEnergy();
				bActivated = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("insufficient_pe_rejected"), !bActivated && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::InsufficientPE, TEXT("InsufficientPE"), bActivated ? TEXT("activated") : TEXT("rejected"), TEXT("PE"));
				AssertTrue(Record, TEXT("insufficient_pe_unchanged"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore), FString::SanitizeFloat(PEBefore), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("PE"));
				SetCameraLagEnabled(Character, true);
				RejectPhase = 0;
				Proof = EProof::ValidActivate;
				break;
			}
			case EProof::ValidActivate:
			{
				FillPE(Character, 100.0f);
				if (!SetReserveExact(Character, 9))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for Neural Slow ammo proof."));
					return;
				}
				Weapon->SetCurrentMagazine(7);
				MagBefore = Weapon->GetCurrentMagazine();
				ReserveBefore = ReserveOf(Character);
				GunfireBefore = Weapon->GetGunfireReportCount();
				HostActor->ActivateBioShield();
				HostHealthBefore = HostActor->Health;
				HostBaselineSpeed = HostActor->GetCharacterMovement() ? HostActor->GetCharacterMovement()->MaxWalkSpeed : 0.0f;
				const bool bHadShield = HostActor->HasBioShield();

				PlaceNearHost(Character, HostActor, 250.0f);
				const float PEBefore = Character->GetPEEnergy();
				const bool bActivated = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("valid_activation"), bActivated, TEXT("true"), bActivated ? TEXT("true") : TEXT("false"), TEXT("NeuralSlow"));
				AssertTrue(Record, TEXT("consumes_exactly_20_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore - 20.0f, 0.05f), TEXT("20"), FString::SanitizeFloat(PEBefore - Character->GetPEEnergy()), TEXT("PE"));
				AssertTrue(Record, TEXT("zero_direct_damage"), FMath::IsNearlyEqual(HostActor->Health, HostHealthBefore), FString::SanitizeFloat(HostHealthBefore), FString::SanitizeFloat(HostActor->Health), TEXT("host"));
				AssertTrue(Record, TEXT("does_not_strip_bio_shield"), bHadShield && HostActor->HasBioShield(), TEXT("shield"), HostActor->HasBioShield() ? TEXT("shield") : TEXT("stripped"), TEXT("host"));
				AssertTrue(Record, TEXT("ammo_unchanged"), ReserveOf(Character) == ReserveBefore, FString::FromInt(ReserveBefore), FString::FromInt(ReserveOf(Character)), TEXT("ammo"));
				AssertTrue(Record, TEXT("magazine_unchanged"), Weapon->GetCurrentMagazine() == MagBefore, FString::FromInt(MagBefore), FString::FromInt(Weapon->GetCurrentMagazine()), TEXT("magazine"));
				AssertTrue(Record, TEXT("no_gunfire_noise"), Weapon->GetGunfireReportCount() == GunfireBefore, FString::FromInt(GunfireBefore), FString::FromInt(Weapon->GetGunfireReportCount()), TEXT("weapon"));

				const float SlowSpeed = HostActor->GetCharacterMovement() ? HostActor->GetCharacterMovement()->MaxWalkSpeed : 0.0f;
				const float ExpectedSlow = HostBaselineSpeed * 0.6f;
				AssertTrue(Record, TEXT("host_speed_reduced_40"), HostActor->IsBiologicalLocomotorSlowActive() && FMath::Abs(SlowSpeed - ExpectedSlow) <= 8.0f, FString::SanitizeFloat(ExpectedSlow), FString::SanitizeFloat(SlowSpeed), TEXT("host"));

				const float PEAfter = Character->GetPEEnergy();
				const bool bRepeat = Adapt->TryActivateEquipped();
				AssertTrue(Record, TEXT("cooldown_rejects_without_pe_loss"), !bRepeat && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::Cooldown && FMath::IsNearlyEqual(Character->GetPEEnergy(), PEAfter), TEXT("Cooldown"), bRepeat ? TEXT("activated") : TEXT("rejected"), TEXT("cooldown"));

				ActivationRealTime = FPlatformTime::Seconds();
				WaitSeconds = 0.0f;
				Proof = EProof::WaitDuration;
				break;
			}
			case EProof::WaitDuration:
			{
				WaitSeconds += DeltaTime;
				const double Elapsed = FPlatformTime::Seconds() - ActivationRealTime;
				if (Elapsed >= 4.25)
				{
					const float Restored = HostActor->GetCharacterMovement() ? HostActor->GetCharacterMovement()->MaxWalkSpeed : 0.0f;
					AssertTrue(Record, TEXT("effect_lasts_about_4s"), Elapsed >= 4.0 && Elapsed < 5.0, TEXT("~4s"), FString::SanitizeFloat(Elapsed), TEXT("duration"));
					AssertTrue(Record, TEXT("host_speed_restored"), !HostActor->IsBiologicalLocomotorSlowActive() && FMath::Abs(Restored - HostBaselineSpeed) <= 8.0f, FString::SanitizeFloat(HostBaselineSpeed), FString::SanitizeFloat(Restored), TEXT("host"));
					Proof = EProof::TacticalCooldown;
				}
				else if (WaitSeconds > 6.0f)
				{
					FailAndStop(Owner, Record, TEXT("Neural Slow duration did not expire."));
				}
				break;
			}
			case EProof::TacticalCooldown:
			{
				FillPE(Character, 100.0f);
				if (!Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				AssertTrue(Record, TEXT("tactical_coexists_with_neural_slow"), Character->IsTacticalModeActive(), TEXT("true"), Character->IsTacticalModeActive() ? TEXT("true") : TEXT("false"), TEXT("tactical"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitTacticalCooldown;
				break;
			}
			case EProof::WaitTacticalCooldown:
			{
				WaitSeconds += DeltaTime;
				const double Elapsed = FPlatformTime::Seconds() - ActivationRealTime;
				if (Elapsed >= 8.15)
				{
					FillPE(Character, 100.0f);
					if (!Character->IsTacticalModeActive())
					{
						Character->ToggleTacticalMode();
					}
					PlaceNearHost(Character, HostActor, 250.0f);
					const bool bReady = !Adapt->IsOnCooldown();
					const bool bActivated = Adapt->TryActivateEquipped();
					AssertTrue(Record, TEXT("cooldown_8s_wall_clock"), bReady, TEXT("ready"), Adapt->IsOnCooldown() ? TEXT("cooling") : TEXT("ready"), TEXT("cooldown"));
					AssertTrue(Record, TEXT("tactical_does_not_multiply_cooldown"), bActivated, TEXT("activated"), bActivated ? TEXT("activated") : TEXT("blocked"), TEXT("cooldown"));
					if (Character->IsTacticalModeActive())
					{
						Character->ToggleTacticalMode();
					}
					Proof = EProof::BarrelStillWorks;
				}
				else if (WaitSeconds > 12.0f)
				{
					FailAndStop(Owner, Record, TEXT("Wall-clock cooldown did not expire under tactical dilation."));
				}
				break;
			}
			case EProof::BarrelStillWorks:
			{
				UProjectOrganoidWeaponModData* Barrel = UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve();
				UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent();
				AProjectOrganoidResearchStation* Alive = Station.Get();
				if (!Barrel || !ModComp || !Alive)
				{
					FailAndStop(Owner, Record, TEXT("Barrel or station missing."));
					return;
				}
				if (AProjectOrganoidHostAIController* HostAI = GetHostAI(HostActor))
				{
					HostAI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
				}
				Character->UnlockWeaponMod(Barrel);
				const bool bInstalled = Alive->TryInstallUnlockedMod(Character, Barrel);
				AssertTrue(Record, TEXT("barrel_path_still_functional"), bInstalled && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == Barrel, TEXT("installed"), bInstalled ? TEXT("installed") : TEXT("failed"), TEXT("WeaponMod"));
				Alive->TryRemoveInstalledMod(Character, EProjectOrganoidWeaponModSlot::Barrel);
				Proof = EProof::SaveLoad;
				break;
			}
			case EProof::SaveLoad:
			{
				UGameInstance* GI = Character->GetGameInstance();
				UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
				if (!Saves)
				{
					FailAndStop(Owner, Record, TEXT("Save subsystem missing."));
					return;
				}
				Adapt->EquipAdaptation(Neural);
				Saves->DeleteSave(SaveSlot);
				const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
				AssertTrue(Record, TEXT("saveload_wrote"), bSaved, TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), TEXT("save"));

				Adapt->UnequipAdaptation();
				Adapt->ApplyUnlockedAdaptations(TArray<FSoftObjectPath>());
				AssertTrue(Record, TEXT("runtime_cleared_before_load"), !Adapt->IsAdaptationUnlocked(Neural) && Adapt->GetEquippedAdaptation() == nullptr, TEXT("cleared"), TEXT("still-set"), TEXT("runtime"));

				const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				Adapt = GetAdapt(Character);
				AssertTrue(Record, TEXT("saveload_restored"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), TEXT("save"));
				AssertTrue(Record, TEXT("saveload_ownership"), Adapt && Adapt->IsAdaptationUnlocked(Neural), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
				AssertTrue(Record, TEXT("saveload_equipped"), Adapt && Adapt->GetEquippedAdaptation() != nullptr, TEXT("equipped"), Adapt && Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("loadout"));
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
				if (!Adapt->IsAdaptationUnlocked(Neural))
				{
					Adapt->UnlockAdaptation(Neural);
				}
				Adapt->EquipAdaptation(Neural);
				CheckpointSot = CountSot(Character);
				const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
				AssertTrue(Record, TEXT("checkpoint_saved_adaptations"), bSaved && Character->HasActivatedCheckpoint(), TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), OrganoidPlaytestActions::ActorLabel(Checkpoint));
				Proof = EProof::CheckpointChange;
				break;
			}
			case EProof::CheckpointChange:
			{
				Adapt->UnequipAdaptation();
				GrantSot(Character, 2);
				AssertTrue(Record, TEXT("config_diverged_after_checkpoint"), Adapt->GetEquippedAdaptation() == nullptr, TEXT("unequipped"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("loadout"));
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
				Adapt = GetAdapt(Character);
				if (!Character->IsPlayerDead() && Character->GetHealth() > 0.0f && Adapt)
				{
					AssertTrue(Record, TEXT("death_restores_ownership"), Adapt->IsAdaptationUnlocked(Neural), TEXT("owned"), TEXT("lost"), TEXT("ownership"));
					AssertTrue(Record, TEXT("death_restores_equipped"), Adapt->GetEquippedAdaptation() != nullptr, TEXT("equipped"), Adapt->GetEquippedAdaptation() ? TEXT("equipped") : TEXT("empty"), TEXT("checkpoint"));
					Proof = EProof::Done;
					break;
				}
				if (WaitSeconds > 4.0f)
				{
					FailAndStop(Owner, Record, TEXT("Death restart did not restore Biological Adaptation loadout."));
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

	struct FBiologicalAdaptationAutoRegister
	{
		FBiologicalAdaptationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FBiologicalAdaptationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FBiologicalAdaptationAutoRegister GRegisterBiologicalAdaptation;
}
