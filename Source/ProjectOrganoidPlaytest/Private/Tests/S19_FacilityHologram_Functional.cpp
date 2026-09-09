#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestLogSink.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Editor.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S19_FacilityHologram_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 19 Facility Hologram Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HologramBpPackage[] = TEXT("/Game/ProjectOrganoid/Environment/Admin/Blueprints/BP_AdminFacilityHologram");
	constexpr TCHAR HologramLabel[] = TEXT("Admin_FacilityHologram");
	constexpr TCHAR DoorLabel[] = TEXT("BP_AdminAccessDoor");
	constexpr TCHAR SectorControllerLabel[] = TEXT("Admin_SectorController");
	constexpr float OnlineIntensity = 8000.0f;
	constexpr float UnknownIntensity = 350.0f;
	constexpr float IntensityTolerance = 1.0f;

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

	UWorld* GetEditorWorld()
	{
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
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

	bool SetBool(AActor* Actor, const TCHAR* Name, bool bValue)
	{
		if (!Actor)
		{
			return false;
		}
		FBoolProperty* Property = FindFProperty<FBoolProperty>(Actor->GetClass(), FName(Name));
		if (!Property)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(Actor, bValue);
		return true;
	}

	bool CallApplyHologramState(AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}
		UFunction* Function = Actor->FindFunction(FName(TEXT("ApplyHologramState")));
		if (!Function)
		{
			return false;
		}
		Actor->ProcessEvent(Function, nullptr);
		return true;
	}

	struct FSectorSpec
	{
		const TCHAR* BoolName;
		const TCHAR* OnlineMesh;
		const TCHAR* UnknownMesh;
		const TCHAR* Light;
		bool bDefault;
	};

	const FSectorSpec Sectors[] = {
		{TEXT("bAdminOnline"), TEXT("Indicator_Admin_Online"), TEXT("Indicator_Admin_Unknown"), TEXT("Light_Admin"), true},
		{TEXT("bNeuroGeneticsOnline"), TEXT("Indicator_NeuroGenetics_Online"), TEXT("Indicator_NeuroGenetics_Unknown"), TEXT("Light_NeuroGenetics"), false},
		{TEXT("bCryoOnline"), TEXT("Indicator_Cryo_Online"), TEXT("Indicator_Cryo_Unknown"), TEXT("Light_Cryo"), false},
		{TEXT("bComputeOnline"), TEXT("Indicator_Compute_Online"), TEXT("Indicator_Compute_Unknown"), TEXT("Light_Compute"), false},
		{TEXT("bReactorOnline"), TEXT("Indicator_Reactor_Online"), TEXT("Indicator_Reactor_Unknown"), TEXT("Light_Reactor"), false},
	};

	struct FSectorLook
	{
		bool bOnlineVisible = false;
		bool bUnknownVisible = false;
		float Intensity = 0.0f;
		bool bValid = false;
	};

	struct FStreamLook
	{
		bool bShouldBeLoaded = false;
		bool bShouldBeVisible = false;
		bool bIsLoaded = false;
		bool bFound = false;
	};

	struct FIsolationLook
	{
		FStreamLook Streams[UE_ARRAY_COUNT(StreamPackages)];
		bool bTerminalActivated[UE_ARRAY_COUNT(TerminalLabels)] = {};
		bool bTerminalPowered[UE_ARRAY_COUNT(TerminalLabels)] = {};
		FVector DoorLocation = FVector::ZeroVector;
		bool bDoorOpen = false;
		bool bDoorLocked = false;
		FString FacilityState;
		FString CurrentRoom;
		bool bAdminInitialized = false;
		bool bSecurityScanned = false;
		bool bDirectorOfficeVisited = false;
		bool bRecordsAccessed = false;
		bool bOperationsActivated = false;
	};

	bool IsVisible(USceneComponent* Scene)
	{
		return Scene && Scene->GetVisibleFlag() && !Scene->bHiddenInGame;
	}

	FSectorLook ReadSectorLook(AActor* Actor, const FSectorSpec& Spec)
	{
		FSectorLook Look;
		USceneComponent* Online = Cast<USceneComponent>(FindComp(Actor, Spec.OnlineMesh));
		USceneComponent* Unknown = Cast<USceneComponent>(FindComp(Actor, Spec.UnknownMesh));
		UPointLightComponent* Light = Cast<UPointLightComponent>(FindComp(Actor, Spec.Light));
		Look.bOnlineVisible = IsVisible(Online);
		Look.bUnknownVisible = IsVisible(Unknown);
		Look.Intensity = Light ? Light->Intensity : -1.0f;
		Look.bValid = Online && Unknown && Light;
		return Look;
	}

	FString LookText(const FSectorLook& Look)
	{
		return FString::Printf(
			TEXT("online=%s unknown=%s intensity=%.1f valid=%s"),
			Look.bOnlineVisible ? TEXT("true") : TEXT("false"),
			Look.bUnknownVisible ? TEXT("true") : TEXT("false"),
			Look.Intensity,
			Look.bValid ? TEXT("true") : TEXT("false"));
	}

	bool LooksEqual(const FSectorLook& A, const FSectorLook& B)
	{
		return A.bValid && B.bValid
			&& A.bOnlineVisible == B.bOnlineVisible
			&& A.bUnknownVisible == B.bUnknownVisible
			&& FMath::IsNearlyEqual(A.Intensity, B.Intensity, IntensityTolerance);
	}

	bool LooksOnline(const FSectorLook& Look)
	{
		return Look.bValid && Look.bOnlineVisible && !Look.bUnknownVisible
			&& FMath::IsNearlyEqual(Look.Intensity, OnlineIntensity, IntensityTolerance);
	}

	bool LooksUnknown(const FSectorLook& Look)
	{
		return Look.bValid && !Look.bOnlineVisible && Look.bUnknownVisible
			&& FMath::IsNearlyEqual(Look.Intensity, UnknownIntensity, IntensityTolerance);
	}

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
			Look.DoorLocation = Door->GetActorLocation();
			const FOrganoidPlaytestPropValue Open = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bIsOpen"));
			const FOrganoidPlaytestPropValue Locked = OrganoidPlaytestActions::ReadProperty(Door, TEXT("bLocked"));
			Look.bDoorOpen = Open.bHasBool && Open.bBool;
			Look.bDoorLocked = Locked.bHasBool && Locked.bBool;
		}
		if (AActor* Controller = OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel))
		{
			Look.FacilityState = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("FacilityState")).Text;
			Look.CurrentRoom = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("CurrentRoom")).Text;
			Look.bAdminInitialized = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bAdminInitialized")).bBool;
			Look.bSecurityScanned = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bSecurityScanned")).bBool;
			Look.bDirectorOfficeVisited = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bDirectorOfficeVisited")).bBool;
			Look.bRecordsAccessed = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bRecordsAccessed")).bBool;
			Look.bOperationsActivated = OrganoidPlaytestActions::ReadProperty(Controller, TEXT("bOperationsActivated")).bBool;
		}
		return Look;
	}

	class FS19FacilityHologramFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			SectorIndex = 0;
			bCheckingRestore = false;
			bAnyAssertFailed = false;
			bConfigFailed = false;
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
			case EStage::AssertBaseline:
				TickAssertBaseline(Owner, *Record);
				break;
			case EStage::ExerciseSectors:
				TickExerciseSectors(Owner, *Record);
				break;
			case EStage::Restore:
				TickRestore(Owner, *Record);
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
			AssertBaseline,
			ExerciseSectors,
			Restore,
			EndPie,
			WaitPieStopped,
			AssertDurable,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		int32 SectorIndex = 0;
		bool bCheckingRestore = false;
		bool bAnyAssertFailed = false;
		bool bConfigFailed = false;
		TWeakObjectPtr<AActor> Hologram;
		FIsolationLook BaselineIsolation;
		FSectorLook OtherLooks[UE_ARRAY_COUNT(Sectors)];

		void FailAndStop(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			TryRestoreDefaults();
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

		void TryRestoreDefaults()
		{
			AActor* Actor = Hologram.Get();
			if (!Actor)
			{
				return;
			}
			for (const FSectorSpec& Sector : Sectors)
			{
				SetBool(Actor, Sector.BoolName, Sector.bDefault);
			}
			CallApplyHologramState(Actor);
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(HologramBpPackage))
			{
				Record.RecommendedNextAction = TEXT("Leave unsaved map/Blueprint work as-is. Save or discard it yourself, then rerun. The bot will not save or discard.");
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope, SL_Epitope_Admin, or BP_AdminFacilityHologram is dirty. Refusing to start."));
				return;
			}
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
			TArray<AActor*> Matches = World ? OrganoidPlaytestActions::FindActorsByLabel(World, HologramLabel) : TArray<AActor*>();
			const FStreamLook AdminStream = ReadStream(World, AdminPackage);
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}
			if (World && Character && Matches.Num() == 1 && AdminStream.bIsLoaded && (bWalking || WaitSeconds > 8.0f))
			{
				Stage = EStage::AssertBaseline;
				Owner.SetStage(TEXT("AssertBaseline"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Record.AddActor(TEXT("player"), Character ? Character->GetName() : TEXT(""));
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE hologram (world=%s pawn=%s holograms=%d admin_loaded=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						Matches.Num(),
						AdminStream.bIsLoaded ? TEXT("true") : TEXT("false")));
			}
		}

		bool AssertBool(FOrganoidPlaytestRecord& Record, AActor* Actor, const TCHAR* Name, bool bExpected, const FString& Id, bool bDurable)
		{
			const FOrganoidPlaytestPropValue Value = OrganoidPlaytestActions::ReadProperty(Actor, Name);
			return AssertTrue(
				Record, Id,
				Value.bHasBool && Value.bBool == bExpected,
				BoolText(bExpected),
				Value.bFound ? Value.Text : TEXT("<missing>"),
				HologramLabel,
				bDurable);
		}

		bool AssertPresentation(
			FOrganoidPlaytestRecord& Record,
			AActor* Actor,
			const FSectorSpec& Spec,
			bool bOnline,
			const FString& Prefix)
		{
			const FSectorLook Look = ReadSectorLook(Actor, Spec);
			const bool bPass = bOnline ? LooksOnline(Look) : LooksUnknown(Look);
			return AssertTrue(
				Record,
				FString::Printf(TEXT("%s.%s"), *Prefix, Spec.BoolName),
				bPass,
				bOnline ? TEXT("online visible / unknown hidden / 8000") : TEXT("online hidden / unknown visible / 350"),
				LookText(Look),
				HologramLabel,
				false);
		}

		bool AssertIsolation(FOrganoidPlaytestRecord& Record, UWorld* World, const FString& Prefix)
		{
			const FIsolationLook Live = CaptureIsolation(World);
			bool bAll = true;
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(StreamPackages); ++Index)
			{
				const FStreamLook& Expected = BaselineIsolation.Streams[Index];
				const FStreamLook& Actual = Live.Streams[Index];
				const bool bPass = Expected.bFound && Actual.bFound
					&& Expected.bShouldBeLoaded == Actual.bShouldBeLoaded
					&& Expected.bShouldBeVisible == Actual.bShouldBeVisible
					&& Expected.bIsLoaded == Actual.bIsLoaded;
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.stream_%s"), *Prefix, StreamPackages[Index]),
					bPass,
					FString::Printf(TEXT("loaded=%s visible=%s isLoaded=%s"),
						Expected.bShouldBeLoaded ? TEXT("1") : TEXT("0"),
						Expected.bShouldBeVisible ? TEXT("1") : TEXT("0"),
						Expected.bIsLoaded ? TEXT("1") : TEXT("0")),
					Actual.bFound
						? FString::Printf(TEXT("loaded=%s visible=%s isLoaded=%s"),
							Actual.bShouldBeLoaded ? TEXT("1") : TEXT("0"),
							Actual.bShouldBeVisible ? TEXT("1") : TEXT("0"),
							Actual.bIsLoaded ? TEXT("1") : TEXT("0"))
						: TEXT("<missing>"),
					StreamPackages[Index],
					false);
			}
			for (int32 Index = 0; Index < UE_ARRAY_COUNT(TerminalLabels); ++Index)
			{
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.terminal_idle_%s"), *Prefix, TerminalLabels[Index]),
					Live.bTerminalActivated[Index] == BaselineIsolation.bTerminalActivated[Index]
						&& Live.bTerminalPowered[Index] == BaselineIsolation.bTerminalPowered[Index],
					FString::Printf(TEXT("activated=%s powered=%s"),
						BaselineIsolation.bTerminalActivated[Index] ? TEXT("true") : TEXT("false"),
						BaselineIsolation.bTerminalPowered[Index] ? TEXT("true") : TEXT("false")),
					FString::Printf(TEXT("activated=%s powered=%s"),
						Live.bTerminalActivated[Index] ? TEXT("true") : TEXT("false"),
						Live.bTerminalPowered[Index] ? TEXT("true") : TEXT("false")),
					TerminalLabels[Index],
					false);
			}
			if (AActor* Door = OrganoidPlaytestActions::FindAccessDoor(World))
			{
				const bool bDoorStill = Door->GetActorLocation().Equals(BaselineIsolation.DoorLocation, 1.0f)
					&& Live.bDoorOpen == BaselineIsolation.bDoorOpen
					&& Live.bDoorLocked == BaselineIsolation.bDoorLocked;
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.access_door_idle"), *Prefix),
					bDoorStill,
					TEXT("transform + bIsOpen + bLocked unchanged"),
					FString::Printf(TEXT("open=%s locked=%s dist=%.1f"),
						Live.bDoorOpen ? TEXT("true") : TEXT("false"),
						Live.bDoorLocked ? TEXT("true") : TEXT("false"),
						FVector::Dist(Door->GetActorLocation(), BaselineIsolation.DoorLocation)),
					DoorLabel,
					false);
			}
			if (OrganoidPlaytestActions::FindUniqueByLabel(World, SectorControllerLabel))
			{
				const bool bControllerStill = Live.FacilityState.Equals(BaselineIsolation.FacilityState)
					&& Live.CurrentRoom.Equals(BaselineIsolation.CurrentRoom)
					&& Live.bAdminInitialized == BaselineIsolation.bAdminInitialized
					&& Live.bSecurityScanned == BaselineIsolation.bSecurityScanned
					&& Live.bDirectorOfficeVisited == BaselineIsolation.bDirectorOfficeVisited
					&& Live.bRecordsAccessed == BaselineIsolation.bRecordsAccessed
					&& Live.bOperationsActivated == BaselineIsolation.bOperationsActivated;
				bAll &= AssertTrue(
					Record,
					FString::Printf(TEXT("%s.sector_controller_idle"), *Prefix),
					bControllerStill,
					FString::Printf(TEXT("state=%s room=%s"), *BaselineIsolation.FacilityState, *BaselineIsolation.CurrentRoom),
					FString::Printf(TEXT("state=%s room=%s init=%s"), *Live.FacilityState, *Live.CurrentRoom, Live.bAdminInitialized ? TEXT("true") : TEXT("false")),
					SectorControllerLabel,
					false);
			}
			return bAll;
		}

		void TickAssertBaseline(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			TArray<AActor*> Matches = World ? OrganoidPlaytestActions::FindActorsByLabel(World, HologramLabel) : TArray<AActor*>();
			AssertTrue(Record, TEXT("baseline.exactly_one"), Matches.Num() == 1, TEXT("1"), FString::FromInt(Matches.Num()), HologramLabel, true);
			if (Matches.Num() != 1)
			{
				Record.MarkNeedsApproval(
					TEXT("Admin_FacilityHologram count is not exactly one."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			AActor* Actor = Matches[0];
			Hologram = Actor;
			const FString OwnerPkg = OrganoidPlaytestActions::ActorPackage(Actor);
			Record.AddActor(TEXT("hologram"), OrganoidPlaytestActions::ActorLabel(Actor));
			Record.AddActor(TEXT("owner_package"), OwnerPkg);

			AssertTrue(Record, TEXT("baseline.class"), Actor->GetClass() && Actor->GetClass()->GetName().Equals(TEXT("BP_AdminFacilityHologram_C")), TEXT("BP_AdminFacilityHologram_C"), Actor->GetClass() ? Actor->GetClass()->GetName() : TEXT("<none>"), HologramLabel, true);
			AssertTrue(Record, TEXT("baseline.owner_package"), OwnerPkg.Equals(AdminPackage), AdminPackage, OwnerPkg, HologramLabel, true);
			AssertTrue(Record, TEXT("baseline.location"), Actor->GetActorLocation().Equals(FVector(2400.f, 0.f, 0.f), 1.0f), TEXT("(2400,0,0)"), Actor->GetActorLocation().ToCompactString(), HologramLabel, true);
			AssertTrue(Record, TEXT("baseline.rotation"), Actor->GetActorRotation().Equals(FRotator::ZeroRotator, 1.0f), TEXT("(0,0,0)"), Actor->GetActorRotation().ToCompactString(), HologramLabel, true);
			AssertTrue(Record, TEXT("baseline.scale"), Actor->GetActorScale3D().Equals(FVector::OneVector, 0.01f), TEXT("(1,1,1)"), Actor->GetActorScale3D().ToCompactString(), HologramLabel, true);

			UClass* InteractableClass = LoadClass<AActor>(nullptr, TEXT("/Script/ProjectOrganoid.ProjectOrganoidInteractable"));
			const bool bIsInteractable = InteractableClass && Actor->IsA(InteractableClass);
			AssertTrue(Record, TEXT("baseline.not_interactable_class"), !bIsInteractable, TEXT("not AProjectOrganoidInteractable"), bIsInteractable ? TEXT("is interactable") : TEXT("Actor"), HologramLabel, true);

			bool bHasInteractionCollision = false;
			TArray<UActorComponent*> Components;
			Actor->GetComponents(Components);
			for (UActorComponent* Component : Components)
			{
				if (Cast<USphereComponent>(Component) || Cast<UBoxComponent>(Component) || Cast<UCapsuleComponent>(Component))
				{
					bHasInteractionCollision = true;
					break;
				}
			}
			AssertTrue(Record, TEXT("baseline.no_interaction_collision"), !bHasInteractionCollision, TEXT("no sphere/box/capsule"), bHasInteractionCollision ? TEXT("found") : TEXT("none"), HologramLabel, true);

			for (const FSectorSpec& Sector : Sectors)
			{
				AssertBool(Record, Actor, Sector.BoolName, Sector.bDefault, FString::Printf(TEXT("baseline.%s"), Sector.BoolName), true);
				AssertPresentation(Record, Actor, Sector, Sector.bDefault, TEXT("baseline"));
			}

			BaselineIsolation = CaptureIsolation(World);
			AssertIsolation(Record, World, TEXT("baseline"));

			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable hologram identity/defaults do not match Section 19."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			SectorIndex = 0;
			bCheckingRestore = false;
			Stage = EStage::ExerciseSectors;
			Owner.SetStage(TEXT("ExerciseSectors"));
		}

		void TickExerciseSectors(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Actor = Hologram.Get();
			if (!World || !Actor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE hologram while exercising sectors."));
				return;
			}

			if (SectorIndex >= UE_ARRAY_COUNT(Sectors))
			{
				Stage = EStage::Restore;
				Owner.SetStage(TEXT("Restore"));
				return;
			}

			const FSectorSpec& Spec = Sectors[SectorIndex];
			const FString Prefix = FString::Printf(TEXT("sector_%s"), Spec.BoolName);
			Owner.SetStage(Prefix);

			if (!bCheckingRestore)
			{
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sectors); ++Index)
				{
					OtherLooks[Index] = ReadSectorLook(Actor, Sectors[Index]);
				}
				const bool bFlipTo = !Spec.bDefault;
				if (!SetBool(Actor, Spec.BoolName, bFlipTo) || !CallApplyHologramState(Actor))
				{
					FailAndStop(Owner, Record, FString::Printf(TEXT("Failed to set/apply %s."), Spec.BoolName));
					return;
				}
				AssertBool(Record, Actor, Spec.BoolName, bFlipTo, FString::Printf(TEXT("%s.bool"), *Prefix), false);
				AssertPresentation(Record, Actor, Spec, bFlipTo, Prefix);
				for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sectors); ++Index)
				{
					if (Index == SectorIndex)
					{
						continue;
					}
					const FSectorLook Live = ReadSectorLook(Actor, Sectors[Index]);
					AssertTrue(
						Record,
						FString::Printf(TEXT("%s.other_%s_unchanged"), *Prefix, Sectors[Index].BoolName),
						LooksEqual(Live, OtherLooks[Index]),
						LookText(OtherLooks[Index]),
						LookText(Live),
						HologramLabel,
						false);
				}
				AssertIsolation(Record, World, Prefix);
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				bCheckingRestore = true;
				return;
			}

			if (!SetBool(Actor, Spec.BoolName, Spec.bDefault) || !CallApplyHologramState(Actor))
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("Failed to restore %s."), Spec.BoolName));
				return;
			}
			AssertBool(Record, Actor, Spec.BoolName, Spec.bDefault, FString::Printf(TEXT("%s.restored_bool"), *Prefix), false);
			AssertPresentation(Record, Actor, Spec, Spec.bDefault, FString::Printf(TEXT("%s.restored"), *Prefix));
			AssertIsolation(Record, World, FString::Printf(TEXT("%s.restored"), *Prefix));
			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			bCheckingRestore = false;
			++SectorIndex;
		}

		void TickRestore(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Actor = Hologram.Get();
			if (!World || !Actor)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE hologram before restore."));
				return;
			}
			Owner.SetStage(TEXT("Restore"));
			TryRestoreDefaults();
			for (const FSectorSpec& Sector : Sectors)
			{
				AssertBool(Record, Actor, Sector.BoolName, Sector.bDefault, FString::Printf(TEXT("restore.%s"), Sector.BoolName), false);
				AssertPresentation(Record, Actor, Sector, Sector.bDefault, TEXT("restore"));
			}
			AssertIsolation(Record, World, TEXT("restore"));
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
				FailAndStop(Owner, Record, TEXT("Timed out waiting for PIE to stop."));
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, Record.FailureReason);
			}
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("AssertDurable"));
			const bool bDirty = PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage) || PackageIsDirty(HologramBpPackage);
			AssertTrue(Record, TEXT("durable.packages_clean"), !bDirty, TEXT("not dirty"), bDirty ? TEXT("dirty") : TEXT("clean"), TEXT(""), true);

			UWorld* EditorWorld = GetEditorWorld();
			TArray<AActor*> Matches = EditorWorld ? OrganoidPlaytestActions::FindActorsByLabel(EditorWorld, HologramLabel) : TArray<AActor*>();
			AssertTrue(Record, TEXT("durable.exactly_one"), Matches.Num() == 1, TEXT("1"), FString::FromInt(Matches.Num()), HologramLabel, true);
			if (Matches.Num() == 1)
			{
				AActor* Actor = Matches[0];
				AssertTrue(
					Record, TEXT("durable.owner_package"),
					OrganoidPlaytestActions::ActorPackage(Actor).Equals(AdminPackage),
					AdminPackage, OrganoidPlaytestActions::ActorPackage(Actor),
					HologramLabel, true);
				for (const FSectorSpec& Sector : Sectors)
				{
					AssertBool(Record, Actor, Sector.BoolName, Sector.bDefault, FString::Printf(TEXT("durable.%s"), Sector.BoolName), true);
				}
			}

			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable hologram state changed. The playtest bot will not repair it."),
					TEXT("Inspect Admin_FacilityHologram. Use OrganoidAIBridge if a mutation is required. Do not let the bot write."));
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

	struct FS19AutoRegister
	{
		FS19AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS19FacilityHologramFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS19AutoRegister GRegisterS19Hologram;
}
