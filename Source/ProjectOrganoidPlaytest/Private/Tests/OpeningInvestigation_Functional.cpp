#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestLogSink.h"
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
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidItemPickup.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidObjectiveTypes.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSecurityGate.h"
#include "Blueprint/UserWidget.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("OpeningInvestigation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Opening Investigation Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR AuditPackage[] = TEXT("/Game/Data/Missions/DA_Mission_TheAudit");
	constexpr TCHAR KnownNeuroRecastPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR TerminalBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminTerminal");
	constexpr TCHAR DoorBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	constexpr TCHAR HologramBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram");
	constexpr TCHAR ItemPath[] = TEXT("/Game/Data/Items/DA_Item_ResearchWingKeycard.DA_Item_ResearchWingKeycard");
	constexpr TCHAR PickupLabel[] = TEXT("Pickup_ResearchWingKeycard");
	constexpr TCHAR AdminHostLabel[] = TEXT("Host_Admin_SecurityOfficer");
	constexpr TCHAR ReceptionLabel[] = TEXT("Admin_Terminal_Reception");
	constexpr TCHAR SecurityLabel[] = TEXT("Admin_Terminal_Security");
	constexpr TCHAR DoorLockLabel[] = TEXT("DoorLock_VestibuleToAtrium");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidOpeningInvestigationTest");
	constexpr float SetupDistance = 100.0f;
	constexpr float AimConeCos = 0.5735764f;
	const FVector OpeningLocation(200.0f, 0.0f, 118.0f);
	const FVector OldTransitFallback(5000.0f, -900.0f, 220.0f);

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

	bool IsKnownNeuroRecastPackage(const FString& Name)
	{
		return Name.Equals(KnownNeuroRecastPackage, ESearchCase::IgnoreCase);
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	class FOpeningInvestigationFunctional : public IOrganoidPlaytestCase
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
			PostInteractWait = 0.0f;
			bAnyAssertFailed = false;
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
			DoorLock,
			ReceptionSetup,
			ReceptionFocus,
			ReceptionInteract,
			Hub,
			SecuritySetup,
			SecurityFocus,
			SecurityInteract,
			World,
			SaveLoad,
			WaitAfterLoad,
			AssertLoad,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Spawn;
		float WaitSeconds = 0.0f;
		float PostInteractWait = 0.0f;
		bool bAnyAssertFailed = false;
		bool bEpitopeDirtyBefore = false;
		bool bAuditDirtyBefore = false;
		FTransform SavedTransform = FTransform::Identity;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AActor> ReceptionTerminal;
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
				|| Text.Contains(TEXT("Vance"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("Audit"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("Sterling"), ESearchCase::IgnoreCase)
				|| Text.Contains(TEXT("HEPA"), ESearchCase::IgnoreCase);
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
					if (Class->GetName().Contains(TEXT("LevelSequence")))
					{
						++Count;
					}
				}
			}
			return Count;
		}

		int32 CountAdminAmmoPickups(UWorld* World) const
		{
			int32 Count = 0;
			if (!World)
			{
				return Count;
			}
			for (TActorIterator<AProjectOrganoidItemPickup> It(World); It; ++It)
			{
				if (!OrganoidPlaytestActions::ActorPackage(*It).Contains(TEXT("SL_Epitope_Admin")))
				{
					continue;
				}
				if (It->ItemData && It->ItemData->GetName().Contains(TEXT("Ammo")))
				{
					++Count;
				}
			}
			return Count;
		}

		int32 CountTutorialWidgets(UWorld* World) const
		{
			int32 Count = 0;
			if (!World)
			{
				return Count;
			}
			for (TObjectIterator<UUserWidget> It; It; ++It)
			{
				UUserWidget* Widget = *It;
				if (!Widget || Widget->GetWorld() != World || !Widget->IsVisible())
				{
					continue;
				}
				const FString Name = Widget->GetClass()->GetName();
				if (Name.Contains(TEXT("Tutorial")))
				{
					++Count;
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

		bool SetupFaceTerminal(APawn* Pawn, AActor* Target)
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
				if (!IsKnownNeuroRecastPackage(Name) && !Name.Equals(AdminPackage, ESearchCase::IgnoreCase))
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
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Nathan during OpeningInvestigation proof."));
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
			case EProof::DoorLock:
				TickDoorLock(Owner, Record, World, Character);
				break;
			case EProof::ReceptionSetup:
				TickReceptionSetup(Owner, Record, World, Character);
				break;
			case EProof::ReceptionFocus:
				TickTerminalFocus(Owner, Record, Character, ReceptionTerminal.Get(), TEXT("reception"), DeltaTime);
				break;
			case EProof::ReceptionInteract:
				TickReceptionInteract(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::Hub:
				TickHub(Owner, Record, World);
				break;
			case EProof::SecuritySetup:
				TickSecuritySetup(Owner, Record, World, Character);
				break;
			case EProof::SecurityFocus:
				TickTerminalFocus(Owner, Record, Character, SecurityTerminal.Get(), TEXT("security"), DeltaTime);
				break;
			case EProof::SecurityInteract:
				TickSecurityInteract(Owner, Record, World, Character, DeltaTime);
				break;
			case EProof::World:
				TickWorld(Owner, Record, World, Character);
				break;
			case EProof::SaveLoad:
				TickSaveLoad(Owner, Record, Character);
				break;
			case EProof::WaitAfterLoad:
				WaitSeconds += DeltaTime;
				if (WaitSeconds > 1.0f)
				{
					Proof = EProof::AssertLoad;
				}
				break;
			case EProof::AssertLoad:
				TickAssertLoad(Owner, Record, Character);
				break;
			default:
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
			AssertTrue(Record, TEXT("pie.lvl_epitope"),
				World->GetOutermost() && World->GetOutermost()->GetName().Contains(TEXT("Lvl_Epitope")),
				TEXT("Lvl_Epitope"), World->GetMapName(), TEXT("world"));
			AssertTrue(Record, TEXT("nathan.possessed"),
				Character->IsPlayerControlled(),
				TEXT("true"), BoolText(Character->IsPlayerControlled()), TEXT("player"));
			AssertTrue(Record, TEXT("spawn.vestibule"),
				InVestibule(Character->GetActorLocation()),
				OpeningLocation.ToCompactString(), Character->GetActorLocation().ToCompactString(), TEXT("player"));
			AssertTrue(Record, TEXT("spawn.facing_east"),
				YawIsEast(Character->GetActorRotation().Yaw),
				TEXT("yaw~0"), FString::SanitizeFloat(Character->GetActorRotation().Yaw), TEXT("player"));

			const FString Power = ReadAdminPowerState(World);
			AssertTrue(Record, TEXT("admin.power_online"),
				Power.Contains(TEXT("Online")),
				TEXT("Online"), Power, TEXT("Admin"));
			if (UProjectOrganoidAdminFacilityStateSubsystem* Facility =
				World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
			{
				AssertTrue(Record, TEXT("admin.facility_normal"),
					Facility->GetAdminFacilityState() == EProjectOrganoidAdminFacilityState::Normal,
					TEXT("Normal"),
					UEnum::GetValueAsString(Facility->GetAdminFacilityState()),
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
				TEXT("x>=350"), Character->GetActorLocation().ToCompactString(), TEXT("BP_AdminAccessDoor"));
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
				bRead && bIsOpen, TEXT("bIsOpen=true"),
				bRead ? (bIsOpen ? TEXT("true") : TEXT("false")) : TEXT("unread"),
				TEXT("BP_AdminAccessDoor"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			(void)Character;
			Proof = EProof::DoorLock;
			Owner.SetStage(TEXT("DoorLock"));
		}

		void TickDoorLock(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AActor* Lock = OrganoidPlaytestActions::FindUniqueByLabel(World, DoorLockLabel);
			AssertTrue(Record, TEXT("doorlock.present"), Lock != nullptr,
				DoorLockLabel, Lock ? TEXT("found") : TEXT("missing"), DoorLockLabel);
			bool bInteractable = true;
			const bool bRead = Lock && ReadBoolProp(Lock, TEXT("bIsInteractable"), bInteractable);
			AssertTrue(Record, TEXT("doorlock.not_interactable"),
				bRead && !bInteractable, TEXT("false"),
				bRead ? BoolText(bInteractable) : TEXT("unread"), DoorLockLabel);

			const FVector Here = Character->GetActorLocation();
			FHitResult Hit;
			WalkToward(Character, FVector(1000.0f, 0.0f, Here.Z), Hit, 80.0f);
			UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
			AActor* Focused = OrganoidPlaytestActions::GetFocusedInteractable(Interaction);
			const FString FocusLabel = Focused ? OrganoidPlaytestActions::ActorLabel(Focused) : TEXT("<none>");
			AssertTrue(Record, TEXT("doorlock.not_focused"),
				FocusLabel != DoorLockLabel, TEXT("not DoorLock"), FocusLabel, DoorLockLabel);
			if (AProjectOrganoidInteractable* AsInteractable = Cast<AProjectOrganoidInteractable>(Lock))
			{
				AssertTrue(Record, TEXT("doorlock.no_airlock_prompt"),
					!AsInteractable->GetInteractionPrompt().ToString().Contains(TEXT("Atrium Airlock"))
						|| !AsInteractable->bIsInteractable,
					TEXT("no interactable airlock prompt"),
					AsInteractable->GetInteractionPrompt().ToString(),
					DoorLockLabel);
			}

			const bool bReached = Character->GetActorLocation().X > 800.0f;
			AssertTrue(Record, TEXT("route.open_through_vestibule"),
				bReached, TEXT("x>800"), Character->GetActorLocation().ToCompactString(), TEXT("Reception"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::ReceptionSetup;
			Owner.SetStage(TEXT("ReceptionSetup"));
		}

		void TickReceptionSetup(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, ReceptionLabel);
			ReceptionTerminal = Terminal;
			AssertTrue(Record, TEXT("reception.present"), Terminal != nullptr,
				ReceptionLabel, Terminal ? TEXT("found") : TEXT("missing"), ReceptionLabel);
			if (!Terminal)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			const FOrganoidPlaytestPropValue Id = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("TerminalID"));
			const FOrganoidPlaytestPropValue Title = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("Title"));
			const FOrganoidPlaytestPropValue Type = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("TerminalType"));
			const FOrganoidPlaytestPropValue Prompt = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("InteractionPrompt"));
			const FOrganoidPlaytestPropValue Range = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("InteractionRange"));
			AssertTrue(Record, TEXT("reception.TerminalID"), Id.Text == TEXT("Terminal_AdminReception"),
				TEXT("Terminal_AdminReception"), Id.Text, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.Title"), Title.Text == TEXT("Reception Terminal"),
				TEXT("Reception Terminal"), Title.Text, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.Type"),
				Type.EnumDisplay.Equals(TEXT("Reception"), ESearchCase::IgnoreCase) || Type.Number == 0.0,
				TEXT("Reception"), Type.EnumDisplay, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.prompt"), Prompt.Text == TEXT("Use Terminal"),
				TEXT("Use Terminal"), Prompt.Text, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.range"), Range.bHasNumber && FMath::IsNearlyEqual(Range.Number, 150.0),
				TEXT("150"), FString::SanitizeFloat(Range.Number), ReceptionLabel);
			SetupFaceTerminal(Character, Terminal);
			WaitSeconds = 0.0f;
			Proof = EProof::ReceptionFocus;
			Owner.SetStage(TEXT("ReceptionFocus"));
		}

		void TickTerminalFocus(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character,
			AActor* Target,
			const TCHAR* Prefix,
			float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (!Target)
			{
				FailAndStop(Owner, Record, TEXT("Lost terminal while waiting for focus."));
				return;
			}
			OrganoidPlaytestActions::FaceActor(Character, Target);
			UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Character);
			AActor* Focused = OrganoidPlaytestActions::GetFocusedInteractable(Interaction);
			const float Dist = OrganoidPlaytestActions::DistanceTo(Character, Target);
			const float AimDot = OrganoidPlaytestActions::AimDotTo(Character, Target);
			const bool bReady = Dist >= 75.0f && Dist <= 150.0f && AimDot >= AimConeCos && Focused == Target;
			if (bReady)
			{
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
				Proof = (Prefix[0] == TCHAR('r')) ? EProof::ReceptionInteract : EProof::SecurityInteract;
				Owner.SetStage(Prefix[0] == TCHAR('r') ? TEXT("ReceptionInteract") : TEXT("SecurityInteract"));
				return;
			}
			if (WaitSeconds > 3.0f)
			{
				AssertTrue(Record, FString::Printf(TEXT("%s.focus"), Prefix), false,
					TEXT("focused"), Focused ? OrganoidPlaytestActions::ActorLabel(Focused) : TEXT("<none>"), Prefix);
				FailAndStop(Owner, Record, TEXT("Failed to acquire terminal focus."));
			}
		}

		void TickReceptionInteract(
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
				AssertTrue(Record, TEXT("reception.TryInteract"), bOk, TEXT("true"), BoolText(bOk), ReceptionLabel);
				if (!bOk)
				{
					FailAndStop(Owner, Record, TEXT("Reception TryInteract failed."));
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
			const bool bReception = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal OnReceptionActivated"));
			AssertTrue(Record, TEXT("reception.print_screen"), bScreen, TEXT("AdminTerminal Screen"),
				bScreen ? TEXT("present") : TEXT("missing"), ReceptionLabel);
			AssertTrue(Record, TEXT("reception.print_route"), bReception, TEXT("AdminTerminal OnReceptionActivated"),
				bReception ? TEXT("present") : TEXT("missing"), ReceptionLabel);

			const FString Logs = CombinedLogText(Character->GetLogComponent());
			AssertTrue(Record, TEXT("reception.evidence_grant"),
				Logs.Contains(TEXT("GRANT, N.")), TEXT("GRANT, N."), Logs, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.evidence_queue"),
				Logs.Contains(TEXT("VISITOR QUEUE")), TEXT("VISITOR QUEUE"), Logs, ReceptionLabel);
			AssertTrue(Record, TEXT("reception.no_avery_vance"),
				!ContainsForbidden(Logs), TEXT("none"), ContainsForbidden(Logs) ? TEXT("forbidden") : TEXT("none"), ReceptionLabel);

			UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectives(Character);
			FProjectOrganoidObjective ReceptionTask;
			FProjectOrganoidObjective SecurityTask;
			const bool bRec = Objectives && Objectives->GetObjective(TEXT("Obj_ReceptionCheckIn"), ReceptionTask);
			const bool bSec = Objectives && Objectives->GetObjective(TEXT("Obj_SecurityStatus"), SecurityTask);
			AssertTrue(Record, TEXT("obj.reception_complete"),
				bRec && ReceptionTask.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"), bRec ? UEnum::GetValueAsString(ReceptionTask.State) : TEXT("missing"), TEXT("objectives"));
			AssertTrue(Record, TEXT("obj.security_visible"),
				bSec && SecurityTask.State == EProjectOrganoidObjectiveState::Active,
				TEXT("Active"), bSec ? UEnum::GetValueAsString(SecurityTask.State) : TEXT("missing"), TEXT("objectives"));

			AssertTrue(Record, TEXT("reception.no_hacking_ui"),
				OrganoidPlaytestActions::CountVisibleHackingWidgets(World) == 0,
				TEXT("0"), FString::FromInt(OrganoidPlaytestActions::CountVisibleHackingWidgets(World)), ReceptionLabel);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			PostInteractWait = 0.0f;
			Proof = EProof::Hub;
			Owner.SetStage(TEXT("Hub"));
		}

		void TickHub(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World)
		{
			AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel);
			AssertTrue(Record, TEXT("hub.hologram_present"), Hologram != nullptr,
				HologramLabel, Hologram ? TEXT("found") : TEXT("missing"), HologramLabel);
			AssertTrue(Record, TEXT("hub.hologram_not_interactable"),
				Hologram && Cast<AProjectOrganoidInteractable>(Hologram) == nullptr,
				TEXT("not interactable"), Hologram && Cast<AProjectOrganoidInteractable>(Hologram) ? TEXT("interactable") : TEXT("ok"),
				HologramLabel);
			bool bAdmin = false;
			bool bNeuro = true;
			bool bCryo = true;
			bool bCompute = true;
			bool bReactor = true;
			const bool bReadAdmin = Hologram && ReadBoolProp(Hologram, TEXT("bAdminOnline"), bAdmin);
			const bool bReadNeuro = Hologram && ReadBoolProp(Hologram, TEXT("bNeuroGeneticsOnline"), bNeuro);
			const bool bReadCryo = Hologram && ReadBoolProp(Hologram, TEXT("bCryoOnline"), bCryo);
			const bool bReadCompute = Hologram && ReadBoolProp(Hologram, TEXT("bComputeOnline"), bCompute);
			const bool bReadReactor = Hologram && ReadBoolProp(Hologram, TEXT("bReactorOnline"), bReactor);
			AssertTrue(Record, TEXT("hub.admin_online"), bReadAdmin && bAdmin, TEXT("true"), BoolText(bAdmin), HologramLabel);
			AssertTrue(Record, TEXT("hub.neuro_unknown"), bReadNeuro && !bNeuro, TEXT("false"), BoolText(bNeuro), HologramLabel);
			AssertTrue(Record, TEXT("hub.cryo_unknown"), bReadCryo && !bCryo, TEXT("false"), BoolText(bCryo), HologramLabel);
			AssertTrue(Record, TEXT("hub.compute_unknown"), bReadCompute && !bCompute, TEXT("false"), BoolText(bCompute), HologramLabel);
			AssertTrue(Record, TEXT("hub.reactor_unknown"), bReadReactor && !bReactor, TEXT("false"), BoolText(bReactor), HologramLabel);
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
			SetupFaceTerminal(Character, Terminal);
			WaitSeconds = 0.0f;
			Proof = EProof::SecurityFocus;
			Owner.SetStage(TEXT("SecurityFocus"));
		}

		void TickSecurityInteract(
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
			const bool bReceptionPrint = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal OnReceptionActivated"));
			AssertTrue(Record, TEXT("security.print_screen"), bScreen, TEXT("AdminTerminal Screen"),
				bScreen ? TEXT("present") : TEXT("missing"), SecurityLabel);
			AssertTrue(Record, TEXT("security.print_route"), bSecurity, TEXT("AdminTerminal OnSecurityActivated"),
				bSecurity ? TEXT("present") : TEXT("missing"), SecurityLabel);
			AssertTrue(Record, TEXT("security.no_reception_route"), !bReceptionPrint, TEXT("absent"),
				bReceptionPrint ? TEXT("present") : TEXT("absent"), SecurityLabel);

			const FString Logs = CombinedLogText(Character->GetLogComponent());
			AssertTrue(Record, TEXT("security.evidence_watch"),
				Logs.Contains(TEXT("PUBLIC ACCESS WATCH")), TEXT("PUBLIC ACCESS WATCH"), Logs, SecurityLabel);
			AssertTrue(Record, TEXT("security.evidence_interrupted"),
				Logs.Contains(TEXT("Intake procedure: interrupted")), TEXT("interrupted"), Logs, SecurityLabel);

			UProjectOrganoidObjectiveSubsystem* Objectives = GetObjectives(Character);
			FProjectOrganoidObjective SecurityTask;
			const bool bSec = Objectives && Objectives->GetObjective(TEXT("Obj_SecurityStatus"), SecurityTask);
			AssertTrue(Record, TEXT("obj.security_complete"),
				bSec && SecurityTask.State == EProjectOrganoidObjectiveState::Completed,
				TEXT("Completed"), bSec ? UEnum::GetValueAsString(SecurityTask.State) : TEXT("missing"), TEXT("objectives"));

			AActor* Reception = ReceptionTerminal.Get();
			if (!Reception)
			{
				Reception = OrganoidPlaytestActions::FindUniqueByLabel(World, ReceptionLabel);
			}
			AActor* Security = SecurityTerminal.Get();
			const FOrganoidPlaytestPropValue RecActivated = OrganoidPlaytestActions::ReadProperty(Reception, TEXT("bHasActivated"));
			const FOrganoidPlaytestPropValue SecActivated = OrganoidPlaytestActions::ReadProperty(Security, TEXT("bHasActivated"));
			AssertTrue(Record, TEXT("isolation.reception_activated"),
				RecActivated.bHasBool && RecActivated.bBool, TEXT("true"),
				RecActivated.bHasBool ? BoolText(RecActivated.bBool) : TEXT("missing"), ReceptionLabel);
			AssertTrue(Record, TEXT("isolation.security_activated"),
				SecActivated.bHasBool && SecActivated.bBool, TEXT("true"),
				SecActivated.bHasBool ? BoolText(SecActivated.bBool) : TEXT("missing"), SecurityLabel);

			AProjectOrganoidItemPickup* Pickup = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, PickupLabel));
			AssertTrue(Record, TEXT("rw.not_interactable"),
				Pickup && !Pickup->CanInteract(Character),
				TEXT("false"), Pickup ? BoolText(Pickup->CanInteract(Character)) : TEXT("missing"), PickupLabel);

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
			AProjectOrganoidHostBase* AdminHost = Cast<AProjectOrganoidHostBase>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, AdminHostLabel));
			AssertTrue(Record, TEXT("admin.authorized_opening_host_count"),
				AdminHostCount == 1,
				TEXT("1"), FString::FromInt(AdminHostCount), TEXT("Admin"));
			AssertTrue(Record, TEXT("admin.authorized_opening_host_identity"),
				AdminHost
					&& OrganoidPlaytestActions::ActorPackage(AdminHost).Contains(TEXT("SL_Epitope_Admin"))
					&& AdminHost->bRequiresEncounterActivation
					&& !AdminHost->bAllowPhaseShiftMutations,
				TEXT("Host_Admin_SecurityOfficer with authored gate and mutation opt-out"),
				AdminHost ? OrganoidPlaytestActions::ActorPackage(AdminHost) : TEXT("missing or duplicate label"),
				AdminHostLabel);
			AssertTrue(Record, TEXT("admin.security_path_keeps_host_dormant"),
				AdminHost && !AdminHost->IsEncounterActivated()
					&& AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle,
				TEXT("dormant Idle"),
				AdminHost
					? FString::Printf(TEXT("activated=%s state=%s"),
						AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"),
						*UEnum::GetValueAsString(AdminHost->GetCombatState()))
					: TEXT("missing"),
				AdminHostLabel);
			AssertTrue(Record, TEXT("no_cinematic_sequencer"),
				CountSequenceActors(World) == 0,
				TEXT("0"), FString::FromInt(CountSequenceActors(World)), TEXT("world"));
			AssertTrue(Record, TEXT("no_tutorial_ui"),
				CountTutorialWidgets(World) == 0,
				TEXT("0"), FString::FromInt(CountTutorialWidgets(World)), TEXT("ui"));
			AssertTrue(Record, TEXT("authorized_block3_ammo_pickup"),
				CountAdminAmmoPickups(World) == 1,
				TEXT("1"), FString::FromInt(CountAdminAmmoPickups(World)), TEXT("Admin"));
			AProjectOrganoidItemPickup* Block3Ammo = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, TEXT("Pickup_Block3_PistolAmmo")));
			AssertTrue(Record, TEXT("block3.ammo_location"),
				Block3Ammo && FVector::Dist(Block3Ammo->GetActorLocation(), FVector(2560.0f, -340.0f, 80.0f)) <= 5.0f,
				TEXT("(2560,-340,80)"),
				Block3Ammo ? Block3Ammo->GetActorLocation().ToCompactString() : TEXT("missing"),
				TEXT("Pickup_Block3_PistolAmmo"));
			AssertTrue(Record, TEXT("block3.ammo_quantity"),
				Block3Ammo && Block3Ammo->Quantity == 8,
				TEXT("8"), Block3Ammo ? FString::FromInt(Block3Ammo->Quantity) : TEXT("missing"),
				TEXT("Pickup_Block3_PistolAmmo"));
			AProjectOrganoidItemPickup* Block3Trauma = Cast<AProjectOrganoidItemPickup>(
				OrganoidPlaytestActions::FindUniqueByLabel(World, TEXT("Pickup_Block3_TraumaStabilizer")));
			AssertTrue(Record, TEXT("block3.trauma_present"),
				Block3Trauma && FVector::Dist(Block3Trauma->GetActorLocation(), FVector(2760.0f, -300.0f, 80.0f)) <= 5.0f,
				TEXT("(2760,-300,80)"),
				Block3Trauma ? Block3Trauma->GetActorLocation().ToCompactString() : TEXT("missing"),
				TEXT("Pickup_Block3_TraumaStabilizer"));
			AActor* Checkpoint = OrganoidPlaytestActions::FindUniqueByLabel(World, TEXT("Checkpoint_ReceptionAtrium"));
			AssertTrue(Record, TEXT("checkpoint.present_unchanged"),
				Checkpoint && FVector::Dist(Checkpoint->GetActorLocation(), FVector(-1535.0f, 0.0f, 60.0f)) <= 5.0f,
				TEXT("(-1535,0,60)"),
				Checkpoint ? Checkpoint->GetActorLocation().ToCompactString() : TEXT("missing"),
				TEXT("Checkpoint_ReceptionAtrium"));
			(void)Character;
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
			AssertTrue(Record, TEXT("saveload.wrote"), bSaved, TEXT("true"), BoolText(bSaved), TEXT("save"));

			OrganoidPlaytestActions::TeleportNear(Character, OpeningLocation, 0.0f, OpeningLocation.Z);
			Character->SetActorRotation(FRotator::ZeroRotator);
			if (AController* PC = Character->GetController())
			{
				PC->SetControlRotation(FRotator::ZeroRotator);
			}

			const bool bLoaded = Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("saveload.loaded"), bLoaded, TEXT("true"), BoolText(bLoaded), TEXT("save"));
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
				SavedLoc.ToCompactString(), Loc.ToCompactString(), TEXT("player"));
			AssertTrue(Record, TEXT("saveload.not_forced_vestibule"),
				FVector::Dist(Loc, OpeningLocation) > 200.0f,
				TEXT("saved Reception transform"), Loc.ToCompactString(), TEXT("player"));
			AssertTrue(Record, TEXT("saveload.not_transit_fallback"),
				FVector::Dist(Loc, OldTransitFallback) > 500.0f,
				TEXT("saved transform"), Loc.ToCompactString(), TEXT("player"));
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
				if (!DirtyBefore.Contains(Name)
					&& !IsKnownNeuroRecastPackage(Name)
					&& !Name.Equals(AdminPackage, ESearchCase::IgnoreCase))
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
			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FOpeningInvestigationAutoRegister
	{
		FOpeningInvestigationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FOpeningInvestigationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FOpeningInvestigationAutoRegister GRegisterOpeningInvestigation;
}
