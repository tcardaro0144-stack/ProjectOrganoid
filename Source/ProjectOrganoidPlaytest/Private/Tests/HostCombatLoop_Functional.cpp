#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/CapsuleComponent.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Perception/AISense_Hearing.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidPerceptionTypes.h"
#include "ProjectOrganoidWeaponTypes.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("HostCombatLoop_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Host Combat Loop Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR Host1Label[] = TEXT("Host_Neuro_1");
	constexpr TCHAR Host2Label[] = TEXT("Host_Neuro_2");
	constexpr TCHAR Host3Label[] = TEXT("Host_Neuro_3");

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

	class FHostCombatLoopFunctional : public IOrganoidPlaytestCase
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
			DirtyBefore.Reset();
			CheckpointLocation = FVector::ZeroVector;
			HealthBeforeAttack = 0.0f;
			bRequestedNeuroStream = false;
			HostStartLocation = FVector::ZeroVector;
			InvestigateNoiseLocation = FVector(-400.0f, 0.0f, -1100.0f);
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
			Subjects,
			AdminIsolation,
			Nav,
			ActivateCheckpoint,
			HearingInvestigate,
			WaitInvestigate,
			SightPursue,
			WaitPursue,
			AttackDamage,
			WaitAttack,
			WallBlock,
			InvalidRange,
			StaggerCancel,
			WaitStagger,
			Blind,
			WeakPointDeath,
			PlayerDeath,
			WaitRestart,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::Subjects;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		TArray<FString> DirtyBefore;
		FVector CheckpointLocation;
		FVector HostStartLocation;
		FVector InvestigateNoiseLocation;
		float HealthBeforeAttack = 0.0f;
		bool bRequestedNeuroStream = false;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host1;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host2;
		TWeakObjectPtr<AProjectOrganoidHostBase> Host3;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> ActivatedCheckpoint;
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

		AProjectOrganoidHostBase* FindHost(UWorld* World, const TCHAR* Label) const
		{
			return Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, Label));
		}

		AProjectOrganoidCheckpoint* FindNeuroCheckpoint(UWorld* World) const
		{
			if (!World)
			{
				return nullptr;
			}
			AProjectOrganoidCheckpoint* Found = nullptr;
			for (TActorIterator<AProjectOrganoidCheckpoint> It(World); It; ++It)
			{
				AProjectOrganoidCheckpoint* Checkpoint = *It;
				const FString Owner = OrganoidPlaytestActions::ActorPackage(Checkpoint);
				if (Owner.Contains(TEXT("SL_Epitope_NeuroGenetics")))
				{
					if (Found)
					{
						return Found;
					}
					Found = Checkpoint;
				}
			}
			return Found;
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
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage))
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Blocked,
					TEXT("Lvl_Epitope or Admin is dirty. Refusing to start."));
				return;
			}
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
			bRequestedNeuroStream = false;
			Stage = EStage::WaitReady;
			Owner.SetStage(TEXT("WaitReady"));
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World
				? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>()
				: nullptr;
			bool bNeuroReady = false;
			if (Levels && Character && !bRequestedNeuroStream)
			{
				const FName NeuroName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics);
				if (!NeuroName.IsNone())
				{
					Levels->AddStreamRequest(NeuroName, Character);
					Levels->ReconcileStreamingNow();
					bRequestedNeuroStream = true;
				}
			}
			if (Levels)
			{
				const FName NeuroName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics);
				bNeuroReady = Levels->IsPartitionReady(NeuroName);
			}
			if (Character && bNeuroReady && !FindHost(World, Host1Label))
			{
				TeleportPlayer(Character, FVector(-400.0f, 1400.0f, -1100.0f));
			}
			AProjectOrganoidHostBase* FoundHost = World ? FindHost(World, Host1Label) : nullptr;
			if (World && Character && FoundHost)
			{
				Player = Character;
				Host1 = FoundHost;
				Host2 = FindHost(World, Host2Label);
				Host3 = FindHost(World, Host3Label);
				Record.AddActor(TEXT("host1"), OrganoidPlaytestActions::ActorLabel(FoundHost));
				Record.AddActor(TEXT("neuro_stream"), bNeuroReady ? TEXT("ready") : TEXT("pending"));
				WaitSeconds = 0.0f;
				Proof = EProof::Subjects;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 90.0f)
			{
				Owner.CompleteActive(
					EOrganoidPlaytestState::Fail,
					FString::Printf(
						TEXT("Timed out waiting for PIE player and Host_Neuro_1 (world=%s pawn=%s neuro=%s host=%s)."),
						World ? TEXT("yes") : TEXT("no"),
						Character ? TEXT("yes") : TEXT("no"),
						bNeuroReady ? TEXT("ready") : TEXT("not-ready"),
						FoundHost ? TEXT("yes") : TEXT("no")));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			AProjectOrganoidHostBase* Host = Host1.Get();
			if (!World || !Character || !Host)
			{
				FailAndStop(Owner, Record, TEXT("PIE player or Host_Neuro_1 vanished."));
				return;
			}

			switch (Proof)
			{
			case EProof::Subjects:
			{
				AssertTrue(Record, TEXT("host1_present"), Host != nullptr, TEXT("true"), Host ? TEXT("true") : TEXT("false"), Host1Label);
				AssertTrue(Record, TEXT("host2_present"), Host2.IsValid(), TEXT("true"), Host2.IsValid() ? TEXT("true") : TEXT("false"), Host2Label);
				AssertTrue(Record, TEXT("host3_present"), Host3.IsValid(), TEXT("true"), Host3.IsValid() ? TEXT("true") : TEXT("false"), Host3Label);
				AssertTrue(
					Record, TEXT("host1_uses_host_ai"),
					Cast<AProjectOrganoidHostAIController>(Host->GetController()) != nullptr,
					TEXT("AProjectOrganoidHostAIController"),
					Host->GetController() ? Host->GetController()->GetClass()->GetName() : TEXT("none"),
					Host1Label);
				Proof = EProof::AdminIsolation;
				break;
			}
			case EProof::AdminIsolation:
			{
				const int32 AdminHosts = CountHostsInPackage(World, TEXT("SL_Epitope_Admin"));
				AssertTrue(Record, TEXT("no_admin_hosts"), AdminHosts == 0, TEXT("0"), FString::FromInt(AdminHosts), TEXT("Admin"));
				Proof = EProof::Nav;
				break;
			}
			case EProof::Nav:
			{
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation Projected;
				const bool bHasNav = NavSys && NavSys->ProjectPointToNavigation(
					Host->GetActorLocation(), Projected, FVector(300.0f, 300.0f, 500.0f));
				if (!AssertTrue(
					Record, TEXT("neuro_recast_nav"),
					bHasNav,
					TEXT("projectable"),
					bHasNav ? TEXT("projectable") : TEXT("no nav"),
					Host1Label))
				{
					FailAndStop(Owner, Record, TEXT("Neuro Recast navigation is missing. Fail closed."));
					return;
				}
				HostStartLocation = Host->GetActorLocation();
				Proof = EProof::ActivateCheckpoint;
				break;
			}
			case EProof::ActivateCheckpoint:
			{
				AProjectOrganoidCheckpoint* Checkpoint = FindNeuroCheckpoint(World);
				if (!AssertTrue(
					Record, TEXT("checkpoint_found_by_class"),
					Checkpoint != nullptr,
					TEXT("AProjectOrganoidCheckpoint in Neuro"),
					Checkpoint ? OrganoidPlaytestActions::ActorLabel(Checkpoint) : TEXT("none"),
					TEXT("checkpoint")))
				{
					FailAndStop(Owner, Record, TEXT("No Neuro checkpoint actor found by class/package. Fail closed."));
					return;
				}
				ActivatedCheckpoint = Checkpoint;
				CheckpointLocation = Checkpoint->GetActorLocation();
				const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
				AssertTrue(Record, TEXT("checkpoint_activated"), bSaved && Character->HasActivatedCheckpoint(), TEXT("true"), bSaved ? TEXT("true") : TEXT("false"), OrganoidPlaytestActions::ActorLabel(Checkpoint));
				AssertTrue(
					Record, TEXT("checkpoint_identity_is_activated_actor"),
					Character->GetLastActivatedCheckpoint() == Checkpoint,
					TEXT("same actor"),
					Character->GetLastActivatedCheckpoint() ? TEXT("same actor") : TEXT("null"),
					TEXT("player"));
				Proof = EProof::HearingInvestigate;
				break;
			}
			case EProof::HearingInvestigate:
			{
				TeleportPlayer(Character, FVector(400.0f, -1800.0f, -1100.0f));
				HostStartLocation = Host->GetActorLocation();
				InvestigateNoiseLocation = HostStartLocation + FVector(0.0f, -450.0f, 0.0f);
				UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
				FNavLocation ProjectedNoise;
				const bool bNoiseOnNav = NavSys && NavSys->ProjectPointToNavigation(
					InvestigateNoiseLocation, ProjectedNoise, FVector(250.0f, 250.0f, 400.0f));
				if (!AssertTrue(
					Record, TEXT("investigate_noise_on_nav"),
					bNoiseOnNav,
					TEXT("projectable"),
					bNoiseOnNav ? TEXT("projectable") : TEXT("no nav"),
					Host1Label))
				{
					FailAndStop(Owner, Record, TEXT("Investigate noise location is not on Recast. Fail closed."));
					return;
				}
				InvestigateNoiseLocation = ProjectedNoise.Location;
				UAISense_Hearing::ReportNoiseEvent(
					World,
					InvestigateNoiseLocation,
					2.5f,
					Character,
					3500.0f,
					ProjectOrganoidNoiseTags::Gunfire);
				WaitSeconds = 0.0f;
				Proof = EProof::WaitInvestigate;
				break;
			}
			case EProof::WaitInvestigate:
			{
				WaitSeconds += DeltaTime;
				const EProjectOrganoidHostCombatState State = Host->GetCombatState();
				const float DistToNoise = FVector::Dist2D(Host->GetActorLocation(), InvestigateNoiseLocation);
				const float DistToPlayer = FVector::Dist2D(Host->GetActorLocation(), Character->GetActorLocation());
				if (State == EProjectOrganoidHostCombatState::Investigate
					&& DistToNoise < FVector::Dist2D(HostStartLocation, InvestigateNoiseLocation) - 40.0f)
				{
					AssertTrue(Record, TEXT("unseen_gunfire_investigates_noise"), true, TEXT("Investigate toward noise"), StateName(State), Host1Label);
					AssertTrue(
						Record, TEXT("unseen_gunfire_not_omniscient"),
						DistToPlayer > 800.0f,
						TEXT(">800 from hidden player"),
						FString::SanitizeFloat(DistToPlayer),
						Host1Label);
					Proof = EProof::SightPursue;
					break;
				}
				if (WaitSeconds > 8.0f)
				{
					AssertTrue(Record, TEXT("unseen_gunfire_investigates_noise"), false, TEXT("Investigate toward noise"), StateName(State), Host1Label);
					FailAndStop(
						Owner,
						Record,
						FString::Printf(
							TEXT("Host did not investigate unseen gunfire (state=%s heard=%s distNoise=%.0f distPlayer=%.0f)."),
							*StateName(State),
							Host->HasRecentNoiseStimulus() ? TEXT("yes") : TEXT("no"),
							DistToNoise,
							DistToPlayer));
				}
				break;
			}
			case EProof::SightPursue:
			{
				const FVector SightStand = InvestigateNoiseLocation.IsNearlyZero()
					? (Host->GetActorLocation() + Host->GetActorForwardVector() * 220.0f)
					: InvestigateNoiseLocation;
				TeleportPlayer(Character, SightStand);
				const FVector ToPlayer = Character->GetActorLocation() - Host->GetActorLocation();
				if (ToPlayer.SizeSquared2D() > 1.0f)
				{
					const FRotator Yaw(0.0f, ToPlayer.Rotation().Yaw, 0.0f);
					Host->SetActorRotation(Yaw);
					if (AController* HostController = Host->GetController())
					{
						HostController->SetControlRotation(Yaw);
					}
				}
				WaitSeconds = 0.0f;
				Proof = EProof::WaitPursue;
				break;
			}
			case EProof::WaitPursue:
			{
				WaitSeconds += DeltaTime;
				const EProjectOrganoidHostCombatState State = Host->GetCombatState();
				if (State == EProjectOrganoidHostCombatState::Pursue || State == EProjectOrganoidHostCombatState::Attack)
				{
					AssertTrue(Record, TEXT("sight_causes_pursuit"), true, TEXT("Pursue|Attack"), StateName(State), Host1Label);
					Proof = EProof::AttackDamage;
					break;
				}
				if (WaitSeconds > 8.0f)
				{
					AssertTrue(Record, TEXT("sight_causes_pursuit"), false, TEXT("Pursue|Attack"), StateName(State), Host1Label);
					FailAndStop(
						Owner,
						Record,
						FString::Printf(
							TEXT("Host did not pursue after sight setup (state=%s sight=%s dist=%.0f)."),
							*StateName(State),
							Host->HasSightOnPlayer() ? TEXT("yes") : TEXT("no"),
							FVector::Dist2D(Host->GetActorLocation(), Character->GetActorLocation())));
				}
				break;
			}
			case EProof::AttackDamage:
			{
				HealthBeforeAttack = Character->GetHealth();
				TeleportPlayer(Character, Host->GetActorLocation() + Host->GetActorForwardVector() * 140.0f);
				Host->TryBeginMeleeAttack(Character);
				WaitSeconds = 0.0f;
				Proof = EProof::WaitAttack;
				break;
			}
			case EProof::WaitAttack:
			{
				WaitSeconds += DeltaTime;
				if (Character->GetHealth() < HealthBeforeAttack - 0.5f || Host->DidLastMeleeCommitDamage())
				{
					AssertTrue(Record, TEXT("melee_damages_player"), true, TEXT("health dropped"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
					Proof = EProof::WallBlock;
					break;
				}
				if (WaitSeconds > 3.0f)
				{
					AssertTrue(Record, TEXT("melee_damages_player"), false, TEXT("health dropped"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
					FailAndStop(Owner, Record, TEXT("Host melee did not damage Nathan."));
				}
				break;
			}
			case EProof::WallBlock:
			{
				const float HealthBefore = Character->GetHealth();
				TeleportPlayer(Character, FVector(-1600.0f, 1400.0f, -1100.0f));
				Host->SetActorRotation(FRotator(0.0f, 180.0f, 0.0f));
				const bool bBegan = Host->TryBeginMeleeAttack(Character);
				AssertTrue(Record, TEXT("no_wall_melee_begin"), !bBegan, TEXT("false"), bBegan ? TEXT("true") : TEXT("false"), Host1Label);
				AssertTrue(Record, TEXT("no_wall_damage"), FMath::IsNearlyEqual(Character->GetHealth(), HealthBefore), FString::SanitizeFloat(HealthBefore), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
				Proof = EProof::InvalidRange;
				break;
			}
			case EProof::InvalidRange:
			{
				const float HealthBefore = Character->GetHealth();
				TeleportPlayer(Character, Host->GetActorLocation() + FVector(0.0f, -900.0f, 0.0f));
				const bool bBegan = Host->TryBeginMeleeAttack(Character);
				AssertTrue(Record, TEXT("no_invalid_range_melee"), !bBegan, TEXT("false"), bBegan ? TEXT("true") : TEXT("false"), Host1Label);
				AssertTrue(Record, TEXT("no_invalid_range_damage"), FMath::IsNearlyEqual(Character->GetHealth(), HealthBefore), FString::SanitizeFloat(HealthBefore), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
				Proof = EProof::StaggerCancel;
				break;
			}
			case EProof::StaggerCancel:
			{
				HealthBeforeAttack = Character->GetHealth();
				TeleportPlayer(Character, Host->GetActorLocation() + Host->GetActorForwardVector() * 140.0f);
				Host->TryBeginMeleeAttack(Character);
				FProjectOrganoidBallisticHit Hit;
				Hit.HitActor = Host;
				Hit.FinalDamage = 4.0f;
				Hit.WeakPoint = EProjectOrganoidWeakPointType::None;
				Host->ApplyResolvedOrganoidHit(Hit, Character);
				WaitSeconds = 0.0f;
				Proof = EProof::WaitStagger;
				break;
			}
			case EProof::WaitStagger:
			{
				WaitSeconds += DeltaTime;
				if (WaitSeconds > Host->MeleeWindupSeconds + 0.35f)
				{
					AssertTrue(Record, TEXT("stagger_cancels_windup"), !Host->DidLastMeleeCommitDamage(), TEXT("no commit"), Host->DidLastMeleeCommitDamage() ? TEXT("committed") : TEXT("cancelled"), Host1Label);
					AssertTrue(Record, TEXT("stagger_no_bonus_damage"), FMath::IsNearlyEqual(Character->GetHealth(), HealthBeforeAttack), FString::SanitizeFloat(HealthBeforeAttack), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
					Proof = EProof::Blind;
				}
				break;
			}
			case EProof::Blind:
			{
				FProjectOrganoidBallisticHit Hit;
				Hit.HitActor = Host;
				Hit.FinalDamage = 12.0f;
				Hit.WeakPoint = EProjectOrganoidWeakPointType::OpticalNodes;
				Hit.bTacticalModeHit = true;
				Host->ApplyResolvedOrganoidHit(Hit, Character);
				AssertTrue(Record, TEXT("blind_applies"), Host->bIsBlinded, TEXT("true"), Host->bIsBlinded ? TEXT("true") : TEXT("false"), Host1Label);
				const EProjectOrganoidHostCombatState State = Host->GetCombatState();
				AssertTrue(
					Record, TEXT("blind_drops_pursuit"),
					State != EProjectOrganoidHostCombatState::Pursue && State != EProjectOrganoidHostCombatState::Attack,
					TEXT("Search|Investigate|Idle"),
					StateName(State),
					Host1Label);
				Proof = EProof::WeakPointDeath;
				break;
			}
			case EProof::WeakPointDeath:
			{
				FProjectOrganoidBallisticHit Hit;
				Hit.HitActor = Host;
				Hit.FinalDamage = 400.0f;
				Hit.WeakPoint = EProjectOrganoidWeakPointType::OrganoidCore;
				Hit.bTacticalModeHit = true;
				Host->ApplyResolvedOrganoidHit(Hit, Character);
				AssertTrue(
					Record, TEXT("weak_point_core_accepted"),
					Host->bBioCoreDestroyed,
					TEXT("core destroyed"),
					Host->bBioCoreDestroyed ? TEXT("core destroyed") : TEXT("core intact"),
					Host1Label);
				if (!Host->bIsDead)
				{
					FProjectOrganoidBallisticHit KillHit;
					KillHit.HitActor = Host;
					KillHit.FinalDamage = 400.0f;
					KillHit.WeakPoint = EProjectOrganoidWeakPointType::OrganoidCore;
					KillHit.bTacticalModeHit = true;
					Host->ApplyResolvedOrganoidHit(KillHit, Character);
				}
				AssertTrue(Record, TEXT("host_death"), Host->bIsDead, TEXT("true"), Host->bIsDead ? TEXT("true") : TEXT("false"), Host1Label);
				AssertTrue(Record, TEXT("host_dead_state"), Host->GetCombatState() == EProjectOrganoidHostCombatState::Dead, TEXT("Dead"), StateName(Host->GetCombatState()), Host1Label);
				Proof = EProof::PlayerDeath;
				break;
			}
			case EProof::PlayerDeath:
			{
				if (!Character->HasActivatedCheckpoint())
				{
					FailAndStop(Owner, Record, TEXT("Player death test has no activated checkpoint. Fail closed."));
					return;
				}
				Character->ApplyHealthDelta(-Character->GetMaxHealth(), EProjectOrganoidHealthDeltaSource::Generic);
				AssertTrue(Record, TEXT("player_enters_death"), Character->IsPlayerDead(), TEXT("true"), Character->IsPlayerDead() ? TEXT("true") : TEXT("false"), TEXT("player"));
				WaitSeconds = 0.0f;
				Proof = EProof::WaitRestart;
				break;
			}
			case EProof::WaitRestart:
			{
				WaitSeconds += DeltaTime;
				if (!Character->IsPlayerDead() && Character->GetHealth() > 0.0f)
				{
					AProjectOrganoidCheckpoint* Checkpoint = Character->GetLastActivatedCheckpoint();
					const float Dist = Checkpoint ? FVector::Dist2D(Character->GetActorLocation(), Checkpoint->GetActorLocation()) : 99999.0f;
					AssertTrue(Record, TEXT("player_restarted"), true, TEXT("alive"), TEXT("alive"), TEXT("player"));
					AssertTrue(Record, TEXT("restart_at_activated_checkpoint"), Dist < 280.0f, TEXT("<280"), FString::SanitizeFloat(Dist), TEXT("player"));
					AssertTrue(Record, TEXT("restart_health_restored"), Character->GetHealth() > 0.0f, TEXT(">0"), FString::SanitizeFloat(Character->GetHealth()), TEXT("player"));
					Proof = EProof::Done;
					break;
				}
				if (WaitSeconds > 4.0f)
				{
					FailAndStop(Owner, Record, TEXT("Nathan did not restart from the last activated checkpoint."));
				}
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
				Record, TEXT("dirty_unchanged"),
				DirtyAfter == DirtyBefore,
				FString::Join(DirtyBefore, TEXT(",")),
				FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"));
			Record.AddActor(TEXT("dirty_count"), FString::FromInt(DirtyAfter.Num()));
			Stage = EStage::Finalize;
		}
	};

	struct FHostCombatAutoRegister
	{
		FHostCombatAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FHostCombatLoopFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FHostCombatAutoRegister GRegisterHostCombatLoop;
}
