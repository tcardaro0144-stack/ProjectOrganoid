#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponTypes.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("PETactical_Functional");
	constexpr TCHAR DisplayName[] = TEXT("PE Tactical Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
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

	class FPETacticalFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Defaults;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			SampleStartReal = 0.0;
			SampleStartPE = 0.0f;
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
			Defaults,
			RechargeSample,
			WaitRecharge,
			DrainSample,
			WaitDrain,
			ZeroRefuse,
			ZeroExitStart,
			WaitZeroExit,
			MovementFireReload,
			WaitReload,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Defaults;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		double SampleStartReal = 0.0;
		float SampleStartPE = 0.0f;
		int32 MagBeforeReload = 0;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;

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

		void FillPE(AProjectOrganoidCharacter* Character, float Target)
		{
			if (!Character)
			{
				return;
			}
			Character->ApplyPEEnergyDelta(-Character->GetMaxPEEnergy());
			Character->ApplyPEEnergyDelta(Target);
		}

		float WorldDilation(UWorld* World) const
		{
			return (World && World->GetWorldSettings()) ? World->GetWorldSettings()->TimeDilation : 1.0f;
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
			Stage = EStage::WaitReady;
			Owner.SetStage(TEXT("WaitReady"));
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			if (World && Character && GetWeapon(Character))
			{
				Player = Character;
				WaitSeconds = 0.0f;
				Proof = EProof::Defaults;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE player and weapon."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			if (!World || !Character || !Weapon)
			{
				FailAndStop(Owner, Record, TEXT("PIE player or weapon vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Defaults:
			{
				if (Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				AssertTrue(Record, TEXT("tactical_radius_800"), FMath::IsNearlyEqual(Character->GetTacticalSphereRadius(), 800.0f), TEXT("800"), FString::SanitizeFloat(Character->GetTacticalSphereRadius()), TEXT("PROVISIONAL"));
				AssertTrue(Record, TEXT("tactical_dilation_0_2"), FMath::IsNearlyEqual(Character->GetTacticalTimeDilation(), 0.2f), TEXT("0.2"), FString::SanitizeFloat(Character->GetTacticalTimeDilation()), TEXT("PROVISIONAL"));
				AssertTrue(Record, TEXT("pe_recharge_rate"), FMath::IsNearlyEqual(Character->GetPERechargeRate(), 2.5f), TEXT("2.5"), FString::SanitizeFloat(Character->GetPERechargeRate()), TEXT("PE"));
				AssertTrue(Record, TEXT("pe_drain_rate"), FMath::IsNearlyEqual(Character->GetPEDrainRate(), 10.0f), TEXT("10"), FString::SanitizeFloat(Character->GetPEDrainRate()), TEXT("PE"));
				AssertTrue(Record, TEXT("tactical_off_dilation_1"), FMath::IsNearlyEqual(WorldDilation(World), 1.0f, 0.01f), TEXT("1"), FString::SanitizeFloat(WorldDilation(World)), TEXT("world"));
				Proof = EProof::RechargeSample;
				break;
			}
			case EProof::RechargeSample:
			{
				FillPE(Character, 50.0f);
				SampleStartPE = Character->GetPEEnergy();
				SampleStartReal = FPlatformTime::Seconds();
				WaitSeconds = 0.0f;
				Proof = EProof::WaitRecharge;
				break;
			}
			case EProof::WaitRecharge:
			{
				WaitSeconds += DeltaTime;
				const double Elapsed = FPlatformTime::Seconds() - SampleStartReal;
				if (Elapsed >= 1.0)
				{
					const float Gained = Character->GetPEEnergy() - SampleStartPE;
					const float Expected = Character->GetPERechargeRate() * static_cast<float>(Elapsed);
					AssertTrue(
						Record, TEXT("pe_recharges_undilated"),
						FMath::Abs(Gained - Expected) <= 1.25f,
						FString::SanitizeFloat(Expected),
						FString::SanitizeFloat(Gained),
						TEXT("PE"));
					Proof = EProof::DrainSample;
				}
				else if (WaitSeconds > 3.0f)
				{
					FailAndStop(Owner, Record, TEXT("PE recharge sample timed out."));
				}
				break;
			}
			case EProof::DrainSample:
			{
				FillPE(Character, 80.0f);
				if (!Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				AssertTrue(Record, TEXT("tactical_activates_with_pe"), Character->IsTacticalModeActive(), TEXT("true"), Character->IsTacticalModeActive() ? TEXT("true") : TEXT("false"), TEXT("tactical"));
				AssertTrue(Record, TEXT("tactical_applies_0_2_dilation"), FMath::IsNearlyEqual(WorldDilation(World), 0.2f, 0.01f), TEXT("0.2"), FString::SanitizeFloat(WorldDilation(World)), TEXT("world"));
				SampleStartPE = Character->GetPEEnergy();
				SampleStartReal = FPlatformTime::Seconds();
				WaitSeconds = 0.0f;
				Proof = EProof::WaitDrain;
				break;
			}
			case EProof::WaitDrain:
			{
				WaitSeconds += DeltaTime;
				const double Elapsed = FPlatformTime::Seconds() - SampleStartReal;
				if (Elapsed >= 1.0)
				{
					const float Lost = SampleStartPE - Character->GetPEEnergy();
					const float Expected = Character->GetPEDrainRate() * static_cast<float>(Elapsed);
					const float DilatedWouldLose = Expected * Character->GetTacticalTimeDilation();
					AssertTrue(
						Record, TEXT("pe_drains_undilated"),
						FMath::Abs(Lost - Expected) <= 2.0f && Lost > DilatedWouldLose + 3.0f,
						FString::SanitizeFloat(Expected),
						FString::SanitizeFloat(Lost),
						TEXT("PE"));
					if (Character->IsTacticalModeActive())
					{
						Character->ToggleTacticalMode();
					}
					Proof = EProof::ZeroRefuse;
				}
				else if (WaitSeconds > 3.0f)
				{
					FailAndStop(Owner, Record, TEXT("PE drain sample timed out."));
				}
				break;
			}
			case EProof::ZeroRefuse:
			{
				if (Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				FillPE(Character, 0.0f);
				Character->ToggleTacticalMode();
				AssertTrue(Record, TEXT("tactical_refuses_at_zero_pe"), !Character->IsTacticalModeActive(), TEXT("false"), Character->IsTacticalModeActive() ? TEXT("true") : TEXT("false"), TEXT("tactical"));
				Proof = EProof::ZeroExitStart;
				break;
			}
			case EProof::ZeroExitStart:
			{
				FillPE(Character, 3.0f);
				Character->ToggleTacticalMode();
				AssertTrue(Record, TEXT("tactical_starts_with_low_pe"), Character->IsTacticalModeActive(), TEXT("true"), Character->IsTacticalModeActive() ? TEXT("true") : TEXT("false"), TEXT("tactical"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitZeroExit;
				break;
			}
			case EProof::WaitZeroExit:
			{
				WaitSeconds += DeltaTime;
				if (!Character->IsTacticalModeActive())
				{
					AssertTrue(Record, TEXT("tactical_exits_at_zero_pe"), Character->GetPEEnergy() <= KINDA_SMALL_NUMBER, TEXT("0"), FString::SanitizeFloat(Character->GetPEEnergy()), TEXT("tactical"));
					Proof = EProof::MovementFireReload;
					break;
				}
				if (WaitSeconds > 3.0f)
				{
					FailAndStop(Owner, Record, TEXT("Tactical did not exit after PE depleted."));
				}
				break;
			}
			case EProof::MovementFireReload:
			{
				FillPE(Character, 80.0f);
				if (!Character->IsTacticalModeActive())
				{
					Character->ToggleTacticalMode();
				}
				UCharacterMovementComponent* Move = Character->GetCharacterMovement();
				AssertTrue(Record, TEXT("movement_available_in_tactical"), Move && Move->IsActive() && !Move->IsMoveInputIgnored(), TEXT("active"), (Move && Move->IsActive()) ? TEXT("active") : TEXT("disabled"), TEXT("movement"));
				Character->DoMove(0.0f, 1.0f);
				if (!SetReserveExact(Character, 12))
				{
					FailAndStop(Owner, Record, TEXT("Failed to grant reserve for tactical fire/reload."));
					return;
				}
				Weapon->SetCurrentMagazine(6);
				const bool bFired = Character->GetWeaponComponent()->FireEquippedWeapon();
				AssertTrue(Record, TEXT("firearm_available_in_tactical"), bFired, TEXT("true"), bFired ? TEXT("true") : TEXT("false"), TEXT("weapon"));
				MagBeforeReload = Weapon->GetCurrentMagazine();
				const bool bReload = Character->GetWeaponComponent()->ReloadEquippedWeapon();
				AssertTrue(Record, TEXT("reload_available_in_tactical"), bReload && Weapon->IsReloading(), TEXT("reloading"), bReload ? TEXT("started") : TEXT("rejected"), TEXT("weapon"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitReload;
				break;
			}
			case EProof::WaitReload:
			{
				WaitSeconds += DeltaTime;
				if (!Weapon->IsReloading())
				{
					AssertTrue(Record, TEXT("reload_completes_in_tactical"), Weapon->GetCurrentMagazine() > MagBeforeReload, TEXT("increased"), FString::FromInt(Weapon->GetCurrentMagazine()), TEXT("weapon"));
					if (Character->IsTacticalModeActive())
					{
						Character->ToggleTacticalMode();
					}
					AssertTrue(Record, TEXT("tactical_off_restores_dilation"), FMath::IsNearlyEqual(WorldDilation(World), 1.0f, 0.01f), TEXT("1"), FString::SanitizeFloat(WorldDilation(World)), TEXT("world"));
					Proof = EProof::Done;
					break;
				}
				if (WaitSeconds > 3.0f)
				{
					FailAndStop(Owner, Record, TEXT("Tactical reload did not complete."));
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

	struct FPETacticalAutoRegister
	{
		FPETacticalAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FPETacticalFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FPETacticalAutoRegister GRegisterPETactical;
}
