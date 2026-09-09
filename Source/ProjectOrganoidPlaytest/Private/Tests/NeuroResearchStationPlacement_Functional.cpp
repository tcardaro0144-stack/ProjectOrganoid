#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "AI/NavigationSystemBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/CollisionProfile.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidCorridorTrapVolume.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidInteractionComponent.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("NeuroResearchStationPlacement_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Neuro Research Station Placement");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR StationLabel[] = TEXT("ResearchStation_NeuroGenetics");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_NeuroAirlock");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR SotPath[] = TEXT("/Game/Data/Items/DA_Item_SOT.DA_Item_SOT");
	constexpr TCHAR PistolAmmoPath[] = TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo");
	const FVector ApprovedLocation(800.0f, -1600.0f, -1100.0f);
	const FRotator ApprovedRotation(0.0f, 180.0f, 0.0f);

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

	class FNeuroResearchStationPlacementFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::UniquePlacement;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
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
			UniquePlacement,
			Shell,
			StreamAndWalk,
			ApproachAndBlock,
			Doorway,
			HostNav,
			Clearances,
			InteractIdle,
			HostStates,
			NoSaveHeal,
			Barrel,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::UniquePlacement;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		int32 SotAtUnlock = 0;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidResearchStation> Station;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> Checkpoint;

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

		float StandZ(float FloorTop, APawn* Pawn) const
		{
			float Half = 88.0f;
			if (const ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					Half = Capsule->GetScaledCapsuleHalfHeight();
				}
			}
			return FloorTop + Half;
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
			const FVector Current = Pawn->GetActorLocation();
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

		AProjectOrganoidWeapon* GetWeapon(AProjectOrganoidCharacter* Character) const
		{
			return Character && Character->GetWeaponComponent()
				? Character->GetWeaponComponent()->GetEquippedWeapon()
				: nullptr;
		}

		int32 CountSot(AProjectOrganoidCharacter* Character) const
		{
			UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
			return Inventory ? Inventory->CountItemsOfType(EProjectOrganoidItemType::SOT) : 0;
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

		int32 CountStations(UWorld* World) const
		{
			int32 Count = 0;
			if (!World)
			{
				return 0;
			}
			for (AActor* Actor : OrganoidPlaytestActions::FindActorsByLabel(World, StationLabel))
			{
				if (Actor)
				{
					++Count;
				}
			}
			return Count;
		}

		bool IsCursorGameplay(AProjectOrganoidCharacter* Character) const
		{
			APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
			return PC && !PC->bShowMouseCursor;
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

			AProjectOrganoidResearchStation* FoundStation = Cast<AProjectOrganoidResearchStation>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, StationLabel));
			AProjectOrganoidCheckpoint* FoundCheckpoint = Cast<AProjectOrganoidCheckpoint>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, CheckpointLabel));
			AProjectOrganoidHostBase* FoundHost = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, Host1Label));
			if (World && Character && GetWeapon(Character) && FoundStation && FoundCheckpoint && FoundHost
				&& Cast<AProjectOrganoidHostAIController>(FoundHost->GetController()))
			{
				Player = Character;
				Station = FoundStation;
				Checkpoint = FoundCheckpoint;
				Host = FoundHost;
				WaitSeconds = 0.0f;
				Proof = EProof::UniquePlacement;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 90.0f)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					TEXT("Timed out waiting for PIE player, ResearchStation_NeuroGenetics, checkpoint, and Host_Neuro_1."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidResearchStation* Alive = Station.Get();
			AProjectOrganoidCheckpoint* NeuroCheckpoint = Checkpoint.Get();
			AProjectOrganoidHostBase* HostActor = Host.Get();
			AProjectOrganoidHostAIController* HostAI = HostActor
				? Cast<AProjectOrganoidHostAIController>(HostActor->GetController())
				: nullptr;
			AProjectOrganoidWeapon* Weapon = GetWeapon(Character);
			if (!World || !Character || !Alive || !NeuroCheckpoint || !HostActor || !HostAI || !Weapon)
			{
				FailAndStop(Owner, Record, TEXT("PIE player, station, checkpoint, or Host vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::UniquePlacement:
			{
				AssertTrue(Record, TEXT("unique_station"), CountStations(World) == 1, TEXT("1"), FString::FromInt(CountStations(World)), StationLabel);
				AssertTrue(Record, TEXT("station_class"), Alive->IsA(AProjectOrganoidResearchStation::StaticClass()), TEXT("AProjectOrganoidResearchStation"), Alive->GetClass()->GetName(), StationLabel);
				AssertTrue(
					Record, TEXT("owning_package_neuro"),
					OrganoidPlaytestActions::ActorPackage(Alive).Contains(TEXT("SL_Epitope_NeuroGenetics")),
					NeuroPackage,
					OrganoidPlaytestActions::ActorPackage(Alive),
					StationLabel);
				AssertTrue(
					Record, TEXT("approved_location"),
					FVector::Dist(Alive->GetActorLocation(), ApprovedLocation) <= 1.0f,
					TEXT("(800,-1600,-1100)"),
					Alive->GetActorLocation().ToCompactString(),
					StationLabel);
				AssertTrue(
					Record, TEXT("approved_yaw"),
					FMath::Abs(FMath::FindDeltaAngleDegrees(Alive->GetActorRotation().Yaw, ApprovedRotation.Yaw)) <= 1.0f,
					TEXT("180"),
					FString::SanitizeFloat(Alive->GetActorRotation().Yaw),
					StationLabel);
				Proof = EProof::Shell;
				break;
			}
			case EProof::Shell:
			{
				UStaticMeshComponent* Mesh = Alive->StationMesh;
				AssertTrue(Record, TEXT("provisional_shell_present"), Mesh && Mesh->GetStaticMesh() != nullptr, TEXT("StationMesh Cube"), Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetName() : TEXT("none"), TEXT("StationMesh"));
				AssertTrue(
					Record, TEXT("shell_is_engine_cube"),
					Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetPathName().Contains(TEXT("BasicShapes/Cube")),
					TEXT("/Engine/BasicShapes/Cube"),
					Mesh && Mesh->GetStaticMesh() ? Mesh->GetStaticMesh()->GetPathName() : TEXT("none"),
					TEXT("StationMesh"));
				AssertTrue(
					Record, TEXT("shell_query_and_physics"),
					Mesh && Mesh->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics,
					TEXT("QueryAndPhysics"),
					Mesh ? UEnum::GetValueAsString(Mesh->GetCollisionEnabled()) : TEXT("none"),
					TEXT("StationMesh"));
				AssertTrue(
					Record, TEXT("shell_block_all_dynamic"),
					Mesh && Mesh->GetCollisionProfileName() == UCollisionProfile::BlockAllDynamic_ProfileName,
					TEXT("BlockAllDynamic"),
					Mesh ? Mesh->GetCollisionProfileName().ToString() : TEXT("none"),
					TEXT("StationMesh"));
				Proof = EProof::StreamAndWalk;
				break;
			}
			case EProof::StreamAndWalk:
			{
				const float NeuroStand = StandZ(-1170.0f, Character);
				if (!OrganoidPlaytestActions::TeleportNear(Character, FVector(2700.0f, 900.0f, NeuroStand), 0.0f, NeuroStand))
				{
					FailAndStop(Owner, Record, TEXT("Failed to place Nathan on the Neuro plate after the gate. Ramp is not modified."));
					return;
				}
				const TArray<FVector> Path = {
					FVector(1950.0f, 900.0f, NeuroStand),
					FVector(1950.0f, 0.0f, NeuroStand),
				};
				for (const FVector& Dest : Path)
				{
					FHitResult Hit;
					if (!WalkToward(Character, Dest, Hit, 140.0f))
					{
						FailAndStop(Owner, Record, TEXT("Failed walking the accepted Neuro landing path to Checkpoint_NeuroAirlock."));
						return;
					}
				}
				AssertTrue(
					Record, TEXT("admin_neuro_route_reached_checkpoint"),
					FVector::Dist2D(Character->GetActorLocation(), NeuroCheckpoint->GetActorLocation()) < 280.0f,
					TEXT("<280"),
					FString::SanitizeFloat(FVector::Dist2D(Character->GetActorLocation(), NeuroCheckpoint->GetActorLocation())),
					CheckpointLabel);
				Proof = EProof::ApproachAndBlock;
				break;
			}
			case EProof::ApproachAndBlock:
			{
				const float NeuroStand = StandZ(-1170.0f, Character);
				// Gowning-ring traps already occupy the corridor. Do not require the bot
				// to survive that existing hazard to prove station approach, same as the 34° ramp.
				if (!OrganoidPlaytestActions::TeleportNear(Character, FVector(400.0f, -1200.0f, NeuroStand), 0.0f, NeuroStand))
				{
					FailAndStop(Owner, Record, TEXT("Failed to place Nathan in SE after the accepted Neuro landing path."));
					return;
				}
				FHitResult ApproachHit;
				if (!WalkToward(Character, FVector(600.0f, -1600.0f, NeuroStand), ApproachHit, 140.0f))
				{
					FailAndStop(Owner, Record, TEXT("Failed walking the SE-room approach to the Research Station."));
					return;
				}
				AssertTrue(
					Record, TEXT("nathan_can_approach"),
					FVector::Dist2D(Character->GetActorLocation(), ApprovedLocation) <= 220.0f,
					TEXT("<=220"),
					FString::SanitizeFloat(FVector::Dist2D(Character->GetActorLocation(), ApprovedLocation)),
					StationLabel);

				FHitResult BlockHit;
				const bool bWalkedThrough = WalkToward(Character, FVector(800.0f, -1600.0f, NeuroStand), BlockHit, 40.0f);
				const bool bHitStation = BlockHit.bBlockingHit && (BlockHit.GetActor() == Alive || BlockHit.GetComponent() == Alive->StationMesh);
				AssertTrue(Record, TEXT("cannot_walk_through_shell"), !bWalkedThrough && bHitStation, TEXT("blocked by StationMesh"), bWalkedThrough ? TEXT("walked through") : (bHitStation ? TEXT("blocked") : TEXT("blocked other")), TEXT("StationMesh"));
				Proof = EProof::Doorway;
				break;
			}
			case EProof::Doorway:
			{
				const float NeuroStand = StandZ(-1170.0f, Character);
				if (!OrganoidPlaytestActions::TeleportNear(Character, FVector(0.0f, -480.0f, NeuroStand), 0.0f, NeuroStand))
				{
					FailAndStop(Owner, Record, TEXT("Failed to place Nathan on the SE side of the gowning doorway."));
					return;
				}
				FHitResult Hit;
				const bool bThrough = WalkToward(Character, FVector(0.0f, -320.0f, NeuroStand), Hit, 40.0f);
				AssertTrue(Record, TEXT("se_doorway_traversable"), bThrough, TEXT("through"), bThrough ? TEXT("through") : TEXT("blocked"), TEXT("SE door"));
				Proof = EProof::HostNav;
				break;
			}
			case EProof::HostNav:
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation HostNav;
				FNavLocation StationNav;
				const bool bHostOnNav = NavSys && NavSys->ProjectPointToNavigation(HostActor->GetActorLocation(), HostNav, FVector(300.0f, 300.0f, 500.0f));
				const bool bStationOnNav = NavSys && NavSys->ProjectPointToNavigation(FVector(600.0f, -1600.0f, -1100.0f), StationNav, FVector(300.0f, 300.0f, 500.0f));
				UNavigationPath* Path = (bHostOnNav && bStationOnNav)
					? NavSys->FindPathToLocationSynchronously(World, HostNav.Location, StationNav.Location)
					: nullptr;
				AssertTrue(Record, TEXT("host_nav_into_se"), Path && Path->IsValid() && Path->PathPoints.Num() > 1, TEXT("path"), (Path && Path->IsValid()) ? FString::FromInt(Path->PathPoints.Num()) : TEXT("none"), Host1Label);
				Proof = EProof::Clearances;
				break;
			}
			case EProof::Clearances:
			{
				auto DistTo = [&](const TCHAR* Label) -> float
				{
					AActor* Other = OrganoidPlaytestActions::FindUniqueByLabel(World, Label);
					return Other ? FVector::Dist(Alive->GetActorLocation(), Other->GetActorLocation()) : 0.0f;
				};
				AssertTrue(Record, TEXT("no_overlap_checkpoint"), DistTo(CheckpointLabel) > 400.0f, TEXT(">400"), FString::SanitizeFloat(DistTo(CheckpointLabel)), CheckpointLabel);
				AssertTrue(Record, TEXT("no_overlap_npc"), DistTo(TEXT("NPC_IncineratorSurvivor")) > 420.0f, TEXT(">420"), FString::SanitizeFloat(DistTo(TEXT("NPC_IncineratorSurvivor"))), TEXT("NPC"));
				AssertTrue(Record, TEXT("no_overlap_ethics"), DistTo(TEXT("DataPad_EthicsObjection")) > 400.0f, TEXT(">400"), FString::SanitizeFloat(DistTo(TEXT("DataPad_EthicsObjection"))), TEXT("pad"));
				AssertTrue(Record, TEXT("no_overlap_specimen"), DistTo(TEXT("DataPad_SpecimenBadge")) > 400.0f, TEXT(">400"), FString::SanitizeFloat(DistTo(TEXT("DataPad_SpecimenBadge"))), TEXT("pad"));
				AssertTrue(Record, TEXT("no_overlap_panel"), DistTo(TEXT("PowerPanel_NeuroBackup")) > 400.0f, TEXT(">400"), FString::SanitizeFloat(DistTo(TEXT("PowerPanel_NeuroBackup"))), TEXT("panel"));

				AActor* Trap = OrganoidPlaytestActions::FindUniqueByLabel(World, TEXT("CorridorTraps_GowningRing"));
				bool bTrapClear = false;
				if (AProjectOrganoidCorridorTrapVolume* TrapVolume = Cast<AProjectOrganoidCorridorTrapVolume>(Trap))
				{
					const FVector TrapLoc = TrapVolume->GetActorLocation();
					const FVector TrapExtent(1600.0f, 340.0f, 200.0f);
					const FVector Delta = (Alive->GetActorLocation() - TrapLoc).GetAbs();
					bTrapClear = !(Delta.X <= TrapExtent.X + 200.0f && Delta.Y <= TrapExtent.Y + 200.0f && Delta.Z <= TrapExtent.Z + 200.0f);
				}
				AssertTrue(Record, TEXT("no_overlap_trap"), bTrapClear, TEXT("clear"), bTrapClear ? TEXT("clear") : TEXT("overlap"), TEXT("trap"));
				WaitSeconds = 0.0f;
				Proof = EProof::InteractIdle;
				break;
			}
			case EProof::InteractIdle:
			{
				WaitSeconds += DeltaTime;
				const float NeuroStand = StandZ(-1170.0f, Character);
				const FVector StandLoc(720.0f, -1600.0f, NeuroStand);
				if (FVector::Dist2D(Character->GetActorLocation(), StandLoc) > 40.0f)
				{
					OrganoidPlaytestActions::TeleportNear(Character, StandLoc, 0.0f, NeuroStand);
					WaitSeconds = 0.0f;
					break;
				}
				{
					const FVector Eye = Character->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
					const FRotator LookAt = (Alive->GetActorLocation() - Eye).Rotation();
					Character->SetActorRotation(FRotator(0.0f, LookAt.Yaw, 0.0f));
					if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
					{
						PC->SetViewTarget(Character);
						PC->SetControlRotation(LookAt);
					}
				}
				if (WaitSeconds < 0.35f)
				{
					break;
				}
				UProjectOrganoidInteractionComponent* Interaction = Character->GetInteractionComponent();
				AActor* Focus = Interaction ? Interaction->GetFocusedInteractable() : nullptr;
				AssertTrue(
					Record, TEXT("station_in_interaction_range"),
					FVector::Dist(Character->GetActorLocation(), Alive->GetActorLocation()) <= Alive->InteractionRange,
					FString::SanitizeFloat(Alive->InteractionRange),
					FString::SanitizeFloat(FVector::Dist(Character->GetActorLocation(), Alive->GetActorLocation())),
					TEXT("scan"));
				AssertTrue(Record, TEXT("station_focus"), Focus == Alive, TEXT("ResearchStation"), Focus ? Focus->GetName() : TEXT("none"), TEXT("scan"));
				AssertTrue(Record, TEXT("station_prompt"), Alive->GetInteractionPrompt().ToString().Contains(TEXT("Research Station")), TEXT("Use Research Station"), Alive->GetInteractionPrompt().ToString(), TEXT("prompt"));
				AssertTrue(Record, TEXT("can_interact_idle"), Alive->CanInteract(Character), TEXT("allowed"), Alive->CanInteract(Character) ? TEXT("allowed") : TEXT("locked"), TEXT("Idle"));
				WaitSeconds = 0.0f;
				Proof = EProof::HostStates;
				break;
			}
			case EProof::HostStates:
			{
				UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>();
				if (!Presence)
				{
					FailAndStop(Owner, Record, TEXT("Encounter presence missing."));
					return;
				}
				auto CheckState = [&](EProjectOrganoidHostCombatState State, bool bExpectLock, const TCHAR* Id)
				{
					HostAI->ApplyCombatState(State);
					const bool bLocked = Alive->IsLockedByEncounter() || !Alive->CanInteract(Character);
					const bool bOk = bExpectLock ? (bLocked && Presence->IsEncounterActive()) : (!bLocked && !Presence->DoesCombatStateLockStations(State));
					AssertTrue(Record, Id, bOk, bExpectLock ? TEXT("locked") : TEXT("allowed"), bLocked ? TEXT("locked") : TEXT("allowed"), StateName(State));
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
				Proof = EProof::NoSaveHeal;
				break;
			}
			case EProof::NoSaveHeal:
			{
				const FName CheckpointIdBefore = NeuroCheckpoint->CheckpointId;
				const bool bHadCheckpoint = Character->HasActivatedCheckpoint();
				AProjectOrganoidCheckpoint* LastBefore = Character->GetLastActivatedCheckpoint();
				const float HealthBefore = Character->GetHealth();
				Character->ApplyHealthDelta(-15.0f, EProjectOrganoidHealthDeltaSource::Generic);
				const float Damaged = Character->GetHealth();

				{
					const FVector Eye = Character->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
					const FRotator LookAt = (Alive->GetActorLocation() - Eye).Rotation();
					Character->SetActorRotation(FRotator(0.0f, LookAt.Yaw, 0.0f));
					if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
					{
						PC->SetViewTarget(Character);
						PC->SetControlRotation(LookAt);
					}
				}
				const bool bInteracted = Alive->Interact(Character);
				AssertTrue(Record, TEXT("open_outside_combat"), bInteracted && Alive->IsStationUIOpen(), TEXT("open"), bInteracted ? TEXT("open") : TEXT("failed"), TEXT("ui"));
				AssertTrue(Record, TEXT("no_heal_on_interact"), FMath::IsNearlyEqual(Character->GetHealth(), Damaged), FString::SanitizeFloat(Damaged), FString::SanitizeFloat(Character->GetHealth()), TEXT("vitals"));
				AssertTrue(Record, TEXT("checkpoint_id_unchanged"), NeuroCheckpoint->CheckpointId == CheckpointIdBefore, CheckpointIdBefore.ToString(), NeuroCheckpoint->CheckpointId.ToString(), CheckpointLabel);
				AssertTrue(Record, TEXT("respawn_not_relocated"), Character->HasActivatedCheckpoint() == bHadCheckpoint && Character->GetLastActivatedCheckpoint() == LastBefore, TEXT("unchanged"), TEXT("changed"), TEXT("checkpoint"));

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
				Character->ApplyHealthDelta(HealthBefore - Character->GetHealth(), EProjectOrganoidHealthDeltaSource::Generic);
				Proof = EProof::Barrel;
				break;
			}
			case EProof::Barrel:
			{
				UProjectOrganoidWeaponModData* Barrel = UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve();
				UProjectOrganoidWeaponModComponent* ModComp = Weapon->GetWeaponModComponent();
				if (!Barrel || !ModComp)
				{
					FailAndStop(Owner, Record, TEXT("Barrel or mod component missing."));
					return;
				}
				AssertTrue(Record, TEXT("provisional_damage_1_05"), FMath::IsNearlyEqual(Barrel->DamageMultiplier, 1.05f), TEXT("1.05"), FString::SanitizeFloat(Barrel->DamageMultiplier), TEXT("PROVISIONAL DESIGN TUNING"));
				AssertTrue(Record, TEXT("placement_does_not_unlock_barrel"), !Character->IsWeaponModUnlocked(Barrel), TEXT("locked"), TEXT("unlocked"), TEXT("ownership"));

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

				AssertTrue(Record, TEXT("unlock_succeeds"), Character->UnlockWeaponMod(Barrel), TEXT("true"), TEXT("false"), TEXT("ownership"));
				const bool bOpened = Alive->Interact(Character);
				UProjectOrganoidResearchStationWidget* Widget = Alive->GetActiveStationWidget();
				AssertTrue(Record, TEXT("ui_open_for_remount"), bOpened && Widget != nullptr, TEXT("open"), Widget ? TEXT("open") : TEXT("none"), TEXT("ui"));
				AssertTrue(Record, TEXT("install_succeeds"), Widget && Widget->InstallUnlockedMod(Barrel), TEXT("true"), TEXT("false"), TEXT("loadout"));
				AssertTrue(Record, TEXT("install_zero_sot"), CountSot(Character) == SotAtUnlock, FString::FromInt(SotAtUnlock), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				AssertTrue(Record, TEXT("install_mag_unchanged"), Weapon->GetCurrentMagazine() == MagBefore, FString::FromInt(MagBefore), FString::FromInt(Weapon->GetCurrentMagazine()), TEXT("magazine"));
				AssertTrue(Record, TEXT("install_reserve_unchanged"), ReserveOf(Character) == ReserveBefore, FString::FromInt(ReserveBefore), FString::FromInt(ReserveOf(Character)), TEXT("ammo"));
				AssertTrue(Record, TEXT("remove_succeeds"), Widget && Widget->RemoveInstalledMod() && ModComp->GetModInSlot(EProjectOrganoidWeaponModSlot::Barrel) == nullptr, TEXT("removed"), TEXT("failed"), TEXT("loadout"));
				AssertTrue(Record, TEXT("remove_zero_sot"), CountSot(Character) == SotAtUnlock, FString::FromInt(SotAtUnlock), FString::FromInt(CountSot(Character)), TEXT("SOT"));
				if (Widget)
				{
					Widget->CloseStationUI();
				}
				Proof = EProof::Done;
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
			(void)DeltaTime;
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

	struct FNeuroResearchStationPlacementAutoRegister
	{
		FNeuroResearchStationPlacementAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FNeuroResearchStationPlacementFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FNeuroResearchStationPlacementAutoRegister GRegisterNeuroResearchStationPlacement;
}
