#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestLogSink.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("OpeningResources_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Opening Resources Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR AuditPackage[] = TEXT("/Game/Data/Missions/DA_Mission_TheAudit");
	constexpr TCHAR KnownNeuroRecastPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR TraumaPackage[] = TEXT("/Game/Data/Items/DA_Item_TraumaStabilizer");
	constexpr TCHAR PistolAmmoPath[] = TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo");
	constexpr TCHAR TraumaPath[] = TEXT("/Game/Data/Items/DA_Item_TraumaStabilizer.DA_Item_TraumaStabilizer");
	constexpr TCHAR AmmoPickupLabel[] = TEXT("Pickup_Block3_PistolAmmo");
	constexpr TCHAR TraumaPickupLabel[] = TEXT("Pickup_Block3_TraumaStabilizer");
	constexpr TCHAR AdminHostLabel[] = TEXT("Host_Admin_SecurityOfficer");
	constexpr TCHAR RwPickupLabel[] = TEXT("Pickup_ResearchWingKeycard");
	constexpr TCHAR SecurityLabel[] = TEXT("Admin_Terminal_Security");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR TerminalBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal");
	constexpr TCHAR DoorBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	constexpr TCHAR HologramBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram");
	constexpr float SetupDistance = 100.0f;
	const FVector AmmoLocation(2560.0f, -340.0f, 80.0f);
	const FVector TraumaLocation(2760.0f, -300.0f, 80.0f);
	const FVector HostStaging(2820.0f, -600.0f, 100.0f);
	const FVector RwLocation(2580.0f, -560.0f, 80.0f);

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

	bool IsAuthorizedDirty(const FString& Name)
	{
		return Name.Equals(KnownNeuroRecastPackage, ESearchCase::IgnoreCase)
			|| Name.Equals(AdminPackage, ESearchCase::IgnoreCase)
			|| Name.Equals(TraumaPackage, ESearchCase::IgnoreCase);
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	class FOpeningResourcesFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Fresh;
			WaitSeconds = 0.0f;
			PostInteractWait = 0.0f;
			bAnyAssertFailed = false;
			bEpitopeDirtyBefore = false;
			bAuditDirtyBefore = false;
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
			Fresh,
			AmmoSetup,
			AmmoFocus,
			AmmoInteract,
			TraumaSetup,
			TraumaFocus,
			TraumaInteract,
			Use,
			SecuritySetup,
			SecurityFocus,
			SecurityInteract,
			World,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Fresh;
		float WaitSeconds = 0.0f;
		float PostInteractWait = 0.0f;
		bool bAnyAssertFailed = false;
		bool bEpitopeDirtyBefore = false;
		bool bAuditDirtyBefore = false;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AActor> AmmoPickup;
		TWeakObjectPtr<AActor> TraumaPickup;
		TWeakObjectPtr<AActor> SecurityTerminal;

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

		int32 ReserveOf(AProjectOrganoidCharacter* Character) const
		{
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			return (Weapon && Inventory) ? Inventory->CountAmmoOfType(Weapon->AmmoType) : -1;
		}

		UProjectOrganoidItemData* LoadTrauma() const
		{
			return LoadObject<UProjectOrganoidItemData>(nullptr, TraumaPath);
		}

		UProjectOrganoidItemData* LoadAmmo() const
		{
			return LoadObject<UProjectOrganoidItemData>(nullptr, PistolAmmoPath);
		}

		int32 CountHostsInPackage(UWorld* World, const TCHAR* Package) const
		{
			int32 Count = 0;
			if (!World)
			{
				return Count;
			}
			for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
			{
				if (OrganoidPlaytestActions::ActorPackage(*It).Contains(Package))
				{
					++Count;
				}
			}
			return Count;
		}

		AProjectOrganoidHostBase* FindAdminHost(UWorld* World) const
		{
			return Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, AdminHostLabel));
		}

		int32 CountSequenceActors(UWorld* World) const
		{
			int32 Count = 0;
			if (!World)
			{
				return Count;
			}
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (UClass* Class = It->GetClass())
				{
					if (Class->GetName().Contains(TEXT("LevelSequence")))
					{
						++Count;
					}
				}
			}
			return Count;
		}

		FString CombinedLogText(UProjectOrganoidLogComponent* Log) const
		{
			if (!Log)
			{
				return FString();
			}
			FString Combined;
			for (const FProjectOrganoidLogEntry& Entry : Log->GetAllEntries())
			{
				Combined += Entry.Title.ToString();
				Combined += TEXT("\n");
				Combined += Entry.Body.ToString();
				Combined += TEXT("\n");
			}
			return Combined;
		}

		UProjectOrganoidObjectiveSubsystem* GetObjectives(AProjectOrganoidCharacter* Character) const
		{
			UGameInstance* GI = Character ? Character->GetGameInstance() : nullptr;
			return GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
		}

		UProjectOrganoidHUDWidget* FindHud(UWorld* World) const
		{
			for (TObjectIterator<UProjectOrganoidHUDWidget> It; It; ++It)
			{
				UProjectOrganoidHUDWidget* Hud = *It;
				if (Hud && Hud->GetWorld() == World)
				{
					return Hud;
				}
			}
			return nullptr;
		}

		bool SetupFaceActor(APawn* Pawn, AActor* Target)
		{
			if (!Pawn || !Target)
			{
				return false;
			}
			float CapsuleZ = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleZ = Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
				}
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->GravityScale = 1.0f;
					Move->SetMovementMode(MOVE_Walking);
				}
			}
			OrganoidPlaytestActions::TeleportNear(Pawn, Target->GetActorLocation(), SetupDistance, CapsuleZ);
			OrganoidPlaytestActions::FaceActor(Pawn, Target);
			return true;
		}

		bool AssertOpeningObjectivesUnchanged(FOrganoidPlaytestRecord& Record, AProjectOrganoidCharacter* Character, const TCHAR* Phase)
		{
			UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectives(Character);
			FProjectOrganoidObjective Reception;
			FProjectOrganoidObjective Security;
			FProjectOrganoidObjective FindAmmo;
			FProjectOrganoidObjective FindMed;
			FProjectOrganoidObjective OpenInv;
			const bool bRec = Objectives && Objectives->GetObjective(TEXT("Obj_ReceptionCheckIn"), Reception);
			const bool bSec = Objectives && Objectives->GetObjective(TEXT("Obj_SecurityStatus"), Security);
			const bool bFindAmmo = Objectives && Objectives->GetObjective(TEXT("Find ammunition"), FindAmmo);
			const bool bFindMed = Objectives && Objectives->GetObjective(TEXT("Find medicine"), FindMed);
			const bool bOpenInv = Objectives && Objectives->GetObjective(TEXT("Open inventory"), OpenInv);
			const FString MissionId = Objectives ? Objectives->GetActiveMissionId().ToString() : FString();
			AssertTrue(Record, FString::Printf(TEXT("%s.mission"), Phase),
				MissionId == TEXT("Mission_OpeningFoundation"),
				TEXT("Mission_OpeningFoundation"), MissionId, TEXT("objectives"));
			AssertTrue(Record, FString::Printf(TEXT("%s.obj_reception"), Phase),
				bRec, TEXT("present"), bRec ? TEXT("present") : TEXT("missing"), TEXT("objectives"));
			AssertTrue(Record, FString::Printf(TEXT("%s.obj_security"), Phase),
				bSec, TEXT("present"), bSec ? TEXT("present") : TEXT("missing"), TEXT("objectives"));
			AssertTrue(Record, FString::Printf(TEXT("%s.no_ammo_objective"), Phase),
				!bFindAmmo, TEXT("absent"), bFindAmmo ? TEXT("present") : TEXT("absent"), TEXT("objectives"));
			AssertTrue(Record, FString::Printf(TEXT("%s.no_medicine_objective"), Phase),
				!bFindMed, TEXT("absent"), bFindMed ? TEXT("present") : TEXT("absent"), TEXT("objectives"));
			AssertTrue(Record, FString::Printf(TEXT("%s.no_inventory_objective"), Phase),
				!bOpenInv, TEXT("absent"), bOpenInv ? TEXT("present") : TEXT("absent"), TEXT("objectives"));
			return !bAnyAssertFailed;
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AuditPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope or DA_Mission_TheAudit is dirty. Refusing to start."));
				return;
			}

			bEpitopeDirtyBefore = PackageIsDirty(MapPackage);
			bAuditDirtyBefore = PackageIsDirty(AuditPackage);
			CollectDirtyPackageNames(DirtyBefore);
			for (const FString& Name : DirtyBefore)
			{
				if (!IsAuthorizedDirty(Name))
				{
					Owner.CompleteActive(
						EOrganoidPlaytestState::Blocked,
						FString::Printf(TEXT("Unauthorized dirty package before start: %s"), *Name));
					return;
				}
			}
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
			bool bReady = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bReady = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling || WaitSeconds > 10.0f;
				}
			}
			if (World && Character && bReady)
			{
				Player = Character;
				WaitSeconds = 0.0f;
				Proof = EProof::Fresh;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE player."));
			}
			(void)Record;
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			if (!Character)
			{
				Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
				Player = Character;
			}
			if (!World || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Nathan during OpeningResources proof."));
				return;
			}

			switch (Proof)
			{
			case EProof::Fresh:
				TickFresh(Owner, Record, World, Character);
				break;
			case EProof::AmmoSetup:
				TickAmmoSetup(Owner, Record, World, Character);
				break;
			case EProof::AmmoFocus:
				TickPickupFocus(Owner, Record, Character, AmmoPickup.Get(), TEXT("ammo"), EProof::AmmoInteract, DeltaTime);
				break;
			case EProof::AmmoInteract:
				TickAmmoInteract(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::TraumaSetup:
				TickTraumaSetup(Owner, Record, World, Character);
				break;
			case EProof::TraumaFocus:
				TickPickupFocus(Owner, Record, Character, TraumaPickup.Get(), TEXT("trauma"), EProof::TraumaInteract, DeltaTime);
				break;
			case EProof::TraumaInteract:
				TickTraumaInteract(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::Use:
				TickUse(Owner, Record, World, Character);
				break;
			case EProof::SecuritySetup:
				TickSecuritySetup(Owner, Record, World, Character);
				break;
			case EProof::SecurityFocus:
				TickPickupFocus(Owner, Record, Character, SecurityTerminal.Get(), TEXT("security"), EProof::SecurityInteract, DeltaTime);
				break;
			case EProof::SecurityInteract:
				TickSecurityInteract(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::World:
				TickWorld(Owner, Record, World, Character);
				break;
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}
		}

		void TickFresh(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			UProjectOrganoidItemData* Trauma = LoadTrauma();
			AssertTrue(Record, TEXT("fresh.health"),
				FMath::IsNearlyEqual(Character->GetHealth(), 100.0f) && FMath::IsNearlyEqual(Character->GetMaxHealth(), 100.0f),
				TEXT("100/100"),
				FString::Printf(TEXT("%.0f/%.0f"), Character->GetHealth(), Character->GetMaxHealth()),
				TEXT("player"));
			AssertTrue(Record, TEXT("fresh.magazine"),
				Weapon && Weapon->GetCurrentMagazine() == 12 && Weapon->GetMagazineCapacity() == 12,
				TEXT("12/12"),
				Weapon ? FString::Printf(TEXT("%d/%d"), Weapon->GetCurrentMagazine(), Weapon->GetMagazineCapacity()) : TEXT("missing"),
				TEXT("pistol"));
			AssertTrue(Record, TEXT("fresh.reserve"),
				ReserveOf(Character) == 0, TEXT("0"), FString::FromInt(ReserveOf(Character)), TEXT("pistol"));
			AssertTrue(Record, TEXT("fresh.trauma_count"),
				Inventory && Trauma && Inventory->CountItem(Trauma) == 0,
				TEXT("0"),
				Inventory && Trauma ? FString::FromInt(Inventory->CountItem(Trauma)) : TEXT("missing"),
				TEXT("inventory"));
			const int32 AdminHostCount = CountHostsInPackage(World, TEXT("SL_Epitope_Admin"));
			AProjectOrganoidHostBase* AdminHost = FindAdminHost(World);
			AssertTrue(Record, TEXT("fresh.authorized_admin_host_count"),
				AdminHostCount == 1, TEXT("1"), FString::FromInt(AdminHostCount), TEXT("Admin"));
			AssertTrue(Record, TEXT("fresh.authorized_admin_host_identity"),
				AdminHost
					&& OrganoidPlaytestActions::ActorPackage(AdminHost).Contains(TEXT("SL_Epitope_Admin"))
					&& AdminHost->bRequiresEncounterActivation
					&& !AdminHost->bAllowPhaseShiftMutations
					&& !AdminHost->IsEncounterActivated()
					&& AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle,
				TEXT("one dormant authorized Host_Admin_SecurityOfficer"),
				AdminHost ? OrganoidPlaytestActions::ActorPackage(AdminHost) : TEXT("missing or duplicate label"),
				AdminHostLabel);
			AssertOpeningObjectivesUnchanged(Record, Character, TEXT("fresh"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::AmmoSetup;
			Owner.SetStage(TEXT("AmmoSetup"));
		}

		void TickAmmoSetup(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, AmmoPickupLabel));
			AmmoPickup = Pickup;
			UProjectOrganoidItemData* Ammo = LoadAmmo();
			AssertTrue(Record, TEXT("ammo.actor_exists"), Pickup != nullptr,
				AmmoPickupLabel, Pickup ? TEXT("found") : TEXT("missing"), AmmoPickupLabel);
			if (!Pickup)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			AssertTrue(Record, TEXT("ammo.item_asset"),
				Ammo && Pickup->ItemData == Ammo,
				PistolAmmoPath,
				Pickup->ItemData ? Pickup->ItemData->GetPathName() : TEXT("null"),
				AmmoPickupLabel);
			AssertTrue(Record, TEXT("ammo.quantity"),
				Pickup->Quantity == 8, TEXT("8"), FString::FromInt(Pickup->Quantity), AmmoPickupLabel);
			AssertTrue(Record, TEXT("ammo.location"),
				FVector::Dist(Pickup->GetActorLocation(), AmmoLocation) <= 5.0f,
				TEXT("(2560,-340,80)"), Pickup->GetActorLocation().ToCompactString(), AmmoPickupLabel);
			AssertTrue(Record, TEXT("ammo.reachable"),
				Pickup->CanInteract(Character), TEXT("true"), BoolText(Pickup->CanInteract(Character)), AmmoPickupLabel);
			SetupFaceActor(Character, Pickup);
			WaitSeconds = 0.0f;
			Proof = EProof::AmmoFocus;
			Owner.SetStage(TEXT("AmmoFocus"));
		}

		void TickPickupFocus(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character,
			AActor* Target,
			const TCHAR* Prefix,
			EProof Next,
			float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (!Target)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("Lost %s target while waiting for focus."), Prefix));
				return;
			}
			OrganoidPlaytestActions::FaceActor(Character, Target);
			UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
			AActor* Focused = OrganoidPlaytestActions::GetFocusedInteractable(Interaction);
			const float Dist = OrganoidPlaytestActions::DistanceTo(Character, Target);
			const bool bReady = Dist <= 170.0f && Focused == Target;
			if (bReady || WaitSeconds > 2.0f)
			{
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
				Proof = Next;
				Owner.SetStage(FString::Printf(TEXT("%sInteract"), Prefix));
				return;
			}
			if (WaitSeconds > 4.0f)
			{
				AssertTrue(Record, FString::Printf(TEXT("%s.focus"), Prefix), false,
					TEXT("focused"), Focused ? OrganoidPlaytestActions::ActorLabel(Focused) : TEXT("<none>"), Prefix);
				FailAndStop(Owner, Record, FString::Printf(TEXT("Failed to acquire %s focus."), Prefix));
			}
		}

		void TickAmmoInteract(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			float DeltaTime)
		{
			if (PostInteractWait <= 0.0f)
			{
				UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
				const bool bOk = OrganoidPlaytestActions::TryInteract(Interaction);
				AssertTrue(Record, TEXT("ammo.TryInteract"), bOk, TEXT("true"), BoolText(bOk), AmmoPickupLabel);
				if (!bOk)
				{
					FailAndStop(Owner, Record, TEXT("Ammo TryInteract failed."));
					return;
				}
				PostInteractWait = 0.001f;
				return;
			}

			PostInteractWait += DeltaTime;
			if (PostInteractWait < 0.1f)
			{
				return;
			}

			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			UProjectOrganoidItemData* Ammo = LoadAmmo();
			AssertTrue(Record, TEXT("ammo.magazine_unchanged"),
				Weapon && Weapon->GetCurrentMagazine() == 12,
				TEXT("12"), Weapon ? FString::FromInt(Weapon->GetCurrentMagazine()) : TEXT("missing"), TEXT("pistol"));
			AssertTrue(Record, TEXT("ammo.reserve_8"),
				ReserveOf(Character) == 8, TEXT("8"), FString::FromInt(ReserveOf(Character)), TEXT("pistol"));
			AssertTrue(Record, TEXT("ammo.inventory_stack"),
				Inventory && Ammo && Inventory->CountItem(Ammo) == 8,
				TEXT("8"),
				Inventory && Ammo ? FString::FromInt(Inventory->CountItem(Ammo)) : TEXT("missing"),
				TEXT("inventory"));
			AssertTrue(Record, TEXT("ammo.pickup_consumed"),
				OrganoidPlaytestActions::FindUniqueByLabel(World, AmmoPickupLabel) == nullptr,
				TEXT("removed"),
				OrganoidPlaytestActions::FindUniqueByLabel(World, AmmoPickupLabel) ? TEXT("present") : TEXT("removed"),
				AmmoPickupLabel);
			AssertTrue(Record, TEXT("ammo.feedback"),
				Character->GetResourceFeedbackCount() >= 1
					&& (Character->GetLastResourceFeedback().Contains(TEXT("reserve"), ESearchCase::IgnoreCase)
						|| Character->GetLastResourceFeedback().Contains(TEXT("Pistol"), ESearchCase::IgnoreCase)),
				TEXT("ammo reserve feedback"),
				Character->GetLastResourceFeedback(),
				TEXT("feedback"));
			if (UProjectOrganoidHUDWidget* Hud = FindHud(World))
			{
				AssertTrue(Record, TEXT("ammo.hud_reserve"),
					Hud->GetDisplayedAmmoText().ToString().Contains(TEXT("Reserve 8")),
					TEXT("Reserve 8"), Hud->GetDisplayedAmmoText().ToString(), TEXT("hud"));
				AssertTrue(Record, TEXT("ammo.hud_notification"),
					!Hud->GetLastResourceNotification().IsEmpty(),
					TEXT("present"), Hud->GetLastResourceNotification().ToString(), TEXT("hud"));
			}
			AssertTrue(Record, TEXT("ammo.no_rw_event"),
				!Character->GetLastResourceFeedback().Contains(TEXT("Research Wing"), ESearchCase::IgnoreCase),
				TEXT("absent"), Character->GetLastResourceFeedback(), TEXT("rw"));
			AProjectOrganoidItemPickup* Rw = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, RwPickupLabel));
			AssertTrue(Record, TEXT("ammo.rw_still_held"),
				Rw && !Rw->CanInteract(Character),
				TEXT("false"), Rw ? BoolText(Rw->CanInteract(Character)) : TEXT("missing"), RwPickupLabel);
			AssertOpeningObjectivesUnchanged(Record, Character, TEXT("ammo"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			PostInteractWait = 0.0f;
			Proof = EProof::TraumaSetup;
			Owner.SetStage(TEXT("TraumaSetup"));
		}

		void TickTraumaSetup(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, TraumaPickupLabel));
			TraumaPickup = Pickup;
			UProjectOrganoidItemData* Trauma = LoadTrauma();
			AssertTrue(Record, TEXT("trauma.actor_exists"), Pickup != nullptr,
				TraumaPickupLabel, Pickup ? TEXT("found") : TEXT("missing"), TraumaPickupLabel);
			if (!Pickup)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			AssertTrue(Record, TEXT("trauma.data_asset"),
				Trauma && Pickup->ItemData == Trauma,
				TraumaPath,
				Pickup->ItemData ? Pickup->ItemData->GetPathName() : TEXT("null"),
				TraumaPickupLabel);
			AssertTrue(Record, TEXT("trauma.quantity"),
				Pickup->Quantity == 1, TEXT("1"), FString::FromInt(Pickup->Quantity), TraumaPickupLabel);
			AssertTrue(Record, TEXT("trauma.location"),
				FVector::Dist(Pickup->GetActorLocation(), TraumaLocation) <= 5.0f,
				TEXT("(2760,-300,80)"), Pickup->GetActorLocation().ToCompactString(), TraumaPickupLabel);
			AssertTrue(Record, TEXT("trauma.reachable"),
				Pickup->CanInteract(Character), TEXT("true"), BoolText(Pickup->CanInteract(Character)), TraumaPickupLabel);
			SetupFaceActor(Character, Pickup);
			WaitSeconds = 0.0f;
			Proof = EProof::TraumaFocus;
			Owner.SetStage(TEXT("TraumaFocus"));
		}

		void TickTraumaInteract(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			float DeltaTime)
		{
			if (PostInteractWait <= 0.0f)
			{
				UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
				const bool bOk = OrganoidPlaytestActions::TryInteract(Interaction);
				AssertTrue(Record, TEXT("trauma.TryInteract"), bOk, TEXT("true"), BoolText(bOk), TraumaPickupLabel);
				if (!bOk)
				{
					FailAndStop(Owner, Record, TEXT("Trauma TryInteract failed."));
					return;
				}
				PostInteractWait = 0.001f;
				return;
			}

			PostInteractWait += DeltaTime;
			if (PostInteractWait < 0.1f)
			{
				return;
			}

			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			UProjectOrganoidItemData* Trauma = LoadTrauma();
			AssertTrue(Record, TEXT("trauma.inventory_one"),
				Inventory && Trauma && Inventory->CountItem(Trauma) == 1,
				TEXT("1"),
				Inventory && Trauma ? FString::FromInt(Inventory->CountItem(Trauma)) : TEXT("missing"),
				TEXT("inventory"));
			AssertTrue(Record, TEXT("trauma.pickup_consumed"),
				OrganoidPlaytestActions::FindUniqueByLabel(World, TraumaPickupLabel) == nullptr,
				TEXT("removed"),
				OrganoidPlaytestActions::FindUniqueByLabel(World, TraumaPickupLabel) ? TEXT("present") : TEXT("removed"),
				TraumaPickupLabel);
			AssertTrue(Record, TEXT("trauma.feedback"),
				Character->GetLastResourceFeedback().Contains(TEXT("Trauma Stabilizer"), ESearchCase::IgnoreCase),
				TEXT("Trauma Stabilizer acquired"),
				Character->GetLastResourceFeedback(),
				TEXT("feedback"));
			if (UProjectOrganoidHUDWidget* Hud = FindHud(World))
			{
				AssertTrue(Record, TEXT("trauma.hud_notification"),
					Hud->GetLastResourceNotification().ToString().Contains(TEXT("Trauma"), ESearchCase::IgnoreCase),
					TEXT("Trauma"), Hud->GetLastResourceNotification().ToString(), TEXT("hud"));
			}
			AssertTrue(Record, TEXT("trauma.first_hint_already_shown"),
				Character->HasShownFirstResourceHint(),
				TEXT("true"),
				BoolText(Character->HasShownFirstResourceHint()),
				TEXT("feedback"));
			AProjectOrganoidItemPickup* Rw = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, RwPickupLabel));
			AssertTrue(Record, TEXT("trauma.rw_still_held"),
				Rw && !Rw->CanInteract(Character),
				TEXT("false"), Rw ? BoolText(Rw->CanInteract(Character)) : TEXT("missing"), RwPickupLabel);
			AssertOpeningObjectivesUnchanged(Record, Character, TEXT("trauma"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			PostInteractWait = 0.0f;
			Proof = EProof::Use;
			Owner.SetStage(TEXT("Use"));
		}

		void TickUse(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* /*World*/,
			AProjectOrganoidCharacter* Character)
		{
			UProjectOrganoidInventoryComponent* Inventory = GetInventory(Character);
			UProjectOrganoidItemData* Trauma = LoadTrauma();
			if (!Inventory || !Trauma)
			{
				FailAndStop(Owner, Record, TEXT("Trauma item or inventory missing for use fixture."));
				return;
			}

			AssertTrue(Record, TEXT("use.full_health_start"),
				FMath::IsNearlyEqual(Character->GetHealth(), 100.0f),
				TEXT("100"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
			const bool bFullRefused = !Character->TryUseConsumable(Trauma);
			AssertTrue(Record, TEXT("use.full_refused"), bFullRefused, TEXT("false"), BoolText(!bFullRefused), TEXT("use"));
			AssertTrue(Record, TEXT("use.full_not_consumed"),
				Inventory->CountItem(Trauma) == 1, TEXT("1"), FString::FromInt(Inventory->CountItem(Trauma)), TEXT("inventory"));
			AssertTrue(Record, TEXT("use.full_health_unchanged"),
				FMath::IsNearlyEqual(Character->GetHealth(), 100.0f),
				TEXT("100"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));

			Character->ApplyHealthDelta(-35.0f);
			AssertTrue(Record, TEXT("use.damaged_65"),
				FMath::IsNearlyEqual(Character->GetHealth(), 65.0f),
				TEXT("65"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
			const bool bHeal65 = Character->TryUseFirstHealingConsumable();
			AssertTrue(Record, TEXT("use.65_success"), bHeal65, TEXT("true"), BoolText(bHeal65), TEXT("use"));
			AssertTrue(Record, TEXT("use.65_to_100"),
				FMath::IsNearlyEqual(Character->GetHealth(), 100.0f),
				TEXT("100"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
			AssertTrue(Record, TEXT("use.65_consumed_one"),
				Inventory->CountItem(Trauma) == 0, TEXT("0"), FString::FromInt(Inventory->CountItem(Trauma)), TEXT("inventory"));

			FGuid InjectedId;
			AssertTrue(Record, TEXT("use.reset_add"),
				Inventory->TryAddItem(Trauma, InjectedId, 1), TEXT("true"), TEXT("added"), TEXT("inventory"));
			Character->ApplyHealthDelta(-10.0f);
			AssertTrue(Record, TEXT("use.damaged_90"),
				FMath::IsNearlyEqual(Character->GetHealth(), 90.0f),
				TEXT("90"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
			const bool bHeal90 = Character->TryUseConsumable(Trauma);
			AssertTrue(Record, TEXT("use.90_success"), bHeal90, TEXT("true"), BoolText(bHeal90), TEXT("use"));
			AssertTrue(Record, TEXT("use.90_to_100"),
				FMath::IsNearlyEqual(Character->GetHealth(), 100.0f),
				TEXT("100"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
			AssertTrue(Record, TEXT("use.90_consumed_one"),
				Inventory->CountItem(Trauma) == 0, TEXT("0"), FString::FromInt(Inventory->CountItem(Trauma)), TEXT("inventory"));
			AssertTrue(Record, TEXT("use.player_path_h"),
				Character->TryUseFirstHealingConsumable() == false && Inventory->CountItem(Trauma) == 0,
				TEXT("empty refuse"), TEXT("ok"), TEXT("input"));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::SecuritySetup;
			Owner.SetStage(TEXT("SecuritySetup"));
		}

		void TickSecuritySetup(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, SecurityLabel);
			SecurityTerminal = Terminal;
			AssertTrue(Record, TEXT("security.present"), Terminal != nullptr,
				SecurityLabel, Terminal ? TEXT("found") : TEXT("missing"), SecurityLabel);
			if (!Terminal)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			const FOrganoidPlaytestPropValue Id = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("TerminalID"));
			const FOrganoidPlaytestPropValue Title = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("Title"));
			AssertTrue(Record, TEXT("security.TerminalID"), Id.Text == TEXT("Terminal_AdminSecurity"),
				TEXT("Terminal_AdminSecurity"), Id.Text, SecurityLabel);
			AssertTrue(Record, TEXT("security.Title"), Title.Text == TEXT("Security Terminal"),
				TEXT("Security Terminal"), Title.Text, SecurityLabel);
			SetupFaceActor(Character, Terminal);
			WaitSeconds = 0.0f;
			Proof = EProof::SecurityFocus;
			Owner.SetStage(TEXT("SecurityFocus"));
		}

		void TickSecurityInteract(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* /*World*/,
			AProjectOrganoidCharacter* Character,
			float DeltaTime)
		{
			if (PostInteractWait <= 0.0f)
			{
				UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
				const bool bOk = OrganoidPlaytestActions::TryInteract(Interaction);
				AssertTrue(Record, TEXT("security.TryInteract"), bOk, TEXT("true"), BoolText(bOk), SecurityLabel);
				if (!bOk)
				{
					FailAndStop(Owner, Record, TEXT("Security TryInteract failed."));
					return;
				}
				PostInteractWait = 0.001f;
				return;
			}

			PostInteractWait += DeltaTime;
			if (PostInteractWait < 0.15f)
			{
				return;
			}

			FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink();
			const bool bScreen = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal Screen"));
			const bool bSecurity = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal OnSecurityActivated"));
			AssertTrue(Record, TEXT("security.print_screen"), bScreen, TEXT("AdminTerminal Screen"),
				bScreen ? TEXT("present") : TEXT("missing"), SecurityLabel);
			AssertTrue(Record, TEXT("security.print_route"), bSecurity, TEXT("AdminTerminal OnSecurityActivated"),
				bSecurity ? TEXT("present") : TEXT("missing"), SecurityLabel);
			const FString Logs = CombinedLogText(Character->GetLogComponent());
			AssertTrue(Record, TEXT("security.evidence_watch"),
				Logs.Contains(TEXT("PUBLIC ACCESS WATCH")), TEXT("PUBLIC ACCESS WATCH"), Logs, SecurityLabel);
			AssertTrue(Record, TEXT("security.evidence_interrupted"),
				Logs.Contains(TEXT("Intake procedure: interrupted")), TEXT("interrupted"), Logs, SecurityLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			PostInteractWait = 0.0f;
			Proof = EProof::World;
			Owner.SetStage(TEXT("World"));
		}

		void TickWorld(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			const int32 AdminHostCount = CountHostsInPackage(World, TEXT("SL_Epitope_Admin"));
			AProjectOrganoidHostBase* AdminHost = FindAdminHost(World);
			AssertTrue(Record, TEXT("admin.authorized_opening_host_count"),
				AdminHostCount == 1, TEXT("1"), FString::FromInt(AdminHostCount), TEXT("Admin"));
			AssertTrue(Record, TEXT("admin.resource_path_keeps_host_dormant"),
				AdminHost
					&& AdminHost->bRequiresEncounterActivation
					&& !AdminHost->bAllowPhaseShiftMutations
					&& !AdminHost->IsEncounterActivated()
					&& AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle,
				TEXT("authorized Host remains dormant Idle through resources/security"),
				AdminHost
					? FString::Printf(TEXT("activated=%s state=%s"),
						AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"),
						*UEnum::GetValueAsString(AdminHost->GetCombatState()))
					: TEXT("missing or duplicate label"),
				AdminHostLabel);
			AssertTrue(Record, TEXT("no_cinematic_sequencer"),
				CountSequenceActors(World) == 0,
				TEXT("0"), FString::FromInt(CountSequenceActors(World)), TEXT("world"));

			AProjectOrganoidItemPickup* Rw = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, RwPickupLabel));
			AssertTrue(Record, TEXT("rw.held"),
				Rw && FVector::Dist(Rw->GetActorLocation(), RwLocation) <= 5.0f && !Rw->CanInteract(Character),
				TEXT("held at (2580,-560,80)"),
				Rw ? Rw->GetActorLocation().ToCompactString() : TEXT("missing"),
				RwPickupLabel);

			int32 StagingLoot = 0;
			for (TActorIterator<AProjectOrganoidItemPickup> It(World); It; ++It)
			{
				if (FVector::Dist2D(It->GetActorLocation(), HostStaging) < 80.0f)
				{
					++StagingLoot;
				}
			}
			AssertTrue(Record, TEXT("host_staging.clear_of_loot"),
				StagingLoot == 0, TEXT("0"), FString::FromInt(StagingLoot), TEXT("staging"));

			AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel);
			AssertTrue(Record, TEXT("hologram.present"), Hologram != nullptr,
				HologramLabel, Hologram ? TEXT("found") : TEXT("missing"), HologramLabel);
			bool bAdmin = false;
			const bool bReadAdmin = Hologram && [&]()
			{
				const FOrganoidPlaytestPropValue Value = OrganoidPlaytestActions::ReadProperty(Hologram, TEXT("bAdminOnline"));
				if (Value.bFound && Value.bHasBool)
				{
					bAdmin = Value.bBool;
					return true;
				}
				return false;
			}();
			AssertTrue(Record, TEXT("hologram.admin_online"), bReadAdmin && bAdmin, TEXT("true"), BoolText(bAdmin), HologramLabel);

			AProjectOrganoidCheckpoint* AdminCheckpoint = Cast<AProjectOrganoidCheckpoint>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, TEXT("Checkpoint_ReceptionAtrium")));
			AssertTrue(Record, TEXT("checkpoint.Checkpoint_ReceptionAtrium.present"),
				AdminCheckpoint && FVector::Dist(AdminCheckpoint->GetActorLocation(), FVector(-1535.0f, 0.0f, 60.0f)) <= 5.0f,
				TEXT("(-1535,0,60)"),
				AdminCheckpoint ? AdminCheckpoint->GetActorLocation().ToCompactString() : TEXT("missing"),
				TEXT("Checkpoint_ReceptionAtrium"));
			AssertTrue(Record, TEXT("checkpoint.Checkpoint_ReceptionAtrium.floor_percent"),
				AdminCheckpoint && FMath::IsNearlyEqual(AdminCheckpoint->HealthStabilizationFloorPercent, 0.25f),
				TEXT("0.25"),
				AdminCheckpoint ? FString::SanitizeFloat(AdminCheckpoint->HealthStabilizationFloorPercent) : TEXT("missing"),
				TEXT("Checkpoint_ReceptionAtrium"));

			AssertOpeningObjectivesUnchanged(Record, Character, TEXT("world"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Done;
			Stage = EStage::EndPie;
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			AssertTrue(Record, TEXT("dirty.lvl_epitope_unchanged"),
				PackageIsDirty(MapPackage) == bEpitopeDirtyBefore,
				bEpitopeDirtyBefore ? TEXT("dirty") : TEXT("clean"),
				PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("Lvl_Epitope"));
			AssertTrue(Record, TEXT("dirty.theaudit_unchanged"),
				PackageIsDirty(AuditPackage) == bAuditDirtyBefore,
				bAuditDirtyBefore ? TEXT("dirty") : TEXT("clean"),
				PackageIsDirty(AuditPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("DA_Mission_TheAudit"));
			AssertTrue(Record, TEXT("dirty.terminal_bp_clean"),
				!PackageIsDirty(TerminalBpPackage), TEXT("clean"),
				PackageIsDirty(TerminalBpPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("BP_AdminTerminal"));
			AssertTrue(Record, TEXT("dirty.access_door_bp_clean"),
				!PackageIsDirty(DoorBpPackage), TEXT("clean"),
				PackageIsDirty(DoorBpPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("BP_AdminAccessDoor"));
			AssertTrue(Record, TEXT("dirty.hologram_bp_clean"),
				!PackageIsDirty(HologramBpPackage), TEXT("clean"),
				PackageIsDirty(HologramBpPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("BP_AdminFacilityHologram"));

			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			TArray<FString> UnauthorizedNew;
			for (const FString& Name : DirtyAfter)
			{
				if (!DirtyBefore.Contains(Name) && !IsAuthorizedDirty(Name))
				{
					UnauthorizedNew.Add(Name);
				}
			}
			AssertTrue(Record, TEXT("durable.no_unauthorized_new_dirty"),
				UnauthorizedNew.Num() == 0, TEXT("0"),
				FString::FromInt(UnauthorizedNew.Num()),
				UnauthorizedNew.Num() > 0 ? FString::Join(UnauthorizedNew, TEXT(",")) : TEXT(""));
			Record.AddActor(TEXT("dirty_before"), FString::Join(DirtyBefore, TEXT(",")));
			Record.AddActor(TEXT("dirty_after"), FString::Join(DirtyAfter, TEXT(",")));
			AssertTrue(Record, TEXT("playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""));

			UWorld* EditorWorld = (GEditor && GEditor->PlayWorld == nullptr)
				? GEditor->GetEditorWorldContext().World()
				: (GEditor ? GEditor->GetEditorWorldContext().World() : nullptr);
			const TArray<TPair<const TCHAR*, FVector>> Checkpoints = {
				{TEXT("Checkpoint_ReceptionAtrium"), FVector(-1535.0f, 0.0f, 60.0f)},
				{TEXT("Checkpoint_NeuroAirlock"), FVector(1950.0f, 0.0f, -1140.0f)},
				{TEXT("Checkpoint_FreightAirlock"), FVector(1950.0f, 0.0f, -2340.0f)},
				{TEXT("Checkpoint_InterfaceChamber"), FVector(-2425.0f, -1650.0f, -3540.0f)},
				{TEXT("Checkpoint_BasinRim"), FVector(25.0f, 0.0f, -4740.0f)},
			};
			for (const TPair<const TCHAR*, FVector>& Entry : Checkpoints)
			{
				AProjectOrganoidCheckpoint* Checkpoint = Cast<AProjectOrganoidCheckpoint>(
					OrganoidPlaytestActions::FindUniqueByLabel(EditorWorld, Entry.Key));
				AssertTrue(Record, FString::Printf(TEXT("durable.checkpoint.%s.present"), Entry.Key),
					Checkpoint && FVector::Dist(Checkpoint->GetActorLocation(), Entry.Value) <= 5.0f,
					Entry.Value.ToCompactString(),
					Checkpoint ? Checkpoint->GetActorLocation().ToCompactString() : TEXT("missing"),
					Entry.Key);
				AssertTrue(Record, FString::Printf(TEXT("durable.checkpoint.%s.floor_percent"), Entry.Key),
					Checkpoint && FMath::IsNearlyEqual(Checkpoint->HealthStabilizationFloorPercent, 0.25f),
					TEXT("0.25"),
					Checkpoint ? FString::SanitizeFloat(Checkpoint->HealthStabilizationFloorPercent) : TEXT("missing"),
					Entry.Key);
			}

			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FOpeningResourcesAutoRegister
	{
		FOpeningResourcesAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FOpeningResourcesFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FOpeningResourcesAutoRegister GRegisterOpeningResources;
}
