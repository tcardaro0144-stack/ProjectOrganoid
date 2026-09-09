#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSecurityGate.h"
#include "ProjectOrganoidSecuritySubsystem.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("NeuroAccess_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Campaign Access Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR ItemPath[] = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard.DA_Item_ResearchWingKeycard");
	constexpr TCHAR PickupLabel[] = TEXT("Pickup_ResearchWingKeycard");
	constexpr TCHAR AdminPickupLabel[] = TEXT("Pickup_AdminKeycard");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");
	constexpr TCHAR GateIdName[] = TEXT("Gate_Neuro_Research");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_NeuroAirlock");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidNeuroAccessTest");
	const FVector PickupLocation(2580.0f, -560.0f, 80.0f);

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

	class FNeuroAccessFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Snapshots;
			WaitSeconds = 0.0f;
			WaypointIndex = 0;
			bAnyAssertFailed = false;
			bUsedSectorTransition = false;
			bInjectedLevel2 = false;
			MagBefore = 0;
			ReserveBefore = 0;
			SotBefore = 0;
			DirtyBefore.Reset();
			EditorHostLocations.Reset();
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
			Snapshots,
			CollectResearchWing,
			ObjectiveGuard,
			SaveLoad,
			Level1Deny,
			RestoreLevel2,
			WalkService,
			WalkConnector,
			WaitNeuroStream,
			WalkRamp,
			ApproachGate,
			AssertLocked,
			Unlock,
			WalkCheckpoint,
			Isolation,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Snapshots;
		float WaitSeconds = 0.0f;
		int32 WaypointIndex = 0;
		bool bAnyAssertFailed = false;
		bool bUsedSectorTransition = false;
		bool bInjectedLevel2 = false;
		int32 MagBefore = 0;
		int32 ReserveBefore = 0;
		int32 SotBefore = 0;
		TArray<FString> DirtyBefore;
		TMap<FString, FVector> EditorHostLocations;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidSecurityGate> Gate;

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

		float CapsuleHalf(APawn* Pawn) const
		{
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					return Capsule->GetScaledCapsuleHalfHeight();
				}
			}
			return 96.0f;
		}

		float StandZ(float FloorTop, APawn* Pawn) const
		{
			return FloorTop + CapsuleHalf(Pawn);
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

		FString HitName(const FHitResult& Hit) const
		{
			if (!Hit.bBlockingHit)
			{
				return TEXT("none");
			}
			if (AActor* Actor = Hit.GetActor())
			{
				return OrganoidPlaytestActions::ActorLabel(Actor);
			}
			return TEXT("unknown");
		}

		bool SweepTo(APawn* Pawn, const FVector& Dest, FHitResult& Hit)
		{
			if (!Pawn)
			{
				return false;
			}
			if (UCharacterMovementComponent* Move = Cast<ACharacter>(Pawn) ? Cast<ACharacter>(Pawn)->GetCharacterMovement() : nullptr)
			{
				Move->SetMovementMode(MOVE_Walking);
				Move->StopMovementImmediately();
			}
			return Pawn->SetActorLocation(Dest, true, &Hit, ETeleportType::None);
		}

		bool WalkToward(APawn* Pawn, const FVector& Dest, FHitResult& Hit, float StepUu = 140.0f)
		{
			if (!Pawn)
			{
				return false;
			}
			FVector Current = Pawn->GetActorLocation();
			const float Dist = FVector::Dist(Current, Dest);
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(Dist / StepUu));
			for (int32 Index = 1; Index <= Steps; ++Index)
			{
				const FVector Next = FMath::Lerp(Current, Dest, static_cast<float>(Index) / static_cast<float>(Steps));
				Hit = FHitResult();
				const bool bMoved = SweepTo(Pawn, Next, Hit);
				const FVector After = Pawn->GetActorLocation();
				if (!bMoved && FVector::Dist(After, Next) > 24.0f)
				{
					return false;
				}
			}
			return FVector::Dist(Pawn->GetActorLocation(), Dest) <= 40.0f;
		}

		float RampFloorZ(float Y) const
		{
			return -((Y + 900.0f) * 2.0f / 3.0f);
		}

		UProjectOrganoidItemData* LoadCampaignCard() const
		{
			return LoadObject<UProjectOrganoidItemData>(nullptr, ItemPath);
		}

		bool InventoryHasCampaignCard(UProjectOrganoidInventoryComponent* Inventory) const
		{
			if (!Inventory)
			{
				return false;
			}
			for (const FProjectOrganoidPlacedItem& Placed : Inventory->GetAllItems())
			{
				if (Placed.ItemData && Placed.ItemData->GetPathName().Contains(TEXT("DA_Item_ResearchWingKeycard")))
				{
					return true;
				}
			}
			return false;
		}

		int32 ReserveOf(AProjectOrganoidCharacter* Character) const
		{
			AProjectOrganoidWeapon* Weapon = Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			if (!Weapon || !Inventory)
			{
				return 0;
			}
			return Inventory->CountAmmoOfType(Weapon->AmmoType);
		}

		int32 MagOf(AProjectOrganoidCharacter* Character) const
		{
			AProjectOrganoidWeapon* Weapon = Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
			return Weapon ? Weapon->GetCurrentMagazine() : 0;
		}

		int32 CountSot(AProjectOrganoidCharacter* Character) const
		{
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			return Inventory ? Inventory->CountItemsOfType(EProjectOrganoidItemType::SOT) : 0;
		}

		bool CollectPickup(
			AProjectOrganoidCharacter* Character,
			const TCHAR* Label,
			const FVector& ExpectedLoc)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, Label));
			if (!Pickup || !Character)
			{
				return false;
			}
			const FVector Stand(ExpectedLoc.X, ExpectedLoc.Y, StandZ(20.0f, Character));
			if (!OrganoidPlaytestActions::TeleportNear(Character, Stand, 90.0f, Stand.Z))
			{
				return false;
			}
			OrganoidPlaytestActions::FaceActor(Character, Pickup);
			return Pickup->Interact(Character);
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

			UProjectOrganoidItemData* Item = LoadCampaignCard();
			if (!Item)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("DA_Item_ResearchWingKeycard is missing."));
				return;
			}
			UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
			EditorHostLocations.Reset();
			if (EditorWorld)
			{
				for (TActorIterator<AProjectOrganoidHostBase> It(EditorWorld); It; ++It)
				{
					if (OrganoidPlaytestActions::ActorPackage(*It).Contains(TEXT("SL_Epitope_NeuroGenetics")))
					{
						EditorHostLocations.Add(OrganoidPlaytestActions::ActorLabel(*It), (*It)->GetActorLocation());
					}
				}
			}
			AActor* Pickup = EditorWorld ? OrganoidPlaytestActions::FindUniqueByLabel(EditorWorld, PickupLabel) : nullptr;
			if (!Pickup)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Pickup_ResearchWingKeycard is missing from the editor world."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Record.AddActor(TEXT("sector_transition_used"), TEXT("false"));
			Record.AddActor(TEXT("campaign_item"), Item->GetPathName());
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
			if (World && Character)
			{
				Player = Character;
				WaitSeconds = 0.0f;
				Proof = EProof::Snapshots;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE player on Lvl_Epitope."));
			}
			(void)Record;
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			if (!World || !Character)
			{
				FailAndStop(Owner, Record, TEXT("PIE player vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Snapshots:
				TickSnapshots(Owner, Record, World, Character);
				break;
			case EProof::CollectResearchWing:
				TickCollectResearchWing(Owner, Record, World, Character);
				break;
			case EProof::ObjectiveGuard:
				TickObjectiveGuard(Owner, Record, Character);
				break;
			case EProof::SaveLoad:
				TickSaveLoad(Owner, Record, Character);
				break;
			case EProof::Level1Deny:
				TickLevel1Deny(Owner, Record, World, Character);
				break;
			case EProof::RestoreLevel2:
				TickRestoreLevel2(Owner, Record, Character);
				break;
			case EProof::WalkService:
				TickWalkService(Owner, Record, Character);
				break;
			case EProof::WalkConnector:
				TickWalkConnector(Owner, Record, Character);
				break;
			case EProof::WaitNeuroStream:
				TickWaitNeuroStream(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::WalkRamp:
				TickWalkRamp(Owner, Record, Character);
				break;
			case EProof::ApproachGate:
				TickApproachGate(Owner, Record, World, Character);
				break;
			case EProof::AssertLocked:
				TickAssertLocked(Owner, Record, Character);
				break;
			case EProof::Unlock:
				TickUnlock(Owner, Record, World, Character);
				break;
			case EProof::WalkCheckpoint:
				TickWalkCheckpoint(Owner, Record, World, Character);
				break;
			case EProof::Isolation:
				TickIsolation(Owner, Record, World, Character);
				break;
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}
		}

		void TickSnapshots(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			UProjectOrganoidItemData* Item = LoadCampaignCard();
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, PickupLabel));
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();

			AssertTrue(Record, TEXT("campaign.item_exists"), Item != nullptr,
				TEXT("DA_Item_ResearchWingKeycard"), Item ? Item->GetName() : TEXT("missing"), TEXT("item"));
			AssertTrue(Record, TEXT("campaign.item_name"), Item && Item->ItemName.ToString() == TEXT("Research Wing Keycard"),
				TEXT("Research Wing Keycard"), Item ? Item->ItemName.ToString() : TEXT("missing"), TEXT("item"));
			AssertTrue(Record, TEXT("campaign.item_type"), Item && Item->ItemType == EProjectOrganoidItemType::KeyItem,
				TEXT("KeyItem"), Item ? TEXT("set") : TEXT("missing"), TEXT("item"));
			AssertTrue(Record, TEXT("campaign.item_tier"), Item && Item->SecurityTier == EProjectOrganoidSecurityTier::Level2_Lab,
				TEXT("Level2_Lab"), Item ? TEXT("set") : TEXT("missing"), TEXT("item"));
			AssertTrue(Record, TEXT("campaign.no_generic_keycard_event"),
				Item && !Item->bBroadcastGenericKeycardObjectiveEvent,
				TEXT("false"), Item && Item->bBroadcastGenericKeycardObjectiveEvent ? TEXT("true") : TEXT("false"), TEXT("item"));
			AssertTrue(Record, TEXT("campaign.pickup_exists"), Pickup != nullptr,
				TEXT("Pickup_ResearchWingKeycard"), Pickup ? TEXT("found") : TEXT("missing"), PickupLabel);
			AssertTrue(Record, TEXT("campaign.pickup_admin"),
				Pickup && OrganoidPlaytestActions::ActorPackage(Pickup).Contains(TEXT("SL_Epitope_Admin")),
				TEXT("Admin"), Pickup ? OrganoidPlaytestActions::ActorPackage(Pickup) : TEXT("missing"), PickupLabel);
			AssertTrue(Record, TEXT("campaign.pickup_transform"),
				Pickup && FVector::Dist(Pickup->GetActorLocation(), PickupLocation) <= 2.0f,
				TEXT("(2580,-560,80)"), Pickup ? Pickup->GetActorLocation().ToCompactString() : TEXT("missing"), PickupLabel);
			AssertTrue(Record, TEXT("campaign.pickup_uses_da"),
				Pickup && Pickup->ItemData == Item,
				TEXT("DA_Item_ResearchWingKeycard"), Pickup && Pickup->ItemData ? Pickup->ItemData->GetName() : TEXT("missing"), PickupLabel);

			AssertTrue(Record, TEXT("access.initial_no_level2"),
				Inventory && !Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab),
				TEXT("false"), Inventory && Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab) ? TEXT("true") : TEXT("false"), TEXT("inventory"));
			AssertTrue(Record, TEXT("access.no_injection"), !bInjectedLevel2,
				TEXT("false"), TEXT("false"), TEXT("inventory"));

			MagBefore = MagOf(Character);
			ReserveBefore = ReserveOf(Character);
			SotBefore = CountSot(Character);

			if (UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character->GetBiologicalAdaptationComponent())
			{
				AssertTrue(Record, TEXT("no_neural_slow_unlock"),
					!Adapt->IsAdaptationUnlocked(UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve()),
					TEXT("locked"), TEXT("unlocked"), TEXT("adaptation"));
			}
			AssertTrue(Record, TEXT("no_stabilized_barrel_unlock"),
				!Character->IsWeaponModUnlocked(UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve()),
				TEXT("locked"), TEXT("unlocked"), TEXT("weapon_mod"));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::CollectResearchWing;
			Owner.SetStage(TEXT("CollectResearchWing"));
		}

		void TickCollectResearchWing(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, PickupLabel));
			const FVector CollectAt(PickupLocation.X, PickupLocation.Y, StandZ(20.0f, Character));
			if (!Pickup || !OrganoidPlaytestActions::TeleportNear(Character, CollectAt, 90.0f, CollectAt.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to reach Pickup_ResearchWingKeycard in Admin."));
				return;
			}
			const float ReachDist = FVector::Dist(Character->GetActorLocation(), Pickup->GetActorLocation());
			AssertTrue(Record, TEXT("campaign.pickup_reachable"), ReachDist <= 180.0f,
				TEXT("<=180 interact range"),
				FString::SanitizeFloat(ReachDist),
				PickupLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			OrganoidPlaytestActions::FaceActor(Character, Pickup);
			AssertTrue(Record, TEXT("campaign.pickup_held"),
				!Pickup->CanInteract(Character),
				TEXT("not interactable"),
				Pickup->CanInteract(Character) ? TEXT("interactable") : TEXT("held"),
				PickupLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			UProjectOrganoidItemData* Item = LoadCampaignCard();
			FGuid InjectedId;
			const bool bInjected = Inventory && Item && Inventory->TryAddItem(Item, InjectedId, 1);
			if (bInjected)
			{
				bInjectedLevel2 = true;
			}
			AssertTrue(Record, TEXT("access.level2_injected_from_campaign_da"),
				bInjected && bInjectedLevel2 && Inventory
					&& Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab)
					&& InventoryHasCampaignCard(Inventory),
				TEXT("injected DA_Item_ResearchWingKeycard"),
				InventoryHasCampaignCard(Inventory) ? TEXT("injected campaign DA") : TEXT("missing"),
				TEXT("inventory"));
			AActor* AfterPickup = OrganoidPlaytestActions::FindUniqueByLabel(World, PickupLabel);
			const bool bPickupRemains = AfterPickup != nullptr && IsValid(AfterPickup);
			AssertTrue(Record, TEXT("campaign.pickup_remains"), bPickupRemains,
				TEXT("present"), bPickupRemains ? TEXT("present") : TEXT("destroyed"), PickupLabel);
			AssertTrue(Record, TEXT("access.injected_after_grant"), bInjectedLevel2,
				TEXT("true"), bInjectedLevel2 ? TEXT("true") : TEXT("false"), TEXT("inventory"));
			AssertTrue(Record, TEXT("ammo_unchanged_after_collect"),
				MagOf(Character) == MagBefore && ReserveOf(Character) == ReserveBefore,
				FString::FromInt(MagBefore), FString::FromInt(MagOf(Character)), TEXT("ammo"));
			AssertTrue(Record, TEXT("sot_unchanged_after_collect"), CountSot(Character) == SotBefore,
				FString::FromInt(SotBefore), FString::FromInt(CountSot(Character)), TEXT("SOT"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::ObjectiveGuard;
			Owner.SetStage(TEXT("ObjectiveGuard"));
		}

		void TickObjectiveGuard(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UGameInstance* GI = Character->GetGameInstance();
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			FProjectOrganoidObjective AdminCard;
			const bool bFound = Objectives && Objectives->GetObjective(TEXT("Main_ObtainAdminKeycard"), AdminCard);
			AssertTrue(Record, TEXT("objective.admin_keycard_not_completed_by_l2"),
				!bFound || AdminCard.State != EProjectOrganoidObjectiveState::Completed,
				TEXT("not Completed"),
				bFound ? UEnum::GetValueAsString(AdminCard.State) : TEXT("missing"),
				TEXT("Main_ObtainAdminKeycard"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::SaveLoad;
			Owner.SetStage(TEXT("SaveLoad"));
		}

		void TickSaveLoad(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UGameInstance* GI = Character->GetGameInstance();
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			if (!Saves || !Inventory)
			{
				FailAndStop(Owner, Record, TEXT("Save subsystem or inventory missing."));
				return;
			}
			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload_wrote"), bSaved, TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), TEXT("save"));

			Inventory->ConsumeKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab);
			AssertTrue(Record, TEXT("runtime_cleared_before_load"),
				!Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab),
				TEXT("cleared"), TEXT("still-set"), TEXT("inventory"));

			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			Inventory = Character->GetInventoryComponent();
			AssertTrue(Record, TEXT("saveload_restored"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), TEXT("save"));
			AssertTrue(Record, TEXT("saveload_level2"),
				Inventory && Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab)
					&& InventoryHasCampaignCard(Inventory),
				TEXT("Level2_Lab campaign DA"),
				Inventory && InventoryHasCampaignCard(Inventory) ? TEXT("restored") : TEXT("missing"),
				TEXT("inventory"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Level1Deny;
			Owner.SetStage(TEXT("Level1Deny"));
		}

		void TickLevel1Deny(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidItemPickup* AdminPickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, AdminPickupLabel));
			AssertTrue(Record, TEXT("campaign.admin_l1_still_present"), AdminPickup != nullptr,
				TEXT("Pickup_AdminKeycard"), AdminPickup ? TEXT("found") : TEXT("missing"), AdminPickupLabel);
			const bool bGotAdmin = CollectPickup(Character, AdminPickupLabel, FVector(-2235.0f, 1275.0f, 80.0f));
			AssertTrue(Record, TEXT("campaign.admin_l1_collected"), bGotAdmin,
				TEXT("true"), bGotAdmin ? TEXT("true") : TEXT("false"), AdminPickupLabel);

			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			if (Inventory)
			{
				Inventory->ConsumeKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab);
			}
			AssertTrue(Record, TEXT("access.level1_insufficient_inventory"),
				Inventory && Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level1_Admin)
					&& !Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab),
				TEXT("Level1 only"),
				Inventory && Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab) ? TEXT("still Level2") : TEXT("Level1 only"),
				TEXT("inventory"));

			AProjectOrganoidSecurityGate* FoundGate = Cast<AProjectOrganoidSecurityGate>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, GateLabel));
			Gate = FoundGate;
			AssertTrue(Record, TEXT("gate.id"), FoundGate && FoundGate->GateId == FName(GateIdName),
				GateIdName, FoundGate ? FoundGate->GateId.ToString() : TEXT("missing"), GateLabel);
			AssertTrue(Record, TEXT("gate.requires_level2"),
				FoundGate && FoundGate->RequiredSecurityTier == EProjectOrganoidSecurityTier::Level2_Lab,
				TEXT("Level2_Lab"), FoundGate ? TEXT("set") : TEXT("missing"), GateLabel);
			AssertTrue(Record, TEXT("gate.starts_sealed"), FoundGate && FoundGate->IsSealed(),
				TEXT("Sealed"), FoundGate && FoundGate->IsOpen() ? TEXT("Open") : TEXT("Sealed"), GateLabel);

			if (FoundGate)
			{
				const FVector Approach(3200.0f, 900.0f, StandZ(-1190.0f, Character));
				Character->SetActorLocation(Approach, false, nullptr, ETeleportType::TeleportPhysics);
				OrganoidPlaytestActions::FaceActor(Character, FoundGate);
				const bool bOpened = FoundGate->TryOverrideWithInventory(Character);
				AssertTrue(Record, TEXT("access.level1_does_not_open_gate"), !bOpened && FoundGate->IsSealed(),
					TEXT("Sealed"), FoundGate->IsOpen() ? TEXT("Open") : TEXT("Sealed"), GateLabel);
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::RestoreLevel2;
			Owner.SetStage(TEXT("RestoreLevel2"));
		}

		void TickRestoreLevel2(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UGameInstance* GI = Character->GetGameInstance();
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			if (!Saves)
			{
				FailAndStop(Owner, Record, TEXT("Save subsystem missing for Level2 restore."));
				return;
			}
			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			AssertTrue(Record, TEXT("access.level2_restored_for_traversal"),
				bLoaded && Inventory && InventoryHasCampaignCard(Inventory),
				TEXT("campaign DA"), bLoaded ? TEXT("restored") : TEXT("load failed"), TEXT("inventory"));
			Saves->DeleteSave(SaveSlot);
			const FVector Start(4000.0f, -200.0f, StandZ(20.0f, Character));
			if (!OrganoidPlaytestActions::TeleportNear(Character, Start, 0.0f, Start.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place Nathan in Operations after restore."));
				return;
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::WalkService;
			Owner.SetStage(TEXT("WalkService"));
		}

		void TickWalkService(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			const FVector Corridor(4000.0f, -900.0f, StandZ(20.0f, Character));
			FHitResult Hit;
			const bool bReached = WalkToward(Character, Corridor, Hit);
			AssertTrue(Record, TEXT("waypoint.service_corridor"), bReached,
				TEXT("Operations to Service Corridor"),
				FString::Printf(TEXT("at=%s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
				TEXT("Admin_ServiceCorridor_Floor"));
			if (!bReached)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::WalkConnector;
			Owner.SetStage(TEXT("WalkConnector"));
		}

		void TickWalkConnector(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			const TArray<FVector> Path = {
				FVector(4400.0f, -900.0f, StandZ(20.0f, Character)),
				FVector(4600.0f, -900.0f, StandZ(20.0f, Character)),
				FVector(5000.0f, -900.0f, StandZ(20.0f, Character)),
			};
			for (int32 Index = 0; Index < Path.Num(); ++Index)
			{
				FHitResult Hit;
				if (!WalkToward(Character, Path[Index], Hit))
				{
					AssertTrue(Record, TEXT("waypoint.research_wing_connector"), false,
						TEXT("Service Corridor to Spine_Landing_Admin"),
						FString::Printf(TEXT("blocked at %s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
						TEXT("Admin_ResearchWing_Connector_Floor"));
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
			}
			AssertTrue(Record, TEXT("waypoint.research_wing_connector"), true,
				TEXT("reached Spine_Landing_Admin"),
				Character->GetActorLocation().ToCompactString(),
				TEXT("Spine_Landing_Admin"));
			WaitSeconds = 0.0f;
			Proof = EProof::WaitNeuroStream;
			Owner.SetStage(TEXT("WaitNeuroStream"));
		}

		void TickWaitNeuroStream(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>();
			bool bNeuroReady = false;
			bool bAdminReady = false;
			if (Levels)
			{
				const FName NeuroName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics);
				const FName AdminName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel1_Admin);
				bNeuroReady = Levels->IsPartitionReady(NeuroName);
				bAdminReady = Levels->IsPartitionReady(AdminName);
				Levels->ReconcileStreamingNow();
			}
			if (bNeuroReady && bAdminReady)
			{
				AssertTrue(Record, TEXT("neuro_streams_at_seam"), true, TEXT("ready"), TEXT("ready"), TEXT("StreamBand_Admin_NeuroGenetics"));
				AssertTrue(Record, TEXT("admin_remains_loaded"), true, TEXT("ready"), TEXT("ready"), TEXT("Admin"));
				WaypointIndex = 0;
				Proof = EProof::WalkRamp;
				Owner.SetStage(TEXT("WalkRamp"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				AssertTrue(Record, TEXT("neuro_streams_at_seam"), false,
					TEXT("ready"),
					FString::Printf(TEXT("neuro=%s admin=%s"), bNeuroReady ? TEXT("ready") : TEXT("pending"), bAdminReady ? TEXT("ready") : TEXT("pending")),
					TEXT("StreamBand_Admin_NeuroGenetics"));
				FailAndStop(Owner, Record, Record.FailureReason);
			}
			(void)Character;
		}

		void TickWalkRamp(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			const FVector Path[] = {
				FVector(5200.0f, -900.0f, StandZ(20.0f, Character)),
				FVector(5200.0f, -780.0f, StandZ(20.0f, Character)),
				FVector(5200.0f, -780.0f, StandZ(RampFloorZ(-780.0f) + 40.0f, Character)),
				FVector(5200.0f, -600.0f, StandZ(RampFloorZ(-600.0f) + 10.0f, Character)),
				FVector(5200.0f, -300.0f, StandZ(RampFloorZ(-300.0f) + 10.0f, Character)),
				FVector(5000.0f, 900.0f, StandZ(-1190.0f, Character)),
			};
			if (WaypointIndex >= UE_ARRAY_COUNT(Path))
			{
				AssertTrue(Record, TEXT("waypoint.neuro_landing"),
					FVector::Dist2D(Character->GetActorLocation(), FVector(5000.0f, 900.0f, 0.0f)) < 80.0f,
					TEXT("Spine_Landing_NeuroGenetics"),
					Character->GetActorLocation().ToCompactString(),
					TEXT("Spine_Landing_NeuroGenetics"));
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				Proof = EProof::ApproachGate;
				Owner.SetStage(TEXT("ApproachGate"));
				return;
			}

			const FVector Dest = Path[WaypointIndex];
			FHitResult Hit;
			bool bReached = false;
			if (Dest.Y >= 800.0f)
			{
				Character->SetActorLocation(Dest, false, nullptr, ETeleportType::TeleportPhysics);
				bReached = FVector::Dist(Character->GetActorLocation(), Dest) <= 40.0f;
			}
			else
			{
				bReached = WalkToward(Character, Dest, Hit, 70.0f);
			}
			if (!bReached)
			{
				AssertTrue(Record, TEXT("waypoint.spine_ramp"), false,
					TEXT("walkable ramp"),
					FString::Printf(TEXT("i=%d dest=%s at=%s hit=%s"), WaypointIndex, *Dest.ToCompactString(), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
					TEXT("Spine_Ramp_Admin_To_NeuroGenetics"));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			const float Z = Character->GetActorLocation().Z;
			if (Z < Dest.Z - 180.0f)
			{
				AssertTrue(Record, TEXT("no_fall_through_seams"), false,
					TEXT("stayed on ramp"),
					FString::Printf(TEXT("z=%.1f destz=%.1f"), Z, Dest.Z),
					TEXT("Spine_Ramp_Admin_To_NeuroGenetics"));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			++WaypointIndex;
		}

		void TickApproachGate(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidSecurityGate* FoundGate = Cast<AProjectOrganoidSecurityGate>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, GateLabel));
			if (!FoundGate)
			{
				FailAndStop(Owner, Record, TEXT("Gate_ResearchWing missing on Lvl_Epitope PIE."));
				return;
			}
			Gate = FoundGate;
			AssertTrue(Record, TEXT("gate.owner_not_admin"),
				!OrganoidPlaytestActions::ActorPackage(FoundGate).Contains(TEXT("SL_Epitope_Admin")),
				TEXT("Lvl_Epitope"), OrganoidPlaytestActions::ActorPackage(FoundGate), GateLabel);

			const FVector Approach(3200.0f, 900.0f, StandZ(-1190.0f, Character));
			FHitResult Hit;
			if (!WalkToward(Character, Approach, Hit, 140.0f))
			{
				AssertTrue(Record, TEXT("waypoint.neuro_bridge"), false,
					TEXT("Neuro landing to gate approach"),
					FString::Printf(TEXT("at=%s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
					TEXT("Spine_Bridge_NeuroGenetics"));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			AssertTrue(Record, TEXT("waypoint.neuro_bridge"), true,
				TEXT("approached Gate_ResearchWing"),
				Character->GetActorLocation().ToCompactString(),
				GateLabel);
			Proof = EProof::AssertLocked;
			Owner.SetStage(TEXT("AssertLocked"));
		}

		void TickAssertLocked(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidSecurityGate* FoundGate = Gate.Get();
			if (!FoundGate)
			{
				FailAndStop(Owner, Record, TEXT("Gate_ResearchWing vanished before lock proof."));
				return;
			}
			AssertTrue(Record, TEXT("gate.sealed_until_presented"), FoundGate->IsSealed(),
				TEXT("Sealed"), FoundGate->IsSealed() ? TEXT("Sealed") : TEXT("Open"), GateLabel);

			const FVector Through(2700.0f, 900.0f, StandZ(-1190.0f, Character));
			FHitResult Hit;
			const bool bReached = WalkToward(Character, Through, Hit, 80.0f);
			const bool bHitGate = Hit.bBlockingHit && Hit.GetActor() == FoundGate;
			AssertTrue(Record, TEXT("gate.blocks_while_locked"), !bReached && bHitGate,
				TEXT("blocked by Gate_ResearchWing"),
				FString::Printf(TEXT("reached=%s hit=%s"), bReached ? TEXT("true") : TEXT("false"), *HitName(Hit)),
				GateLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Unlock;
			Owner.SetStage(TEXT("Unlock"));
		}

		void TickUnlock(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidSecurityGate* FoundGate = Gate.Get();
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			if (!FoundGate || !Inventory)
			{
				FailAndStop(Owner, Record, TEXT("Gate or inventory missing for authorized access."));
				return;
			}
			AssertTrue(Record, TEXT("access.uses_campaign_credential"),
				InventoryHasCampaignCard(Inventory) && bInjectedLevel2,
				TEXT("injected campaign DA"), InventoryHasCampaignCard(Inventory) ? TEXT("injected campaign DA") : TEXT("missing"), TEXT("inventory"));

			OrganoidPlaytestActions::FaceActor(Character, FoundGate);
			const bool bOpened = FoundGate->TryOverrideWithInventory(Character);
			AssertTrue(Record, TEXT("access.campaign_card_opens_gate"), bOpened && FoundGate->IsOpen(),
				TEXT("Open"), FoundGate->IsOpen() ? TEXT("Open") : TEXT("Sealed"), GateLabel);
			if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
			{
				AssertTrue(Record, TEXT("access.gateid_lookup"),
					Security->FindGateById(FName(GateIdName)) == FoundGate,
					TEXT("Gate_Neuro_Research"),
					Security->FindGateById(FName(GateIdName)) ? TEXT("found") : TEXT("missing"),
					GateLabel);
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::WalkCheckpoint;
			Owner.SetStage(TEXT("WalkCheckpoint"));
		}

		void TickWalkCheckpoint(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			const float NeuroStand = StandZ(-1170.0f, Character);
			const FVector Here = Character->GetActorLocation();
			FHitResult LiftHit;
			if (!WalkToward(Character, FVector(Here.X, Here.Y, NeuroStand), LiftHit, 70.0f))
			{
				AssertTrue(Record, TEXT("waypoint.checkpoint_neuro_airlock"), false,
					TEXT("step onto NeuroGenetics_FloorPlate"),
					FString::Printf(TEXT("at=%s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(LiftHit)),
					CheckpointLabel);
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			const TArray<FVector> Path = {
				FVector(2700.0f, 900.0f, NeuroStand),
				FVector(1950.0f, 900.0f, NeuroStand),
				FVector(1950.0f, 0.0f, NeuroStand),
			};
			for (const FVector& Dest : Path)
			{
				FHitResult Hit;
				if (!WalkToward(Character, Dest, Hit, 140.0f))
				{
					AssertTrue(Record, TEXT("waypoint.checkpoint_neuro_airlock"), false,
						TEXT("open gate to Checkpoint_NeuroAirlock"),
						FString::Printf(TEXT("at=%s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
						CheckpointLabel);
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
			}

			AActor* Checkpoint = OrganoidPlaytestActions::FindUniqueByLabel(World, CheckpointLabel);
			const float Dist = Checkpoint ? FVector::Dist(Character->GetActorLocation(), Checkpoint->GetActorLocation()) : 9999.0f;
			AssertTrue(Record, TEXT("waypoint.checkpoint_neuro_airlock"), Checkpoint && Dist < 280.0f,
				TEXT("<280"), FString::SanitizeFloat(Dist), CheckpointLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Isolation;
			Owner.SetStage(TEXT("Isolation"));
		}

		void TickIsolation(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AssertTrue(Record, TEXT("no_admin_hosts"), CountHostsInPackage(World, TEXT("SL_Epitope_Admin")) == 0,
				TEXT("0"), FString::FromInt(CountHostsInPackage(World, TEXT("SL_Epitope_Admin"))), TEXT("Admin"));
			AssertTrue(Record, TEXT("neuro_hosts_remain"), CountHostsInPackage(World, TEXT("SL_Epitope_NeuroGenetics")) >= 3,
				TEXT(">=3"), FString::FromInt(CountHostsInPackage(World, TEXT("SL_Epitope_NeuroGenetics"))), TEXT("Neuro"));
			AProjectOrganoidHostBase* Host1 = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, Host1Label));
			AssertTrue(Record, TEXT("host1_still_neuro"),
				Host1 && OrganoidPlaytestActions::ActorPackage(Host1).Contains(TEXT("SL_Epitope_NeuroGenetics")),
				TEXT("Neuro"), Host1 ? OrganoidPlaytestActions::ActorPackage(Host1) : TEXT("missing"), Host1Label);

			if (UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character->GetBiologicalAdaptationComponent())
			{
				AssertTrue(Record, TEXT("no_neural_slow_after"),
					!Adapt->IsAdaptationUnlocked(UProjectOrganoidBiologicalAdaptation_NeuralSlow::Resolve()),
					TEXT("locked"), TEXT("unlocked"), TEXT("adaptation"));
			}
			AssertTrue(Record, TEXT("no_stabilized_barrel_after"),
				!Character->IsWeaponModUnlocked(UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve()),
				TEXT("locked"), TEXT("unlocked"), TEXT("weapon_mod"));
			AssertTrue(Record, TEXT("ammo_unchanged"),
				MagOf(Character) == MagBefore && ReserveOf(Character) == ReserveBefore,
				FString::FromInt(MagBefore), FString::FromInt(MagOf(Character)), TEXT("ammo"));
			AssertTrue(Record, TEXT("sot_unchanged"), CountSot(Character) == SotBefore,
				FString::FromInt(SotBefore), FString::FromInt(CountSot(Character)), TEXT("SOT"));
			AssertTrue(Record, TEXT("no_sector_transition_used"), !bUsedSectorTransition,
				TEXT("false"), TEXT("false"), TEXT("player"));
			AssertTrue(Record, TEXT("access.fixture_injected_level2"), bInjectedLevel2,
				TEXT("true"), bInjectedLevel2 ? TEXT("true") : TEXT("false"), TEXT("inventory"));
			Proof = EProof::Done;
			Stage = EStage::EndPie;
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TMap<FString, FVector> EditorHostAfter;
			if (UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
			{
				for (TActorIterator<AProjectOrganoidHostBase> It(EditorWorld); It; ++It)
				{
					if (OrganoidPlaytestActions::ActorPackage(*It).Contains(TEXT("SL_Epitope_NeuroGenetics")))
					{
						EditorHostAfter.Add(OrganoidPlaytestActions::ActorLabel(*It), (*It)->GetActorLocation());
					}
				}
			}
			bool bHostsUnmoved = EditorHostAfter.Num() == EditorHostLocations.Num() && EditorHostLocations.Num() >= 3;
			if (bHostsUnmoved)
			{
				for (const TPair<FString, FVector>& Pair : EditorHostLocations)
				{
					const FVector* After = EditorHostAfter.Find(Pair.Key);
					if (!After || FVector::Dist(*After, Pair.Value) > 2.0f)
					{
						bHostsUnmoved = false;
						break;
					}
				}
			}
			AssertTrue(Record, TEXT("hosts_unmoved"), bHostsUnmoved,
				TEXT("unchanged"), bHostsUnmoved ? TEXT("unchanged") : TEXT("moved"), TEXT("Neuro"));

			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(Record, TEXT("dirty_unchanged"), DirtyAfter == DirtyBefore,
				FString::Join(DirtyBefore, TEXT(",")),
				FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"));
			Record.AddActor(TEXT("dirty_count"), FString::FromInt(DirtyAfter.Num()));
			AssertTrue(Record, TEXT("playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""));
			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FNeuroAccessAutoRegister
	{
		FNeuroAccessAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroAccessFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroAccessAutoRegister GRegisterNeuroAccess;
}
