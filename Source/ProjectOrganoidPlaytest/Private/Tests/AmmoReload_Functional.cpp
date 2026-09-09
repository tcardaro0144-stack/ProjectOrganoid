#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("AmmoReload_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Ammo Reload Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR PistolAmmoPath[] = TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidAmmoReloadTest");
	constexpr float FireCooldownWait = 0.35f;
	constexpr float ReloadWaitLimit = 2.4f;
	constexpr float DilatedReloadWouldNeed = 6.0f;

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

	class FAmmoReloadFunctional : public IOrganoidPlaytestCase
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
			ReloadStartedRealTime = 0.0;
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
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
			SpawnState,
			FireConsume,
			WaitFireConsume,
			MissConsume,
			WaitMissConsume,
			EmptyCannotFire,
			PartialReload,
			WaitPartialReload,
			InsufficientReserve,
			WaitInsufficient,
			EmptyMagReload,
			WaitEmptyMagReload,
			ZeroReserveReject,
			FullMagReject,
			FireDuringReload,
			WaitFireDuringReload,
			ReloadOnce,
			WaitReloadOnce,
			TacticalDuration,
			WaitTacticalDuration,
			SaveLoad,
			CheckpointSetup,
			CheckpointSpend,
			PlayerDeath,
			WaitRestart,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Content;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		int32 ReportsBeforeEmpty = 0;
		float LastFireBeforeEmpty = 0.0f;
		int32 MagBeforeReload = 0;
		int32 ReserveBeforeReload = 0;
		int32 CheckpointMag = 0;
		int32 CheckpointReserve = 0;
		double ReloadStartedRealTime = 0.0;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> ActivatedCheckpoint;

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

		AProjectOrganoidWeapon* GetWeapon(AProjectOrganoidCharacter* Character) const
		{
			return Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
		}

		UProjectOrganoidInventoryComponent* GetInventory(AProjectOrganoidCharacter* Character) const
		{
			return Character ? Character->GetInventoryComponent() : nullptr;
		}

		int32 ReserveOf(AProjectOrganoidCharacter* Character, EProjectOrganoidAmmoType AmmoType) const
		{
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			return Inventory ? Inventory->CountAmmoOfType(AmmoType) : 0;
		}

		UProjectOrganoidItemData* ResolvePistolAmmo(AProjectOrganoidCharacter* Character) const
		{
			if (UProjectOrganoidItemData* Asset = LoadObject<UProjectOrganoidItemData>(nullptr, PistolAmmoPath))
			{
				return Asset;
			}
			return UProjectOrganoidItemData::CreateTransientPistolAmmo(Character);
		}

		bool GrantReserve(AProjectOrganoidCharacter* Character, int32 Quantity)
		{
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			UProjectOrganoidItemData* Ammo = ResolvePistolAmmo(Character);
			if (!Inventory || !Ammo || Quantity <= 0)
			{
				return false;
			}
			FGuid Id;
			return Inventory->TryAddItem(Ammo, Id, Quantity);
		}

		bool SetReserveExact(AProjectOrganoidCharacter* Character, int32 Quantity)
		{
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
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
				return GrantReserve(Character, Quantity - Current);
			}
			return true;
		}

		void AimAtSky(AProjectOrganoidCharacter* Character)
		{
			if (AController* Controller = Character ? Character->GetController() : nullptr)
			{
				const FRotator Rot = Controller->GetControlRotation();
				Controller->SetControlRotation(FRotator(80.0f, Rot.Yaw, 0.0f));
			}
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
			if (World && Character && GetWeapon(Character) && Checkpoint)
			{
				Player = Character;
				ActivatedCheckpoint = Checkpoint;
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
						TEXT("Timed out waiting for PIE player, weapon, and checkpoint (world=%s pawn=%s weapon=%s checkpoint=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						GetWeapon(Character) ? TEXT("yes") : TEXT("no"),
						Checkpoint ? TEXT("yes") : TEXT("no")));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			if (!World || !Character || !Weapon || !Inventory)
			{
				FailAndStop(Owner, Record, TEXT("PIE player, weapon, or inventory vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Content:
			{
				UProjectOrganoidItemData* DA = LoadObject<UProjectOrganoidItemData>(nullptr, PistolAmmoPath);
				AssertTrue(Record, TEXT("da_pistol_ammo_exists"), DA != nullptr, TEXT("DA_Item_PistolAmmo"), DA ? DA->GetName() : TEXT("missing"), TEXT("content"));
				if (DA)
				{
					AssertTrue(Record, TEXT("da_itemtype_ammo"), DA->ItemType == EProjectOrganoidItemType::Ammo, TEXT("Ammo"), TEXT("other"), TEXT("content"));
					AssertTrue(Record, TEXT("da_ammotype_pistol"), DA->AmmoType == EProjectOrganoidAmmoType::Pistol, TEXT("Pistol"), TEXT("other"), TEXT("content"));
					AssertTrue(Record, TEXT("da_stackable_1x1"), DA->bCanStack && DA->GridWidth == 1 && DA->GridHeight == 1, TEXT("stack 1x1"), TEXT("no"), TEXT("content"));
				}
				Proof = EProof::SpawnState;
				break;
			}
			case EProof::SpawnState:
			{
				AssertTrue(Record, TEXT("pistol_capacity_12"), Weapon->GetMagazineCapacity() == 12, TEXT("12"), FString::FromInt(Weapon->GetMagazineCapacity()), Weapon->GetName());
				AssertTrue(Record, TEXT("pistol_spawn_full_mag"), Weapon->GetCurrentMagazine() == 12, TEXT("12"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				AssertTrue(Record, TEXT("reload_duration_1_6"), FMath::IsNearlyEqual(Weapon->ReloadDurationSeconds, 1.6f, 0.01f), TEXT("1.6"), FString::SanitizeFloat(Weapon->ReloadDurationSeconds), Weapon->GetName());
				Proof = EProof::FireConsume;
				break;
			}
			case EProof::FireConsume:
			{
				if (!SetReserveExact(Character, 8))
				{
					FailAndStop(Owner, Record, TEXT("Failed to inject pistol reserve for fire consume."));
					return;
				}
				Weapon->SetCurrentMagazine(12);
				const int32 ReserveBefore = ReserveOf(Character, Weapon->AmmoType);
				const bool bFired = Weapon->Fire();
				AssertTrue(Record, TEXT("fire_succeeds_with_mag"), bFired, TEXT("true"), bFired ? TEXT("true") : TEXT("false"), Weapon->GetName());
				AssertTrue(Record, TEXT("fire_consumes_one_loaded"), Weapon->GetCurrentMagazine() == 11, TEXT("11"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				AssertTrue(Record, TEXT("fire_does_not_consume_reserve"), ReserveOf(Character, Weapon->AmmoType) == ReserveBefore, FString::FromInt(ReserveBefore), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitFireConsume;
				break;
			}
			case EProof::WaitFireConsume:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds >= FireCooldownWait)
				{
					Proof = EProof::MissConsume;
				}
				break;
			}
			case EProof::MissConsume:
			{
				AimAtSky(Character);
				const int32 MagBefore = Weapon->GetCurrentMagazine();
				const int32 ReportsBefore = Weapon->GetGunfireReportCount();
				const bool bFired = Weapon->Fire();
				AssertTrue(Record, TEXT("miss_still_fires"), bFired, TEXT("true"), bFired ? TEXT("true") : TEXT("false"), Weapon->GetName());
				AssertTrue(Record, TEXT("miss_consumes_one_round"), Weapon->GetCurrentMagazine() == MagBefore - 1, FString::FromInt(MagBefore - 1), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				AssertTrue(Record, TEXT("miss_reports_gunfire"), Weapon->GetGunfireReportCount() == ReportsBefore + 1, FString::FromInt(ReportsBefore + 1), FString::FromInt(Weapon->GetGunfireReportCount()), Weapon->GetName());
				WaitSeconds = 0.0f;
				Proof = EProof::WaitMissConsume;
				break;
			}
			case EProof::WaitMissConsume:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds >= FireCooldownWait)
				{
					Proof = EProof::EmptyCannotFire;
				}
				break;
			}
			case EProof::EmptyCannotFire:
			{
				Weapon->SetCurrentMagazine(0);
				ReportsBeforeEmpty = Weapon->GetGunfireReportCount();
				LastFireBeforeEmpty = Weapon->GetLastFireTimeSeconds();
				const int32 ReserveBefore = ReserveOf(Character, Weapon->AmmoType);
				const bool bFired = Weapon->Fire();
				AssertTrue(Record, TEXT("empty_mag_cannot_fire"), !bFired, TEXT("false"), bFired ? TEXT("true") : TEXT("false"), Weapon->GetName());
				AssertTrue(Record, TEXT("empty_does_not_consume_reserve"), ReserveOf(Character, Weapon->AmmoType) == ReserveBefore, FString::FromInt(ReserveBefore), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
				AssertTrue(Record, TEXT("empty_no_gunfire_report"), Weapon->GetGunfireReportCount() == ReportsBeforeEmpty, FString::FromInt(ReportsBeforeEmpty), FString::FromInt(Weapon->GetGunfireReportCount()), Weapon->GetName());
				AssertTrue(Record, TEXT("empty_no_hearing_event"), Weapon->GetGunfireReportCount() == ReportsBeforeEmpty, TEXT("no ReportGunfireNoise"), FString::FromInt(Weapon->GetGunfireReportCount()), Weapon->GetName());
				AssertTrue(Record, TEXT("empty_does_not_advance_lastfire"), FMath::IsNearlyEqual(Weapon->GetLastFireTimeSeconds(), LastFireBeforeEmpty), TEXT("unchanged"), FString::SanitizeFloat(Weapon->GetLastFireTimeSeconds()), Weapon->GetName());
				Proof = EProof::PartialReload;
				break;
			}
			case EProof::PartialReload:
			{
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(7);
				if (!SetReserveExact(Character, 20))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for partial reload."));
					return;
				}
				MagBeforeReload = 7;
				ReserveBeforeReload = 20;
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("partial_reload_starts"), bStarted && Weapon->IsReloading(), TEXT("reloading"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				ReloadStartedRealTime = FPlatformTime::Seconds();
				WaitSeconds = 0.0f;
				Proof = EProof::WaitPartialReload;
				break;
			}
			case EProof::WaitPartialReload:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					const double Elapsed = FPlatformTime::Seconds() - ReloadStartedRealTime;
					AssertTrue(Record, TEXT("partial_reload_mag"), Weapon->GetCurrentMagazine() == 12, TEXT("12"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
					AssertTrue(Record, TEXT("partial_reload_reserve"), ReserveOf(Character, Weapon->AmmoType) == 15, TEXT("15"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
					AssertTrue(Record, TEXT("partial_transfer_five"), true, TEXT("5"), TEXT("5"), Weapon->GetName());
					AssertTrue(Record, TEXT("partial_reload_realtime"), Elapsed >= 1.5 && Elapsed < DilatedReloadWouldNeed, TEXT("1.6s real"), FString::SanitizeFloat(Elapsed), Weapon->GetName());
					Proof = EProof::InsufficientReserve;
					break;
				}
				if (WaitSeconds > ReloadWaitLimit)
				{
					FailAndStop(Owner, Record, TEXT("Partial reload did not complete in real-time window."));
				}
				break;
			}
			case EProof::InsufficientReserve:
			{
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(10);
				if (!SetReserveExact(Character, 1))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for insufficient reload."));
					return;
				}
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("insufficient_reload_starts"), bStarted && Weapon->IsReloading(), TEXT("reloading"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				WaitSeconds = 0.0f;
				Proof = EProof::WaitInsufficient;
				break;
			}
			case EProof::WaitInsufficient:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					AssertTrue(Record, TEXT("insufficient_reload_mag"), Weapon->GetCurrentMagazine() == 11, TEXT("11"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
					AssertTrue(Record, TEXT("insufficient_reload_reserve"), ReserveOf(Character, Weapon->AmmoType) == 0, TEXT("0"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
					Proof = EProof::EmptyMagReload;
					break;
				}
				if (WaitSeconds > ReloadWaitLimit)
				{
					FailAndStop(Owner, Record, TEXT("Insufficient-reserve reload did not complete."));
				}
				break;
			}
			case EProof::EmptyMagReload:
			{
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(0);
				if (!SetReserveExact(Character, 8))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for empty-mag reload."));
					return;
				}
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("empty_mag_reload_starts"), bStarted && Weapon->IsReloading(), TEXT("reloading"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				WaitSeconds = 0.0f;
				Proof = EProof::WaitEmptyMagReload;
				break;
			}
			case EProof::WaitEmptyMagReload:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					AssertTrue(Record, TEXT("empty_mag_reload_result"), Weapon->GetCurrentMagazine() == 8, TEXT("8"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
					AssertTrue(Record, TEXT("empty_mag_reload_reserve"), ReserveOf(Character, Weapon->AmmoType) == 0, TEXT("0"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
					Proof = EProof::ZeroReserveReject;
					break;
				}
				if (WaitSeconds > ReloadWaitLimit)
				{
					FailAndStop(Owner, Record, TEXT("Empty-mag reload did not complete."));
				}
				break;
			}
			case EProof::ZeroReserveReject:
			{
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(4);
				if (!SetReserveExact(Character, 0))
				{
					FailAndStop(Owner, Record, TEXT("Failed to clear reserve for zero-reserve reject."));
					return;
				}
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("zero_reserve_rejects_reload"), !bStarted && !Weapon->IsReloading(), TEXT("rejected"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				AssertTrue(Record, TEXT("zero_reserve_mag_unchanged"), Weapon->GetCurrentMagazine() == 4, TEXT("4"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				Proof = EProof::FullMagReject;
				break;
			}
			case EProof::FullMagReject:
			{
				Weapon->SetCurrentMagazine(12);
				if (!SetReserveExact(Character, 6))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for full-mag reject."));
					return;
				}
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("full_mag_rejects_reload"), !bStarted && !Weapon->IsReloading(), TEXT("rejected"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				AssertTrue(Record, TEXT("full_mag_reserve_untouched"), ReserveOf(Character, Weapon->AmmoType) == 6, TEXT("6"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
				Proof = EProof::FireDuringReload;
				break;
			}
			case EProof::FireDuringReload:
			{
				Weapon->SetCurrentMagazine(6);
				if (!SetReserveExact(Character, 10))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for fire-during-reload."));
					return;
				}
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				const int32 ReportsBefore = Weapon->GetGunfireReportCount();
				const bool bFired = Weapon->Fire();
				AssertTrue(Record, TEXT("reload_started_for_block_fire"), bStarted && Weapon->IsReloading(), TEXT("reloading"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				AssertTrue(Record, TEXT("fire_during_reload_rejected"), !bFired, TEXT("false"), bFired ? TEXT("true") : TEXT("false"), Weapon->GetName());
				AssertTrue(Record, TEXT("fire_during_reload_no_consume"), Weapon->GetCurrentMagazine() == 6, TEXT("6"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				AssertTrue(Record, TEXT("fire_during_reload_no_gunfire"), Weapon->GetGunfireReportCount() == ReportsBefore, FString::FromInt(ReportsBefore), FString::FromInt(Weapon->GetGunfireReportCount()), Weapon->GetName());
				WaitSeconds = 0.0f;
				Proof = EProof::WaitFireDuringReload;
				break;
			}
			case EProof::WaitFireDuringReload:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					AssertTrue(Record, TEXT("blocked_reload_still_completes"), Weapon->GetCurrentMagazine() == 12, TEXT("12"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
					AssertTrue(Record, TEXT("blocked_reload_reserve"), ReserveOf(Character, Weapon->AmmoType) == 4, TEXT("4"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
					Proof = EProof::ReloadOnce;
					break;
				}
				if (WaitSeconds > ReloadWaitLimit)
				{
					FailAndStop(Owner, Record, TEXT("Reload after blocked fire did not complete."));
				}
				break;
			}
			case EProof::ReloadOnce:
			{
				const int32 Mag = Weapon->GetCurrentMagazine();
				const int32 Reserve = ReserveOf(Character, Weapon->AmmoType);
				WaitSeconds = 0.0f;
				MagBeforeReload = Mag;
				ReserveBeforeReload = Reserve;
				Proof = EProof::WaitReloadOnce;
				break;
			}
			case EProof::WaitReloadOnce:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds >= 0.75f)
				{
					AssertTrue(Record, TEXT("reload_transfers_exactly_once"), Weapon->GetCurrentMagazine() == MagBeforeReload && ReserveOf(Character, Weapon->AmmoType) == ReserveBeforeReload, TEXT("unchanged"), FString::Printf(TEXT("mag=%d reserve=%d"), Weapon->GetCurrentMagazine(), ReserveOf(Character, Weapon->AmmoType)), Weapon->GetName());
					AssertTrue(Record, TEXT("not_reloading_after_complete"), !Weapon->IsReloading(), TEXT("idle"), Weapon->IsReloading() ? TEXT("reloading") : TEXT("idle"), Weapon->GetName());
					Proof = EProof::TacticalDuration;
				}
				break;
			}
			case EProof::TacticalDuration:
			{
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(3);
				if (!SetReserveExact(Character, 12))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set reserve for tactical reload."));
					return;
				}
				if (!Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				AssertTrue(Record, TEXT("tactical_active_for_reload"), Character->IsTacticalModeActive(), TEXT("true"), Character->IsTacticalModeActive() ? TEXT("true") : TEXT("false"), TEXT("player"));
				const bool bStarted = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("tactical_reload_starts"), bStarted && Weapon->IsReloading(), TEXT("reloading"), bStarted ? TEXT("started") : TEXT("rejected"), Weapon->GetName());
				ReloadStartedRealTime = FPlatformTime::Seconds();
				WaitSeconds = 0.0f;
				Proof = EProof::WaitTacticalDuration;
				break;
			}
			case EProof::WaitTacticalDuration:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					const double Elapsed = FPlatformTime::Seconds() - ReloadStartedRealTime;
					AssertTrue(Record, TEXT("tactical_reload_completes"), Weapon->GetCurrentMagazine() == 12, TEXT("12"), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
					AssertTrue(
						Record, TEXT("tactical_does_not_multiply_reload"),
						Elapsed < DilatedReloadWouldNeed && Elapsed >= 1.4,
						TEXT("<6s real (~1.6s)"),
						FString::SanitizeFloat(Elapsed),
						Weapon->GetName());
					if (Character->IsTacticalModeActive())
					{
						Character->ToggleTacticalMode();
					}
					Proof = EProof::SaveLoad;
					break;
				}
				if (WaitSeconds > ReloadWaitLimit)
				{
					FailAndStop(Owner, Record, TEXT("Tactical reload used dilated time or failed to finish."));
				}
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
				Saves->DeleteSave(SaveSlot);
				Weapon->SetCurrentMagazine(5);
				if (!SetReserveExact(Character, 9))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set ammo for save/load."));
					return;
				}
				const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
				AssertTrue(Record, TEXT("saveload_wrote"), bSaved, TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), TEXT("save"));
				Weapon->SetCurrentMagazine(1);
				SetReserveExact(Character, 3);
				const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
				Weapon = GetWeapon(Character);
				AssertTrue(Record, TEXT("saveload_restored"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), TEXT("save"));
				AssertTrue(Record, TEXT("saveload_magazine"), Weapon && Weapon->GetCurrentMagazine() == 5, TEXT("5"), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("none"), TEXT("weapon"));
				AssertTrue(Record, TEXT("saveload_reserve"), ReserveOf(Character, Weapon ? Weapon->AmmoType : EProjectOrganoidAmmoType::Pistol) == 9, TEXT("9"), FString::FromInt(ReserveOf(Character, Weapon ? Weapon->AmmoType : EProjectOrganoidAmmoType::Pistol)), TEXT("inventory"));
				Saves->DeleteSave(SaveSlot);
				Proof = EProof::CheckpointSetup;
				break;
			}
			case EProof::CheckpointSetup:
			{
				AProjectOrganoidCheckpoint* Checkpoint = ActivatedCheckpoint.Get();
				if (!Checkpoint)
				{
					Checkpoint = FindAnyCheckpoint(World);
					ActivatedCheckpoint = Checkpoint;
				}
				if (!Checkpoint)
				{
					FailAndStop(Owner, Record, TEXT("No checkpoint actor available for death restore."));
					return;
				}
				Weapon->CancelReload();
				Weapon->SetCurrentMagazine(8);
				if (!SetReserveExact(Character, 7))
				{
					FailAndStop(Owner, Record, TEXT("Failed to set checkpoint ammunition."));
					return;
				}
				CheckpointMag = 8;
				CheckpointReserve = 7;
				const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
				AssertTrue(Record, TEXT("checkpoint_saved_ammo"), bSaved && Character->HasActivatedCheckpoint(), TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), OrganoidPlaytestActions::ActorLabel(Checkpoint));
				Proof = EProof::CheckpointSpend;
				break;
			}
			case EProof::CheckpointSpend:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds < FireCooldownWait)
				{
					break;
				}
				WaitSeconds = 0.0f;
				const bool bFired = Weapon->Fire();
				AssertTrue(Record, TEXT("spent_after_checkpoint"), bFired && Weapon->GetCurrentMagazine() == CheckpointMag - 1, FString::FromInt(CheckpointMag - 1), FString::FromInt(Weapon->GetCurrentMagazine()), Weapon->GetName());
				GrantReserve(Character, 4);
				AssertTrue(Record, TEXT("reserve_diverged_after_checkpoint"), ReserveOf(Character, Weapon->AmmoType) != CheckpointReserve, TEXT("changed"), FString::FromInt(ReserveOf(Character, Weapon->AmmoType)), TEXT("inventory"));
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
				if (!Character->IsPlayerDead() && Character->GetHealth() > 0.0f && Weapon)
				{
					const int32 Mag = Weapon->GetCurrentMagazine();
					const int32 Reserve = ReserveOf(Character, Weapon->AmmoType);
					AssertTrue(Record, TEXT("death_restores_magazine"), Mag == CheckpointMag, FString::FromInt(CheckpointMag), FString::FromInt(Mag), TEXT("weapon"));
					AssertTrue(Record, TEXT("death_restores_reserve"), Reserve == CheckpointReserve, FString::FromInt(CheckpointReserve), FString::FromInt(Reserve), TEXT("inventory"));
					AssertTrue(Record, TEXT("no_ammo_duplication"), Mag + Reserve == CheckpointMag + CheckpointReserve, FString::FromInt(CheckpointMag + CheckpointReserve), FString::FromInt(Mag + Reserve), TEXT("ammo"));
					Proof = EProof::Done;
					break;
				}
				if (WaitSeconds > 4.0f)
				{
					FailAndStop(Owner, Record, TEXT("Death restart did not restore checkpoint ammunition."));
				}
				break;
			}
			case EProof::Done:
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

	struct FAmmoReloadAutoRegister
	{
		FAmmoReloadAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FAmmoReloadFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FAmmoReloadAutoRegister GRegisterAmmoReload;
}
