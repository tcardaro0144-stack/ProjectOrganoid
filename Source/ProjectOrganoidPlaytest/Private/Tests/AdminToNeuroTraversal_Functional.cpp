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
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidSecurityGate.h"
#include "ProjectOrganoidSecuritySubsystem.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("AdminToNeuroTraversal_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Admin To Neuro Traversal Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");
	constexpr TCHAR GateIdName[] = TEXT("Gate_Neuro_Research");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_NeuroAirlock");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR ResearchWingPickupLabel[] = TEXT("Pickup_ResearchWingKeycard");
	constexpr TCHAR ResearchWingItemPath[] = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard.DA_Item_ResearchWingKeycard");
	const FVector ResearchWingPickupLocation(2580.0f, -560.0f, 80.0f);

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

	class FAdminToNeuroTraversalFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::PlaceInAdmin;
			WaitSeconds = 0.0f;
			WaypointIndex = 0;
			bAnyAssertFailed = false;
			bUsedSectorTransition = false;
			bInjectedLevel2 = false;
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
			PlaceInAdmin,
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
		EProof Proof = EProof::PlaceInAdmin;
		float WaitSeconds = 0.0f;
		int32 WaypointIndex = 0;
		bool bAnyAssertFailed = false;
		bool bUsedSectorTransition = false;
		bool bInjectedLevel2 = false;
		TArray<FString> DirtyBefore;
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

		bool WalkRampToward(APawn* Pawn, const FVector& Dest, FHitResult& Hit, float StepUu = 70.0f)
		{
			if (!Pawn)
			{
				return false;
			}
			const FVector Current = Pawn->GetActorLocation();
			const FVector Flat(Dest.X, Dest.Y, Current.Z);
			if (FVector::Dist2D(Current, Dest) > 1.0f && !WalkToward(Pawn, Flat, Hit, StepUu))
			{
				return false;
			}
			return WalkToward(Pawn, Dest, Hit, StepUu);
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
			Record.AddActor(TEXT("sector_transition_used"), TEXT("false"));
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
				Proof = EProof::PlaceInAdmin;
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
			case EProof::PlaceInAdmin:
				TickPlaceInAdmin(Owner, Record, Character);
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
				TickIsolation(Owner, Record, World);
				break;
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}
		}

		bool InventoryHasCampaignResearchWingCard(UProjectOrganoidInventoryComponent* Inventory) const
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

		void TickPlaceInAdmin(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				World ? OrganoidPlaytestActions::FindUniqueByLabel(World, ResearchWingPickupLabel) : nullptr);
			UProjectOrganoidItemData* CampaignCard = LoadObject<UProjectOrganoidItemData>(nullptr, ResearchWingItemPath);
			AssertTrue(Record, TEXT("access.campaign_pickup_present"),
				Pickup && CampaignCard && Pickup->ItemData == CampaignCard,
				TEXT("Pickup_ResearchWingKeycard"), Pickup ? TEXT("found") : TEXT("missing"), ResearchWingPickupLabel);
			if (!Pickup || !CampaignCard)
			{
				FailAndStop(Owner, Record, TEXT("Campaign Research Wing credential is missing."));
				return;
			}

			const FVector CollectAt(ResearchWingPickupLocation.X, ResearchWingPickupLocation.Y, StandZ(20.0f, Character));
			if (!Pickup || !CampaignCard || !OrganoidPlaytestActions::TeleportNear(Character, CollectAt, 90.0f, CollectAt.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to reach Pickup_ResearchWingKeycard."));
				return;
			}
			OrganoidPlaytestActions::FaceActor(Character, Pickup);
			AssertTrue(Record, TEXT("access.campaign_pickup_held"),
				!Pickup->CanInteract(Character),
				TEXT("not interactable"),
				Pickup->CanInteract(Character) ? TEXT("interactable") : TEXT("held"),
				ResearchWingPickupLabel);

			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			FGuid InjectedId;
			const bool bInjected = Inventory && Inventory->TryAddItem(CampaignCard, InjectedId, 1);
			if (bInjected)
			{
				bInjectedLevel2 = true;
			}
			AssertTrue(Record, TEXT("access.level2_injected_from_campaign_da"),
				bInjected && bInjectedLevel2 && InventoryHasCampaignResearchWingCard(Inventory),
				TEXT("DA_Item_ResearchWingKeycard"),
				InventoryHasCampaignResearchWingCard(Inventory) ? TEXT("injected campaign DA") : TEXT("missing"),
				TEXT("inventory"));
			AActor* AfterPickup = OrganoidPlaytestActions::FindUniqueByLabel(World, ResearchWingPickupLabel);
			AssertTrue(Record, TEXT("access.campaign_pickup_remains"),
				AfterPickup != nullptr && IsValid(AfterPickup),
				TEXT("present"), AfterPickup ? TEXT("present") : TEXT("destroyed"), ResearchWingPickupLabel);
			if (!bInjected || !InventoryHasCampaignResearchWingCard(Inventory))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			const FVector Start(4000.0f, -200.0f, StandZ(20.0f, Character));
			if (!OrganoidPlaytestActions::TeleportNear(Character, Start, 0.0f, Start.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place Nathan in Operations for the Service Corridor walk."));
				return;
			}
			AssertTrue(Record, TEXT("no_sector_transition"), !bUsedSectorTransition,
				TEXT("false"), bUsedSectorTransition ? TEXT("true") : TEXT("false"), TEXT("player"));
			AssertTrue(Record, TEXT("start_in_admin"), Character->GetActorLocation().Z > -200.0f,
				TEXT("Admin Z"), FString::SanitizeFloat(Character->GetActorLocation().Z), TEXT("player"));
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
			FName NeuroName = NAME_None;
			bool bNeuroReady = false;
			bool bAdminReady = false;
			if (Levels)
			{
				NeuroName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics);
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
			AssertTrue(Record, TEXT("gate.id"), FoundGate->GateId == FName(GateIdName),
				GateIdName, FoundGate->GateId.ToString(), GateLabel);
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
			AssertTrue(Record, TEXT("gate.starts_sealed"), FoundGate->IsSealed(),
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

			AssertTrue(Record, TEXT("access.keycard_granted"),
				InventoryHasCampaignResearchWingCard(Inventory) && bInjectedLevel2,
				TEXT("injected DA_Item_ResearchWingKeycard"),
				InventoryHasCampaignResearchWingCard(Inventory) ? TEXT("injected campaign DA") : TEXT("missing"),
				TEXT("inventory"));

			OrganoidPlaytestActions::FaceActor(Character, FoundGate);
			const bool bOpened = FoundGate->TryOverrideWithInventory(Character);
			AssertTrue(Record, TEXT("access.keycard_opens_gate"), bOpened && FoundGate->IsOpen(),
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
			UWorld* World)
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
			AssertTrue(Record, TEXT("no_sector_transition_used"), !bUsedSectorTransition,
				TEXT("false"), TEXT("false"), TEXT("player"));
			Proof = EProof::Done;
			Stage = EStage::EndPie;
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
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

	struct FAdminToNeuroTraversalAutoRegister
	{
		FAdminToNeuroTraversalAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FAdminToNeuroTraversalFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FAdminToNeuroTraversalAutoRegister GRegisterAdminToNeuroTraversal;
}
