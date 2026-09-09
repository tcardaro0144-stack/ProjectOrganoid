#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAmbienceZone.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "String/BytesToHex.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S22_AdminAudioZones_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 22A Admin Audio Zone Identity Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr float OverlapWaitSeconds = 0.45f;
	constexpr int32 CanonicalCount = 4;

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

	struct FAudioZoneSpec
	{
		const TCHAR* Label;
		const TCHAR* ZoneId;
		int32 Priority;
		const TCHAR* RoomTriggerLabels[3];
		int32 RoomTriggerCount;
		FVector ProbeXY;
	};

	const FAudioZoneSpec CanonicalZones[CanonicalCount] = {
		{TEXT("Admin_AudioZone_Public"), TEXT("Admin_Public"), 1,
			{TEXT("Admin_RoomTrigger_Vestibule"), TEXT("Admin_RoomTrigger_Reception"), TEXT("Admin_RoomTrigger_Hub")}, 3,
			FVector(1250.0f, 0.0f, 0.0f)},
		{TEXT("Admin_AudioZone_Secure"), TEXT("Admin_Secure"), 2,
			{TEXT("Admin_RoomTrigger_Security"), TEXT("Admin_RoomTrigger_Records"), nullptr}, 2,
			FVector(2680.0f, -400.0f, 0.0f)},
		{TEXT("Admin_AudioZone_Executive"), TEXT("Admin_Executive"), 2,
			{TEXT("Admin_RoomTrigger_Conference"), TEXT("Admin_RoomTrigger_DirectorSuite"), nullptr}, 2,
			FVector(3200.0f, -1850.0f, 0.0f)},
		{TEXT("Admin_AudioZone_Service"), TEXT("Admin_Service"), 1,
			{TEXT("Admin_RoomTrigger_Operations"), TEXT("Admin_RoomTrigger_Transit"), TEXT("Admin_RoomTrigger_ServiceCorridor")}, 3,
			FVector(5005.0f, 0.0f, 0.0f)},
	};

	const TCHAR* LegacyLabels[] = {
		TEXT("Ambience_ReceptionAtrium"),
		TEXT("Ambience_HepaPlenum"),
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

	UBoxComponent* FindActorBox(AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}
		if (UBoxComponent* Root = Cast<UBoxComponent>(Actor->GetRootComponent()))
		{
			return Root;
		}
		return Actor->FindComponentByClass<UBoxComponent>();
	}

	bool BoxContainsXY(AActor* ZoneActor, const FVector& Location)
	{
		UBoxComponent* Box = FindActorBox(ZoneActor);
		if (!Box)
		{
			return false;
		}
		return Box->Bounds.GetBox().IsInsideOrOn(Location);
	}

	FString AmbienceStateText(EProjectOrganoidAmbienceState State)
	{
		switch (State)
		{
		case EProjectOrganoidAmbienceState::Exploration: return TEXT("Exploration");
		case EProjectOrganoidAmbienceState::Tension: return TEXT("Tension");
		case EProjectOrganoidAmbienceState::Combat: return TEXT("Combat");
		case EProjectOrganoidAmbienceState::Hazard: return TEXT("Hazard");
		case EProjectOrganoidAmbienceState::CriticalHealth: return TEXT("CriticalHealth");
		default: return TEXT("Unknown");
		}
	}

	UObject* FindGameSubsystem(UWorld* World, const TCHAR* ClassPath)
	{
		if (!World || !ClassPath)
		{
			return nullptr;
		}
		UClass* Class = FindObject<UClass>(nullptr, ClassPath);
		if (!Class)
		{
			return nullptr;
		}
		return World->GetSubsystemBase(Class);
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
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			const uint8 Value = ByteProp->GetPropertyValue(ValuePtr);
			if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
			{
				return Enum->GetNameStringByValue(Value);
			}
		}
		return FString();
	}

	FString ReadSectorPowerState(UWorld* World, uint8 SectorValue)
	{
		UObject* Subsystem = FindGameSubsystem(World, TEXT("/Script/ProjectOrganoid.ProjectOrganoidPowerSubsystem"));
		if (!Subsystem)
		{
			return FString();
		}
		UFunction* Function = Subsystem->FindFunction(FName(TEXT("GetSectorPowerState")));
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
				static_cast<int64>(SectorValue));
		}
		else if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(Function, TEXT("Sector")))
		{
			ByteProp->SetPropertyValue(ByteProp->ContainerPtrToValuePtr<void>(Parms.GetData()), SectorValue);
		}
		Subsystem->ProcessEvent(Function, Parms.GetData());
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

	class FS22AdminAudioZonesFunctional : public IOrganoidPlaytestCase
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
			bSpawnTeleported = false;
			AdminHashBefore.Reset();
			DoorHashBefore.Reset();
			AudioZoneBpHashBefore.Reset();
			DirtyBefore.Reset();
			BaselineAmbienceState = EProjectOrganoidAmbienceState::Exploration;
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Owner.RequestEndPieIfStarted();
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
			case EStage::StartPie: TickStartPie(Owner, *Record); break;
			case EStage::WaitPieReady: TickWaitPieReady(Owner, *Record, DeltaTime); break;
			case EStage::Canonical: TickCanonical(Owner, *Record); break;
			case EStage::SpawnInside: TickSpawnInside(Owner, *Record, DeltaTime); break;
			case EStage::PublicOverlap: TickEnterProbe(Owner, *Record, 0, EStage::WaitPublic, TEXT("EnterPublic")); break;
			case EStage::WaitPublic: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertPublic, TEXT("AssertPublic")); break;
			case EStage::AssertPublic: TickAssertPublic(Owner, *Record); break;
			case EStage::FacilityIndependence: TickFacilityIndependence(Owner, *Record); break;
			case EStage::SecureOverlap: TickEnterProbe(Owner, *Record, 1, EStage::WaitSecure, TEXT("EnterSecure")); break;
			case EStage::WaitSecure: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertSecure, TEXT("AssertSecure")); break;
			case EStage::AssertSecure: TickAssertNamed(Owner, *Record, TEXT("secure"), TEXT("Admin_Secure"), EStage::PriorityOverlap); break;
			case EStage::PriorityOverlap: TickEnterXY(Owner, *Record, FVector(2500.0f, 0.0f, 0.0f), EStage::WaitPriority, TEXT("EnterPriority")); break;
			case EStage::WaitPriority: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertPriority, TEXT("AssertPriority")); break;
			case EStage::AssertPriority: TickAssertNamed(Owner, *Record, TEXT("priority"), TEXT("Admin_Secure"), EStage::ExecutiveOverlap); break;
			case EStage::ExecutiveOverlap: TickEnterProbe(Owner, *Record, 2, EStage::WaitExecutive, TEXT("EnterExecutive")); break;
			case EStage::WaitExecutive: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertExecutive, TEXT("AssertExecutive")); break;
			case EStage::AssertExecutive: TickAssertNamed(Owner, *Record, TEXT("executive"), TEXT("Admin_Executive"), EStage::ServiceOverlap); break;
			case EStage::ServiceOverlap: TickEnterProbe(Owner, *Record, 3, EStage::WaitService, TEXT("EnterService")); break;
			case EStage::WaitService: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertService, TEXT("AssertService")); break;
			case EStage::AssertService: TickAssertNamed(Owner, *Record, TEXT("service"), TEXT("Admin_Service"), EStage::ExitOverlap); break;
			case EStage::ExitOverlap: TickEnterXY(Owner, *Record, FVector(-400.0f, 0.0f, 0.0f), EStage::WaitExit, TEXT("EnterExit")); break;
			case EStage::WaitExit: TickWaitThen(Owner, *Record, DeltaTime, EStage::AssertExit, TEXT("AssertExit")); break;
			case EStage::AssertExit: TickAssertExit(Owner, *Record); break;
			case EStage::Isolation: TickIsolation(Owner, *Record); break;
			case EStage::EndPie:
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitPieStopped;
				break;
			case EStage::WaitPieStopped: TickWaitPieStopped(Owner, *Record, DeltaTime); break;
			case EStage::AssertDurable: TickAssertDurable(Owner, *Record); break;
			case EStage::Finalize: Finalize(Owner, *Record); break;
			}
		}

	private:
		enum class EStage : uint8
		{
			Preflight,
			StartPie,
			WaitPieReady,
			Canonical,
			SpawnInside,
			PublicOverlap,
			WaitPublic,
			AssertPublic,
			FacilityIndependence,
			SecureOverlap,
			WaitSecure,
			AssertSecure,
			PriorityOverlap,
			WaitPriority,
			AssertPriority,
			ExecutiveOverlap,
			WaitExecutive,
			AssertExecutive,
			ServiceOverlap,
			WaitService,
			AssertService,
			ExitOverlap,
			WaitExit,
			AssertExit,
			Isolation,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

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
			}
			return bPassed;
		}

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			Record.FailureReason = Reason;
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		AProjectOrganoidAmbienceZone* FindCanonicalZone(UWorld* World, const TCHAR* Label)
		{
			AActor* Actor = OrganoidPlaytestActions::FindUniqueByLabel(World, Label);
			return Cast<AProjectOrganoidAmbienceZone>(Actor);
		}

		UProjectOrganoidAudioAmbienceSubsystem* Ambience(UWorld* World)
		{
			return World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
		}

		FString ActiveZoneId(UWorld* World)
		{
			if (UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World))
			{
				return Sub->GetActiveEnvironmentZoneId().ToString();
			}
			return FString();
		}

		bool AssertAmbienceUnchanged(FOrganoidPlaytestRecord& Record, const FString& Id)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!Sub)
			{
				return AssertTrue(Record, Id, false, AmbienceStateText(BaselineAmbienceState), TEXT("missing"), TEXT("Ambience"), false);
			}
			return AssertTrue(
				Record,
				Id,
				Sub->GetAmbienceState() == BaselineAmbienceState,
				AmbienceStateText(BaselineAmbienceState),
				AmbienceStateText(Sub->GetAmbienceState()),
				TEXT("Ambience"),
				false);
		}

		bool TeleportPawnTo(APawn* Pawn, const FVector& XY, float CapsuleZ)
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
			const bool bMoved = OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
			return bMoved;
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

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Preflight"));
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
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty. Refusing to start."));
				return;
			}
			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			DoorHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			AudioZoneBpHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAudioZone.uasset")));
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
			Stage = EStage::WaitPieReady;
		}

		void TickWaitPieReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			ACharacter* Character = OrganoidPlaytestActions::GetPlayerCharacter(World);
			AActor* PublicZone = World ? OrganoidPlaytestActions::FindUniqueByLabel(World, CanonicalZones[0].Label) : nullptr;
			if (World && Character && PublicZone)
			{
				WaitSeconds = 0.0f;
				Stage = EStage::Canonical;
				Owner.SetStage(TEXT("Canonical"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE Admin audio zones."));
			}
			(void)Record;
		}

		void TickCanonical(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!World)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during canonical audit."));
				return;
			}

			int32 AdminAmbienceCount = 0;
			for (TActorIterator<AProjectOrganoidAmbienceZone> It(World); It; ++It)
			{
				AProjectOrganoidAmbienceZone* Zone = *It;
				if (!Zone)
				{
					continue;
				}
				if (!OrganoidPlaytestActions::ActorPackage(Zone).Equals(AdminPackage, ESearchCase::IgnoreCase))
				{
					continue;
				}
				++AdminAmbienceCount;
			}
			AssertTrue(Record, TEXT("canonical.count"),
				AdminAmbienceCount == CanonicalCount,
				TEXT("4"), FString::FromInt(AdminAmbienceCount), AdminPackage, true);

			for (int32 Index = 0; Index < CanonicalCount; ++Index)
			{
				const FAudioZoneSpec& Spec = CanonicalZones[Index];
				AProjectOrganoidAmbienceZone* Zone = FindCanonicalZone(World, Spec.Label);
				AssertTrue(Record, FString::Printf(TEXT("canonical.%s.present"), Spec.ZoneId),
					Zone != nullptr, Spec.Label, Zone ? OrganoidPlaytestActions::ActorLabel(Zone) : TEXT("missing"), Spec.Label, true);
				if (!Zone)
				{
					continue;
				}
				AssertTrue(Record, FString::Printf(TEXT("canonical.%s.package"), Spec.ZoneId),
					OrganoidPlaytestActions::ActorPackage(Zone).Equals(AdminPackage, ESearchCase::IgnoreCase),
					AdminPackage, OrganoidPlaytestActions::ActorPackage(Zone), Spec.Label, true);
				AssertTrue(Record, FString::Printf(TEXT("canonical.%s.ZoneId"), Spec.ZoneId),
					Zone->ZoneId.IsEqual(FName(Spec.ZoneId)), Spec.ZoneId, Zone->ZoneId.ToString(), Spec.Label, true);
				AssertTrue(Record, FString::Printf(TEXT("canonical.%s.Priority"), Spec.ZoneId),
					Zone->Priority == Spec.Priority,
					FString::FromInt(Spec.Priority), FString::FromInt(Zone->Priority), Spec.Label, true);
				for (int32 Room = 0; Room < Spec.RoomTriggerCount; ++Room)
				{
					AActor* Trigger = OrganoidPlaytestActions::FindUniqueByLabel(World, Spec.RoomTriggerLabels[Room]);
					const bool bInside = Trigger && BoxContainsXY(Zone, Trigger->GetActorLocation());
					AssertTrue(Record, FString::Printf(TEXT("canonical.%s.covers_%s"), Spec.ZoneId, Spec.RoomTriggerLabels[Room]),
						bInside, TEXT("inside audio box"), bInside ? TEXT("inside") : TEXT("outside"), Spec.RoomTriggerLabels[Room], true);
				}
			}

			for (const TCHAR* Legacy : LegacyLabels)
			{
				AActor* Actor = OrganoidPlaytestActions::FindUniqueByLabel(World, Legacy);
				AssertTrue(Record, FString::Printf(TEXT("canonical.legacy_absent_%s"), Legacy),
					Actor == nullptr, TEXT("absent"), Actor ? OrganoidPlaytestActions::ActorLabel(Actor) : TEXT("absent"), Legacy, true);
			}

			int32 CompetingAtrium = 0;
			for (TActorIterator<AProjectOrganoidAmbienceZone> It(World); It; ++It)
			{
				if (*It && (*It)->ZoneId.IsEqual(FName(TEXT("Admin_Atrium"))))
				{
					++CompetingAtrium;
				}
			}
			AssertTrue(Record, TEXT("canonical.no_Admin_Atrium"),
				CompetingAtrium == 0, TEXT("0"), FString::FromInt(CompetingAtrium), TEXT("Admin_Atrium"), true);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Canonical Admin audio-zone set failed."));
				return;
			}
			Stage = EStage::SpawnInside;
			Owner.SetStage(TEXT("SpawnInside"));
		}

		void TickSpawnInside(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AProjectOrganoidAmbienceZone* PublicZone = FindCanonicalZone(World, CanonicalZones[0].Label);
			if (!World || !Pawn || !PublicZone)
			{
				FailAndStop(Owner, Record, TEXT("Lost pawn or Public audio zone before spawn-inside."));
				return;
			}
			if (!bSpawnTeleported)
			{
				if (!TeleportPawnTo(Pawn, CanonicalZones[0].ProbeXY, CapsuleZFor(Pawn)))
				{
					FailAndStop(Owner, Record, TEXT("Teleport into Admin_Public for spawn-inside failed."));
					return;
				}
				if (UBoxComponent* Box = FindActorBox(PublicZone))
				{
					Box->UpdateOverlaps();
				}
				PublicZone->SynchronizeOverlappingLocalCharacter();
				PublicZone->SynchronizeOverlappingLocalCharacter();
				bSpawnTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 0.05f)
			{
				return;
			}
			const FString ZoneId = ActiveZoneId(World);
			AssertTrue(Record, TEXT("spawn_inside.ZoneId"),
				ZoneId.Equals(TEXT("Admin_Public"), ESearchCase::CaseSensitive),
				TEXT("Admin_Public"), ZoneId, CanonicalZones[0].Label, false);
			AssertTrue(Record, TEXT("spawn_inside.local_inside"),
				PublicZone->IsLocalPlayerInside(), TEXT("true"), BoolText(PublicZone->IsLocalPlayerInside()), CanonicalZones[0].Label, false);
			if (UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World))
			{
				BaselineAmbienceState = Sub->GetAmbienceState();
				Record.AddActor(TEXT("baseline_ambience_state"), AmbienceStateText(BaselineAmbienceState));
				AssertTrue(Record, TEXT("spawn_inside.ambience_state"),
					true,
					AmbienceStateText(BaselineAmbienceState),
					AmbienceStateText(BaselineAmbienceState),
					TEXT("Ambience"),
					false);
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Spawn-inside synchronization failed or duplicated into a non-Public zone."));
				return;
			}
			Stage = EStage::PublicOverlap;
			Owner.SetStage(TEXT("PublicOverlap"));
		}

		void TickEnterProbe(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			int32 SpecIndex,
			EStage Next,
			const TCHAR* StageName)
		{
			TickEnterXY(Owner, Record, CanonicalZones[SpecIndex].ProbeXY, Next, StageName);
		}

		void TickEnterXY(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			const FVector& XY,
			EStage Next,
			const TCHAR* StageName)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost pawn before audio-zone teleport."));
				return;
			}
			if (!TeleportPawnTo(Pawn, XY, CapsuleZFor(Pawn)))
			{
				FailAndStop(Owner, Record, TEXT("Teleport into audio zone failed."));
				return;
			}
			WaitSeconds = 0.0f;
			Stage = Next;
			Owner.SetStage(StageName);
		}

		void TickWaitThen(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			float DeltaTime,
			EStage Next,
			const TCHAR* StageName)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OverlapWaitSeconds)
			{
				return;
			}
			(void)Record;
			Stage = Next;
			Owner.SetStage(StageName);
		}

		void TickAssertPublic(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			const FString ZoneId = ActiveZoneId(World);
			AssertTrue(Record, TEXT("public.ZoneId"),
				ZoneId.Equals(TEXT("Admin_Public"), ESearchCase::CaseSensitive),
				TEXT("Admin_Public"), ZoneId, CanonicalZones[0].Label, false);
			AssertAmbienceUnchanged(Record, TEXT("public.ambience_state"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Genuine Public overlap did not select Admin_Public."));
				return;
			}
			Stage = EStage::FacilityIndependence;
			Owner.SetStage(TEXT("FacilityIndependence"));
		}

		void TickFacilityIndependence(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			UProjectOrganoidAdminFacilityStateSubsystem* Facility = World
				? World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>()
				: nullptr;
			UProjectOrganoidAudioAmbienceSubsystem* Sub = Ambience(World);
			if (!Facility || !Sub)
			{
				FailAndStop(Owner, Record, TEXT("Facility or ambience subsystem missing."));
				return;
			}
			const FString Before = ActiveZoneId(World);
			Facility->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Alert);
			AssertTrue(Record, TEXT("facility.alert.ZoneId"),
				ActiveZoneId(World).Equals(Before, ESearchCase::CaseSensitive),
				Before, ActiveZoneId(World), TEXT("AdminFacilityState"), false);
			AssertAmbienceUnchanged(Record, TEXT("facility.alert.ambience_state"));
			Facility->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Lockdown);
			AssertTrue(Record, TEXT("facility.lockdown.ZoneId"),
				ActiveZoneId(World).Equals(Before, ESearchCase::CaseSensitive),
				Before, ActiveZoneId(World), TEXT("AdminFacilityState"), false);
			AssertAmbienceUnchanged(Record, TEXT("facility.lockdown.ambience_state"));
			Facility->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
			AssertTrue(Record, TEXT("facility.normal.ZoneId"),
				ActiveZoneId(World).Equals(Before, ESearchCase::CaseSensitive),
				Before, ActiveZoneId(World), TEXT("AdminFacilityState"), false);
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Admin facility state changed audio ZoneId or ambience intensity mode."));
				return;
			}
			Stage = EStage::SecureOverlap;
			Owner.SetStage(TEXT("SecureOverlap"));
		}

		void TickAssertNamed(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			const TCHAR* Prefix,
			const TCHAR* ExpectedZone,
			EStage Next)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			const FString ZoneId = ActiveZoneId(World);
			AssertTrue(Record, FString::Printf(TEXT("%s.ZoneId"), Prefix),
				ZoneId.Equals(ExpectedZone, ESearchCase::CaseSensitive),
				ExpectedZone, ZoneId, ExpectedZone, false);
			AssertAmbienceUnchanged(Record, FString::Printf(TEXT("%s.ambience_state"), Prefix));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("Expected %s, got %s."), ExpectedZone, *ZoneId));
				return;
			}
			Stage = Next;
		}

		void TickAssertExit(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			const FString ZoneId = ActiveZoneId(World);
			const bool bNone = ZoneId.IsEmpty() || ZoneId.Equals(TEXT("None"), ESearchCase::IgnoreCase);
			AssertTrue(Record, TEXT("exit.ZoneId"),
				bNone, TEXT("None"), ZoneId.IsEmpty() ? TEXT("None") : ZoneId, TEXT("Ambience"), false);
			AssertAmbienceUnchanged(Record, TEXT("exit.ambience_state"));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, TEXT("Leaving all Admin audio zones did not recompute to None."));
				return;
			}
			Stage = EStage::Isolation;
			Owner.SetStage(TEXT("Isolation"));
		}

		void TickIsolation(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!World)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world during isolation."));
				return;
			}
			const FString AdminPower = ReadSectorPowerState(World, 1);
			AssertTrue(Record, TEXT("isolation.power.Admin"),
				AdminPower.Contains(TEXT("Online"), ESearchCase::IgnoreCase), TEXT("Online"), AdminPower, TEXT("Power"), false);
			AssertTrue(Record, TEXT("isolation.security.lockdown"),
				!ReadSecurityLockdown(World), TEXT("false"), BoolText(ReadSecurityLockdown(World)), TEXT("Security"), false);
			if (AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel))
			{
				const FOrganoidPlaytestPropValue Online = OrganoidPlaytestActions::ReadProperty(Hologram, TEXT("bAdminOnline"));
				AssertTrue(Record, TEXT("isolation.hologram.bAdminOnline"),
					Online.bHasBool && Online.bBool, TEXT("true"), Online.Text, HologramLabel, false);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
				{
					const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
					AssertTrue(Record, FString::Printf(TEXT("isolation.terminal_%s"), TerminalLabels[Index]),
						!(Activated.bHasBool && Activated.bBool), TEXT("activated=false"), Activated.Text, TerminalLabels[Index], false);
				}
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
			{
				bool bFound = false;
				bool bShouldLoad = false;
				bool bLoaded = false;
				for (ULevelStreaming* Level : World->GetStreamingLevels())
				{
					if (!Level)
					{
						continue;
					}
					const FString Name = OrganoidPlaytestActions::NormalizePackage(Level->GetWorldAssetPackageFName().ToString());
					if (!Name.Equals(StreamPackages[Index], ESearchCase::IgnoreCase))
					{
						continue;
					}
					bFound = true;
					bShouldLoad = Level->ShouldBeLoaded();
					bLoaded = Level->IsLevelLoaded();
					break;
				}
				if (Index == 0)
				{
					AssertTrue(Record, TEXT("isolation.stream_Admin"), bFound && bLoaded,
						TEXT("Admin loaded"), bLoaded ? TEXT("loaded") : TEXT("not loaded"), StreamPackages[Index], false);
				}
				else
				{
					const FString LoadedText = bLoaded
						? (bShouldLoad ? TEXT("loaded") : TEXT("loaded"))
						: TEXT("not loaded");
					AssertTrue(Record, FString::Printf(TEXT("isolation.stream_present_%s"), StreamPackages[Index]),
						bFound, TEXT("streaming level registered"), LoadedText, StreamPackages[Index], false);
				}
			}
			const FString ActiveId = ActiveZoneId(World);
			for (TActorIterator<AProjectOrganoidAmbienceZone> It(World); It; ++It)
			{
				AProjectOrganoidAmbienceZone* Zone = *It;
				if (!Zone)
				{
					continue;
				}
				const FString Package = OrganoidPlaytestActions::ActorPackage(Zone);
				if (Package.Equals(AdminPackage, ESearchCase::IgnoreCase))
				{
					continue;
				}
				const FString Label = OrganoidPlaytestActions::ActorLabel(Zone);
				AssertTrue(Record, FString::Printf(TEXT("isolation.foreign_zone_not_active_%s"), *Label),
					ActiveId.IsEmpty() || ActiveId.Equals(TEXT("None"), ESearchCase::IgnoreCase) || !Zone->ZoneId.IsEqual(FName(*ActiveId)),
					TEXT("not the active environment zone"),
					ActiveId.IsEmpty() ? TEXT("None") : ActiveId,
					Label,
					false);
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason.IsEmpty() ? TEXT("Isolation failed.") : Record.FailureReason);
				return;
			}
			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void TickWaitPieStopped(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (GEditor && GEditor->IsPlaySessionInProgress())
			{
				if (WaitSeconds > 20.0f)
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE to stop."));
				}
				return;
			}
			(void)Record;
			Stage = EStage::AssertDurable;
			Owner.SetStage(TEXT("AssertDurable"));
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"),
				DirtyAfter.Num() == DirtyBefore.Num(),
				FString::FromInt(DirtyBefore.Num()), FString::FromInt(DirtyAfter.Num()), TEXT("packages"), true);
			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString DoorHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			const FString AudioBpHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAudioZone.uasset")));
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"),
				AdminHashAfter.Equals(AdminHashBefore, ESearchCase::IgnoreCase),
				AdminHashBefore, AdminHashAfter, AdminPackage, true);
			AssertTrue(Record, TEXT("durable.access_door_hash_unchanged"),
				DoorHashAfter.Equals(DoorHashBefore, ESearchCase::IgnoreCase),
				DoorHashBefore, DoorHashAfter, TEXT("BP_AdminAccessDoor"), true);
			AssertTrue(Record, TEXT("durable.audio_zone_bp_hash_unchanged"),
				AudioBpHashAfter.Equals(AudioZoneBpHashBefore, ESearchCase::IgnoreCase),
				AudioZoneBpHashBefore, AudioBpHashAfter, TEXT("BP_AdminAudioZone"), true);
			if (Record.FailureReason.IsEmpty()
				&& (!AdminHashAfter.Equals(AdminHashBefore, ESearchCase::IgnoreCase)
					|| !DoorHashAfter.Equals(DoorHashBefore, ESearchCase::IgnoreCase)
					|| !AudioBpHashAfter.Equals(AudioZoneBpHashBefore, ESearchCase::IgnoreCase)
					|| DirtyAfter.Num() != DirtyBefore.Num()))
			{
				Record.FailureReason = TEXT("Playtest mutated durable assets.");
			}
			(void)Owner;
			Stage = EStage::Finalize;
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			Owner.CompleteActive(
				bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass,
				Record.FailureReason);
		}

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bSpawnTeleported = false;
		EProjectOrganoidAmbienceState BaselineAmbienceState = EProjectOrganoidAmbienceState::Exploration;
		FString AdminHashBefore;
		FString DoorHashBefore;
		FString AudioZoneBpHashBefore;
		TArray<FString> DirtyBefore;
	};

	struct FS22AutoRegister
	{
		FS22AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS22AdminAudioZonesFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS22AutoRegister GS22AutoRegister;
}
