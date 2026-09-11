#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Perception/AISense_Hearing.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidPerceptionTypes.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("OpeningBlock4_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Opening Block 4 Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HostLabel[] = TEXT("Host_Admin_SecurityOfficer");
	const FVector AuthoredLocation(2820.0f, -600.0f, 100.0f);
	const FRotator AuthoredRotation(0.0f, 135.0f, 0.0f);
	const FVector AmmoTableauLocation(2560.0f, -340.0f, 100.0f);

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

	FString StateName(EProjectOrganoidHostCombatState State)
	{
		switch (State)
		{
		case EProjectOrganoidHostCombatState::Investigate: return TEXT("Investigate");
		case EProjectOrganoidHostCombatState::Pursue: return TEXT("Pursue");
		case EProjectOrganoidHostCombatState::Attack: return TEXT("Attack");
		case EProjectOrganoidHostCombatState::Search: return TEXT("Search");
		case EProjectOrganoidHostCombatState::Return: return TEXT("Return");
		case EProjectOrganoidHostCombatState::Dead: return TEXT("Dead");
		default: return TEXT("Idle");
		}
	}

	class FOpeningBlock4Functional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::Identity;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			DirtyBefore.Reset();
			HostStartLocation = FVector::ZeroVector;
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
			Identity,
			AuthoredConfiguration,
			Nav,
			SafeTableau,
			WaitSafeTableau,
			FootstepFilter,
			WaitFootstepFilter,
			CloseApproach,
			WaitCloseApproach,
			PermanentActivation,
			MutationOptOut,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Identity;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		TArray<FString> DirtyBefore;
		FVector HostStartLocation = FVector::ZeroVector;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host;
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

		AProjectOrganoidHostBase* FindHost(UWorld* World) const
		{
			return Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, HostLabel));
		}

		int32 CountAdminHosts(UWorld* World) const
		{
			int32 Count = 0;
			if (!World)
			{
				return Count;
			}
			for (TActorIterator<AProjectOrganoidHostBase> It(World); It; ++It)
			{
				if (OrganoidPlaytestActions::ActorPackage(*It).Contains(TEXT("SL_Epitope_Admin")))
				{
					++Count;
				}
			}
			return Count;
		}

		bool TeleportPlayer(AProjectOrganoidCharacter* Character, const FVector& Location)
		{
			if (!Character)
			{
				return false;
			}
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				Move->GravityScale = 1.0f;
				Move->SetMovementMode(MOVE_Walking);
				Move->StopMovementImmediately();
			}
			Character->SetActorEnableCollision(true);
			return OrganoidPlaytestActions::TeleportNear(Character, Location, 0.0f, Location.Z);
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
			if (PackageIsDirty(MapPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope is dirty. Refusing to start."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("playtest_mutates_assets"), TEXT("false"));
			Record.AddActor(TEXT("admin_dirty_before"), PackageIsDirty(AdminPackage) ? TEXT("true") : TEXT("false"));
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
			AProjectOrganoidHostBase* FoundHost = World ? FindHost(World) : nullptr;
			if (World && Character && FoundHost)
			{
				Player = Character;
				Host = FoundHost;
				Record.AddActor(TEXT("admin_host"), OrganoidPlaytestActions::ActorLabel(FoundHost));
				WaitSeconds = 0.0f;
				Proof = EProof::Identity;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 60.0f)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE player and %s (world=%s pawn=%s host=%s)."),
						HostLabel,
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						FoundHost ? TEXT("yes") : TEXT("no")));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidHostBase* AdminHost = Host.Get();
			if (!World || !Character || !AdminHost)
			{
				FailAndStop(Owner, Record, TEXT("PIE player or authorized Admin Host vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Identity:
			{
				const int32 AdminHostCount = CountAdminHosts(World);
				AssertTrue(Record, TEXT("admin_host_count"), AdminHostCount == 1, TEXT("1"), FString::FromInt(AdminHostCount), TEXT("Admin"));
				AssertTrue(
					Record,
					TEXT("admin_host_unique_label"),
					OrganoidPlaytestActions::ActorLabel(AdminHost) == HostLabel,
					HostLabel,
					OrganoidPlaytestActions::ActorLabel(AdminHost),
					HostLabel);
				AssertTrue(
					Record,
					TEXT("admin_host_package"),
					OrganoidPlaytestActions::ActorPackage(AdminHost).Contains(TEXT("SL_Epitope_Admin")),
					AdminPackage,
					OrganoidPlaytestActions::ActorPackage(AdminHost),
					HostLabel);
				AssertTrue(
					Record,
					TEXT("admin_host_uses_host_ai"),
					Cast<AProjectOrganoidHostAIController>(AdminHost->GetController()) != nullptr,
					TEXT("AProjectOrganoidHostAIController"),
					AdminHost->GetController() ? AdminHost->GetController()->GetClass()->GetName() : TEXT("none"),
					HostLabel);
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				Proof = EProof::AuthoredConfiguration;
				break;
			}
			case EProof::AuthoredConfiguration:
			{
				HostStartLocation = AdminHost->GetActorLocation();
				const FVector Scale = AdminHost->GetActorScale3D();
				const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(AdminHost->GetActorRotation().Yaw, AuthoredRotation.Yaw));
				AssertTrue(Record, TEXT("admin_host_location_xy"), FVector::Dist2D(HostStartLocation, AuthoredLocation) <= 2.0f, TEXT("within 2 uu"), HostStartLocation.ToString(), HostLabel);
				// Authored spawn Z is 100; PIE floor settle routinely lands ~108. Allow capsule/floor snap without weakening XY.
				AssertTrue(Record, TEXT("admin_host_location_z"), FMath::Abs(HostStartLocation.Z - AuthoredLocation.Z) <= 20.0f, TEXT("within 20 uu"), FString::SanitizeFloat(HostStartLocation.Z), HostLabel);
				AssertTrue(Record, TEXT("admin_host_rotation"), YawDelta <= 1.0f, TEXT("yaw 135 +/- 1"), AdminHost->GetActorRotation().ToString(), HostLabel);
				AssertTrue(Record, TEXT("admin_host_scale"), Scale.Equals(FVector::OneVector, 0.01f), TEXT("(1,1,1)"), Scale.ToString(), HostLabel);
				AssertTrue(Record, TEXT("admin_host_health"), FMath::IsNearlyEqual(AdminHost->MaxHealth, 100.0f), TEXT("100"), FString::SanitizeFloat(AdminHost->MaxHealth), HostLabel);
				AssertTrue(Record, TEXT("admin_host_melee_damage"), FMath::IsNearlyEqual(AdminHost->MeleeDamage, 15.0f), TEXT("15"), FString::SanitizeFloat(AdminHost->MeleeDamage), HostLabel);
				AssertTrue(Record, TEXT("admin_host_gate_enabled"), AdminHost->bRequiresEncounterActivation, TEXT("true"), AdminHost->bRequiresEncounterActivation ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("admin_host_gate_range"), FMath::IsNearlyEqual(AdminHost->ProximityActivationRange, 200.0f), TEXT("200"), FString::SanitizeFloat(AdminHost->ProximityActivationRange), HostLabel);
				AssertTrue(Record, TEXT("admin_host_starts_dormant"), !AdminHost->IsEncounterActivated(), TEXT("false"), AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("admin_host_starts_idle"), AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle, TEXT("Idle"), StateName(AdminHost->GetCombatState()), HostLabel);
				AssertTrue(Record, TEXT("admin_host_dormant_cannot_melee"), !AdminHost->CanAttemptMelee(), TEXT("false"), AdminHost->CanAttemptMelee() ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("admin_host_mutations_disabled"), !AdminHost->bAllowPhaseShiftMutations, TEXT("false"), AdminHost->bAllowPhaseShiftMutations ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("admin_host_no_initial_rage"), !AdminHost->bIsEnraged, TEXT("false"), AdminHost->bIsEnraged ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("admin_host_no_initial_bioshield"), !AdminHost->bHasBioShield, TEXT("false"), AdminHost->bHasBioShield ? TEXT("true") : TEXT("false"), HostLabel);
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				Proof = EProof::Nav;
				break;
			}
			case EProof::Nav:
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation Projected;
				const bool bHasNav = NavSys && NavSys->ProjectPointToNavigation(
					AdminHost->GetActorLocation(), Projected, FVector(200.0f, 200.0f, 300.0f));
				const float NavXYDelta = bHasNav
					? FVector::Dist2D(AdminHost->GetActorLocation(), Projected.Location)
					: TNumericLimits<float>::Max();
				const float NavZDelta = bHasNav
					? FMath::Abs(AdminHost->GetActorLocation().Z - Projected.Location.Z)
					: TNumericLimits<float>::Max();
				const bool bNavNearHost = bHasNav && NavXYDelta <= 75.0f && NavZDelta <= 150.0f;
				if (!AssertTrue(
					Record,
					TEXT("admin_host_recast_nav"),
					bNavNearHost,
					TEXT("projectable within xy=75 z=150"),
					bHasNav
						? FString::Printf(TEXT("xy=%.1f z=%.1f"), NavXYDelta, NavZDelta)
						: TEXT("no nav"),
					HostLabel))
				{
					FailAndStop(Owner, Record, TEXT("Admin Host location has no Recast navigation. Fail closed."));
					return;
				}
				Proof = EProof::SafeTableau;
				break;
			}
			case EProof::SafeTableau:
			{
				if (!TeleportPlayer(Character, AmmoTableauLocation))
				{
					FailAndStop(Owner, Record, TEXT("Could not place Nathan at the authored ammo tableau location."));
					return;
				}
				const FVector ToHost = AdminHost->GetActorLocation() - Character->GetActorLocation();
				if (ToHost.SizeSquared2D() > 1.0f)
				{
					const FRotator LookAt(0.0f, ToHost.Rotation().Yaw, 0.0f);
					Character->SetActorRotation(LookAt);
					if (AController* PlayerController = Character->GetController())
					{
						PlayerController->SetControlRotation(LookAt);
					}
				}
				WaitSeconds = 0.0f;
				Proof = EProof::WaitSafeTableau;
				break;
			}
			case EProof::WaitSafeTableau:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds < 1.5f)
				{
					break;
				}
				const float HostTravel = FVector::Dist2D(AdminHost->GetActorLocation(), HostStartLocation);
				const float TableauDistance = FVector::Dist2D(AdminHost->GetActorLocation(), Character->GetActorLocation());
				AssertTrue(Record, TEXT("ammo_tableau_outside_activation_range"), TableauDistance > AdminHost->ProximityActivationRange, TEXT(">200"), FString::SanitizeFloat(TableauDistance), HostLabel);
				AActor* SightTarget = AdminHost->GetCurrentSightTarget();
				AssertTrue(Record, TEXT("ammo_tableau_has_sight"), AdminHost->HasSightOnPlayer() && SightTarget == Character, TEXT("Nathan"), SightTarget ? SightTarget->GetName() : TEXT("none"), HostLabel);
				AssertTrue(Record, TEXT("distant_aimed_sight_stays_dormant"), !AdminHost->IsEncounterActivated(), TEXT("false"), AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("distant_sight_stays_idle"), AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle, TEXT("Idle"), StateName(AdminHost->GetCombatState()), HostLabel);
				AssertTrue(Record, TEXT("dormant_host_does_not_move"), HostTravel <= 5.0f, TEXT("<=5 uu"), FString::SanitizeFloat(HostTravel), HostLabel);
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				Proof = EProof::FootstepFilter;
				break;
			}
			case EProof::FootstepFilter:
			{
				AssertTrue(Record, TEXT("gunfire_is_activation_noise"), AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::Gunfire), TEXT("true"), AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::Gunfire) ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("generic_is_activation_noise"), AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::GenericNoise), TEXT("true"), AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::GenericNoise) ? TEXT("true") : TEXT("false"), HostLabel);
				const bool bOrdinaryFootstepsIgnored =
					!AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::FootstepCrouch)
					&& !AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::FootstepWalk)
					&& !AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::FootstepRun)
					&& !AdminHost->IsEncounterActivationNoise(EProjectOrganoidHearingStimulusKind::FootstepIdle);
				AssertTrue(Record, TEXT("ordinary_footsteps_not_activation_noise"), bOrdinaryFootstepsIgnored, TEXT("false for crouch/walk/run/idle"), bOrdinaryFootstepsIgnored ? TEXT("false for all") : TEXT("one or more true"), HostLabel);
				UAISense_Hearing::ReportNoiseEvent(
					World,
					Character->GetActorLocation(),
					1.25f,
					Character,
					3500.0f,
					ProjectOrganoidNoiseTags::FootstepRun);
				WaitSeconds = 0.0f;
				Proof = EProof::WaitFootstepFilter;
				break;
			}
			case EProof::WaitFootstepFilter:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds < 0.75f)
				{
					break;
				}
				const float HostTravel = FVector::Dist2D(AdminHost->GetActorLocation(), HostStartLocation);
				AssertTrue(Record, TEXT("distant_footstep_stays_dormant"), !AdminHost->IsEncounterActivated(), TEXT("false"), AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("distant_footstep_stays_idle"), AdminHost->GetCombatState() == EProjectOrganoidHostCombatState::Idle, TEXT("Idle"), StateName(AdminHost->GetCombatState()), HostLabel);
				AssertTrue(Record, TEXT("distant_footstep_does_not_move_host"), HostTravel <= 5.0f, TEXT("<=5 uu"), FString::SanitizeFloat(HostTravel), HostLabel);
				if (bAnyAssertFailed)
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
				Proof = EProof::CloseApproach;
				break;
			}
			case EProof::CloseApproach:
			{
				const FVector CloseDirection = FRotator(0.0f, AuthoredRotation.Yaw - 45.0f, 0.0f).Vector();
				const FVector CloseLocation = HostStartLocation + CloseDirection * 180.0f;
				if (!TeleportPlayer(Character, CloseLocation))
				{
					FailAndStop(Owner, Record, TEXT("Could not place Nathan inside the close-approach activation range."));
					return;
				}
				WaitSeconds = 0.0f;
				Proof = EProof::WaitCloseApproach;
				break;
			}
			case EProof::WaitCloseApproach:
			{
				WaitSeconds += DeltaTime;
				const EProjectOrganoidHostCombatState State = AdminHost->GetCombatState();
				if (AdminHost->IsEncounterActivated()
					&& (State == EProjectOrganoidHostCombatState::Pursue || State == EProjectOrganoidHostCombatState::Attack))
				{
					AssertTrue(Record, TEXT("close_visible_approach_activates"), true, TEXT("true"), TEXT("true"), HostLabel);
					AssertTrue(Record, TEXT("activated_host_enters_combat"), true, TEXT("Pursue|Attack"), StateName(State), HostLabel);
					TeleportPlayer(Character, FVector(200.0f, 0.0f, 118.0f));
					Proof = EProof::PermanentActivation;
					break;
				}
				if (WaitSeconds > 4.0f)
				{
					AssertTrue(Record, TEXT("close_visible_approach_activates"), false, TEXT("true"), AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"), HostLabel);
					FailAndStop(
						Owner,
						Record,
						FString::Printf(
							TEXT("Close approach did not activate (state=%s sight=%s distance=%.0f)."),
							*StateName(State),
							AdminHost->HasSightOnPlayer() ? TEXT("yes") : TEXT("no"),
							FVector::Dist2D(AdminHost->GetActorLocation(), Character->GetActorLocation())));
				}
				break;
			}
			case EProof::PermanentActivation:
			{
				AProjectOrganoidHostAIController* AI = Cast<AProjectOrganoidHostAIController>(AdminHost->GetController());
				if (AI)
				{
					AI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
				}
				AssertTrue(Record, TEXT("activation_is_permanent"), AdminHost->IsEncounterActivated(), TEXT("true"), AdminHost->IsEncounterActivated() ? TEXT("true") : TEXT("false"), HostLabel);
				Proof = EProof::MutationOptOut;
				break;
			}
			case EProof::MutationOptOut:
			{
				AdminHost->EnterRageState();
				AdminHost->ActivateBioShield();
				AssertTrue(Record, TEXT("rage_opt_out_enforced"), !AdminHost->bIsEnraged, TEXT("false"), AdminHost->bIsEnraged ? TEXT("true") : TEXT("false"), HostLabel);
				AssertTrue(Record, TEXT("bioshield_opt_out_enforced"), !AdminHost->bHasBioShield, TEXT("false"), AdminHost->bHasBioShield ? TEXT("true") : TEXT("false"), HostLabel);
				Proof = EProof::Done;
				break;
			}
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}

			if (bAnyAssertFailed && Stage == EStage::Proof)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
			}
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(
				Record,
				TEXT("dirty_unchanged"),
				DirtyAfter == DirtyBefore,
				FString::Join(DirtyBefore, TEXT(",")),
				FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"));
			Record.AddActor(TEXT("dirty_count"), FString::FromInt(DirtyAfter.Num()));
			Stage = EStage::Finalize;
		}
	};

	struct FOpeningBlock4AutoRegister
	{
		FOpeningBlock4AutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FOpeningBlock4Functional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FOpeningBlock4AutoRegister GRegisterOpeningBlock4;
}
