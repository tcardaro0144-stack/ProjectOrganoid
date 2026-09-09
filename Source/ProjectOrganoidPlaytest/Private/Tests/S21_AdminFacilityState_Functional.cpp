#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"
#include "S21AdminFacilityStateProbe.h"

#include "Editor.h"
#include "Engine/LevelStreaming.h"
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
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S21_AdminFacilityState_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 21 Admin Facility State Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR DoorBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor");
	constexpr TCHAR LightControllerLabel[] = TEXT("Admin_LightController");
	constexpr TCHAR SectorControllerLabel[] = TEXT("Admin_SectorController");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");

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

	struct FStreamLook
	{
		bool bFound = false;
		bool bShouldBeLoaded = false;
		bool bShouldBeVisible = false;
		bool bIsLoaded = false;
	};

	FStreamLook ReadStream(UWorld* World, const TCHAR* PackageName)
	{
		FStreamLook Look;
		if (!World)
		{
			return Look;
		}
		for (ULevelStreaming* Level : World->GetStreamingLevels())
		{
			if (!Level)
			{
				continue;
			}
			const FString Name = OrganoidPlaytestActions::NormalizePackage(Level->GetWorldAssetPackageFName().ToString());
			if (!Name.Equals(PackageName, ESearchCase::IgnoreCase))
			{
				continue;
			}
			Look.bFound = true;
			Look.bShouldBeLoaded = Level->ShouldBeLoaded();
			Look.bShouldBeVisible = Level->ShouldBeVisible();
			Look.bIsLoaded = Level->IsLevelLoaded();
			return Look;
		}
		return Look;
	}

	FString StreamFlagText(const FStreamLook& Look)
	{
		return FString::Printf(
			TEXT("found=%s loaded=%s shouldLoad=%s shouldVis=%s"),
			*BoolText(Look.bFound),
			*BoolText(Look.bIsLoaded),
			*BoolText(Look.bShouldBeLoaded),
			*BoolText(Look.bShouldBeVisible));
	}

	struct FIsolationLook
	{
		FStreamLook Streams[UE_ARRAY_COUNT(StreamPackages)];
		bool bTerminalActivated[UE_ARRAY_COUNT(TerminalLabels)] = {};
		bool bTerminalPowered[UE_ARRAY_COUNT(TerminalLabels)] = {};
		bool bDoorLocked = false;
		bool bDoorAutomatic = false;
		FName CurrentRoom = NAME_None;
		FName CachedFacilityState = NAME_None;
		FName CurrentLightingZone = NAME_None;
		bool bAdminInitialized = false;
		bool bSecurityScanned = false;
		bool bDirectorOfficeVisited = false;
		bool bRecordsAccessed = false;
		bool bOperationsActivated = false;
		bool bHologramAdminOnline = false;
		bool bSecurityLockdown = false;
		FString AdminPower;
		FString NeuroPower;
		FString CryoPower;
		FString ComputePower;
		FString ReactorPower;
		FString FacilityWidePower;
	};

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
			return FString::Printf(TEXT("%lld"), Value);
		}
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
		{
			const uint8 Value = ByteProp->GetPropertyValue(ValuePtr);
			if (UEnum* Enum = ByteProp->GetIntPropertyEnum())
			{
				return Enum->GetNameStringByValue(Value);
			}
			return FString::FromInt(Value);
		}
		return FString();
	}

	FString CallPowerEnumFunction(UObject* Subsystem, const TCHAR* FunctionName, const TCHAR* SectorParam, uint8 SectorValue)
	{
		if (!Subsystem || !FunctionName)
		{
			return FString();
		}
		UFunction* Function = Subsystem->FindFunction(FName(FunctionName));
		if (!Function)
		{
			return FString();
		}
		TArray<uint8> Parms;
		Parms.AddZeroed(Function->ParmsSize);
		if (SectorParam)
		{
			if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, SectorParam))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
					static_cast<int64>(SectorValue));
			}
			else if (FByteProperty* ByteProp = FindFProperty<FByteProperty>(Function, SectorParam))
			{
				ByteProp->SetPropertyValue(ByteProp->ContainerPtrToValuePtr<void>(Parms.GetData()), SectorValue);
			}
		}
		Subsystem->ProcessEvent(Function, Parms.GetData());
		if (FProperty* ReturnProp = Function->GetReturnProperty())
		{
			return EnumPropertyToText(ReturnProp, ReturnProp->ContainerPtrToValuePtr<void>(Parms.GetData()));
		}
		return FString();
	}

	FString ReadSectorPowerState(UWorld* World, uint8 SectorValue)
	{
		return CallPowerEnumFunction(
			FindGameSubsystem(World, TEXT("/Script/ProjectOrganoid.ProjectOrganoidPowerSubsystem")),
			TEXT("GetSectorPowerState"),
			TEXT("Sector"),
			SectorValue);
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

	FIsolationLook CaptureIsolation(UWorld* World)
	{
		FIsolationLook Look;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
		{
			Look.Streams[Index] = ReadStream(World, StreamPackages[Index]);
		}
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
		{
			if (AActor* Terminal = OrganoidPlaytestActions::FindUniqueByLabel(World, TerminalLabels[Index]))
			{
				const FOrganoidPlaytestPropValue Activated = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bHasActivated"));
				const FOrganoidPlaytestPropValue Powered = OrganoidPlaytestActions::ReadProperty(Terminal, TEXT("bIsPowered"));
				Look.bTerminalActivated[Index] = Activated.bHasBool && Activated.bBool;
				Look.bTerminalPowered[Index] = Powered.bHasBool && Powered.bBool;
			}
		}
		if (AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World))
		{
			const FOrganoidPlaytestPropValue Locked = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bLocked"));
			const FOrganoidPlaytestPropValue Automatic = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bAutomatic"));
			Look.bDoorLocked = Locked.bHasBool && Locked.bBool;
			Look.bDoorAutomatic = Automatic.bHasBool && Automatic.bBool;
		}
		if (AActor* Sector = OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel))
		{
			Look.CurrentRoom = FName(*OrganoidPlaytestActions::ReadProperty(Sector, TEXT("CurrentRoom")).Text);
			Look.CachedFacilityState = FName(*OrganoidPlaytestActions::ReadProperty(Sector, TEXT("FacilityState")).Text);
			Look.bAdminInitialized = OrganoidPlaytestActions::ReadProperty(Sector, TEXT("bAdminInitialized")).bBool;
			Look.bSecurityScanned = OrganoidPlaytestActions::ReadProperty(Sector, TEXT("bSecurityScanned")).bBool;
			Look.bDirectorOfficeVisited = OrganoidPlaytestActions::ReadProperty(Sector, TEXT("bDirectorOfficeVisited")).bBool;
			Look.bRecordsAccessed = OrganoidPlaytestActions::ReadProperty(Sector, TEXT("bRecordsAccessed")).bBool;
			Look.bOperationsActivated = OrganoidPlaytestActions::ReadProperty(Sector, TEXT("bOperationsActivated")).bBool;
		}
		if (AActor* Lights = OrganoidPlaytestActions::FindUniqueByLabel(World, LightControllerLabel))
		{
			Look.CurrentLightingZone = FName(*OrganoidPlaytestActions::ReadProperty(Lights, TEXT("CurrentLightingZone")).Text);
		}
		if (AActor* Hologram = OrganoidPlaytestActions::FindUniqueByLabel(World, HologramLabel))
		{
			const FOrganoidPlaytestPropValue AdminOnline = OrganoidPlaytestActions::ReadProperty(Hologram, TEXT("bAdminOnline"));
			Look.bHologramAdminOnline = AdminOnline.bHasBool && AdminOnline.bBool;
		}
		Look.FacilityWidePower = ReadSectorPowerState(World, 0);
		Look.AdminPower = ReadSectorPowerState(World, 1);
		Look.NeuroPower = ReadSectorPowerState(World, 2);
		Look.CryoPower = ReadSectorPowerState(World, 3);
		Look.ComputePower = ReadSectorPowerState(World, 4);
		Look.ReactorPower = ReadSectorPowerState(World, 5);
		Look.bSecurityLockdown = ReadSecurityLockdown(World);
		return Look;
	}

	class FS21AdminFacilityStateFunctional : public IOrganoidPlaytestCase
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
			Probe.Reset();
			Subsystem.Reset();
			DirtyBefore.Reset();
			AdminHashBefore.Reset();
			DoorHashBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			UnbindProbe();
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
			case EStage::RunSequence:
				TickRunSequence(Owner, *Record);
				break;
			case EStage::LateQuery:
				TickLateQuery(Owner, *Record, DeltaTime);
				break;
			case EStage::EndPie:
				UnbindProbe();
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
			RunSequence,
			LateQuery,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		FString AdminHashBefore;
		FString DoorHashBefore;
		TArray<FString> DirtyBefore;
		FIsolationLook Baseline;
		TWeakObjectPtr<UProjectOrganoidAdminFacilityStateSubsystem> Subsystem;
		TStrongObjectPtr<UOrganoidAdminFacilityStateProbe> Probe;

		void UnbindProbe()
		{
			if (UProjectOrganoidAdminFacilityStateSubsystem* State = Subsystem.Get())
			{
				if (UOrganoidAdminFacilityStateProbe* LiveProbe = Probe.Get())
				{
					State->OnAdminFacilityStateChanged.RemoveDynamic(
						LiveProbe,
						&UOrganoidAdminFacilityStateProbe::HandleAdminFacilityStateChanged);
				}
			}
			Probe.Reset();
		}

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
				Record.RecommendedNextAction = TEXT("Stop the existing PIE session, then rerun.");
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(DoorBpPackage))
			{
				Record.RecommendedNextAction = TEXT("Leave unsaved map/Blueprint work as-is. The bot will not save or discard.");
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, or BP_AdminAccessDoor is dirty. Refusing to start."));
				return;
			}

			AdminHashBefore = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			DoorHashBefore = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("admin_hash_before"), AdminHashBefore);
			Record.AddActor(TEXT("access_door_hash_before"), DoorHashBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
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
			const FStreamLook AdminStream = ReadStream(World, AdminPackage);
			if (World && State && Character && AdminStream.bIsLoaded && (bWalking || WaitSeconds > 8.0f))
			{
				Subsystem = State;
				Stage = EStage::RunSequence;
				Owner.SetStage(TEXT("RunSequence"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE Admin facility state (world=%s subsystem=%s pawn=%s admin_loaded=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						State ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						AdminStream.bIsLoaded ? TEXT("true") : TEXT("false")));
			}
		}

		bool AssertIsolation(FOrganoidPlaytestRecord& Record, UWorld* World, const FString& Prefix, bool bExpectDoorLocked = false)
		{
			const FIsolationLook Live = CaptureIsolation(World);
			bool bAll = true;
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Admin"),
				Live.AdminPower.Equals(Baseline.AdminPower, ESearchCase::IgnoreCase),
				Baseline.AdminPower, Live.AdminPower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.NeuroGenetics"),
				Live.NeuroPower.Equals(Baseline.NeuroPower, ESearchCase::IgnoreCase),
				Baseline.NeuroPower, Live.NeuroPower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Cryo"),
				Live.CryoPower.Equals(Baseline.CryoPower, ESearchCase::IgnoreCase),
				Baseline.CryoPower, Live.CryoPower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Compute"),
				Live.ComputePower.Equals(Baseline.ComputePower, ESearchCase::IgnoreCase),
				Baseline.ComputePower, Live.ComputePower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.Reactor"),
				Live.ReactorPower.Equals(Baseline.ReactorPower, ESearchCase::IgnoreCase),
				Baseline.ReactorPower, Live.ReactorPower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".power.FacilityWide"),
				Live.FacilityWidePower.Equals(Baseline.FacilityWidePower, ESearchCase::IgnoreCase),
				Baseline.FacilityWidePower, Live.FacilityWidePower, TEXT("Power"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".security.lockdown"),
				Live.bSecurityLockdown == Baseline.bSecurityLockdown,
				BoolText(Baseline.bSecurityLockdown), BoolText(Live.bSecurityLockdown), TEXT("Security"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.CurrentRoom"),
				Live.CurrentRoom == Baseline.CurrentRoom,
				Baseline.CurrentRoom.ToString(), Live.CurrentRoom.ToString(), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.FacilityState_cache_unchanged"),
				Live.CachedFacilityState == Baseline.CachedFacilityState,
				Baseline.CachedFacilityState.ToString(), Live.CachedFacilityState.ToString(), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bSecurityScanned"),
				Live.bSecurityScanned == Baseline.bSecurityScanned,
				BoolText(Baseline.bSecurityScanned), BoolText(Live.bSecurityScanned), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bDirectorOfficeVisited"),
				Live.bDirectorOfficeVisited == Baseline.bDirectorOfficeVisited,
				BoolText(Baseline.bDirectorOfficeVisited), BoolText(Live.bDirectorOfficeVisited), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bRecordsAccessed"),
				Live.bRecordsAccessed == Baseline.bRecordsAccessed,
				BoolText(Baseline.bRecordsAccessed), BoolText(Live.bRecordsAccessed), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".sector.bOperationsActivated"),
				Live.bOperationsActivated == Baseline.bOperationsActivated,
				BoolText(Baseline.bOperationsActivated), BoolText(Live.bOperationsActivated), SectorControllerLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".s20.CurrentLightingZone"),
				Live.CurrentLightingZone == Baseline.CurrentLightingZone,
				Baseline.CurrentLightingZone.ToString(), Live.CurrentLightingZone.ToString(), LightControllerLabel, false);
			const bool bExpectedDoorLocked = bExpectDoorLocked ? true : Baseline.bDoorLocked;
			bAll &= AssertTrue(Record, Prefix + TEXT(".door.bLocked"),
				Live.bDoorLocked == bExpectedDoorLocked,
				BoolText(bExpectedDoorLocked), BoolText(Live.bDoorLocked), DoorLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".door.bAutomatic"),
				Live.bDoorAutomatic == Baseline.bDoorAutomatic,
				BoolText(Baseline.bDoorAutomatic), BoolText(Live.bDoorAutomatic), DoorLabel, false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".hologram.bAdminOnline"),
				Live.bHologramAdminOnline == Baseline.bHologramAdminOnline,
				BoolText(Baseline.bHologramAdminOnline), BoolText(Live.bHologramAdminOnline), HologramLabel, false);
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.terminal_%s"), *Prefix, TerminalLabels[Index]),
					Live.bTerminalActivated[Index] == Baseline.bTerminalActivated[Index]
						&& Live.bTerminalPowered[Index] == Baseline.bTerminalPowered[Index],
					TEXT("unchanged"),
					FString::Printf(TEXT("activated=%s powered=%s"),
						*BoolText(Live.bTerminalActivated[Index]),
						*BoolText(Live.bTerminalPowered[Index])),
					TerminalLabels[Index],
					false);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
			{
				const FStreamLook& Expected = Baseline.Streams[Index];
				const FStreamLook& Actual = Live.Streams[Index];
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.stream_%s"), *Prefix, StreamPackages[Index]),
					Expected.bFound == Actual.bFound
						&& Expected.bShouldBeLoaded == Actual.bShouldBeLoaded
						&& Expected.bShouldBeVisible == Actual.bShouldBeVisible
						&& Expected.bIsLoaded == Actual.bIsLoaded,
					StreamFlagText(Expected),
					StreamFlagText(Actual),
					StreamPackages[Index],
					false);
			}
			return bAll;
		}

		bool ExpectTransition(
			FOrganoidPlaytestRecord& Record,
			UProjectOrganoidAdminFacilityStateSubsystem* State,
			UOrganoidAdminFacilityStateProbe* LiveProbe,
			EProjectOrganoidAdminFacilityState NewState,
			EProjectOrganoidAdminFacilityState ExpectedPrevious,
			const FString& Prefix)
		{
			const int32 CountBefore = LiveProbe->BroadcastCount;
			State->SetAdminFacilityState(NewState);
			bool bAll = true;
			bAll &= AssertTrue(Record, Prefix + TEXT(".get"),
				State->GetAdminFacilityState() == NewState,
				StateText(NewState), StateText(State->GetAdminFacilityState()), TEXT("AdminFacilityState"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".notify_count"),
				LiveProbe->BroadcastCount == CountBefore + 1,
				FString::FromInt(CountBefore + 1), FString::FromInt(LiveProbe->BroadcastCount), TEXT("AdminFacilityState"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".notify_new"),
				LiveProbe->LastNewState == NewState,
				StateText(NewState), StateText(LiveProbe->LastNewState), TEXT("AdminFacilityState"), false);
			bAll &= AssertTrue(Record, Prefix + TEXT(".notify_previous"),
				LiveProbe->LastPreviousState == ExpectedPrevious,
				StateText(ExpectedPrevious), StateText(LiveProbe->LastPreviousState), TEXT("AdminFacilityState"), false);
			return bAll;
		}

		void TickRunSequence(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			UProjectOrganoidAdminFacilityStateSubsystem* State = Subsystem.Get();
			if (!World || !State)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Admin facility state subsystem."));
				return;
			}

			UOrganoidAdminFacilityStateProbe* LiveProbe = NewObject<UOrganoidAdminFacilityStateProbe>(World);
			Probe.Reset(LiveProbe);
			State->OnAdminFacilityStateChanged.AddDynamic(
				LiveProbe,
				&UOrganoidAdminFacilityStateProbe::HandleAdminFacilityStateChanged);

			Record.AddActor(TEXT("initial_get"), StateText(State->GetAdminFacilityState()));
			if (!AssertTrue(Record, TEXT("initial.Normal"),
				State->GetAdminFacilityState() == EProjectOrganoidAdminFacilityState::Normal,
				TEXT("Normal"), StateText(State->GetAdminFacilityState()), TEXT("AdminFacilityState"), false))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (!AssertTrue(Record, TEXT("initial.no_spurious_notify"),
				LiveProbe->BroadcastCount == 0,
				TEXT("0"), FString::FromInt(LiveProbe->BroadcastCount), TEXT("AdminFacilityState"), false))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			Baseline = CaptureIsolation(World);
			Record.AddActor(TEXT("baseline.AdminPower"), Baseline.AdminPower);
			Record.AddActor(TEXT("baseline.FacilityState_cache"), Baseline.CachedFacilityState.ToString());
			Record.AddActor(TEXT("baseline.CurrentRoom"), Baseline.CurrentRoom.ToString());
			Record.AddActor(TEXT("baseline.CurrentLightingZone"), Baseline.CurrentLightingZone.ToString());
			Record.AddActor(TEXT("baseline.bLocked"), BoolText(Baseline.bDoorLocked));
			Record.AddActor(TEXT("baseline.security_lockdown"), BoolText(Baseline.bSecurityLockdown));

			if (!AssertTrue(Record, TEXT("baseline.power.Admin_Online"),
				Baseline.AdminPower.Contains(TEXT("Online"), ESearchCase::IgnoreCase),
				TEXT("Online"), Baseline.AdminPower, TEXT("Power"), false))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (!AssertTrue(Record, TEXT("baseline.security.lockdown_inactive"),
				!Baseline.bSecurityLockdown,
				TEXT("false"), BoolText(Baseline.bSecurityLockdown), TEXT("Security"), false))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			if (!ExpectTransition(Record, State, LiveProbe,
				EProjectOrganoidAdminFacilityState::Alert,
				EProjectOrganoidAdminFacilityState::Normal,
				TEXT("Normal_to_Alert"))
				|| !AssertIsolation(Record, World, TEXT("after_Alert")))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (!ExpectTransition(Record, State, LiveProbe,
				EProjectOrganoidAdminFacilityState::Lockdown,
				EProjectOrganoidAdminFacilityState::Alert,
				TEXT("Alert_to_Lockdown"))
				|| !AssertIsolation(Record, World, TEXT("after_Lockdown"), true))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (!ExpectTransition(Record, State, LiveProbe,
				EProjectOrganoidAdminFacilityState::Normal,
				EProjectOrganoidAdminFacilityState::Lockdown,
				TEXT("Lockdown_to_Normal"))
				|| !AssertIsolation(Record, World, TEXT("after_return_Normal")))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (!ExpectTransition(Record, State, LiveProbe,
				EProjectOrganoidAdminFacilityState::Alert,
				EProjectOrganoidAdminFacilityState::Normal,
				TEXT("Normal_to_Alert_for_duplicate"))
				|| !AssertIsolation(Record, World, TEXT("after_Alert_for_duplicate")))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			const int32 CountBeforeDup = LiveProbe->BroadcastCount;
			State->SetAdminFacilityState(EProjectOrganoidAdminFacilityState::Alert);
			if (!AssertTrue(Record, TEXT("duplicate.Alert_to_Alert.get"),
				State->GetAdminFacilityState() == EProjectOrganoidAdminFacilityState::Alert,
				TEXT("Alert"), StateText(State->GetAdminFacilityState()), TEXT("AdminFacilityState"), false)
				|| !AssertTrue(Record, TEXT("duplicate.Alert_to_Alert.no_notify"),
					LiveProbe->BroadcastCount == CountBeforeDup,
					FString::FromInt(CountBeforeDup), FString::FromInt(LiveProbe->BroadcastCount), TEXT("AdminFacilityState"), false)
				|| !AssertIsolation(Record, World, TEXT("after_duplicate_Alert")))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			WaitSeconds = 0.0f;
			Stage = EStage::LateQuery;
			Owner.SetStage(TEXT("LateQuery"));
		}

		void TickLateQuery(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 0.35f)
			{
				return;
			}

			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			UProjectOrganoidAdminFacilityStateSubsystem* State = World
				? World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>()
				: nullptr;
			if (!World || !State)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or subsystem before late query."));
				return;
			}

			const EProjectOrganoidAdminFacilityState LateGet = State->GetAdminFacilityState();
			Record.AddActor(TEXT("late_query.get"), StateText(LateGet));
			if (!AssertTrue(Record, TEXT("late_query.independent_Get_Alert"),
				LateGet == EProjectOrganoidAdminFacilityState::Alert,
				TEXT("Alert"), StateText(LateGet), TEXT("AdminFacilityState"), false)
				|| !AssertIsolation(Record, World, TEXT("late_query")))
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			UOrganoidAdminFacilityStateProbe* LiveProbe = Probe.Get();
			if (!LiveProbe || !ExpectTransition(Record, State, LiveProbe,
				EProjectOrganoidAdminFacilityState::Normal,
				EProjectOrganoidAdminFacilityState::Alert,
				TEXT("restore_Normal"))
				|| !AssertIsolation(Record, World, TEXT("after_restore_Normal")))
			{
				FailAndStop(Owner, Record, Record.FailureReason.IsEmpty()
					? TEXT("Failed to restore Normal before EndPie.")
					: Record.FailureReason);
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
			AssertTrue(Record, TEXT("durable.dirty_count_zero"), DirtyAfter.Num() == 0,
				TEXT("0"), FString::FromInt(DirtyAfter.Num()), TEXT(""), true);

			const FString AdminHashAfter = HashFileSha1(ContentFile(TEXT("Maps/Epitope/SL_Epitope_Admin.umap")));
			const FString DoorHashAfter = HashFileSha1(ContentFile(TEXT("ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminAccessDoor.uasset")));
			Record.AddActor(TEXT("admin_hash_after"), AdminHashAfter);
			Record.AddActor(TEXT("access_door_hash_after"), DoorHashAfter);
			AssertTrue(Record, TEXT("durable.admin_hash_unchanged"), AdminHashAfter.Equals(AdminHashBefore),
				AdminHashBefore, AdminHashAfter, TEXT("SL_Epitope_Admin"), true);
			AssertTrue(Record, TEXT("durable.access_door_hash_unchanged"), DoorHashAfter.Equals(DoorHashBefore),
				DoorHashBefore, DoorHashAfter, TEXT("BP_AdminAccessDoor"), true);
			AssertTrue(Record, TEXT("durable.playtest_mutates_assets"), true,
				TEXT("false"), TEXT("false"), TEXT(""), true);

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
	};

	struct FS21AutoRegister
	{
		FS21AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS21AdminFacilityStateFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS21AutoRegister GRegisterS21;
}
