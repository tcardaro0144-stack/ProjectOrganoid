#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Kismet/GameplayStatics.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidDefaultWeapon.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponData.h"

namespace WeaponRosterFirearmFunctional
{
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");

	class FWeaponRosterFirearmCase : public IOrganoidPlaytestCase
	{
	public:
		explicit FWeaponRosterFirearmCase(bool bInCarbine)
			: bCarbine(bInCarbine)
		{
		}

		virtual FString GetTestId() const override
		{
			return bCarbine ? TEXT("PulseCarbine_Functional") : TEXT("BioStabilizerPistol_Functional");
		}
		virtual FString GetDisplayName() const override
		{
			return bCarbine ? TEXT("Pulse Carbine Functional") : TEXT("Bio-Stabilizer Pistol Functional");
		}
		virtual FString GetMapPackage() const override { return MapPackage; }
		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.f;
			bAnyAssertFailed = false;
			bProofDone = false;
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
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie:
				if (!Owner.RequestStartPie(MapPackage))
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
					return;
				}
				WaitSeconds = 0.f;
				Stage = EStage::WaitReady;
				break;
			case EStage::WaitReady:
				WaitSeconds += DeltaTime;
				if (Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(OrganoidPlaytestActions::GetPieWorld())))
				{
					Stage = EStage::Proof;
					WaitSeconds = 0.f;
				}
				else if (WaitSeconds > 30.f)
				{
					FailAndStop(Owner, *Record, TEXT("PIE character was not ready."));
				}
				break;
			case EStage::Proof:
				if (!bProofDone)
				{
					bProofDone = true;
					RunProof(Owner, *Record);
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

		FName WeaponId() const
		{
			return bCarbine ? FName(TEXT("Weapon_PulseCarbine")) : FName(TEXT("Weapon_BioStabilizerPistol"));
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidWeaponData* Data = UProjectOrganoidWeaponData::ResolveById(WeaponId());
			const float ExpectedDamage = bCarbine ? 16.f : 8.f;
			const int32 ExpectedMag = bCarbine ? 8 : 4;
			const float ExpectedRange = bCarbine ? 8000.f : 4500.f;
			const EProjectOrganoidAmmoType ExpectedAmmo = bCarbine ? EProjectOrganoidAmmoType::Rifle : EProjectOrganoidAmmoType::Pistol;
			const EProjectOrganoidWeaponRosterEffect ExpectedEffect = bCarbine ? EProjectOrganoidWeaponRosterEffect::WeakPoint : EProjectOrganoidWeaponRosterEffect::Stun;
			AssertTrue(Record, TEXT("asset.id"), Data && Data->WeaponId == WeaponId(), WeaponId().ToString(), Data ? Data->WeaponId.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.title"), Data && Data->DisplayName.ToString() == (bCarbine ? TEXT("Pulse Carbine") : TEXT("Bio-Stabilizer Pistol")), bCarbine ? TEXT("Pulse Carbine") : TEXT("Bio-Stabilizer Pistol"), Data ? Data->DisplayName.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.role"), Data && Data->Role.ToString() == (bCarbine ? TEXT("Mid-range, reliable") : TEXT("Sidearm, precise")), bCarbine ? TEXT("Mid-range, reliable") : TEXT("Sidearm, precise"), Data ? Data->Role.ToString() : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.damage"), Data && FMath::IsNearlyEqual(Data->Damage, ExpectedDamage), FString::SanitizeFloat(ExpectedDamage), Data ? FString::SanitizeFloat(Data->Damage) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.range"), Data && FMath::IsNearlyEqual(Data->HitscanRange, ExpectedRange), FString::SanitizeFloat(ExpectedRange), Data ? FString::SanitizeFloat(Data->HitscanRange) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.magazine"), Data && Data->MagazineCapacity == ExpectedMag && ExpectedMag < 12, FString::FromInt(ExpectedMag), Data ? FString::FromInt(Data->MagazineCapacity) : TEXT("missing"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.ammo_effect"), Data && Data->AmmoType == ExpectedAmmo && Data->Effect == ExpectedEffect, TEXT("match"), TEXT("checked"), TEXT("DA"));
			AssertTrue(Record, TEXT("asset.not_lytic"), Data && !Data->WeaponId.ToString().Contains(TEXT("Lytic")) && !Data->WeaponId.ToString().Contains(TEXT("Arc")) && !FMath::IsNearlyEqual(Data->Damage, 28.f), TEXT("distinct"), Data ? Data->WeaponId.ToString() : TEXT("missing"), TEXT("DA"));
			if (bAnyAssertFailed || !Data)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason.IsEmpty() ? TEXT("Weapon asset missing.") : Record.FailureReason);
				return;
			}
			Stage = EStage::StartPie;
		}

		void RunProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidWeaponComponent* Weapons = Character ? Character->GetWeaponComponent() : nullptr;
			if (!Weapons)
			{
				FailAndStop(Owner, Record, TEXT("Weapon component missing."));
				return;
			}
			const int32 ExpectedMag = bCarbine ? 8 : 4;
			const float ExpectedDamage = bCarbine ? 16.f : 8.f;
			AssertTrue(Record, TEXT("equip.locked"), !Weapons->EquipWeaponRoster(WeaponId()), TEXT("false"), TEXT("rejected"), WeaponId().ToString());
			AssertTrue(Record, TEXT("unlock"), Weapons->UnlockWeaponRoster(WeaponId()), TEXT("true"), TEXT("unlocked"), WeaponId().ToString());
			AssertTrue(Record, TEXT("equip"), Weapons->EquipWeaponRoster(WeaponId()), TEXT("true"), TEXT("equipped"), WeaponId().ToString());
			AProjectOrganoidWeapon* Weapon = Weapons->GetEquippedWeapon();
			AssertTrue(Record, TEXT("class"), Weapon && Weapon->IsA(AProjectOrganoidDefaultWeapon::StaticClass()), TEXT("DefaultWeapon"), Weapon ? Weapon->GetClass()->GetName() : TEXT("missing"), WeaponId().ToString());
			AssertTrue(Record, TEXT("live.damage"), Weapon && FMath::IsNearlyEqual(Weapon->Damage, ExpectedDamage), FString::SanitizeFloat(ExpectedDamage), Weapon ? FString::SanitizeFloat(Weapon->Damage) : TEXT("missing"), WeaponId().ToString());
			AssertTrue(Record, TEXT("live.magazine"), Weapon && Weapon->GetMagazineCapacity() == ExpectedMag && Weapon->GetCurrentMagazine() == ExpectedMag, FString::FromInt(ExpectedMag), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("missing"), WeaponId().ToString());
			const bool bFired = Weapon && Weapon->Fire();
			AssertTrue(Record, TEXT("fire.consumes"), Weapon && Weapon->GetCurrentMagazine() == ExpectedMag - 1, FString::FromInt(ExpectedMag - 1), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("missing"), WeaponId().ToString());
			(void)bFired;
			Weapon->SetCurrentMagazine(0);
			AssertTrue(Record, TEXT("fire.empty"), Weapon && !Weapon->CanFire() && !Weapon->Fire() && Weapon->GetCurrentMagazine() == 0, TEXT("rejected"), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("missing"), WeaponId().ToString());
			AssertTrue(Record, TEXT("effect"), Weapon && Weapon->GetRosterEffect() == (bCarbine ? EProjectOrganoidWeaponRosterEffect::WeakPoint : EProjectOrganoidWeaponRosterEffect::Stun), bCarbine ? TEXT("WeakPoint") : TEXT("Stun"), TEXT("checked"), WeaponId().ToString());
			Stage = EStage::EndPie;
		}

		bool bCarbine = false;
		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bProofDone = false;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Pistol;
			Pistol.TestId = TEXT("BioStabilizerPistol_Functional");
			Pistol.DisplayName = TEXT("Bio-Stabilizer Pistol Functional");
			Pistol.MapPackage = MapPackage;
			Pistol.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FWeaponRosterFirearmCase>(false); };
			FOrganoidPlaytestRegistry::Register(Pistol);

			FOrganoidPlaytestCatalogEntry Carbine;
			Carbine.TestId = TEXT("PulseCarbine_Functional");
			Carbine.DisplayName = TEXT("Pulse Carbine Functional");
			Carbine.MapPackage = MapPackage;
			Carbine.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FWeaponRosterFirearmCase>(true); };
			FOrganoidPlaytestRegistry::Register(Carbine);
		}
	};
	static FRegister RegisterWeaponRosterFirearms;
}
