#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Editor.h"
#include "Engine/HitResult.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ProjectOrganoidAdminAccessDoorFacilityStateListener.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "String/BytesToHex.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S21_AccessDoorListener_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 21B Access Door Facility-State Listener Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR DoorBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr TCHAR LightControllerLabel[] = TEXT("Admin_LightController");
	constexpr TCHAR SectorControllerLabel[] = TEXT("Admin_SectorController");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr float ExpectedDoorX = 800.0f;
	constexpr float ExpectedOpenTime = 1.0f;
	constexpr float ExpectedOpenDistance = 130.0f;
	constexpr float BaselineOutsideX = 200.0f;
	constexpr float ActivateInsideX = 500.0f;
	constexpr float SweepWestX = 650.0f;
	constexpr float SweepEastX = 1100.0f;
	constexpr float OverlapSettleSeconds = 0.45f;
	constexpr float OpenMarginSeconds = 0.40f;
	constexpr float InstantOpenWindow = 0.25f;
	constexpr float LocationTolerance = 2.0f;
	constexpr float LeafMoveTolerance = 8.0f;

	const TCHAR* TerminalLabels[] = {
		TEXT("Admin_Terminal_Reception"),
		TEXT("Admin_Terminal_Security"),
		TEXT("Admin_Terminal_Records"),
		TEXT("Admin_Terminal_Executive"),
		TEXT("Admin_Terminal_Operations"),
		TEXT("Admin_Terminal_Transit"),
	};

	const TCHAR* StreamPackages[] = {
		TEXT("/Game/Maps/Epitope/SL_Epitope_Admin"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Cryo"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Compute"),
		TEXT("/Game/Maps/Epitope/SL_Epitope_Reactor"),
	};

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	FString StateText(EProjectOrganoidAdminFacilityState State)
	{
		switch (State)
		{
		case EProjectOrganoidAdminFacilityState::Normal:
			return TEXT("Normal");
		case EProjectOrganoidAdminFacilityState::Alert:
			return TEXT("Alert");
		case EProjectOrganoidAdminFacilityState::Lockdown:
			return TEXT("Lockdown");
		default:
			return TEXT("Invalid");
		}
	}

	FString HashFileSha1(const FString& Path)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path))
		{
			return FString();
		}
		uint8 Digest[FSHA1::DigestSize];
		FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
		TStringBuilder<48> Builder;
		UE::String::BytesToHexLower(MakeArrayView(Digest, FSHA1::DigestSize), Builder);
		return Builder.ToString();
	}

	FString ContentFile(const TCHAR* Relative)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / Relative);
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

	UActorComponent* FindComp(AActor* Actor, const TCHAR* Name)
	{
		if (!Actor || !Name)
		{
			return nullptr;
		}
		if (UActorComponent* Direct = OrganoidPlaytestActions::FindNamedComponent(Actor, Name))
		{
			return Direct;
		}
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		const FString Prefix(Name);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetName().StartsWith(Prefix, ESearchCase::IgnoreCase))
			{
				return Component;
			}
		}
		return nullptr;
	}

	bool ReadBoolProp(AActor* Actor, const TCHAR* Name, bool& OutValue)
	{
		const FOrganoidPlaytestPropValue Value = OrganoidPlaytestActions::ReadProperty(Actor, Name);
		if (!Value.bFound || !Value.bHasBool)
		{
			return false;
		}
		OutValue = Value.bBool;
		return true;
	}

	FVector LeafRelative(AActor* Door, const TCHAR* Name)
	{
		if (USceneComponent* Scene = Cast<USceneComponent>(FindComp(Door, Name)))
		{
			return Scene->GetRelativeLocation();
		}
		return FVector(FLT_MAX, FLT_MAX, FLT_MAX);
	}

	FLinearColor StatusLightColor(AActor* Door)
	{
		if (ULightComponent* Light = Cast<ULightComponent>(FindComp(Door, TEXT("StatusLight"))))
		{
			return Light->GetLightColor();
		}
		return FLinearColor::Transparent;
	}

	bool IsDeniedRed(const FLinearColor& Color)
	{
		return Color.R >= 0.85f && Color.G <= 0.25f && Color.B <= 0.20f;
	}

	bool IsPawnInsideNamedBox(APawn* Pawn, AActor* Actor, const TCHAR* ComponentName)
	{
		if (!Pawn || !Actor || !ComponentName)
		{
			return false;
		}
		UBoxComponent* Box = Cast<UBoxComponent>(FindComp(Actor, ComponentName));
		if (!Box)
		{
			return false;
		}
		const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
		const FVector Extent = Box->GetScaledBoxExtent();
		return FMath::Abs(Local.X) <= Extent.X
			&& FMath::Abs(Local.Y) <= Extent.Y
			&& FMath::Abs(Local.Z) <= Extent.Z;
	}

	bool IsDoorLeafComponent(const UPrimitiveComponent* Component)
	{
		if (!Component)
		{
			return false;
		}
		const FString CompName = Component->GetName();
		return CompName.StartsWith(TEXT("Door_Left"), ESearchCase::IgnoreCase)
			|| CompName.StartsWith(TEXT("Door_Right"), ESearchCase::IgnoreCase);
	}

	FString HitDescribe(const FHitResult& Hit)
	{
		if (!Hit.GetActor())
		{
			return TEXT("no blocking hit");
		}
		const FString ActorName = OrganoidPlaytestActions::ActorLabel(Hit.GetActor());
		const FString CompName = Hit.GetComponent() ? Hit.GetComponent()->GetName() : TEXT("none");
		return FString::Printf(TEXT("%s.%s blocking=%s"), *ActorName, *CompName, *BoolText(Hit.bBlockingHit));
	}

	bool SweepFirstBlockingHit(UWorld* World, APawn* Pawn, const FVector& Start, const FVector& End, FHitResult& OutHit)
	{
		OutHit = FHitResult();
		if (!World || !Pawn)
		{
			return false;
		}
		float Radius = 34.0f;
		float HalfHeight = 80.0f;
		ECollisionChannel Channel = ECC_Pawn;
		if (ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				Radius = FMath::Max(10.0f, Capsule->GetScaledCapsuleRadius() - 4.0f);
				HalfHeight = FMath::Max(20.0f, Capsule->GetScaledCapsuleHalfHeight() * 0.5f);
				Channel = Capsule->GetCollisionObjectType();
			}
		}
		FCollisionQueryParams Params(FName(TEXT("S21BAccessDoorSweep")), false, Pawn);
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(
			Hits, Start, End, FQuat::Identity, Channel, FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);
		for (const FHitResult& Hit : Hits)
		{
			if (Hit.bBlockingHit && Hit.GetActor())
			{
				OutHit = Hit;
				return true;
			}
		}
		return false;
	}

	bool SweepHitsDoorLeaf(UWorld* World, APawn* Pawn, AActor* Door, const FVector& Start, const FVector& End, FHitResult& OutHit)
	{
		if (!SweepFirstBlockingHit(World, Pawn, Start, End, OutHit))
		{
			return false;
		}
		return OutHit.GetActor() == Door && IsDoorLeafComponent(OutHit.GetComponent());
	}

	UObject* FindGameSubsystem(UWorld* World, const TCHAR* ClassPath)
	{
		if (!World || !ClassPath)
		{
			return nullptr;
		}
		UClass* Class = FindObject<UClass>(nullptr, ClassPath);
		return Class ? World->GetSubsystemBase(Class) : nullptr;
	}

	FString EnumPropertyToText(FProperty* Property, const void* ValuePtr)
	{
		if (!Property || !ValuePtr)
		{
			return FString();
		}
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
		{
			const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(ValuePtr);
			if (UEnum* Enum = EnumProp->GetEnum())
			{
				return Enum->GetNameStringByValue(Value);
			}
		}
		return FString();
	}

	FString ReadSectorPowerState(UWorld* World, uint8 SectorValue)
	{
		UObject* Power = FindGameSubsystem(World, TEXT("/Script/ProjectOrganoid.ProjectOrganoidPowerSubsystem"));
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
				EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()), static_cast<int64>(SectorValue));
		}
		Power->ProcessEvent(Function, Parms.GetData());
		if (FProperty* ReturnProp = Function->GetReturnProperty())
		{
			return EnumPropertyToText(ReturnProp, ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData()));
		}
		return FString();
	}

	bool ReadSecurityLockdown(UWorld* World)
	{
		UObject* Security = FindGameSubsystem(World, TEXT("/Script/ProjectOrganoid.ProjectOrganoidSecuritySubsystem"));
		if (!Security)
		{
			return false;
		}
		UFunction* Function = Security->FindFunction(FName(TEXT("IsFacilityLockdownActive")));
		if (!Function)
		{
			return false;
		}
		TArray<uint8> Parms;
		Parms.AddZeroed(Function->ParmsSize);
		Security->ProcessEvent(Function, Parms.GetData());
		if (FBoolProperty* ReturnProp = CastField<FBoolProperty>(Function->GetReturnProperty()))
		{
			return ReturnProp->GetPropertyValue(ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData()));
		}
		return false;
	}

	class FS21AccessDoorListenerFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Session = ESession::NormalOpen;
			bAnyAssertFailed = false;
			bConfigFailed = false;
			ResetSessionRuntime();
			DirtyBefore.Reset();
			AdminHashBefore.Reset();
			DoorHashBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Owner.RequestEndPieIfStarted();
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
			case EStage::WaitPieReady:
				TickWaitPieReady(Owner, *Record, DeltaTime);
				break;
			case EStage::ClearOutside:
				TickClearOutside(Owner, *Record, DeltaTime);
				break;
			case EStage::SetupState:
				TickSetupState(Owner, *Record);
				break;
			case EStage::EnterTrigger:
				TickEnterTrigger(Owner, *Record);
				break;
			case EStage::WaitDoor:
				TickWaitDoor(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertDoor:
				TickAssertDoor(Owner, *Record);
				break;
			case EStage::AssertTraversal:
				TickAssertTraversal(Owner, *Record);
				break;
			case EStage::RestoreState:
				TickRestoreState(Owner, *Record);
				break;
			case EStage::Isolation:
				TickIsolation(Owner, *Record);
				break;
			case EStage::EndPie:
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitPieStopped;
				Owner.SetStage(TEXT("WaitPieStopped"));
				break;
			case EStage::WaitPieStopped:
				TickWaitPieStopped(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertDurable:
				TickAssertDurable(Owner, *Record);
				break;
			case EStage::Finalize:
				Finalize(Owner, *Record);
				break;
			}
		}

	private:
		enum class EStage : uint8
		{
			Preflight,
			StartPie,
			WaitPieReady,
			ClearOutside,
			SetupState,
			EnterTrigger,
			WaitDoor,
			AssertDoor,
			AssertTraversal,
			RestoreState,
			Isolation,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		enum class ESession : uint8
		{
			NormalOpen,
			AlertOpen,
			LockdownRestoreNormal,
			LockdownRestoreAlert
		};

		void ResetSessionRuntime()
		{
			WaitSeconds = 0.0f;
			bEvacuatedSpawn = false;
			bBaselineTeleported = false;
			bEnteredTrigger = false;
			bSawTimelineInProgress = false;
			bInstantOpenSampled = false;
			bExpectOpen = true;
			bAfterRestore = false;
			LeafMovedAt = -1.0f;
			OpenAt = -1.0f;
			BaselineLeft = FVector::ZeroVector;
			BaselineRight = FVector::ZeroVector;
			Door.Reset();
			Listener.Reset();
			Subsystem.Reset();
		}

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			bAnyAssertFailed = true;
			Record.FailureReason = Reason;
			Owner.RequestEndPieIfStarted();
			WaitSeconds = 0.0f;
			Stage = EStage::WaitPieStopped;
			Owner.SetStage(TEXT("WaitPieStopped"));
		}

		bool AssertTrue(
			FOrganoidPlaytestRecord& Record,
			const FString& Name,
			bool bOk,
			const FString& Expected,
			const FString& Actual,
			const FString& Actor,
			bool bConfig)
		{
			Record.AddAssertion(Name, bOk, Expected, Actual, Actor, bConfig);
			if (!bOk)
			{
				bAnyAssertFailed = true;
				Record.FailureReason = Name + TEXT(" expected=") + Expected + TEXT(" actual=") + Actual;
				if (bConfig)
				{
					bConfigFailed = true;
				}
			}
			return bOk;
		}

		FString SessionPrefix() const
		{
			switch (Session)
			{
			case ESession::NormalOpen:
				return TEXT("Normal");
			case ESession::AlertOpen:
				return TEXT("Alert");
			case ESession::LockdownRestoreNormal:
				return bAfterRestore ? TEXT("Lockdown_to_Normal") : TEXT("Lockdown");
			case ESession::LockdownRestoreAlert:
				return bAfterRestore ? TEXT("Lockdown_to_Alert") : TEXT("Lockdown_pre_Alert");
			default:
				return TEXT("session");
			}
		}

		float CapsuleZFor(APawn* Pawn) const
		{
			float CapsuleZ = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleZ = Capsule->GetScaledCapsuleHalfHeight();
				}
			}
			return CapsuleZ;
		}

		bool PlacePawnOnFloor(APawn* Pawn)
		{
			if (!Pawn || !Pawn->GetWorld())
			{
				return false;
			}
			float HalfHeight = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
				}
			}
			const FVector Loc = Pawn->GetActorLocation();
			FHitResult FloorHit;
			FCollisionQueryParams Params(FName(TEXT("S21BFloorSnap")), false, Pawn);
			const bool bHit = Pawn->GetWorld()->LineTraceSingleByChannel(
				FloorHit,
				Loc + FVector(0.0f, 0.0f, 250.0f),
				Loc - FVector(0.0f, 0.0f, 600.0f),
				ECC_WorldStatic,
				Params);
			if (!bHit || !FloorHit.bBlockingHit)
			{
				return false;
			}
			FVector Placed = Loc;
			Placed.Z = FloorHit.ImpactPoint.Z + HalfHeight + 2.0f;
			Pawn->SetActorLocation(Placed, false, nullptr, ETeleportType::TeleportPhysics);
			Pawn->UpdateOverlaps();
			return true;
		}

		bool TeleportPawnTo(APawn* Pawn, float X, float Y)
		{
			if (!Pawn)
			{
				return false;
			}
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->GravityScale = 1.0f;
					Move->SetMovementMode(MOVE_Walking);
					Move->StopMovementImmediately();
				}
				Character->SetActorEnableCollision(true);
			}
			const float CapsuleZ = CapsuleZFor(Pawn);
			const bool bMoved = OrganoidPlaytestActions::TeleportNear(Pawn, FVector(X, Y, CapsuleZ), 0.0f, CapsuleZ);
			PlacePawnOnFloor(Pawn);
			Pawn->UpdateOverlaps();
			return bMoved;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Preflight"));
			if (!GEditor)
			{
				Record.State = EOrganoidPlaytestState::Blocked;
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Editor is not available."));
				return;
			}
			if (GEditor->IsPlaySessionInProgress())
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(DoorBpPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, or BP_AdminAccessDoor is dirty. Refusing to start."));
				return;
			}
			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			DoorHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Record.AddActor(TEXT("mid_open_policy"),
				TEXT("Lockdown sets bLocked only. Timeline is not reversed. Already-open or mid-opening leaves keep moving/stay open. New automatic access is blocked."));
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("StartPie"));
			ResetSessionRuntime();
			if (!Owner.RequestStartPie(MapPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
				return;
			}
			(void)Record;
			WaitSeconds = 0.0f;
			Stage = EStage::WaitPieReady;
			Owner.SetStage(TEXT("WaitPieReady"));
		}

		void TickWaitPieReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			ACharacter* Character = OrganoidPlaytestActions::GetPlayerCharacter(World);
			AActor* FoundDoor = World ? OrganoidPlaytestActions::FindAccessDoor(World) : nullptr;
			UProjectOrganoidAdminFacilityStateSubsystem* State = World
				? World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>()
				: nullptr;
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}
			if (World && Character && FoundDoor && State)
			{
				Door = FoundDoor;
				Subsystem = State;
				Listener = FoundDoor->FindComponentByClass<UProjectOrganoidAdminAccessDoorFacilityStateListener>();
				if (IsPawnInsideNamedBox(Character, FoundDoor, TEXT("AccessTrigger")) && !bEvacuatedSpawn)
				{
					TeleportPawnTo(Character, BaselineOutsideX, 0.0f);
					bEvacuatedSpawn = true;
				}
				if (bWalking || WaitSeconds > 8.0f)
				{
					WaitSeconds = 0.0f;
					Stage = EStage::ClearOutside;
					Owner.SetStage(TEXT("ClearOutside"));
					return;
				}
			}
			if (WaitSeconds > 45.0f)
			{
				FailAndStop(Owner, Record, TEXT("Timed out waiting for PIE player, Access Door, and Admin facility state subsystem."));
			}
		}

		void TickClearOutside(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door before setup."));
				return;
			}
			if (!bBaselineTeleported)
			{
				if (!TeleportPawnTo(Pawn, BaselineOutsideX, 0.0f))
				{
					FailAndStop(Owner, Record, TEXT("Failed to teleport west of AccessTrigger."));
					return;
				}
				bBaselineTeleported = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds >= OverlapSettleSeconds)
			{
				Stage = EStage::SetupState;
				Owner.SetStage(TEXT("SetupState"));
			}
		}

		void TickSetupState(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* FoundDoor = Door.Get();
			UProjectOrganoidAdminFacilityStateSubsystem* State = Subsystem.Get();
			UProjectOrganoidAdminAccessDoorFacilityStateListener* LiveListener = Listener.Get();
			if (!World || !FoundDoor || !State)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, Access Door, or facility state subsystem at setup."));
				return;
			}

			const FString Prefix = SessionPrefix();
			if (!AssertTrue(Record, Prefix + TEXT(".listener_present"), LiveListener != nullptr,
				TEXT("FacilityStateListener"), LiveListener ? TEXT("present") : TEXT("missing"), DoorLabel, true))
			{
				FailAndStop(Owner, Record, TEXT("BP_AdminAccessDoor is missing UProjectOrganoidAdminAccessDoorFacilityStateListener."));
				return;
			}

			EProjectOrganoidAdminFacilityState Desired = EProjectOrganoidAdminFacilityState::Normal;
			bool bSetupExpectOpen = true;
			switch (Session)
			{
			case ESession::NormalOpen:
				Desired = EProjectOrganoidAdminFacilityState::Normal;
				bSetupExpectOpen = true;
				break;
			case ESession::AlertOpen:
				Desired = EProjectOrganoidAdminFacilityState::Alert;
				bSetupExpectOpen = true;
				break;
			case ESession::LockdownRestoreNormal:
			case ESession::LockdownRestoreAlert:
				Desired = EProjectOrganoidAdminFacilityState::Lockdown;
				bSetupExpectOpen = false;
				break;
			}

			State->SetAdminFacilityState(Desired);
			const EProjectOrganoidAdminFacilityState LiveState = State->GetAdminFacilityState();
			bool bLocked = true;
			bool bIsOpen = true;
			bool bAutomatic = false;
			ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked);
			ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen);
			ReadBoolProp(FoundDoor, TEXT("bAutomatic"), bAutomatic);
			BaselineLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			BaselineRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			bExpectOpen = bSetupExpectOpen;

			AssertTrue(Record, Prefix + TEXT(".subsystem"), LiveState == Desired,
				StateText(Desired), StateText(LiveState), TEXT("AdminFacilityState"), false);
			AssertTrue(Record, Prefix + TEXT(".bAutomatic"), bAutomatic, TEXT("true"), BoolText(bAutomatic), DoorLabel, false);
			AssertTrue(Record, Prefix + TEXT(".closed_before_overlap"), !bIsOpen, TEXT("false"), BoolText(bIsOpen), DoorLabel, true);
			AssertTrue(Record, Prefix + TEXT(".door_x"),
				FMath::IsNearlyEqual(FoundDoor->GetActorLocation().X, ExpectedDoorX, LocationTolerance),
				FString::SanitizeFloat(ExpectedDoorX), FString::SanitizeFloat(FoundDoor->GetActorLocation().X), DoorLabel, true);

			if (Desired == EProjectOrganoidAdminFacilityState::Lockdown)
			{
				AssertTrue(Record, Prefix + TEXT(".bLocked"), bLocked, TEXT("true"), BoolText(bLocked), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".override_active"),
					LiveListener && LiveListener->bFacilityLockOverrideActive,
					TEXT("true"), BoolText(LiveListener && LiveListener->bFacilityLockOverrideActive), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".baseline_preserved"),
					LiveListener && LiveListener->bBaselineCaptured && !LiveListener->bBaselineLocked,
					TEXT("baseline false"),
					LiveListener
						? FString::Printf(TEXT("captured=%s baseline=%s"),
							*BoolText(LiveListener->bBaselineCaptured), *BoolText(LiveListener->bBaselineLocked))
						: TEXT("missing"),
					DoorLabel, false);
			}
			else
			{
				AssertTrue(Record, Prefix + TEXT(".bLocked"), !bLocked, TEXT("false"), BoolText(bLocked), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".override_inactive"),
					LiveListener && !LiveListener->bFacilityLockOverrideActive,
					TEXT("false"), BoolText(LiveListener && LiveListener->bFacilityLockOverrideActive), DoorLabel, false);
			}

			if (Session == ESession::NormalOpen)
			{
				AdminPowerBaseline = ReadSectorPowerState(World, 1);
				AssertTrue(Record, TEXT("isolation.baseline.Admin_Online"),
					AdminPowerBaseline.Contains(TEXT("Online"), ESearchCase::IgnoreCase),
					TEXT("Online"), AdminPowerBaseline, TEXT("Power"), false);
				AssertTrue(Record, TEXT("isolation.baseline.security_lockdown"),
					!ReadSecurityLockdown(World), TEXT("false"), BoolText(ReadSecurityLockdown(World)), TEXT("Security"), false);
			}

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			if (Session == ESession::LockdownRestoreAlert && !bAfterRestore)
			{
				Stage = EStage::RestoreState;
				Owner.SetStage(TEXT("RestoreState"));
				return;
			}

			Stage = EStage::EnterTrigger;
			Owner.SetStage(TEXT("EnterTrigger"));
		}

		void TickEnterTrigger(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door before overlap."));
				return;
			}
			bEnteredTrigger = false;
			bSawTimelineInProgress = false;
			bInstantOpenSampled = false;
			LeafMovedAt = -1.0f;
			OpenAt = -1.0f;
			WaitSeconds = 0.0f;
			if (!TeleportPawnTo(Pawn, ActivateInsideX, 0.0f))
			{
				FailAndStop(Owner, Record, TEXT("Failed to move the genuine player into AccessTrigger."));
				return;
			}
			bEnteredTrigger = true;
			Record.AddActor(SessionPrefix() + TEXT(".player_enter_trigger"), Pawn->GetActorLocation().ToCompactString());
			Stage = EStage::WaitDoor;
			Owner.SetStage(TEXT("WaitDoor"));
		}

		void TickWaitDoor(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			AActor* FoundDoor = Door.Get();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(OrganoidPlaytestActions::GetPieWorld());
			if (!FoundDoor || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost Access Door or pawn during door wait."));
				return;
			}
			bool bIsOpen = false;
			ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen);
			const FVector LiveLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			const FVector LiveRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			const bool bLeavesMoving =
				FMath::Abs(LiveLeft.Y - BaselineLeft.Y) > 2.0f || FMath::Abs(LiveRight.Y - BaselineRight.Y) > 2.0f;
			if (bLeavesMoving && LeafMovedAt < 0.0f)
			{
				LeafMovedAt = WaitSeconds;
			}
			if (bIsOpen && OpenAt < 0.0f)
			{
				OpenAt = WaitSeconds;
			}
			if (WaitSeconds < InstantOpenWindow)
			{
				bInstantOpenSampled = true;
				if (bExpectOpen && bIsOpen)
				{
					FailAndStop(Owner, Record, TEXT("bIsOpen became true before OpenTime Timeline could finish."));
					return;
				}
			}
			if (!bIsOpen && bLeavesMoving)
			{
				bSawTimelineInProgress = true;
			}
			const float Deadline = bExpectOpen ? (ExpectedOpenTime + OpenMarginSeconds) : OverlapSettleSeconds;
			if (WaitSeconds >= Deadline)
			{
				Stage = EStage::AssertDoor;
				Owner.SetStage(TEXT("AssertDoor"));
			}
		}

		void TickAssertDoor(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door after wait."));
				return;
			}
			const FString Prefix = SessionPrefix();
			const bool bOverlap = IsPawnInsideNamedBox(Pawn, FoundDoor, TEXT("AccessTrigger"));
			AssertTrue(Record, Prefix + TEXT(".legitimate_overlap"), bOverlap,
				TEXT("overlapping"), bOverlap ? TEXT("overlapping") : TEXT("not overlapping"), DoorLabel, false);

			bool bIsOpen = false;
			bool bLocked = true;
			ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen);
			ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked);
			const FVector LiveLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			const FVector LiveRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			const float LeftYDelta = LiveLeft.Y - BaselineLeft.Y;
			const float RightYDelta = LiveRight.Y - BaselineRight.Y;
			const FLinearColor LiveColor = StatusLightColor(FoundDoor);

			if (bExpectOpen)
			{
				AssertTrue(Record, Prefix + TEXT(".automatic_open_condition"), !bLocked,
					TEXT("bLocked=false"), BoolText(bLocked), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".timeline_before_bIsOpen"),
					bSawTimelineInProgress || !bInstantOpenSampled,
					TEXT("leaves moved while bIsOpen false"),
					bSawTimelineInProgress ? TEXT("observed") : TEXT("not observed"), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".opened"), bIsOpen, TEXT("true"), BoolText(bIsOpen), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".Door_Left_dY"),
					FMath::IsNearlyEqual(LeftYDelta, -ExpectedOpenDistance, LeafMoveTolerance),
					FString::Printf(TEXT("dY=%.1f"), -ExpectedOpenDistance),
					FString::Printf(TEXT("dY=%.1f"), LeftYDelta), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".Door_Right_dY"),
					FMath::IsNearlyEqual(RightYDelta, ExpectedOpenDistance, LeafMoveTolerance),
					FString::Printf(TEXT("dY=%.1f"), ExpectedOpenDistance),
					FString::Printf(TEXT("dY=%.1f"), RightYDelta), DoorLabel, false);
				if (!bAfterRestore)
				{
					AssertTrue(Record, Prefix + TEXT(".denied_did_not_fire"), !IsDeniedRed(LiveColor),
						TEXT("not denied-red"), LiveColor.ToString(), DoorLabel, false);
				}
			}
			else
			{
				AssertTrue(Record, Prefix + TEXT(".stayed_locked"), bLocked, TEXT("true"), BoolText(bLocked), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".did_not_open"), !bIsOpen, TEXT("false"), BoolText(bIsOpen), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".leaves_unmoved"),
					FMath::Abs(LeftYDelta) < 2.0f && FMath::Abs(RightYDelta) < 2.0f,
					TEXT("dY~0"),
					FString::Printf(TEXT("left=%.1f right=%.1f"), LeftYDelta, RightYDelta), DoorLabel, false);
				AssertTrue(Record, Prefix + TEXT(".denied_light"), IsDeniedRed(LiveColor),
					TEXT("denied-red"), LiveColor.ToString(), DoorLabel, false);
			}

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			if (bExpectOpen && (Session == ESession::NormalOpen || (Session == ESession::LockdownRestoreNormal && bAfterRestore)))
			{
				Stage = EStage::AssertTraversal;
				Owner.SetStage(TEXT("AssertTraversal"));
				return;
			}
			if (!bExpectOpen && Session == ESession::LockdownRestoreNormal && !bAfterRestore)
			{
				if (!TeleportPawnTo(Pawn, BaselineOutsideX, 0.0f))
				{
					FailAndStop(Owner, Record, TEXT("Failed to leave AccessTrigger before Lockdown restore."));
					return;
				}
				Stage = EStage::RestoreState;
				Owner.SetStage(TEXT("RestoreState"));
				return;
			}

			Stage = EStage::Isolation;
			Owner.SetStage(TEXT("Isolation"));
		}

		void TickAssertTraversal(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door before traversal."));
				return;
			}
			const FString Prefix = SessionPrefix();
			const float CapsuleZ = CapsuleZFor(Pawn);
			FHitResult OpenHit;
			const bool bStillBlocked = SweepHitsDoorLeaf(
				World, Pawn, FoundDoor, FVector(SweepWestX, 0.0f, CapsuleZ), FVector(SweepEastX, 0.0f, CapsuleZ), OpenHit);
			AssertTrue(Record, Prefix + TEXT(".sweep_clears_leaves"), !bStillBlocked,
				TEXT("no blocking Door_Left/Door_Right"), HitDescribe(OpenHit), DoorLabel, false);
			if (!TeleportPawnTo(Pawn, SweepWestX, 0.0f))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place the player west of the open doorway."));
				return;
			}
			const FVector Dest(SweepEastX, 0.0f, Pawn->GetActorLocation().Z);
			FHitResult MoveHit;
			Pawn->SetActorLocation(Dest, true, &MoveHit, ETeleportType::None);
			const FVector After = Pawn->GetActorLocation();
			const bool bReachedEast = After.X >= (SweepEastX - 40.0f);
			const bool bHitLeaf = MoveHit.GetActor() == FoundDoor && IsDoorLeafComponent(MoveHit.GetComponent());
			AssertTrue(Record, Prefix + TEXT(".physical_vestibule_to_reception"),
				bReachedEast && !bHitLeaf,
				TEXT("pawn X>=1060 without hitting door leaves"),
				FString::Printf(TEXT("x=%.1f hit=%s"), After.X, *HitDescribe(MoveHit)), DoorLabel, false);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::Isolation;
			Owner.SetStage(TEXT("Isolation"));
		}

		void TickRestoreState(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* FoundDoor = Door.Get();
			UProjectOrganoidAdminFacilityStateSubsystem* State = Subsystem.Get();
			UProjectOrganoidAdminAccessDoorFacilityStateListener* LiveListener = Listener.Get();
			if (!World || !FoundDoor || !State)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, Access Door, or subsystem before restore."));
				return;
			}

			const EProjectOrganoidAdminFacilityState Restore =
				(Session == ESession::LockdownRestoreAlert)
					? EProjectOrganoidAdminFacilityState::Alert
					: EProjectOrganoidAdminFacilityState::Normal;
			State->SetAdminFacilityState(Restore);
			bAfterRestore = true;
			bExpectOpen = true;
			bEnteredTrigger = false;
			WaitSeconds = 0.0f;
			BaselineLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			BaselineRight = LeafRelative(FoundDoor, TEXT("Door_Right"));

			const FString Prefix = SessionPrefix();
			bool bLocked = true;
			ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked);
			AssertTrue(Record, Prefix + TEXT(".subsystem"), State->GetAdminFacilityState() == Restore,
				StateText(Restore), StateText(State->GetAdminFacilityState()), TEXT("AdminFacilityState"), false);
			AssertTrue(Record, Prefix + TEXT(".baseline_lock_restored"), !bLocked,
				TEXT("false"), BoolText(bLocked), DoorLabel, false);
			AssertTrue(Record, Prefix + TEXT(".override_cleared"),
				LiveListener && !LiveListener->bFacilityLockOverrideActive,
				TEXT("false"), BoolText(LiveListener && LiveListener->bFacilityLockOverrideActive), DoorLabel, false);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::EnterTrigger;
			Owner.SetStage(TEXT("EnterTrigger"));
		}

		void TickIsolation(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!World)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during isolation."));
				return;
			}
			const FString Prefix = TEXT("isolation.") + SessionPrefix();
			const FString AdminPower = ReadSectorPowerState(World, 1);
			const FString NeuroPower = ReadSectorPowerState(World, 2);
			const FString CryoPower = ReadSectorPowerState(World, 3);
			AssertTrue(Record, Prefix + TEXT(".power.Admin"),
				AdminPower.Contains(TEXT("Online"), ESearchCase::IgnoreCase),
				TEXT("Online"), AdminPower, TEXT("Power"), false);
			AssertTrue(Record, Prefix + TEXT(".power.NeuroGenetics"),
				NeuroPower.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase),
				TEXT("Emergency"), NeuroPower, TEXT("Power"), false);
			AssertTrue(Record, Prefix + TEXT(".power.Cryo"),
				CryoPower.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase),
				TEXT("Blackout"), CryoPower, TEXT("Power"), false);
			AssertTrue(Record, Prefix + TEXT(".security.lockdown"),
				!ReadSecurityLockdown(World), TEXT("false"), BoolText(ReadSecurityLockdown(World)), TEXT("Security"), false);

			if (AActor* Sector = OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel))
			{
				const FName Cached = FName(*OrganoidPlaytestActions::ReadProperty(Sector, TEXT("FacilityState")).Text);
				AssertTrue(Record, Prefix + TEXT(".sector.FacilityState_cache"),
					Cached == FName(TEXT("Normal")), TEXT("Normal"), Cached.ToString(), SectorControllerLabel, false);
			}
			if (AActor* Lights = OrganoidPlaytestActions::FindUniqueByLabel(World, LightControllerLabel))
			{
				const FString Zone = OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone")).Text;
				const bool bLegitZone = Zone.IsEmpty()
					|| Zone.Equals(TEXT("None"))
					|| Zone.Contains(TEXT("Vestibule"))
					|| Zone.Contains(TEXT("Reception"));
				AssertTrue(Record, Prefix + TEXT(".s20.CurrentLightingZone"),
					bLegitZone, TEXT("None or legitimate room-overlap Vestibule/Reception"), Zone, LightControllerLabel, false);
			}
			if (AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel))
			{
				const FOrganoidPlaytestPropValue Online = OrganoidPlaytestActions::ReadProperty(Hologram, TEXT("bAdminOnline"));
				AssertTrue(Record, Prefix + TEXT(".hologram.bAdminOnline"),
					Online.bHasBool && Online.bBool, TEXT("true"), Online.Text, HologramLabel, false);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
				{
					const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
					AssertTrue(Record, Prefix + TEXT(".terminal_") + TerminalLabels[Index],
						!(Activated.bHasBool && Activated.bBool), TEXT("activated=false"), Activated.Text, TerminalLabels[Index], false);
				}
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
			{
				FStreamLook Look;
				for (ULevelStreaming* Level : World->GetStreamingLevels())
				{
					if (!Level)
					{
						continue;
					}
					const FString Name = OrganoidPlaytestActions::NormalizePackage(Level->GetWorldAssetPackageFName().ToString());
					if (Name.Equals(StreamPackages[Index], ESearchCase::IgnoreCase))
					{
						Look.bFound = true;
						Look.bShouldBeLoaded = Level->ShouldBeLoaded();
						Look.bShouldBeVisible = Level->ShouldBeVisible();
						Look.bIsLoaded = Level->IsLevelLoaded();
						break;
					}
				}
				const bool bAdmin = Index == 0;
				AssertTrue(Record, Prefix + TEXT(".stream_") + StreamPackages[Index],
					Look.bFound && Look.bIsLoaded == bAdmin && Look.bShouldBeLoaded == bAdmin,
					bAdmin ? TEXT("Admin loaded") : TEXT("other not loaded"),
					FString::Printf(TEXT("found=%s loaded=%s"), *BoolText(Look.bFound), *BoolText(Look.bIsLoaded)),
					StreamPackages[Index], false);
			}

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void TickWaitPieStopped(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			const bool bPie = GEditor && GEditor->IsPlaySessionInProgress();
			if (!bPie && !OrganoidPlaytestActions::GetPieWorld())
			{
				if (bAnyAssertFailed)
				{
					Stage = EStage::AssertDurable;
					Owner.SetStage(TEXT("AssertDurable"));
					return;
				}
				if (Session == ESession::NormalOpen)
				{
					Session = ESession::AlertOpen;
					Stage = EStage::StartPie;
					Owner.SetStage(TEXT("StartPie"));
					return;
				}
				if (Session == ESession::AlertOpen)
				{
					Session = ESession::LockdownRestoreNormal;
					Stage = EStage::StartPie;
					Owner.SetStage(TEXT("StartPie"));
					return;
				}
				if (Session == ESession::LockdownRestoreNormal)
				{
					Session = ESession::LockdownRestoreAlert;
					Stage = EStage::StartPie;
					Owner.SetStage(TEXT("StartPie"));
					return;
				}
				Stage = EStage::AssertDurable;
				Owner.SetStage(TEXT("AssertDurable"));
				return;
			}
			if (WaitSeconds > 30.0f)
			{
				Record.FailureReason = TEXT("Timed out waiting for PIE to stop.");
				bAnyAssertFailed = true;
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record.FailureReason);
			}
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("AssertDurable"));
			const bool bTrackedDirty = PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(DoorBpPackage);
			AssertTrue(Record, TEXT("durable.tracked_packages_clean"), !bTrackedDirty,
				TEXT("not dirty"), bTrackedDirty ? TEXT("dirty") : TEXT("clean"), TEXT(""), true);
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			TArray<FString> NewDirty;
			for (const FString& Name : DirtyAfter)
			{
				if (!DirtyBefore.Contains(Name))
				{
					NewDirty.Add(Name);
				}
			}
			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"), NewDirty.Num() == 0,
				TEXT("0"), FString::FromInt(NewDirty.Num()), TEXT(""), true);
			AssertTrue(Record, TEXT("durable.dirty_count_zero"), DirtyAfter.Num() == 0,
				TEXT("0"), FString::FromInt(DirtyAfter.Num()), TEXT(""), true);
			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString DoorHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"), AdminHashAfter.Equals(AdminHashBefore),
				AdminHashBefore, AdminHashAfter, TEXT("SL_Epitope_Admin"), true);
			AssertTrue(Record, TEXT("durable.access_door_hash_unchanged"), DoorHashAfter.Equals(DoorHashBefore),
				DoorHashBefore, DoorHashAfter, TEXT("BP_AdminAccessDoor"), true);
			AssertTrue(Record, TEXT("durable.playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""), true);
			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable packages changed. The playtest bot will not repair them."),
					TEXT("Inspect unsaved work. Do not let the bot write."));
			}
			Stage = EStage::Finalize;
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			const EOrganoidPlaytestState State = bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass;
			Owner.CompleteActive(State, Record.FailureReason);
		}

		struct FStreamLook
		{
			bool bFound = false;
			bool bShouldBeLoaded = false;
			bool bShouldBeVisible = false;
			bool bIsLoaded = false;
		};

		EStage Stage = EStage::Preflight;
		ESession Session = ESession::NormalOpen;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		bool bEvacuatedSpawn = false;
		bool bBaselineTeleported = false;
		bool bEnteredTrigger = false;
		bool bSawTimelineInProgress = false;
		bool bInstantOpenSampled = false;
		bool bExpectOpen = true;
		bool bAfterRestore = false;
		float LeafMovedAt = -1.0f;
		float OpenAt = -1.0f;
		FVector BaselineLeft = FVector::ZeroVector;
		FVector BaselineRight = FVector::ZeroVector;
		FString AdminHashBefore;
		FString DoorHashBefore;
		FString AdminPowerBaseline;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AActor> Door;
		TWeakObjectPtr<UProjectOrganoidAdminAccessDoorFacilityStateListener> Listener;
		TWeakObjectPtr<UProjectOrganoidAdminFacilityStateSubsystem> Subsystem;
	};

	struct FS21BAutoRegister
	{
		FS21BAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS21AccessDoorListenerFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS21BAutoRegister GRegisterS21B;
}
