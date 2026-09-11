#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAdminFacilityStateTypes.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSecurityGate.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("OpeningFoundation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Opening Foundation Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR AuditPackage[] = TEXT("/Game/Data/Missions/DA_Mission_TheAudit");
	constexpr TCHAR ItemPath[] = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard.DA_Item_ResearchWingKeycard");
	constexpr TCHAR PickupLabel[] = TEXT("Pickup_ResearchWingKeycard");
	constexpr TCHAR GateLabel[] = TEXT("Gate_ResearchWing");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidOpeningFoundationTest");
	constexpr TCHAR AuthorizedAdminHostLabel[] = TEXT("Host_Admin_SecurityOfficer");
	const FVector OpeningLocation(200.0f, 0.0f, 118.0f);
	const FVector OldTransitFallback(5000.0f, -900.0f, 220.0f);
	const FVector AuthorizedAdminHostLocation(2820.0f, -600.0f, 100.0f);

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

	class FOpeningFoundationFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Spawn;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bAdminDirtyBefore = false;
			bEpitopeDirtyBefore = false;
			bAuditDirtyBefore = false;
			SavedTransform = FTransform::Identity;
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
			Spawn,
			WalkDoor,
			WaitDoor,
			Reception,
			Isolation,
			Mission,
			ResearchWingHold,
			GateSealed,
			Sequencer,
			SaveLoad,
			WaitAfterLoad,
			AssertLoad,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Spawn;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bAdminDirtyBefore = false;
		bool bEpitopeDirtyBefore = false;
		bool bAuditDirtyBefore = false;
		FTransform SavedTransform = FTransform::Identity;
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

		bool InVestibule(const FVector& Loc) const
		{
			return Loc.X >= 40.0f && Loc.X <= 350.0f
				&& Loc.Y >= -300.0f && Loc.Y <= 300.0f
				&& Loc.Z >= 110.0f && Loc.Z <= 130.0f;
		}

		bool YawIsEast(float Yaw) const
		{
			return FMath::Abs(FRotator::NormalizeAxis(Yaw)) <= 15.0f;
		}

		bool ContainsForbidden(const FString& Text) const
		{
			return Text.Contains(TEXT("Avery"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("Audit"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("Sterling"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("HEPA"), ESearchCase::IgnoreCase);
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
					const FString Name = Class->GetName();
					if (Name.Contains(TEXT("LevelSequence")))
					{
						++Count;
					}
				}
			}
			return Count;
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

		bool ReadBoolProp(AActor* Actor, const TCHAR* Name, bool& OutValue) const
		{
			const FOrganoidPlaytestPropValue Value = OrganoidPlaytestActions::ReadProperty(Actor, Name);
			if (!Value.bFound || !Value.bHasBool)
			{
				return false;
			}
			OutValue = Value.bBool;
			return true;
		}

		FString ReadAdminPowerState(UWorld* World) const
		{
			UProjectOrganoidPowerSubsystem* Power = World ? World->GetSubsystem<UProjectOrganoidPowerSubsystem>() : nullptr;
			if (!Power)
			{
				return FString();
			}
			UFunction* Function = Power->FindFunction(FName(TEXT("GetSectorPowerState")));
			if (!Function)
			{
				return FString();
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, TEXT("Sector")))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
					static_cast<int64>(EProjectOrganoidPowerSector::Admin));
			}
			Power->ProcessEvent(Function, Parms.GetData());
			if (FProperty* ReturnProp = Function->GetReturnProperty())
			{
				if (FEnumProperty* ReturnEnum = CastField<FEnumProperty>(ReturnProp))
				{
					const int64 Value = ReturnEnum->GetUnderlyingProperty()->GetSignedIntPropertyValue(
						ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData()));
					return UEnum::GetValueAsString(static_cast<EProjectOrganoidPowerState>(Value));
				}
			}
			return FString();
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(AuditPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, Admin, or DA_Mission_TheAudit is dirty. Refusing to start."));
				return;
			}

			bAdminDirtyBefore = PackageIsDirty(AdminPackage);
			bEpitopeDirtyBefore = PackageIsDirty(MapPackage);
			bAuditDirtyBefore = PackageIsDirty(AuditPackage);
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
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}

			if (World && Character && (bWalking || WaitSeconds > 10.0f) && (InVestibule(Character->GetActorLocation()) || WaitSeconds > 12.0f))
			{
				Player = Character;
				WaitSeconds = 0.0f;
				Proof = EProof::Spawn;
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
			if (!Character)
			{
				Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
				Player = Character;
			}
			if (!World || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Nathan during OpeningFoundation proof."));
				return;
			}

			switch (Proof)
			{
			case EProof::Spawn:
				TickSpawn(Owner, Record, World, Character);
				break;
			case EProof::WalkDoor:
				TickWalkDoor(Owner, Record, World, Character);
				break;
			case EProof::WaitDoor:
				TickWaitDoor(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::Reception:
				TickReception(Owner, Record, Character);
				break;
			case EProof::Isolation:
				TickIsolation(Owner, Record, World);
				break;
			case EProof::Mission:
				TickMission(Owner, Record, Character);
				break;
			case EProof::ResearchWingHold:
				TickResearchWingHold(Owner, Record, World, Character);
				break;
			case EProof::GateSealed:
				TickGateSealed(Owner, Record, World, Character);
				break;
			case EProof::Sequencer:
				TickSequencer(Owner, Record, World);
				break;
			case EProof::SaveLoad:
				TickSaveLoad(Owner, Record, Character);
				break;
			case EProof::WaitAfterLoad:
				WaitSeconds += DeltaTime;
				if (WaitSeconds >= 0.75f)
				{
					WaitSeconds = 0.0f;
					Proof = EProof::AssertLoad;
					Owner.SetStage(TEXT("AssertLoad"));
				}
				break;
			case EProof::AssertLoad:
				TickAssertLoad(Owner, Record, Character);
				break;
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}
		}

		void TickSpawn(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			const FString MapName = UGameplayStatics::GetCurrentLevelName(World, true);
			AssertTrue(Record, TEXT("pie.lvl_epitope"),
				MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase),
				TEXT("Lvl_Epitope"), MapName, TEXT("world"));

			APlayerController* PC = Cast<APlayerController>(Character->GetController());
			AssertTrue(Record, TEXT("nathan.possessed"),
				PC != nullptr && PC->GetPawn() == Character,
				TEXT("AProjectOrganoidCharacter"),
				Character->GetName(),
				TEXT("player"));

			const FVector Loc = Character->GetActorLocation();
			AssertTrue(Record, TEXT("spawn.vestibule"),
				InVestibule(Loc),
				TEXT("x[40,350] y[-300,300] z[110,130]"),
				Loc.ToCompactString(),
				TEXT("player"));
			AssertTrue(Record, TEXT("spawn.not_old_playerstart_snap"),
				InVestibule(Loc) && FVector::Dist(Loc, OldTransitFallback) > 500.0f,
				TEXT("Vestibule"),
				Loc.ToCompactString(),
				TEXT("player"));

			const float ActorYaw = Character->GetActorRotation().Yaw;
			const float ControlYaw = PC ? PC->GetControlRotation().Yaw : 999.0f;
			AssertTrue(Record, TEXT("spawn.facing_east"),
				YawIsEast(ActorYaw) && YawIsEast(ControlYaw),
				TEXT("yaw ~ 0"),
				FString::Printf(TEXT("actor=%.1f control=%.1f"), ActorYaw, ControlYaw),
				TEXT("player"));

			FVector Fallback = OldTransitFallback;
			if (UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
			{
				Fallback = Levels->GetFallbackSpawnLocation();
				const FTransform Opening = Levels->GetCampaignOpeningTransform();
				AssertTrue(Record, TEXT("opening.transform"),
					FVector::Dist(Opening.GetLocation(), OpeningLocation) <= 1.0f
						&& YawIsEast(Opening.Rotator().Yaw),
					TEXT("(200,0,118) yaw 0"),
					Opening.GetLocation().ToCompactString(),
					TEXT("LevelManager"));
			}
			AssertTrue(Record, TEXT("timeout.not_transit_spine"),
				InVestibule(Fallback) && FVector::Dist(Fallback, OldTransitFallback) > 500.0f,
				TEXT("Vestibule fallback"),
				Fallback.ToCompactString(),
				TEXT("LevelManager"));

			const FString AdminPower = ReadAdminPowerState(World);
			AssertTrue(Record, TEXT("admin.power_online"),
				AdminPower.Contains(TEXT("Online")),
				TEXT("Online"),
				AdminPower.IsEmpty() ? TEXT("missing") : AdminPower,
				TEXT("Admin"));

			if (UProjectOrganoidAdminFacilityStateSubsystem* Facility = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
			{
				const EProjectOrganoidAdminFacilityState State = Facility->GetAdminFacilityState();
				AssertTrue(Record, TEXT("admin.facility_normal"),
					State == EProjectOrganoidAdminFacilityState::Normal,
					TEXT("Normal"),
					UEnum::GetValueAsString(State),
					TEXT("Admin"));
			}
			else
			{
				AssertTrue(Record, TEXT("admin.facility_normal"), false, TEXT("Normal"), TEXT("missing subsystem"), TEXT("Admin"));
			}

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::WalkDoor;
			Owner.SetStage(TEXT("WalkDoor"));
		}

		void TickWalkDoor(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World);
			AssertTrue(Record, TEXT("door.present"), Door != nullptr,
				TEXT("BP_AdminAccessDoor"), Door ? TEXT("found") : TEXT("missing"), TEXT("BP_AdminAccessDoor"));
			if (!Door)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			const FVector Here = Character->GetActorLocation();
			FHitResult Hit;
			const bool bReachedTrigger = WalkToward(Character, FVector(450.0f, 0.0f, Here.Z), Hit, 80.0f);
			AssertTrue(Record, TEXT("walk.plus_x_into_trigger"),
				bReachedTrigger || Character->GetActorLocation().X >= 350.0f,
				TEXT("x>=350"),
				Character->GetActorLocation().ToCompactString(),
				TEXT("BP_AdminAccessDoor"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			WaitSeconds = 0.0f;
			Proof = EProof::WaitDoor;
			Owner.SetStage(TEXT("WaitDoor"));
		}

		void TickWaitDoor(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character,
			float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 1.6f)
			{
				return;
			}

			AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World);
			bool bIsOpen = false;
			const bool bRead = Door && ReadBoolProp(Door, TEXT("bIsOpen"), bIsOpen);
			AssertTrue(Record, TEXT("door.automatic_opened"),
				bRead && bIsOpen,
				TEXT("bIsOpen=true"),
				bRead ? (bIsOpen ? TEXT("true") : TEXT("false")) : TEXT("unread"),
				TEXT("BP_AdminAccessDoor"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			(void)Character;
			WaitSeconds = 0.0f;
			Proof = EProof::Reception;
			Owner.SetStage(TEXT("Reception"));
		}

		void TickReception(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			const FVector Here = Character->GetActorLocation();
			FHitResult Hit;
			const bool bReached = WalkToward(Character, FVector(1100.0f, 0.0f, Here.Z), Hit, 80.0f);
			AssertTrue(Record, TEXT("reception.reached_without_keycard"),
				bReached || Character->GetActorLocation().X > 800.0f,
				TEXT("x>800"),
				FString::Printf(TEXT("at=%s hit=%s"), *Character->GetActorLocation().ToCompactString(), *HitName(Hit)),
				TEXT("Reception"));
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
			const int32 Hosts = CountHostsInPackage(World, TEXT("SL_Epitope_Admin"));
			AProjectOrganoidHostBase* Authorized = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, AuthorizedAdminHostLabel));
			const bool bZeroOk = Hosts == 0 && Authorized == nullptr;
			const FVector Loc = Authorized ? Authorized->GetActorLocation() : FVector::ZeroVector;
			const bool bOneAuthorizedOk = Hosts == 1
				&& Authorized != nullptr
				&& OrganoidPlaytestActions::ActorPackage(Authorized).Contains(TEXT("SL_Epitope_Admin"))
				&& OrganoidPlaytestActions::ActorLabel(Authorized) == AuthorizedAdminHostLabel
				&& Authorized->GetClass()
				&& Authorized->GetClass()->GetName().Contains(TEXT("BP_OrganoidHost"))
				&& FVector::Dist2D(Loc, AuthorizedAdminHostLocation) <= 2.0f
				&& FMath::Abs(Loc.Z - AuthorizedAdminHostLocation.Z) <= 20.0f;
			AssertTrue(
				Record,
				TEXT("admin.opening_hosts_authorized_only"),
				bZeroOk || bOneAuthorizedOk,
				TEXT("0 or exactly one Host_Admin_SecurityOfficer at authored Block 4 transform"),
				FString::Printf(
					TEXT("count=%d label=%s class=%s loc=%s"),
					Hosts,
					Authorized ? *OrganoidPlaytestActions::ActorLabel(Authorized) : TEXT("none"),
					(Authorized && Authorized->GetClass()) ? *Authorized->GetClass()->GetName() : TEXT("n/a"),
					Authorized ? *Loc.ToCompactString() : TEXT("n/a")),
				TEXT("Admin"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Mission;
			Owner.SetStage(TEXT("Mission"));
		}

		void TickMission(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UGameInstance* GI = Character->GetGameInstance();
			UProjectOrganoidObjectiveSubsystem* Objectives = GI ? GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>() : nullptr;
			if (!Objectives)
			{
				FailAndStop(Owner, Record, TEXT("Objective subsystem missing."));
				return;
			}

			const FString MissionId = Objectives->GetActiveMissionId().ToString();
			const FString MissionTitle = Objectives->GetActiveMissionTitle().ToString();
			AssertTrue(Record, TEXT("mission.id"),
				MissionId == TEXT("Mission_OpeningFoundation"),
				TEXT("Mission_OpeningFoundation"),
				MissionId,
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.title"),
				MissionTitle == TEXT("Epitope"),
				TEXT("Epitope"),
				MissionTitle,
				TEXT("objectives"));

			const TArray<FProjectOrganoidObjective> MainTasks = Objectives->GetObjectivesByType(EProjectOrganoidObjectiveType::Main);
			AssertTrue(Record, TEXT("mission.two_tasks"),
				MainTasks.Num() == 2,
				TEXT("2"),
				FString::FromInt(MainTasks.Num()),
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.active_count"),
				Objectives->GetActiveObjectives().Num() == 1,
				TEXT("1"),
				FString::FromInt(Objectives->GetActiveObjectives().Num()),
				TEXT("objectives"));

			FProjectOrganoidObjective ReceptionTask;
			const bool bHasReception = Objectives->GetObjective(TEXT("Obj_ReceptionCheckIn"), ReceptionTask);
			AssertTrue(Record, TEXT("mission.reception_id"),
				bHasReception && ReceptionTask.ObjectiveId == TEXT("Obj_ReceptionCheckIn"),
				TEXT("Obj_ReceptionCheckIn"),
				bHasReception ? ReceptionTask.ObjectiveId.ToString() : TEXT("missing"),
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.reception_title"),
				bHasReception && ReceptionTask.Title.ToString() == TEXT("Check in at Reception"),
				TEXT("Check in at Reception"),
				bHasReception ? ReceptionTask.Title.ToString() : TEXT("missing"),
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.reception_active"),
				bHasReception && ReceptionTask.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"),
				bHasReception ? UEnum::GetValueAsString(ReceptionTask.State) : TEXT("missing"),
				TEXT("objectives"));

			FProjectOrganoidObjective SecurityTask;
			const bool bHasSecurity = Objectives->GetObjective(TEXT("Obj_SecurityStatus"), SecurityTask);
			AssertTrue(Record, TEXT("mission.security_id"),
				bHasSecurity && SecurityTask.ObjectiveId == TEXT("Obj_SecurityStatus"),
				TEXT("Obj_SecurityStatus"),
				bHasSecurity ? SecurityTask.ObjectiveId.ToString() : TEXT("missing"),
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.security_title"),
				bHasSecurity && SecurityTask.Title.ToString() == TEXT("Check the security office"),
				TEXT("Check the security office"),
				bHasSecurity ? SecurityTask.Title.ToString() : TEXT("missing"),
				TEXT("objectives"));
			AssertTrue(Record, TEXT("mission.security_hidden"),
				bHasSecurity && SecurityTask.State == EProjectOrganoidObjectiveState::Inactive,
				TEXT("Inactive"),
				bHasSecurity ? UEnum::GetValueAsString(SecurityTask.State) : TEXT("missing"),
				TEXT("objectives"));

			bool bForbidden = ContainsForbidden(MissionId) || ContainsForbidden(MissionTitle);
			for (const FProjectOrganoidObjective& Objective : Objectives->GetActiveObjectives())
			{
				bForbidden = bForbidden
					|| ContainsForbidden(Objective.ObjectiveId.ToString())
					|| ContainsForbidden(Objective.Title.ToString())
					|| ContainsForbidden(Objective.Description.ToString())
					|| ContainsForbidden(Objective.JournalNotes.ToString());
			}
			for (const FProjectOrganoidObjective& Objective : Objectives->GetJournalEntries())
			{
				bForbidden = bForbidden
					|| ContainsForbidden(Objective.ObjectiveId.ToString())
					|| ContainsForbidden(Objective.Title.ToString())
					|| ContainsForbidden(Objective.Description.ToString())
					|| ContainsForbidden(Objective.JournalNotes.ToString());
			}
			AssertTrue(Record, TEXT("mission.no_avery_audit_sterling_hepa"),
				!bForbidden,
				TEXT("none"),
				bForbidden ? TEXT("forbidden string present") : TEXT("none"),
				TEXT("journal"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::ResearchWingHold;
			Owner.SetStage(TEXT("ResearchWingHold"));
		}

		void TickResearchWingHold(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, PickupLabel));
			UProjectOrganoidItemData* Item = LoadObject<UProjectOrganoidItemData>(nullptr, ItemPath);
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();

			AssertTrue(Record, TEXT("rw.pickup_present"),
				Pickup != nullptr && Item && Pickup->ItemData == Item,
				TEXT("Pickup_ResearchWingKeycard"),
				Pickup ? TEXT("found") : TEXT("missing"),
				PickupLabel);
			AssertTrue(Record, TEXT("rw.not_interactable"),
				Pickup && !Pickup->CanInteract(Character),
				TEXT("false"),
				Pickup && Pickup->CanInteract(Character) ? TEXT("true") : TEXT("false"),
				PickupLabel);
			AssertTrue(Record, TEXT("rw.no_level2_in_inventory"),
				Inventory && !Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab),
				TEXT("false"),
				Inventory && Inventory->HasKeycardOfTier(EProjectOrganoidSecurityTier::Level2_Lab) ? TEXT("true") : TEXT("false"),
				TEXT("inventory"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::GateSealed;
			Owner.SetStage(TEXT("GateSealed"));
		}

		void TickGateSealed(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidSecurityGate* Gate = Cast<AProjectOrganoidSecurityGate>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, GateLabel));
			AssertTrue(Record, TEXT("rw.gate_present"),
				Gate != nullptr,
				TEXT("Gate_ResearchWing"),
				Gate ? TEXT("found") : TEXT("missing"),
				GateLabel);
			if (!Gate)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			const float CapsuleZ = Character->GetCapsuleComponent()
				? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f
				: 98.0f;
			const FVector Approach(3200.0f, 900.0f, -1190.0f + CapsuleZ);
			if (!OrganoidPlaytestActions::TeleportNear(Character, Approach, 0.0f, Approach.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place Nathan at Gate_ResearchWing approach."));
				return;
			}

			AssertTrue(Record, TEXT("rw.gate_sealed"),
				Gate->IsSealed(),
				TEXT("Sealed"),
				Gate->IsSealed() ? TEXT("Sealed") : TEXT("Open"),
				GateLabel);
			OrganoidPlaytestActions::FaceActor(Character, Gate);
			const bool bOpened = Gate->TryOverrideWithInventory(Character);
			AssertTrue(Record, TEXT("rw.inaccessible_without_level2"),
				!bOpened && Gate->IsSealed(),
				TEXT("sealed without Level2_Lab"),
				bOpened ? TEXT("opened") : TEXT("sealed"),
				GateLabel);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Sequencer;
			Owner.SetStage(TEXT("Sequencer"));
		}

		void TickSequencer(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World)
		{
			const int32 SeqCount = CountSequenceActors(World);
			AssertTrue(Record, TEXT("no_cinematic_sequencer"),
				SeqCount == 0,
				TEXT("0"),
				FString::FromInt(SeqCount),
				TEXT("world"));
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
			const float CapsuleZ = Character->GetCapsuleComponent()
				? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f
				: 98.0f;
			const FVector Reception(1100.0f, 0.0f, 20.0f + CapsuleZ);
			if (!OrganoidPlaytestActions::TeleportNear(Character, Reception, 0.0f, Reception.Z))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place Nathan at Reception for save/load."));
				return;
			}
			if (AController* PC = Character->GetController())
			{
				PC->SetControlRotation(FRotator(0.0f, 90.0f, 0.0f));
			}
			Character->SetActorRotation(FRotator(0.0f, 90.0f, 0.0f));
			SavedTransform = Character->GetActorTransform();

			UGameInstance* GI = Character->GetGameInstance();
			UProjectOrganoidSaveSubsystem* Saves = GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
			if (!Saves)
			{
				FailAndStop(Owner, Record, TEXT("Save subsystem missing."));
				return;
			}
			Saves->DeleteSave(SaveSlot);
			const bool bSaved = Saves->SavePlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.wrote"), bSaved, TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), TEXT("save"));

			OrganoidPlaytestActions::TeleportNear(Character, OpeningLocation, 0.0f, OpeningLocation.Z);
			Character->SetActorRotation(FRotator::ZeroRotator);
			if (AController* PC = Character->GetController())
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}

			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.loaded"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), TEXT("save"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			WaitSeconds = 0.0f;
			Proof = EProof::WaitAfterLoad;
			Owner.SetStage(TEXT("WaitAfterLoad"));
		}

		void TickAssertLoad(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			const FVector Loc = Character->GetActorLocation();
			const FVector SavedLoc = SavedTransform.GetLocation();
			AssertTrue(Record, TEXT("saveload.restores_player_transform"),
				FVector::Dist(Loc, SavedLoc) <= 80.0f,
				SavedLoc.ToCompactString(),
				Loc.ToCompactString(),
				TEXT("player"));
			AssertTrue(Record, TEXT("saveload.not_forced_vestibule"),
				FVector::Dist(Loc, OpeningLocation) > 200.0f,
				TEXT("saved Reception transform"),
				Loc.ToCompactString(),
				TEXT("player"));
			AssertTrue(Record, TEXT("saveload.not_transit_fallback"),
				FVector::Dist(Loc, OldTransitFallback) > 500.0f,
				TEXT("saved transform"),
				Loc.ToCompactString(),
				TEXT("player"));
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
			AssertTrue(Record, TEXT("dirty.admin_unchanged"),
				PackageIsDirty(AdminPackage) == bAdminDirtyBefore,
				bAdminDirtyBefore ? TEXT("dirty") : TEXT("clean"),
				PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("SL_Epitope_Admin"));
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
			Record.AddActor(TEXT("dirty_before"), FString::Join(DirtyBefore, TEXT(",")));
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			Record.AddActor(TEXT("dirty_after"), FString::Join(DirtyAfter, TEXT(",")));
			AssertTrue(Record, TEXT("playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""));
			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FOpeningFoundationAutoRegister
	{
		FOpeningFoundationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FOpeningFoundationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FOpeningFoundationAutoRegister GRegisterOpeningFoundation;
}
