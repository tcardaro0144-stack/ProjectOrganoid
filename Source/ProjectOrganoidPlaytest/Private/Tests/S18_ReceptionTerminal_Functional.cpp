#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestLogSink.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/Package.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("S18_ReceptionTerminal_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Section 18 Reception Terminal Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR TerminalLabel[] = TEXT("Admin_Terminal_Reception");
	constexpr TCHAR ExpectedTerminalId[] = TEXT("Terminal_AdminReception");
	constexpr TCHAR ExpectedTitle[] = TEXT("Reception Terminal");
	constexpr TCHAR ExpectedPrompt[] = TEXT("Use Terminal");
	constexpr float ExpectedRange = 150.0f;
	constexpr float SetupDistance = 100.0f;
	constexpr float AimConeCos = 0.5735764f; // cos(55 deg)

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	bool IsReceptionEnum(const FOrganoidPlaytestPropValue& Value)
	{
		if (Value.EnumDisplay.Equals(TEXT("Reception"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (Value.EnumInternal.Equals(TEXT("NewEnumerator0"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		if (Value.Text.Equals(TEXT("Reception"), ESearchCase::IgnoreCase))
		{
			return true;
		}
		return Value.bHasNumber && FMath::IsNearlyZero(Value.Number);
	}

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	class FS18ReceptionTerminalFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			PostInteractWait = 0.0f;
			InteractAttempt = 0;
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
			case EStage::AssertConfig:
				TickAssertConfig(Owner, *Record);
				break;
			case EStage::TeleportFace:
				TickTeleportFace(Owner, *Record);
				break;
			case EStage::WaitFocus:
				TickWaitFocus(Owner, *Record, DeltaTime);
				break;
			case EStage::Interact:
				TickInteract(Owner, *Record);
				break;
			case EStage::AssertAfterInteract:
				TickAssertAfterInteract(Owner, *Record, DeltaTime);
				break;
			case EStage::EndPie:
				Owner.SetStage(TEXT("EndPie"));
				Owner.RequestEndPieIfStarted();
				Stage = EStage::Finalize;
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
			AssertConfig,
			TeleportFace,
			WaitFocus,
			Interact,
			AssertAfterInteract,
			EndPie,
			Finalize
		};

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		float PostInteractWait = 0.0f;
		int32 InteractAttempt = 0;
		TWeakObjectPtr<AActor> Terminal;
		TWeakObjectPtr<AActor> Door;
		FVector DoorLocation = FVector::ZeroVector;
		FVector DoorTriggerExtent = FVector::ZeroVector;
		bool bConfigFailed = false;
		bool bAnyAssertFailed = false;

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
				Record.RecommendedNextAction = TEXT("Stop the existing PIE session, then rerun. The bot will not steal an in-progress Play session.");
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("PIE is already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage))
			{
				Record.RecommendedNextAction = TEXT("Leave unsaved map work as-is. Save or discard it yourself, then rerun. The bot will not save or discard.");
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty. Refusing to start so unsaved work is not discarded."));
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
			TArray<AActor*> Terminals = World ? OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel) : TArray<AActor*>();
			bool bWalking = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bWalking = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling;
				}
			}
			if (World && Character && Terminals.Num() >= 1 && (bWalking || WaitSeconds > 8.0f))
			{
				Stage = EStage::AssertConfig;
				Owner.SetStage(TEXT("AssertConfig"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Record.AddActor(TEXT("player"), Character ? Character->GetName() : TEXT(""));
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE player + Admin_Terminal_Reception (world=%s pawn=%s terminals=%d walking=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						Terminals.Num(),
						bWalking ? TEXT("true") : TEXT("false")));
			}
		}

		void TickAssertConfig(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			TArray<AActor*> Terminals = OrganoidPlaytestActions::FindActorsByLabel(World, TerminalLabel);
			Record.AddActor(TEXT("player"), Pawn ? OrganoidPlaytestActions::ActorLabel(Pawn) : TEXT(""));

			AssertTrue(
				Record,
				TEXT("exactly_one_reception_terminal"),
				Terminals.Num() == 1,
				TEXT("1"),
				FString::FromInt(Terminals.Num()),
				TerminalLabel,
				true);

			if (Terminals.Num() != 1)
			{
				Record.MarkNeedsApproval(
					TEXT("Reception terminal count is not exactly one."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			AActor* Target = Terminals[0];
			Terminal = Target;
			const FString OwnerPkg = OrganoidPlaytestActions::ActorPackage(Target);
			Record.AddActor(TEXT("target"), OrganoidPlaytestActions::ActorLabel(Target));
			Record.AddActor(TEXT("owner_package"), OwnerPkg);

			AssertTrue(Record, TEXT("owner_package"), OwnerPkg.Equals(AdminPackage), AdminPackage, OwnerPkg, TerminalLabel, true);

			const FOrganoidPlaytestPropValue TerminalId = OrganoidPlaytestActions::ReadProperty(Target, TEXT("TerminalID"));
			AssertTrue(
				Record, TEXT("TerminalID"),
				TerminalId.bFound && TerminalId.Text.Equals(ExpectedTerminalId),
				ExpectedTerminalId, TerminalId.bFound ? TerminalId.Text : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue Title = OrganoidPlaytestActions::ReadProperty(Target, TEXT("Title"));
			AssertTrue(
				Record, TEXT("Title"),
				Title.bFound && Title.Text.Equals(ExpectedTitle),
				ExpectedTitle, Title.bFound ? Title.Text : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue TerminalType = OrganoidPlaytestActions::ReadProperty(Target, TEXT("TerminalType"));
			AssertTrue(
				Record, TEXT("TerminalType"),
				TerminalType.bFound && IsReceptionEnum(TerminalType),
				TEXT("Reception / NewEnumerator0 / 0"),
				TerminalType.bFound ? FString::Printf(TEXT("%s (%s / %s)"), *TerminalType.Text, *TerminalType.EnumInternal, *TerminalType.EnumDisplay) : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue InitiallyPowered = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bInitiallyPowered"));
			AssertTrue(
				Record, TEXT("bInitiallyPowered"),
				InitiallyPowered.bHasBool && InitiallyPowered.bBool,
				TEXT("true"), InitiallyPowered.bFound ? InitiallyPowered.Text : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue IsPowered = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bIsPowered"));
			AssertTrue(
				Record, TEXT("bIsPowered_after_BeginPlay"),
				IsPowered.bHasBool && IsPowered.bBool,
				TEXT("true"), IsPowered.bFound ? IsPowered.Text : TEXT("<missing>"),
				TerminalLabel, false);

			const FOrganoidPlaytestPropValue HasActivated = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bHasActivated"));
			AssertTrue(
				Record, TEXT("bHasActivated_initial"),
				HasActivated.bHasBool && !HasActivated.bBool,
				TEXT("false"), HasActivated.bFound ? HasActivated.Text : TEXT("<missing>"),
				TerminalLabel, false);

			const FOrganoidPlaytestPropValue IsInteractable = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bIsInteractable"));
			AssertTrue(
				Record, TEXT("bIsInteractable_initial"),
				IsInteractable.bHasBool && IsInteractable.bBool,
				TEXT("true"), IsInteractable.bFound ? IsInteractable.Text : TEXT("<missing>"),
				TerminalLabel, false);

			const FOrganoidPlaytestPropValue OneShot = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bOneShot"));
			AssertTrue(
				Record, TEXT("bOneShot"),
				OneShot.bHasBool && !OneShot.bBool,
				TEXT("false"), OneShot.bFound ? OneShot.Text : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue Prompt = OrganoidPlaytestActions::ReadProperty(Target, TEXT("InteractionPrompt"));
			AssertTrue(
				Record, TEXT("InteractionPrompt"),
				Prompt.bFound && Prompt.Text.Equals(ExpectedPrompt),
				ExpectedPrompt, Prompt.bFound ? Prompt.Text : TEXT("<missing>"),
				TerminalLabel, true);

			const FOrganoidPlaytestPropValue Range = OrganoidPlaytestActions::ReadProperty(Target, TEXT("InteractionRange"));
			const bool bRangeOk = Range.bHasNumber && FMath::IsNearlyEqual(static_cast<float>(Range.Number), ExpectedRange, 0.1f);
			AssertTrue(
				Record, TEXT("InteractionRange"),
				bRangeOk,
				TEXT("150"), Range.bFound ? Range.Text : TEXT("<missing>"),
				TerminalLabel, true);

			if (AActor* FoundDoor = OrganoidPlaytestActions::FindAccessDoor(World))
			{
				Door = FoundDoor;
				DoorLocation = FoundDoor->GetActorLocation();
				DoorTriggerExtent = OrganoidPlaytestActions::ReadBoxExtent(FoundDoor, TEXT("AccessTrigger"));
				Record.AddActor(TEXT("access_door"), OrganoidPlaytestActions::ActorLabel(FoundDoor));
			}

			if (bConfigFailed)
			{
				Record.MarkNeedsApproval(
					TEXT("Durable Reception configuration does not match the expected Section 18 instance."),
					TEXT("Use OrganoidAIBridge prepare_write / dual approve / execute_write. The playtest bot will not mutate."));
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			Stage = EStage::TeleportFace;
			Owner.SetStage(TEXT("TeleportFace"));
		}

		void TickTeleportFace(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Target = Terminal.Get();
			if (!Pawn || !Target)
			{
				FailAndStop(Owner, Record, TEXT("Lost player or terminal before teleport."));
				return;
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
				Character->SetActorEnableCollision(true);
			}

			OrganoidPlaytestActions::TeleportNear(Pawn, Target->GetActorLocation(), SetupDistance, CapsuleZ);
			OrganoidPlaytestActions::FaceActor(Pawn, Target);
			WaitSeconds = 0.0f;
			Stage = EStage::WaitFocus;
			Owner.SetStage(TEXT("WaitFocus"));
		}

		void TickWaitFocus(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AActor* Target = Terminal.Get();
			if (!Pawn || !Target)
			{
				FailAndStop(Owner, Record, TEXT("Lost player or terminal while waiting for focus."));
				return;
			}

			OrganoidPlaytestActions::FaceActor(Pawn, Target);
			UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Pawn);
			AActor* Focused = OrganoidPlaytestActions::GetFocusedInteractable(Interaction);
			const float Dist = OrganoidPlaytestActions::DistanceTo(Pawn, Target);
			const float AimDot = OrganoidPlaytestActions::AimDotTo(Pawn, Target);
			const bool bRange = Dist >= 75.0f && Dist <= 150.0f;
			const bool bFacing = AimDot >= AimConeCos;
			const bool bFocused = Focused == Target;

			if (bRange && bFacing && bFocused)
			{
				AssertTrue(Record, TEXT("player_in_range"), true, TEXT("75-150"), FString::SanitizeFloat(Dist), TerminalLabel, false);
				AssertTrue(Record, TEXT("player_facing_target"), true, TEXT("aim_dot>=cos(55)"), FString::SanitizeFloat(AimDot), TerminalLabel, false);
				AssertTrue(Record, TEXT("interaction_focused"), true, TerminalLabel, OrganoidPlaytestActions::ActorLabel(Focused), TerminalLabel, false);
				InteractAttempt = 1;
				if (FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink())
				{
					Sink->MarkCursor();
				}
				Stage = EStage::Interact;
				Owner.SetStage(TEXT("Interact1"));
				return;
			}

			if (WaitSeconds > 3.0f)
			{
				AssertTrue(Record, TEXT("player_in_range"), bRange, TEXT("75-150"), FString::SanitizeFloat(Dist), TerminalLabel, false);
				AssertTrue(Record, TEXT("player_facing_target"), bFacing, TEXT("aim_dot>=cos(55)"), FString::SanitizeFloat(AimDot), TerminalLabel, false);
				AssertTrue(
					Record, TEXT("interaction_focused"), false, TerminalLabel,
					Focused ? OrganoidPlaytestActions::ActorLabel(Focused) : TEXT("<none>"),
					TerminalLabel, false);
				FailAndStop(Owner, Record, TEXT("Failed to acquire genuine interaction focus after teleport/face."));
			}
		}

		void TickInteract(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			UActorComponent* Interaction = OrganoidPlaytestActions::FindInteractionComponent(Pawn);
			const bool bOk = OrganoidPlaytestActions::TryInteract(Interaction);
			const FString Id = InteractAttempt == 1 ? TEXT("TryInteract_first") : TEXT("TryInteract_second");
			AssertTrue(Record, Id, bOk, TEXT("true"), BoolText(bOk), TerminalLabel, false);
			if (!bOk)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("%s failed — genuine interaction path returned false."), *Id));
				return;
			}
			PostInteractWait = 0.0f;
			Stage = EStage::AssertAfterInteract;
			Owner.SetStage(InteractAttempt == 1 ? TEXT("AssertAfterFirst") : TEXT("AssertAfterSecond"));
		}

		void TickAssertAfterInteract(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			PostInteractWait += DeltaTime;
			if (PostInteractWait < 0.35f)
			{
				return;
			}

			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Target = Terminal.Get();
			APlayerController* PC = OrganoidPlaytestActions::GetPlayerController(World);
			FOrganoidPlaytestLogSink* Sink = Owner.GetLogSink();
			const FString Prefix = InteractAttempt == 1 ? TEXT("after_first") : TEXT("after_second");

			const bool bScreen = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal Screen"));
			const bool bReception = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal OnReceptionActivated"));
			const bool bUnpowered = Sink && Sink->ContainsSinceCursor(TEXT("AdminTerminal Error: unpowered"));
			const int32 HackWidgets = OrganoidPlaytestActions::CountVisibleHackingWidgets(World);

			if (InteractAttempt == 1)
			{
				AssertTrue(Record, TEXT("powered_path"), bScreen && !bUnpowered, TEXT("AdminTerminal Screen, no unpowered error"),
					FString::Printf(TEXT("screen=%s unpowered=%s"), *BoolText(bScreen), *BoolText(bUnpowered)),
					TerminalLabel, false);
				AssertTrue(Record, TEXT("reception_route"), bReception, TEXT("AdminTerminal OnReceptionActivated"),
					bReception ? TEXT("AdminTerminal OnReceptionActivated") : TEXT("<missing>"),
					TerminalLabel, false);
			}
			else
			{
				AssertTrue(Record, TEXT("repeat_powered_or_reception"), bScreen || bReception, TEXT("powered/reception print on second use"),
					FString::Printf(TEXT("screen=%s reception=%s"), *BoolText(bScreen), *BoolText(bReception)),
					TerminalLabel, false);
			}

			AssertTrue(Record, TEXT("no_unpowered_error"), !bUnpowered, TEXT("false"), BoolText(bUnpowered), TerminalLabel, false);
			AssertTrue(Record, FString::Printf(TEXT("%s.no_hacking_ui"), *Prefix), HackWidgets == 0, TEXT("0"), FString::FromInt(HackWidgets), TerminalLabel, false);

			const bool bCursor = PC && PC->bShowMouseCursor;
			AssertTrue(Record, FString::Printf(TEXT("%s.no_input_mode_steal"), *Prefix), !bCursor, TEXT("mouse_cursor=false"), BoolText(bCursor), TerminalLabel, false);

			if (AActor* FoundDoor = Door.Get())
			{
				const FVector Loc = FoundDoor->GetActorLocation();
				const FVector Extent = OrganoidPlaytestActions::ReadBoxExtent(FoundDoor, TEXT("AccessTrigger"));
				const bool bDoorStill = Loc.Equals(DoorLocation, 1.0f);
				const bool bTriggerStill = Extent.Equals(DoorTriggerExtent, 0.5f);
				AssertTrue(
					Record, FString::Printf(TEXT("%s.no_door_unlock"), *Prefix),
					bDoorStill && bTriggerStill,
					TEXT("door transform + AccessTrigger unchanged"),
					FString::Printf(TEXT("loc_delta=%.1f extent_delta=%.1f"), FVector::Dist(Loc, DoorLocation), FVector::Dist(Extent, DoorTriggerExtent)),
					OrganoidPlaytestActions::ActorLabel(FoundDoor),
					false);
			}
			else
			{
				Record.Warnings.Add(TEXT("BP_AdminAccessDoor not found in PIE; door-unlock assertion skipped."));
			}

			if (Target)
			{
				const FOrganoidPlaytestPropValue Interactable = OrganoidPlaytestActions::ReadProperty(Target, TEXT("bIsInteractable"));
				AssertTrue(
					Record, FString::Printf(TEXT("%s.bIsInteractable"), *Prefix),
					Interactable.bHasBool && Interactable.bBool,
					TEXT("true"),
					Interactable.bFound ? Interactable.Text : TEXT("<missing>"),
					TerminalLabel, false);
			}

			const bool bBlueprintError = Sink && Sink->Errors.Num() > 0;
			AssertTrue(
				Record, FString::Printf(TEXT("%s.no_blueprint_errors"), *Prefix),
				!bBlueprintError,
				TEXT("0"),
				Sink ? FString::FromInt(Sink->Errors.Num()) : TEXT("<no-sink>"),
				TerminalLabel, false);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			if (InteractAttempt == 1)
			{
				InteractAttempt = 2;
				if (Sink)
				{
					Sink->MarkCursor();
				}
				Stage = EStage::Interact;
				Owner.SetStage(TEXT("Interact2"));
				return;
			}

			Stage = EStage::EndPie;
			Owner.SetStage(TEXT("EndPie"));
		}

		void Finalize(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Finalize"));
			const EOrganoidPlaytestState State = bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass;
			Owner.CompleteActive(State, Record.FailureReason);
		}
	};

	struct FS18AutoRegister
	{
		FS18AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FS18ReceptionTerminalFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FS18AutoRegister GRegisterS18;
}
