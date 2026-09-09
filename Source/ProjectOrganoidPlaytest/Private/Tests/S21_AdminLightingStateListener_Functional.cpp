#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/PointLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAdminLightControllerFacilityStateListener.h"
#include "ProjectOrganoidAdminLightingLibrary.h"
#include "String/BytesToHex.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S21_AdminLightingStateListener_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 21C Admin Lighting Facility-State Listener Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR LightControllerBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController");
	constexpr TCHAR LightControllerLabel[] = TEXT("Admin_LightController");
	constexpr TCHAR SectorControllerLabel[] = TEXT("Admin_SectorController");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr TCHAR ZoneLightPrefix[] = TEXT("Admin_ZoneLight_");
	constexpr TCHAR ZoneLightTag[] = TEXT("Admin_ZoneLight");
	constexpr float ActiveIntensity = 5000.0f;
	constexpr float InactiveIntensity = 1250.0f;
	constexpr float IntensityTolerance = 1.0f;
	constexpr float ColorTolerance = 0.02f;
	constexpr float AdminSectorIntensity = 4500.0f;
	constexpr float OverlapWaitSeconds = 0.45f;
	constexpr int32 RoomCount = 10;

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

	struct FRoomSpec
	{
		const TCHAR* RoomId;
		const TCHAR* TriggerLabel;
		const TCHAR* LightLabel;
	};

	const FRoomSpec Rooms[RoomCount] = {
		{TEXT("Vestibule"), TEXT("Admin_RoomTrigger_Vestibule"), TEXT("Admin_ZoneLight_Vestibule")},
		{TEXT("Reception"), TEXT("Admin_RoomTrigger_Reception"), TEXT("Admin_ZoneLight_Reception")},
		{TEXT("Hub"), TEXT("Admin_RoomTrigger_Hub"), TEXT("Admin_ZoneLight_Hub")},
		{TEXT("Security"), TEXT("Admin_RoomTrigger_Security"), TEXT("Admin_ZoneLight_Security")},
		{TEXT("Records"), TEXT("Admin_RoomTrigger_Records"), TEXT("Admin_ZoneLight_Records")},
		{TEXT("Conference"), TEXT("Admin_RoomTrigger_Conference"), TEXT("Admin_ZoneLight_Conference")},
		{TEXT("DirectorSuite"), TEXT("Admin_RoomTrigger_DirectorSuite"), TEXT("Admin_ZoneLight_DirectorSuite")},
		{TEXT("Operations"), TEXT("Admin_RoomTrigger_Operations"), TEXT("Admin_ZoneLight_Operations")},
		{TEXT("Transit"), TEXT("Admin_RoomTrigger_Transit"), TEXT("Admin_ZoneLight_Transit")},
		{TEXT("ServiceCorridor"), TEXT("Admin_RoomTrigger_ServiceCorridor"), TEXT("Admin_ZoneLight_ServiceCorridor")},
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

	UBoxComponent* FindTriggerBox(AActor* Trigger)
	{
		if (!Trigger)
		{
			return nullptr;
		}
		if (UBoxComponent* Root = Cast<UBoxComponent>(Trigger->GetRootComponent()))
		{
			return Root;
		}
		return Trigger->FindComponentByClass<UBoxComponent>();
	}

	bool IsS20ZoneLight(AActor* Actor)
	{
		APointLight* Point = Cast<APointLight>(Actor);
		if (!Point || !IsValid(Point))
		{
			return false;
		}
		if (!OrganoidPlaytestActions::ActorPackage(Point).Equals(AdminPackage, ESearchCase::IgnoreCase))
		{
			return false;
		}
		if (!Point->ActorHasTag(FName(ZoneLightTag)))
		{
			return false;
		}
		return OrganoidPlaytestActions::ActorLabel(Point).StartsWith(ZoneLightPrefix, ESearchCase::CaseSensitive);
	}

	float LightIntensity(AActor* Actor)
	{
		if (APointLight* Point = Cast<APointLight>(Actor))
		{
			if (ULightComponent* Light = Point->GetLightComponent())
			{
				return Light->Intensity;
			}
		}
		return -1.0f;
	}

	FLinearColor ZoneLightColor(AActor* Actor)
	{
		if (APointLight* Point = Cast<APointLight>(Actor))
		{
			if (ULightComponent* Light = Point->GetLightComponent())
			{
				return Light->GetLightColor();
			}
		}
		return FLinearColor::Transparent;
	}

	bool IntensityEquals(float Actual, float Expected)
	{
		return FMath::IsNearlyEqual(Actual, Expected, IntensityTolerance);
	}

	bool ColorEquals(const FLinearColor& Actual, const FLinearColor& Expected)
	{
		return Actual.Equals(Expected, ColorTolerance);
	}

	FString IntensityText(float Value)
	{
		return FString::Printf(TEXT("%.1f"), Value);
	}

	FString ColorText(const FLinearColor& Color)
	{
		return FString::Printf(TEXT("R=%.4f G=%.4f B=%.4f"), Color.R, Color.G, Color.B);
	}

	bool ReadBoolProp(UObject* Object, const TCHAR* Name, bool& OutValue)
	{
		OutValue = false;
		if (!Object)
		{
			return false;
		}
		const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Object->GetClass(), Name);
		if (!Prop)
		{
			return false;
		}
		OutValue = Prop->GetPropertyValue_InContainer(Object);
		return true;
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

	class FS21AdminLightingStateListenerFunctional : public IOrganoidPlaytestCase
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
			bClearTeleported = false;
			PendingRoomId.Reset();
			AdminHashBefore.Reset();
			LightControllerHashBefore.Reset();
			DoorHashBefore.Reset();
			DirtyBefore.Reset();
			HologramAdminColor = FLinearColor::Transparent;
			HologramAdminIntensity = -1.0f;
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
			case EStage::Preflight:
				TickPreflight(Owner, *Record);
				break;
			case EStage::StartPie:
				TickStartPie(Owner, *Record);
				break;
			case EStage::WaitPieReady:
				TickWaitPieReady(Owner, *Record, DeltaTime);
				break;
			case EStage::ClearSpawn:
				TickClearSpawn(Owner, *Record, DeltaTime);
				break;
			case EStage::AssertNormalNone:
				TickAssertNormalNone(Owner, *Record);
				break;
			case EStage::EnterRoom:
				TickEnterRoom(Owner, *Record);
				break;
			case EStage::WaitOverlap:
				TickWaitOverlap(Owner, *Record, DeltaTime);
				break;
			case EStage::AfterVestibuleNormal:
				TickAfterVestibuleNormal(Owner, *Record);
				break;
			case EStage::AfterAlertSameZone:
				TickAfterAlertSameZone(Owner, *Record);
				break;
			case EStage::AfterSecurityAlert:
				TickAfterSecurityAlert(Owner, *Record);
				break;
			case EStage::AfterLockdownSameZone:
				TickAfterLockdownSameZone(Owner, *Record);
				break;
			case EStage::AfterRecordsLockdown:
				TickAfterRecordsLockdown(Owner, *Record);
				break;
			case EStage::AfterLockdownToAlert:
				TickAfterLockdownToAlert(Owner, *Record);
				break;
			case EStage::AfterRelock:
				TickAfterRelock(Owner, *Record);
				break;
			case EStage::AfterLockdownToNormal:
				TickAfterLockdownToNormal(Owner, *Record);
				break;
			case EStage::Isolation:
				TickIsolation(Owner, *Record);
				break;
			case EStage::EndPie:
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.0f;
				Stage = EStage::WaitPieStopped;
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
			ClearSpawn,
			AssertNormalNone,
			EnterRoom,
			WaitOverlap,
			AfterVestibuleNormal,
			AfterAlertSameZone,
			AfterSecurityAlert,
			AfterLockdownSameZone,
			AfterRecordsLockdown,
			AfterLockdownToAlert,
			AfterRelock,
			AfterLockdownToNormal,
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
				if (bDurable)
				{
					bConfigFailed = true;
				}
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

		AActor* FindZoneLight(UWorld* World, const TCHAR* Label)
		{
			return OrganoidPlaytestActions::FindUniqueByLabel(World, Label);
		}

		int32 RoomIndexById(const TCHAR* RoomId) const
		{
			for (int32 Index = 0; Index < RoomCount; ++Index)
			{
				if (FCString::Strcmp(Rooms[Index].RoomId, RoomId) == 0)
				{
					return Index;
				}
			}
			return INDEX_NONE;
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

		bool AssertZoneContract(
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			const FString& Prefix,
			const TCHAR* ExpectedZone,
			EProjectOrganoidAdminFacilityState ExpectedState,
			bool bExpectDoorLocked)
		{
			AActor* Lights = LightController.Get();
			UProjectOrganoidAdminFacilityStateSubsystem* State = Subsystem.Get();
			const FLinearColor ExpectedColor = UProjectOrganoidAdminLightingLibrary::GetAdminFacilityStateLightColor(ExpectedState);
			bool bAll = true;
			if (State)
			{
				bAll &= AssertTrue(Record, Prefix + TEXT(".subsystem"),
					State->GetAdminFacilityState() == ExpectedState,
					StateText(ExpectedState), StateText(State->GetAdminFacilityState()),
					TEXT("AdminFacilityState"), false);
			}
			const FString Zone = Lights ? OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone")).Text : FString();
			const bool bZoneOk = ExpectedZone == nullptr
				|| FCString::Strcmp(ExpectedZone, TEXT("None")) == 0
				? (Zone.IsEmpty() || Zone.Equals(TEXT("None"), ESearchCase::IgnoreCase))
				: Zone.Equals(ExpectedZone, ESearchCase::CaseSensitive);
			bAll &= AssertTrue(Record, Prefix + TEXT(".CurrentLightingZone"),
				bZoneOk, ExpectedZone ? ExpectedZone : TEXT("None"), Zone, LightControllerLabel, false);

			const int32 ActiveIndex = (ExpectedZone && FCString::Strcmp(ExpectedZone, TEXT("None")) != 0)
				? RoomIndexById(ExpectedZone)
				: INDEX_NONE;
			for (int32 Index = 0; Index < RoomCount; ++Index)
			{
				AActor* Light = FindZoneLight(World, Rooms[Index].LightLabel);
				const float Intensity = LightIntensity(Light);
				const float ExpectedIntensity = (ActiveIndex == INDEX_NONE || Index == ActiveIndex)
					? ActiveIntensity
					: InactiveIntensity;
				bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.intensity_%s"), *Prefix, Rooms[Index].RoomId),
					IntensityEquals(Intensity, ExpectedIntensity),
					IntensityText(ExpectedIntensity), IntensityText(Intensity), Rooms[Index].LightLabel, false);
				const FLinearColor ActualColor = ZoneLightColor(Light);
				bAll &= AssertTrue(Record, FString::Printf(TEXT("%s.color_%s"), *Prefix, Rooms[Index].RoomId),
					ColorEquals(ActualColor, ExpectedColor),
					ColorText(ExpectedColor), ColorText(ActualColor), Rooms[Index].LightLabel, false);
			}

			if (AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World))
			{
				bool bLocked = false;
				ReadBoolProp(Door, TEXT("bLocked"), bLocked);
				bAll &= AssertTrue(Record, Prefix + TEXT(".door.bLocked"),
					bLocked == bExpectDoorLocked,
					BoolText(bExpectDoorLocked), BoolText(bLocked), DoorLabel, false);
			}
			return bAll;
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(LightControllerBpPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, or BP_AdminLightController is dirty. Refusing to start."));
				return;
			}
			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			LightControllerHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController.uasset")));
			DoorHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
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
			TArray<AActor*> Controllers = World ? OrganoidPlaytestActions::FindActorsByLabel(World, LightControllerLabel) : TArray<AActor*>();
			if (World && Character && Controllers.Num() == 1)
			{
				bClearTeleported = false;
				WaitSeconds = 0.0f;
				Stage = EStage::ClearSpawn;
				Owner.SetStage(TEXT("ClearSpawn"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE Admin lighting listener."));
			}
			(void)Record;
		}

		void TickClearSpawn(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!World || !Pawn)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or pawn before spawn clear."));
				return;
			}
			if (!bClearTeleported)
			{
				TeleportPawnTo(Pawn, FVector(950.0f, 250.0f, 0.0f), CapsuleZFor(Pawn));
				bClearTeleported = true;
				WaitSeconds = 0.0f;
				return;
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 0.25f)
			{
				return;
			}

			AActor* Lights = OrganoidPlaytestActions::FindUniqueByLabel(World, LightControllerLabel);
			LightController = Lights;
			Subsystem = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>();
			if (!Lights || !Subsystem.Get())
			{
				FailAndStop(Owner, Record, TEXT("Admin_LightController or AdminFacilityStateSubsystem missing."));
				return;
			}
			Listener = Lights->FindComponentByClass<UProjectOrganoidAdminLightControllerFacilityStateListener>();
			if (FNameProperty* ZoneProp = FindFProperty<FNameProperty>(Lights->GetClass(), TEXT("CurrentLightingZone")))
			{
				ZoneProp->SetPropertyValue_InContainer(Lights, NAME_None);
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
			UProjectOrganoidAdminLightingLibrary::ApplyAdminZoneLighting(World, NAME_None, ActiveIntensity, 0.25f);
			Stage = EStage::AssertNormalNone;
			Owner.SetStage(TEXT("AssertNormalNone"));
		}

		void TickAssertNormalNone(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertTrue(Record, TEXT("baseline.listener_present"), Listener.Get() != nullptr,
				TEXT("FacilityStateListener"), Listener.Get() ? TEXT("present") : TEXT("missing"), LightControllerLabel, true))
			{
				FailAndStop(Owner, Record, TEXT("BP_AdminLightController is missing UProjectOrganoidAdminLightControllerFacilityStateListener."));
				return;
			}
			if (!AssertZoneContract(Record, World, TEXT("normal.none"), TEXT("None"),
				EProjectOrganoidAdminFacilityState::Normal, false))
			{
				FailAndStop(Owner, Record, TEXT("Initial Normal presentation failed."));
				return;
			}
			if (AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel))
			{
				if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Hologram, TEXT("Light_Admin"))))
				{
					HologramAdminColor = Light->GetLightColor();
					HologramAdminIntensity = Light->Intensity;
				}
			}
			PendingRoomId = TEXT("Vestibule");
			AfterOverlapStage = EStage::AfterVestibuleNormal;
			Stage = EStage::EnterRoom;
			Owner.SetStage(TEXT("EnterRoom_Vestibule"));
		}

		void TickEnterRoom(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			const int32 Index = RoomIndexById(*PendingRoomId);
			AActor* Trigger = (Index != INDEX_NONE)
				? OrganoidPlaytestActions::FindUniqueByLabel(World, Rooms[Index].TriggerLabel)
				: nullptr;
			if (!Pawn || !Trigger)
			{
				FailAndStop(Owner, Record, TEXT("Lost pawn or room trigger before teleport."));
				return;
			}
			if (!TeleportPawnTo(Pawn, Trigger->GetActorLocation(), CapsuleZFor(Pawn)))
			{
				FailAndStop(Owner, Record, TEXT("Teleport into room trigger failed."));
				return;
			}
			if (UBoxComponent* Box = FindTriggerBox(Trigger))
			{
				Box->UpdateOverlaps();
			}
			WaitSeconds = 0.0f;
			Stage = EStage::WaitOverlap;
			Owner.SetStage(FString::Printf(TEXT("WaitOverlap_%s"), *PendingRoomId));
		}

		void TickWaitOverlap(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OverlapWaitSeconds)
			{
				return;
			}
			(void)Record;
			Stage = AfterOverlapStage;
			Owner.SetStage(TEXT("AfterOverlap"));
		}

		void TickAfterVestibuleNormal(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("normal.vestibule"), TEXT("Vestibule"),
				EProjectOrganoidAdminFacilityState::Normal, false))
			{
				FailAndStop(Owner, Record, TEXT("Normal Vestibule 5000/1250 white failed."));
				return;
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Alert);
			Stage = EStage::AfterAlertSameZone;
			Owner.SetStage(TEXT("AfterAlertSameZone"));
		}

		void TickAfterAlertSameZone(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("alert.vestibule"), TEXT("Vestibule"),
				EProjectOrganoidAdminFacilityState::Alert, false))
			{
				FailAndStop(Owner, Record, TEXT("Alert did not keep Vestibule zone or S20 intensity, or door locked on Alert."));
				return;
			}
			PendingRoomId = TEXT("Security");
			AfterOverlapStage = EStage::AfterSecurityAlert;
			Stage = EStage::EnterRoom;
			Owner.SetStage(TEXT("EnterRoom_Security"));
		}

		void TickAfterSecurityAlert(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("alert.security"), TEXT("Security"),
				EProjectOrganoidAdminFacilityState::Alert, false))
			{
				FailAndStop(Owner, Record, TEXT("Room change while Alert did not compose Security + amber."));
				return;
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Lockdown);
			Stage = EStage::AfterLockdownSameZone;
			Owner.SetStage(TEXT("AfterLockdownSameZone"));
		}

		void TickAfterLockdownSameZone(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("lockdown.security"), TEXT("Security"),
				EProjectOrganoidAdminFacilityState::Lockdown, true))
			{
				FailAndStop(Owner, Record, TEXT("Lockdown did not keep Security zone, S20 intensity, red presentation, or §21B lock."));
				return;
			}
			PendingRoomId = TEXT("Records");
			AfterOverlapStage = EStage::AfterRecordsLockdown;
			Stage = EStage::EnterRoom;
			Owner.SetStage(TEXT("EnterRoom_Records"));
		}

		void TickAfterRecordsLockdown(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("lockdown.records"), TEXT("Records"),
				EProjectOrganoidAdminFacilityState::Lockdown, true))
			{
				FailAndStop(Owner, Record, TEXT("Room change while Lockdown did not compose Records + red."));
				return;
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Alert);
			Stage = EStage::AfterLockdownToAlert;
			Owner.SetStage(TEXT("AfterLockdownToAlert"));
		}

		void TickAfterLockdownToAlert(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("restore.alert.records"), TEXT("Records"),
				EProjectOrganoidAdminFacilityState::Alert, false))
			{
				FailAndStop(Owner, Record, TEXT("Lockdown→Alert did not restore amber with Records preserved."));
				return;
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Lockdown);
			Stage = EStage::AfterRelock;
			Owner.SetStage(TEXT("AfterRelock"));
		}

		void TickAfterRelock(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("relock.records"), TEXT("Records"),
				EProjectOrganoidAdminFacilityState::Lockdown, true))
			{
				FailAndStop(Owner, Record, TEXT("Re-entering Lockdown after Alert restore failed."));
				return;
			}
			Subsystem->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Normal);
			Stage = EStage::AfterLockdownToNormal;
			Owner.SetStage(TEXT("AfterLockdownToNormal"));
		}

		void TickAfterLockdownToNormal(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!AssertZoneContract(Record, World, TEXT("restore.normal.records"), TEXT("Records"),
				EProjectOrganoidAdminFacilityState::Normal, false))
			{
				FailAndStop(Owner, Record, TEXT("Lockdown→Normal did not restore white with Records 5000/1250."));
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
			const FString NeuroPower = ReadSectorPowerState(World, 2);
			const FString CryoPower = ReadSectorPowerState(World, 3);
			AssertTrue(Record, TEXT("isolation.power.Admin"),
				AdminPower.Contains(TEXT("Online"), ESearchCase::IgnoreCase), TEXT("Online"), AdminPower, TEXT("Power"), false);
			AssertTrue(Record, TEXT("isolation.power.NeuroGenetics"),
				NeuroPower.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase), TEXT("Emergency"), NeuroPower, TEXT("Power"), false);
			AssertTrue(Record, TEXT("isolation.power.Cryo"),
				CryoPower.Contains(TEXT("Blackout"), ESearchCase::IgnoreCase), TEXT("Blackout"), CryoPower, TEXT("Power"), false);
			AssertTrue(Record, TEXT("isolation.security.lockdown"),
				!ReadSecurityLockdown(World), TEXT("false"), BoolText(ReadSecurityLockdown(World)), TEXT("Security"), false);

			int32 AdminSectorCount = 0;
			int32 AdminEmergencyCount = 0;
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor || !Actor->GetClass() || !Actor->GetClass()->GetName().Contains(TEXT("FacilityLight")))
				{
					continue;
				}
				if (IsS20ZoneLight(Actor))
				{
					continue;
				}
				if (!OrganoidPlaytestActions::ActorPackage(Actor).Equals(AdminPackage, ESearchCase::IgnoreCase))
				{
					continue;
				}
				const FOrganoidPlaytestPropValue Role = OrganoidPlaytestActions::ReadProperty(Actor, TEXT("LightRole"));
				const bool bEmergency = Role.EnumInternal.Equals(TEXT("Emergency"), ESearchCase::IgnoreCase)
					|| Role.Text.Contains(TEXT("Emergency"), ESearchCase::IgnoreCase);
				TArray<ULightComponent*> Lights;
				Actor->GetComponents<ULightComponent>(Lights);
				for (ULightComponent* Light : Lights)
				{
					if (!Light)
					{
						continue;
					}
					if (bEmergency)
					{
						++AdminEmergencyCount;
						AssertTrue(Record, TEXT("isolation.facilitylight_admin_emergency"),
							IntensityEquals(Light->Intensity, 0.0f), TEXT("0"), IntensityText(Light->Intensity),
							OrganoidPlaytestActions::ActorLabel(Actor), false);
					}
					else
					{
						++AdminSectorCount;
						AssertTrue(Record, TEXT("isolation.facilitylight_admin_sector"),
							IntensityEquals(Light->Intensity, AdminSectorIntensity),
							IntensityText(AdminSectorIntensity), IntensityText(Light->Intensity),
							OrganoidPlaytestActions::ActorLabel(Actor), false);
					}
				}
			}
			AssertTrue(Record, TEXT("isolation.facilitylight_admin_present"),
				AdminSectorCount > 0 && AdminEmergencyCount > 0,
				TEXT("Admin sector + emergency FacilityLights present"),
				FString::Printf(TEXT("sector=%d emergency=%d"), AdminSectorCount, AdminEmergencyCount),
				AdminPackage, false);

			if (AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel))
			{
				const FOrganoidPlaytestPropValue Online = OrganoidPlaytestActions::ReadProperty(Hologram, TEXT("bAdminOnline"));
				AssertTrue(Record, TEXT("isolation.hologram.bAdminOnline"),
					Online.bHasBool && Online.bBool, TEXT("true"), Online.Text, HologramLabel, false);
				if (UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Hologram, TEXT("Light_Admin"))))
				{
					AssertTrue(Record, TEXT("isolation.hologram.Light_Admin.intensity"),
						IntensityEquals(Light->Intensity, HologramAdminIntensity),
						IntensityText(HologramAdminIntensity), IntensityText(Light->Intensity), HologramLabel, false);
					AssertTrue(Record, TEXT("isolation.hologram.Light_Admin.color"),
						ColorEquals(Light->GetLightColor(), HologramAdminColor),
						ColorText(HologramAdminColor), ColorText(Light->GetLightColor()), HologramLabel, false);
				}
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
					AssertTrue(Record, FString::Printf(TEXT("isolation.stream_%s"), StreamPackages[Index]),
						bFound && !bShouldLoad && !bLoaded,
						TEXT("unrelated partition not requested"),
						bLoaded ? TEXT("loaded") : TEXT("not loaded"), StreamPackages[Index], false);
				}
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
				TEXT("0"), FString::FromInt(NewDirty.Num()),
				NewDirty.Num() > 0 ? FString::Join(NewDirty, TEXT(",")) : TEXT(""), true);
			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString LightControllerHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminLightController.uasset")));
			const FString DoorHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"), AdminHashAfter.Equals(AdminHashBefore),
				AdminHashBefore, AdminHashAfter, TEXT("SL_Epitope_Admin"), true);
			AssertTrue(Record, TEXT("durable.light_controller_hash_unchanged"), LightControllerHashAfter.Equals(LightControllerHashBefore),
				LightControllerHashBefore, LightControllerHashAfter, TEXT("BP_AdminLightController"), true);
			AssertTrue(Record, TEXT("durable.access_door_hash_unchanged"), DoorHashAfter.Equals(DoorHashBefore),
				DoorHashBefore, DoorHashAfter, TEXT("BP_AdminAccessDoor"), true);
			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable Section 21C state changed. The playtest bot will not repair it."),
					TEXT("Use OrganoidAIBridge if a mutation is required. Do not let the bot write."));
			}
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
		EStage AfterOverlapStage = EStage::AfterVestibuleNormal;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		bool bClearTeleported = false;
		FString PendingRoomId;
		FString AdminHashBefore;
		FString LightControllerHashBefore;
		FString DoorHashBefore;
		TArray<FString> DirtyBefore;
		FLinearColor HologramAdminColor;
		float HologramAdminIntensity = -1.0f;
		TWeakObjectPtr<AActor> LightController;
		TWeakObjectPtr<UProjectOrganoidAdminLightControllerFacilityStateListener> Listener;
		TWeakObjectPtr<UProjectOrganoidAdminFacilityStateSubsystem> Subsystem;
	};

	struct FS21CAutoRegister
	{
		FS21CAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS21AdminLightingStateListenerFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS21CAutoRegister GRegisterS21C;
}
