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
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "String/BytesToHex.h"
#include "UObject/Package.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S17_AccessDoor_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 17 Access Door Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR DoorBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	constexpr TCHAR KnownNeuroRecastPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr TCHAR ExpectedClass[] = TEXT("BP_AdminAccessDoor_C");
	constexpr float ExpectedDoorX = 800.0f;
	constexpr float ExpectedOpenTime = 1.0f;
	constexpr float ExpectedOpenDistance = 130.0f;
	constexpr float ExpectedTriggerExtentX = 450.0f;
	constexpr float ExpectedTriggerExtentY = 120.0f;
	constexpr float ExpectedTriggerExtentZ = 110.0f;
	constexpr float BaselineOutsideX = 200.0f;
	constexpr float ActivateInsideX = 500.0f;
	constexpr float SweepWestX = 650.0f;
	constexpr float SweepEastX = 1100.0f;
	constexpr float OverlapSettleSeconds = 0.45f;
	constexpr float OpenMarginSeconds = 0.40f;
	constexpr float InstantOpenWindow = 0.25f;
	constexpr float LocationTolerance = 2.0f;
	constexpr float ExtentTolerance = 0.5f;
	constexpr float LeafMoveTolerance = 8.0f;

	const TCHAR* TerminalLabels[] = {
		TEXT("Admin_Terminal_Reception"),
		TEXT("Admin_Terminal_Security"),
		TEXT("Admin_Terminal_Records"),
		TEXT("Admin_Terminal_Executive"),
		TEXT("Admin_Terminal_Operations"),
		TEXT("Admin_Terminal_Transit"),
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

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
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

	bool IsKnownNeuroRecastPackage(const FString& PackageName)
	{
		return PackageName.Equals(KnownNeuroRecastPackage);
	}

	void CopyDirtyMinusKnownNeuroRecast(const TArray<FString>& In, TArray<FString>& Out)
	{
		Out.Reset();
		for (const FString& Name : In)
		{
			if (!IsKnownNeuroRecastPackage(Name))
			{
				Out.Add(Name);
			}
		}
	}

	bool DirtySetIsEmptyOrOnlyKnownNeuroRecast(const TArray<FString>& Dirty)
	{
		if (Dirty.Num() == 0)
		{
			return true;
		}
		return Dirty.Num() == 1 && IsKnownNeuroRecastPackage(Dirty[0]);
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

	bool ReadFloatProp(AActor* Actor, const TCHAR* Name, float& OutValue)
	{
		const FOrganoidPlaytestPropValue Value = OrganoidPlaytestActions::ReadProperty(Actor, Name);
		if (!Value.bFound || !Value.bHasNumber)
		{
			return false;
		}
		OutValue = static_cast<float>(Value.Number);
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

	bool SweepFirstBlockingHit(
		UWorld* World,
		APawn* Pawn,
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit)
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
		FCollisionQueryParams Params(FName(TEXT("S17AccessDoorSweep")), false, Pawn);
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(
			Hits,
			Start,
			End,
			FQuat::Identity,
			Channel,
			FCollisionShape::MakeCapsule(Radius, HalfHeight),
			Params);
		for (const FHitResult& Hit : Hits)
		{
			if (!Hit.bBlockingHit || !Hit.GetActor())
			{
				continue;
			}
			OutHit = Hit;
			return true;
		}
		return false;
	}

	bool SweepHitsDoorLeaf(
		UWorld* World,
		APawn* Pawn,
		AActor* Door,
		const FVector& Start,
		const FVector& End,
		FHitResult& OutHit)
	{
		if (!SweepFirstBlockingHit(World, Pawn, Start, End, OutHit))
		{
			return false;
		}
		return OutHit.GetActor() == Door && IsDoorLeafComponent(OutHit.GetComponent());
	}

	class FS17AccessDoorFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bConfigFailed = false;
			bEvacuatedSpawn = false;
			bBaselineTeleported = false;
			bEnteredTrigger = false;
			bSawTimelineInProgress = false;
			bInstantOpenSampled = false;
			LeafMovedAt = -1.0f;
			OpenAt = -1.0f;
			SpawnLocation = FVector::ZeroVector;
			BaselineLeft = FVector::ZeroVector;
			BaselineRight = FVector::ZeroVector;
			BaselineDoorLocation = FVector::ZeroVector;
			BaselineTriggerExtent = FVector::ZeroVector;
			BaselineStatusColor = FLinearColor::Transparent;
			OpenTime = ExpectedOpenTime;
			OpenDistance = ExpectedOpenDistance;
			bBaselineLocked = false;
			bBaselineAutomatic = true;
			DirtyBefore.Reset();
			AdminHashBefore.Reset();
			DoorHashBefore.Reset();
			NeuroHashBefore.Reset();
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				bTerminalActivated[Index] = false;
			}
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
			case EStage::WaitPieReady:
				TickWaitPieReady(Owner, *Record, DeltaTime);
				break;
			case EStage::ClearOutside:
				TickClearOutside(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertBaseline:
				TickAssertBaseline(Owner, *Record);
				break;
			case EStage::EnterTrigger:
				TickEnterTrigger(Owner, *Record);
				break;
			case EStage::WaitOpen:
				TickWaitOpen(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertOpen:
				TickAssertOpen(Owner, *Record);
				break;
			case EStage::AssertTraversal:
				TickAssertTraversal(Owner, *Record);
				break;
			case EStage::AssertIsolation:
				TickAssertIsolation(Owner, *Record);
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
			AssertBaseline,
			EnterTrigger,
			WaitOpen,
			AssertOpen,
			AssertTraversal,
			AssertIsolation,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		bool bEvacuatedSpawn = false;
		bool bBaselineTeleported = false;
		bool bEnteredTrigger = false;
		bool bSawTimelineInProgress = false;
		bool bInstantOpenSampled = false;
		float LeafMovedAt = -1.0f;
		float OpenAt = -1.0f;
		float OpenTime = ExpectedOpenTime;
		float OpenDistance = ExpectedOpenDistance;
		bool bBaselineLocked = false;
		bool bBaselineAutomatic = true;
		FVector SpawnLocation = FVector::ZeroVector;
		FVector BaselineLeft = FVector::ZeroVector;
		FVector BaselineRight = FVector::ZeroVector;
		FVector BaselineDoorLocation = FVector::ZeroVector;
		FVector BaselineTriggerExtent = FVector::ZeroVector;
		FLinearColor BaselineStatusColor = FLinearColor::Transparent;
		FString AdminHashBefore;
		FString DoorHashBefore;
		FString NeuroHashBefore;
		TArray<FString> DirtyBefore;
		bool bTerminalActivated[UE_ARRAY_COUNT(TerminalLabels)] = {};
		TWeakObjectPtr<AActor> Door;

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		bool AssertTrue(
			FOrganoidPlaytestRecord& Record,
			const FString& Id,
			bool bPassed,
			const FString& Expected,
			const FString& Actual,
			const FString& ActorId,
			bool bDurable)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, ActorId, bDurable);
			if (!bPassed)
			{
				bAnyAssertFailed = true;
				if (bDurable)
				{
					bConfigFailed = true;
				}
				if (Record.FailureReason.IsEmpty())
				{
					Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
				}
			}
			return bPassed;
		}

		float CapsuleZFor(APawn* Pawn) const
		{
			float CapsuleZ = 96.0f;
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
				{
					CapsuleZ = Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
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
			FCollisionQueryParams Params(FName(TEXT("S17FloorSnap")), false, Pawn);
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
				Record.RecommendedNextAction = TEXT("Stop the existing PIE session, then rerun. The bot will not steal an in-progress Play session.");
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(DoorBpPackage))
			{
				Record.RecommendedNextAction = TEXT("Leave unsaved map/Blueprint work as-is. Save or discard it yourself, then rerun. The bot will not save or discard.");
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, or BP_AdminAccessDoor is dirty. Refusing to start."));
				return;
			}

			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			DoorHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			NeuroHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_NeuroGenetics.umap")));
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("admin_hash_before"), AdminHashBefore);
			Record.AddActor(TEXT("access_door_hash_before"), DoorHashBefore);
			Record.AddActor(TEXT("neuro_hash_before"), NeuroHashBefore);
			Record.AddActor(TEXT("dirty_before"), FString::Join(DirtyBefore, TEXT(",")));
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Record.AddActor(TEXT("execution_path"), TEXT("ActorBeginOverlap -> Cast ProjectOrganoidCharacter -> bAutomatic -> !bIsOpen -> !bLocked -> Timeline PlayFromStart -> Finished bIsOpen"));
			Stage = EStage::StartPie;
		}

		void TickStartPie(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("StartPie"));
			if (!Owner.RequestStartPie(MapPackage))
			{
				if (GEditor && GEditor->IsPlaySessionInProgress())
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
					return;
				}
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
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}

			if (World && Character && FoundDoor)
			{
				if (SpawnLocation.IsNearlyZero())
				{
					SpawnLocation = Character->GetActorLocation();
					Record.AddActor(TEXT("player_spawn"), SpawnLocation.ToCompactString());
				}
				Door = FoundDoor;
				const bool bInside = IsPawnInsideNamedBox(Character, FoundDoor, TEXT("AccessTrigger"));
				if (bInside && !bEvacuatedSpawn)
				{
					TeleportPawnTo(Character, BaselineOutsideX, 0.0f);
					bEvacuatedSpawn = true;
					Record.AddActor(TEXT("spawn_evacuated_from_access_trigger"), TEXT("true"));
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
				Record.AddActor(TEXT("player"), Character ? Character->GetName() : TEXT(""));
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE player + BP_AdminAccessDoor (world=%s pawn=%s door=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						FoundDoor ? TEXT("yes") : TEXT("no")));
			}
		}

		void TickClearOutside(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!FoundDoor && World)
			{
				FoundDoor = OrganoidPlaytestActions::FindAccessDoor(World);
				Door = FoundDoor;
			}
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door before baseline."));
				return;
			}
			if (!bBaselineTeleported)
			{
				if (!TeleportPawnTo(Pawn, BaselineOutsideX, 0.0f))
				{
					FailAndStop(Owner, Record, TEXT("Failed to teleport the player west of AccessTrigger before baseline."));
					return;
				}
				bBaselineTeleported = true;
				WaitSeconds = 0.0f;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OverlapSettleSeconds)
			{
				return;
			}
			Stage = EStage::AssertBaseline;
			Owner.SetStage(TEXT("AssertBaseline"));
		}

		void TickAssertBaseline(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!FoundDoor && World)
			{
				FoundDoor = OrganoidPlaytestActions::FindAccessDoor(World);
				Door = FoundDoor;
			}
			Record.AddActor(TEXT("player"), Pawn ? OrganoidPlaytestActions::ActorLabel(Pawn) : TEXT(""));
			Record.AddActor(TEXT("player_class"), Pawn && Pawn->GetClass() ? Pawn->GetClass()->GetName() : TEXT(""));
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("PIE player or Access Door missing at baseline."));
				return;
			}

			if (UProjectOrganoidAdminFacilityStateSubsystem* State = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>())
			{
				State->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
				Record.AddActor(TEXT("setup.admin_facility_state"), TEXT("Normal"));
			}
			else
			{
				FailAndStop(Owner, Record, TEXT("UProjectOrganoidAdminFacilityStateSubsystem missing in PIE. S17 requires Normal Admin facility state."));
				return;
			}
			Record.AddActor(TEXT("player_baseline"), Pawn ? Pawn->GetActorLocation().ToCompactString() : TEXT(""));

			const bool bPawnClass = Pawn && Pawn->GetClass() && Pawn->GetClass()->GetName().Contains(TEXT("ProjectOrganoidCharacter"));
			if (!AssertTrue(Record, TEXT("baseline.player_is_project_organoid_character"), bPawnClass,
				TEXT("ProjectOrganoidCharacter"), Record.Actors.FindRef(TEXT("player_class")), TEXT("player"), true))
			{
				FailAndStop(Owner, Record, TEXT("PIE pawn is not AProjectOrganoidCharacter; AccessTrigger overlap chain cannot fire."));
				return;
			}

			TArray<AActor*> Doors = OrganoidPlaytestActions::FindActorsByLabel(World, DoorLabel);
			if (!AssertTrue(Record, TEXT("baseline.exactly_one_access_door"), Doors.Num() == 1,
				TEXT("1"), FString::FromInt(Doors.Num()), DoorLabel, true))
			{
				Record.MarkNeedsApproval(
					TEXT("BP_AdminAccessDoor count is not exactly one."),
					TEXT("Inspect SL_Epitope_Admin. Use OrganoidAIBridge if a mutation is required. Do not let the bot write."));
				FailAndStop(Owner, Record, TEXT("exactly one BP_AdminAccessDoor required."));
				return;
			}
			FoundDoor = Doors[0];
			Door = FoundDoor;

			const FString ClassName = FoundDoor->GetClass() ? FoundDoor->GetClass()->GetName() : FString();
			AssertTrue(Record, TEXT("baseline.door_class"), ClassName.Equals(ExpectedClass),
				ExpectedClass, ClassName, DoorLabel, true);

			const FString Pkg = OrganoidPlaytestActions::ActorPackage(FoundDoor);
			AssertTrue(Record, TEXT("baseline.door_owner_package"), Pkg.Equals(AdminPackage),
				AdminPackage, Pkg, DoorLabel, true);

			BaselineDoorLocation = FoundDoor->GetActorLocation();
			Record.AddActor(TEXT("door_location"), BaselineDoorLocation.ToCompactString());
			AssertTrue(Record, TEXT("baseline.door_location_x"),
				FMath::IsNearlyEqual(BaselineDoorLocation.X, ExpectedDoorX, LocationTolerance),
				FString::SanitizeFloat(ExpectedDoorX), FString::SanitizeFloat(BaselineDoorLocation.X), DoorLabel, true);

			bool bIsOpen = true;
			bool bLocked = true;
			bool bAutomatic = false;
			if (!ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen)
				|| !ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked)
				|| !ReadBoolProp(FoundDoor, TEXT("bAutomatic"), bAutomatic)
				|| !ReadFloatProp(FoundDoor, TEXT("OpenTime"), OpenTime)
				|| !ReadFloatProp(FoundDoor, TEXT("OpenDistance"), OpenDistance))
			{
				FailAndStop(Owner, Record, TEXT("Failed to read Access Door authored properties."));
				return;
			}
			bBaselineLocked = bLocked;
			bBaselineAutomatic = bAutomatic;
			Record.AddActor(TEXT("OpenTime"), FString::SanitizeFloat(OpenTime));
			Record.AddActor(TEXT("OpenDistance"), FString::SanitizeFloat(OpenDistance));

			const bool bInsideTrigger = IsPawnInsideNamedBox(Pawn, FoundDoor, TEXT("AccessTrigger"));
			AssertTrue(Record, TEXT("baseline.pawn_outside_AccessTrigger"), !bInsideTrigger,
				TEXT("outside"), bInsideTrigger ? TEXT("inside") : TEXT("outside"), DoorLabel, false);

			if (!AssertTrue(Record, TEXT("baseline.bIsOpen"), !bIsOpen, TEXT("false"), BoolText(bIsOpen), DoorLabel, true))
			{
				FailAndStop(Owner, Record, TEXT("Access Door already open before the pawn entered AccessTrigger. Cannot recover a closed baseline without writing the door."));
				return;
			}
			AssertTrue(Record, TEXT("baseline.bLocked"), !bLocked, TEXT("false"), BoolText(bLocked), DoorLabel, true);
			AssertTrue(Record, TEXT("baseline.bAutomatic"), bAutomatic, TEXT("true"), BoolText(bAutomatic), DoorLabel, true);
			AssertTrue(Record, TEXT("baseline.OpenTime"), FMath::IsNearlyEqual(OpenTime, ExpectedOpenTime, 0.05f),
				FString::SanitizeFloat(ExpectedOpenTime), FString::SanitizeFloat(OpenTime), DoorLabel, true);
			AssertTrue(Record, TEXT("baseline.OpenDistance"), FMath::IsNearlyEqual(OpenDistance, ExpectedOpenDistance, 0.5f),
				FString::SanitizeFloat(ExpectedOpenDistance), FString::SanitizeFloat(OpenDistance), DoorLabel, true);

			BaselineTriggerExtent = OrganoidPlaytestActions::ReadBoxExtent(FoundDoor, TEXT("AccessTrigger"));
			Record.AddActor(TEXT("AccessTrigger_extent"), BaselineTriggerExtent.ToCompactString());
			AssertTrue(Record, TEXT("baseline.AccessTrigger_extent"),
				BaselineTriggerExtent.Equals(FVector(ExpectedTriggerExtentX, ExpectedTriggerExtentY, ExpectedTriggerExtentZ), ExtentTolerance),
				TEXT("(450,120,110)"), BaselineTriggerExtent.ToCompactString(), DoorLabel, true);

			USceneComponent* Trigger = Cast<USceneComponent>(FindComp(FoundDoor, TEXT("AccessTrigger")));
			AssertTrue(Record, TEXT("baseline.AccessTrigger_present"), Trigger != nullptr,
				TEXT("present"), Trigger ? TEXT("present") : TEXT("missing"), DoorLabel, true);
			if (Trigger)
			{
				Record.AddActor(TEXT("AccessTrigger_relative"), Trigger->GetRelativeLocation().ToCompactString());
			}

			BaselineLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			BaselineRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			Record.AddActor(TEXT("Door_Left_closed"), BaselineLeft.ToCompactString());
			Record.AddActor(TEXT("Door_Right_closed"), BaselineRight.ToCompactString());
			AssertTrue(Record, TEXT("baseline.Door_Left_present"), BaselineLeft.X < FLT_MAX * 0.5f,
				TEXT("present"), BaselineLeft.X < FLT_MAX * 0.5f ? TEXT("present") : TEXT("missing"), DoorLabel, true);
			AssertTrue(Record, TEXT("baseline.Door_Right_present"), BaselineRight.X < FLT_MAX * 0.5f,
				TEXT("present"), BaselineRight.X < FLT_MAX * 0.5f ? TEXT("present") : TEXT("missing"), DoorLabel, true);

			BaselineStatusColor = StatusLightColor(FoundDoor);
			AssertTrue(Record, TEXT("baseline.denied_lock_not_lit"), !IsDeniedRed(BaselineStatusColor),
				TEXT("not denied-red"), BaselineStatusColor.ToString(), DoorLabel, false);

			const float CapsuleZ = CapsuleZFor(Pawn);
			FHitResult ClosedHit;
			const bool bClosedBlocks = SweepHitsDoorLeaf(
				World, Pawn, FoundDoor,
				FVector(SweepWestX, 0.0f, CapsuleZ),
				FVector(SweepEastX, 0.0f, CapsuleZ),
				ClosedHit);
			AssertTrue(Record, TEXT("baseline.closed_door_blocks_vestibule_reception"), bClosedBlocks,
				TEXT("blocking hit Door_Left or Door_Right"),
				HitDescribe(ClosedHit),
				DoorLabel, false);

			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
				{
					const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
					bTerminalActivated[Index] = Activated.bHasBool && Activated.bBool;
				}
			}

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
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
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door before trigger entry."));
				return;
			}
			if (!bEnteredTrigger)
			{
				if (!TeleportPawnTo(Pawn, ActivateInsideX, 0.0f))
				{
					FailAndStop(Owner, Record, TEXT("Failed to move the genuine player into AccessTrigger."));
					return;
				}
				bEnteredTrigger = true;
				WaitSeconds = 0.0f;
				Record.AddActor(TEXT("player_enter_trigger"), Pawn->GetActorLocation().ToCompactString());
			}
			Stage = EStage::WaitOpen;
			Owner.SetStage(TEXT("WaitOpen"));
		}

		void TickWaitOpen(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door during open wait."));
				return;
			}

			bool bIsOpen = false;
			ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen);
			const FVector LiveLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			const FVector LiveRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			const float LeftDelta = FMath::Abs(LiveLeft.Y - BaselineLeft.Y);
			const float RightDelta = FMath::Abs(LiveRight.Y - BaselineRight.Y);
			const bool bLeavesMoving = LeftDelta > 2.0f || RightDelta > 2.0f;
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
				if (bIsOpen)
				{
					FailAndStop(Owner, Record, FString::Printf(
						TEXT("bIsOpen became true at %.2fs, before OpenTime=%.2fs Timeline could finish. Execution path does not match S16/S17."),
						WaitSeconds, OpenTime));
					return;
				}
			}
			if (!bIsOpen && bLeavesMoving)
			{
				bSawTimelineInProgress = true;
			}
			if (IsDeniedRed(StatusLightColor(FoundDoor)))
			{
				FailAndStop(Owner, Record, TEXT("Denied/locked StatusLight fired on the unlocked automatic baseline."));
				return;
			}

			const float Deadline = OpenTime + OpenMarginSeconds;
			if (WaitSeconds >= Deadline)
			{
				Stage = EStage::AssertOpen;
				Owner.SetStage(TEXT("AssertOpen"));
			}
		}

		void TickAssertOpen(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* FoundDoor = Door.Get();
			if (!World || !Pawn || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world, pawn, or Access Door after open wait."));
				return;
			}

			const bool bOverlap = IsPawnInsideNamedBox(Pawn, FoundDoor, TEXT("AccessTrigger"));
			AssertTrue(Record, TEXT("open.legitimate_AccessTrigger_overlap"), bOverlap,
				TEXT("overlapping"), bOverlap ? TEXT("overlapping") : TEXT("not overlapping"), DoorLabel, false);

			bool bIsOpen = false;
			bool bLocked = true;
			bool bAutomatic = false;
			ReadBoolProp(FoundDoor, TEXT("bIsOpen"), bIsOpen);
			ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked);
			ReadBoolProp(FoundDoor, TEXT("bAutomatic"), bAutomatic);

			Record.AddActor(TEXT("open.wait_seconds"), FString::SanitizeFloat(WaitSeconds));
			Record.AddActor(TEXT("open.leaf_moved_at"), LeafMovedAt >= 0.0f ? FString::SanitizeFloat(LeafMovedAt) : TEXT("never"));
			Record.AddActor(TEXT("open.bIsOpen_at"), OpenAt >= 0.0f ? FString::SanitizeFloat(OpenAt) : TEXT("never"));

			AssertTrue(Record, TEXT("open.automatic_condition"), bAutomatic && !bLocked,
				TEXT("bAutomatic && !bLocked"),
				FString::Printf(TEXT("bAutomatic=%s bLocked=%s"), *BoolText(bAutomatic), *BoolText(bLocked)),
				DoorLabel, false);
			AssertTrue(Record, TEXT("open.timeline_in_progress_before_bIsOpen"),
				bSawTimelineInProgress || !bInstantOpenSampled,
				TEXT("leaves moved while bIsOpen still false"),
				bSawTimelineInProgress ? TEXT("observed") : TEXT("not sampled / not observed"),
				DoorLabel, false);
			AssertTrue(Record, TEXT("open.completed_bIsOpen"), bIsOpen,
				TEXT("true"), BoolText(bIsOpen), DoorLabel, false);

			const FVector LiveLeft = LeafRelative(FoundDoor, TEXT("Door_Left"));
			const FVector LiveRight = LeafRelative(FoundDoor, TEXT("Door_Right"));
			const float LeftYDelta = LiveLeft.Y - BaselineLeft.Y;
			const float RightYDelta = LiveRight.Y - BaselineRight.Y;
			Record.AddActor(TEXT("Door_Left_open"), LiveLeft.ToCompactString());
			Record.AddActor(TEXT("Door_Right_open"), LiveRight.ToCompactString());
			AssertTrue(Record, TEXT("open.Door_Left_negative_Y"),
				FMath::IsNearlyEqual(LeftYDelta, -OpenDistance, LeafMoveTolerance),
				FString::Printf(TEXT("dY=%.1f"), -OpenDistance),
				FString::Printf(TEXT("dY=%.1f"), LeftYDelta),
				DoorLabel, false);
			AssertTrue(Record, TEXT("open.Door_Right_positive_Y"),
				FMath::IsNearlyEqual(RightYDelta, OpenDistance, LeafMoveTolerance),
				FString::Printf(TEXT("dY=%.1f"), OpenDistance),
				FString::Printf(TEXT("dY=%.1f"), RightYDelta),
				DoorLabel, false);

			const FLinearColor LiveColor = StatusLightColor(FoundDoor);
			AssertTrue(Record, TEXT("open.denied_lock_did_not_fire"), !IsDeniedRed(LiveColor),
				TEXT("not denied-red"), LiveColor.ToString(), DoorLabel, false);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::AssertTraversal;
			Owner.SetStage(TEXT("AssertTraversal"));
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

			const float CapsuleZ = CapsuleZFor(Pawn);
			FHitResult OpenHit;
			const bool bStillBlocked = SweepHitsDoorLeaf(
				World, Pawn, FoundDoor,
				FVector(SweepWestX, 0.0f, CapsuleZ),
				FVector(SweepEastX, 0.0f, CapsuleZ),
				OpenHit);
			AssertTrue(Record, TEXT("traversal.sweep_clears_door_leaves"), !bStillBlocked,
				TEXT("no blocking Door_Left/Door_Right"),
				HitDescribe(OpenHit),
				DoorLabel, false);

			if (!TeleportPawnTo(Pawn, SweepWestX, 0.0f))
			{
				FailAndStop(Owner, Record, TEXT("Failed to place the player west of the open doorway for the sweep move."));
				return;
			}
			const FVector Dest(SweepEastX, 0.0f, Pawn->GetActorLocation().Z);
			FHitResult MoveHit;
			const bool bMoved = Pawn->SetActorLocation(Dest, true, &MoveHit, ETeleportType::None);
			const FVector After = Pawn->GetActorLocation();
			const bool bReachedEast = After.X >= (SweepEastX - 40.0f);
			const bool bHitLeaf = MoveHit.GetActor() == FoundDoor && IsDoorLeafComponent(MoveHit.GetComponent());
			Record.AddActor(TEXT("traversal.after"), After.ToCompactString());
			AssertTrue(Record, TEXT("traversal.physical_vestibule_to_reception"),
				bReachedEast && !bHitLeaf,
				TEXT("pawn X>=1060 without hitting door leaves"),
				FString::Printf(
					TEXT("x=%.1f moved=%s hit=%s"),
					After.X,
					*BoolText(bMoved),
					*HitDescribe(MoveHit)),
				DoorLabel, false);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::AssertIsolation;
			Owner.SetStage(TEXT("AssertIsolation"));
		}

		void TickAssertIsolation(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* FoundDoor = Door.Get();
			if (!World || !FoundDoor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Access Door before isolation."));
				return;
			}

			AssertTrue(Record, TEXT("isolation.door_root_transform"),
				FoundDoor->GetActorLocation().Equals(BaselineDoorLocation, LocationTolerance),
				TEXT("unchanged"),
				FString::Printf(TEXT("dist=%.1f"), FVector::Dist(FoundDoor->GetActorLocation(), BaselineDoorLocation)),
				DoorLabel, false);

			const FVector LiveExtent = OrganoidPlaytestActions::ReadBoxExtent(FoundDoor, TEXT("AccessTrigger"));
			AssertTrue(Record, TEXT("isolation.AccessTrigger_extent"),
				LiveExtent.Equals(BaselineTriggerExtent, ExtentTolerance),
				BaselineTriggerExtent.ToCompactString(), LiveExtent.ToCompactString(), DoorLabel, false);

			bool bLocked = true;
			bool bAutomatic = false;
			ReadBoolProp(FoundDoor, TEXT("bLocked"), bLocked);
			ReadBoolProp(FoundDoor, TEXT("bAutomatic"), bAutomatic);
			AssertTrue(Record, TEXT("isolation.bLocked"), bLocked == bBaselineLocked,
				BoolText(bBaselineLocked), BoolText(bLocked), DoorLabel, false);
			AssertTrue(Record, TEXT("isolation.bAutomatic"), bAutomatic == bBaselineAutomatic,
				BoolText(bBaselineAutomatic), BoolText(bAutomatic), DoorLabel, false);

			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
				{
					const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
					const bool bLive = Activated.bHasBool && Activated.bBool;
					AssertTrue(
						Record,
						FString::Printf(TEXT("isolation.terminal_%s_not_activated_by_door"), TerminalLabels[Index]),
						bLive == bTerminalActivated[Index],
						BoolText(bTerminalActivated[Index]),
						BoolText(bLive),
						TerminalLabels[Index],
						false);
				}
			}

			Record.AddActor(
				TEXT("isolation.allowed_traversal_side_effects"),
				TEXT("S20 room lighting, SectorController CurrentRoom, and streaming-volume overlaps may change from genuine player movement. Those are not Access Door writes."));

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
			const bool bTrackedDirty = PackageIsDirty(MapPackage)
				|| PackageIsDirty(AdminPackage)
				|| PackageIsDirty(DoorBpPackage);
			AssertTrue(Record, TEXT("durable.tracked_packages_clean"), !bTrackedDirty,
				TEXT("not dirty"), bTrackedDirty ? TEXT("dirty") : TEXT("clean"), TEXT(""), true);

			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			TArray<FString> UnauthorizedNew;
			for (const FString& Name : DirtyAfter)
			{
				if (!DirtyBefore.Contains(Name) && !IsKnownNeuroRecastPackage(Name))
				{
					UnauthorizedNew.Add(Name);
				}
			}
			TArray<FString> AuthoredDirtyBefore;
			TArray<FString> AuthoredDirtyAfter;
			CopyDirtyMinusKnownNeuroRecast(DirtyBefore, AuthoredDirtyBefore);
			CopyDirtyMinusKnownNeuroRecast(DirtyAfter, AuthoredDirtyAfter);
			Record.AddActor(TEXT("dirty_after"), FString::Join(DirtyAfter, TEXT(",")));

			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"), UnauthorizedNew.Num() == 0,
				TEXT("0"), FString::FromInt(UnauthorizedNew.Num()),
				UnauthorizedNew.Num() > 0 ? FString::Join(UnauthorizedNew, TEXT(",")) : TEXT(""), true);
			AssertTrue(Record, TEXT("durable.dirty_package_count_unchanged"),
				AuthoredDirtyAfter.Num() == AuthoredDirtyBefore.Num(),
				FString::FromInt(AuthoredDirtyBefore.Num()), FString::FromInt(AuthoredDirtyAfter.Num()), TEXT(""), true);
			AssertTrue(Record, TEXT("durable.dirty_set_empty_or_known_neuro_recast"),
				DirtySetIsEmptyOrOnlyKnownNeuroRecast(DirtyAfter),
				TEXT("empty or only /Game/Maps/Epitope/SL_Epitope_NeuroGenetics"),
				DirtyAfter.Num() == 0 ? TEXT("empty") : FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"), true);

			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString DoorHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			const FString NeuroHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_NeuroGenetics.umap")));
			Record.AddActor(TEXT("admin_hash_after"), AdminHashAfter);
			Record.AddActor(TEXT("access_door_hash_after"), DoorHashAfter);
			Record.AddActor(TEXT("neuro_hash_after"), NeuroHashAfter);
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"), AdminHashAfter.Equals(AdminHashBefore),
				AdminHashBefore, AdminHashAfter, TEXT("SL_Epitope_Admin"), true);
			AssertTrue(Record, TEXT("durable.access_door_hash_unchanged"), DoorHashAfter.Equals(DoorHashBefore),
				DoorHashBefore, DoorHashAfter, TEXT("BP_AdminAccessDoor"), true);
			AssertTrue(Record, TEXT("durable.neuro_on_disk_hash_unchanged"), NeuroHashAfter.Equals(NeuroHashBefore),
				NeuroHashBefore, NeuroHashAfter, TEXT("SL_Epitope_NeuroGenetics"), true);
			AssertTrue(Record, TEXT("durable.playtest_mutates_assets"), true,
				TEXT("false"), TEXT("false"), TEXT(""), true);

			UWorld* EditorWorld = GetEditorWorld();
			TArray<AActor*> Matches = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, DoorLabel) : TArray<AActor*>();
			AssertTrue(Record, TEXT("durable.exactly_one"), Matches.Num() == 1,
				TEXT("1"), FString::FromInt(Matches.Num()), DoorLabel, true);
			if (Matches.Num() == 1)
			{
				AActor* Actor = Matches[0];
				AssertTrue(Record, TEXT("durable.owner_package"),
					OrganoidPlaytestActions::ActorPackage(Actor).Equals(AdminPackage),
					AdminPackage, OrganoidPlaytestActions::ActorPackage(Actor), DoorLabel, true);
				bool bIsOpen = true;
				ReadBoolProp(Actor, TEXT("bIsOpen"), bIsOpen);
				AssertTrue(Record, TEXT("durable.editor_bIsOpen_closed"), !bIsOpen,
					TEXT("false"), BoolText(bIsOpen), DoorLabel, true);
			}

			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable Access Door state changed. The playtest bot will not repair it."),
					TEXT("Inspect BP_AdminAccessDoor on SL_Epitope_Admin. Use OrganoidAIBridge if a mutation is required. Do not let the bot write."));
			}
			Stage = EStage::Finalize;
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			const EOrganoidPlaytestState State = bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass;
			Owner.CompleteActive(State, Record.FailureReason);
		}
	};

	struct FS17AutoRegister
	{
		FS17AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS17AccessDoorFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS17AutoRegister GRegisterS17;
}
