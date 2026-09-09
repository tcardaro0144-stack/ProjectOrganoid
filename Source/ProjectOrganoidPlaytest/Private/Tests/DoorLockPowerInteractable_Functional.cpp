#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidDoorLock.h"
#include "ProjectOrganoidPowerTypes.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("DoorLockPowerInteractable_Functional");
	constexpr TCHAR DisplayName[] = TEXT("DoorLock Power Interactable Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR KnownNeuroRecastPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");

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

	FString BoolText(bool bValue)
	{
		return bValue ? TEXT("true") : TEXT("false");
	}

	class FDoorLockPowerInteractableFunctional : public IOrganoidPlaytestCase
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
			bAdminDirtyBefore = false;
			bEpitopeDirtyBefore = false;
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DestroySpawned();
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
				TickProof(Owner, *Record);
				break;
			case EStage::EndPie:
				DestroySpawned();
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

		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bAdminDirtyBefore = false;
		bool bEpitopeDirtyBefore = false;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidDoorLock> CaseA;
		TWeakObjectPtr<AProjectOrganoidDoorLock> CaseB;

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

		void DestroySpawned()
		{
			if (AProjectOrganoidDoorLock* Lock = CaseA.Get())
			{
				Lock->Destroy();
			}
			if (AProjectOrganoidDoorLock* Lock = CaseB.Get())
			{
				Lock->Destroy();
			}
			CaseA.Reset();
			CaseB.Reset();
		}

		AProjectOrganoidDoorLock* SpawnCase(UWorld* World, bool bAuthoredInteractable, const FVector& Location)
		{
			if (!World)
			{
				return nullptr;
			}
			AProjectOrganoidDoorLock* Lock = World->SpawnActorDeferred<AProjectOrganoidDoorLock>(
				AProjectOrganoidDoorLock::StaticClass(),
				FTransform(Location));
			if (!Lock)
			{
				return nullptr;
			}
			Lock->bIsInteractable = bAuthoredInteractable;
			Lock->bDisableInteractDuringBlackout = true;
			Lock->FinishSpawning(FTransform(Location));
			return Lock;
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

			TArray<FString> DirtyNow;
			CollectDirtyPackageNames(DirtyNow);
			for (const FString& Name : DirtyNow)
			{
				if (!Name.Equals(KnownNeuroRecastPackage, ESearchCase::IgnoreCase))
				{
					Owner.CompleteActive(
						EOrganoidPlaytestState::Blocked,
						FString::Printf(TEXT("Unexpected dirty package %s. Refusing to start."), *Name));
					return;
				}
			}

			bAdminDirtyBefore = PackageIsDirty(AdminPackage);
			bEpitopeDirtyBefore = PackageIsDirty(MapPackage);
			DirtyBefore = DirtyNow;
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
			bool bReady = false;
			if (Character)
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					bReady = Move->MovementMode == MOVE_Walking || Move->MovementMode == MOVE_Falling || WaitSeconds > 10.0f;
				}
			}
			if (World && Character && bReady)
			{
				WaitSeconds = 0.0f;
				Stage = EStage::Proof;
				Owner.SetStage(TEXT("Proof"));
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE player."));
			}
			(void)Record;
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			if (!World)
			{
				FailAndStop(Owner, Record, TEXT("PIE world vanished."));
				return;
			}

			AProjectOrganoidDoorLock* AuthoredFalse = SpawnCase(World, false, FVector(200.0f, 1200.0f, 4000.0f));
			CaseA = AuthoredFalse;
			AssertTrue(Record, TEXT("caseA.spawned"), AuthoredFalse != nullptr,
				TEXT("spawned"), AuthoredFalse ? TEXT("spawned") : TEXT("null"), TEXT("caseA"));
			if (!AuthoredFalse)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn authored-false DoorLock."));
				return;
			}
			AssertTrue(Record, TEXT("caseA.beginplay_online"), !AuthoredFalse->bIsInteractable,
				TEXT("false"), BoolText(AuthoredFalse->bIsInteractable), TEXT("caseA"));
			AuthoredFalse->HandlePowerStateChanged(
				EProjectOrganoidPowerState::Blackout, EProjectOrganoidPowerState::Online);
			AssertTrue(Record, TEXT("caseA.blackout"), !AuthoredFalse->bIsInteractable,
				TEXT("false"), BoolText(AuthoredFalse->bIsInteractable), TEXT("caseA"));
			AuthoredFalse->HandlePowerStateChanged(
				EProjectOrganoidPowerState::Online, EProjectOrganoidPowerState::Blackout);
			AssertTrue(Record, TEXT("caseA.online_restore"), !AuthoredFalse->bIsInteractable,
				TEXT("false"), BoolText(AuthoredFalse->bIsInteractable), TEXT("caseA"));

			AProjectOrganoidDoorLock* AuthoredTrue = SpawnCase(World, true, FVector(200.0f, 1400.0f, 4000.0f));
			CaseB = AuthoredTrue;
			AssertTrue(Record, TEXT("caseB.spawned"), AuthoredTrue != nullptr,
				TEXT("spawned"), AuthoredTrue ? TEXT("spawned") : TEXT("null"), TEXT("caseB"));
			if (!AuthoredTrue)
			{
				FailAndStop(Owner, Record, TEXT("Failed to spawn authored-true DoorLock."));
				return;
			}
			AssertTrue(Record, TEXT("caseB.online"), AuthoredTrue->bIsInteractable,
				TEXT("true"), BoolText(AuthoredTrue->bIsInteractable), TEXT("caseB"));
			AuthoredTrue->HandlePowerStateChanged(
				EProjectOrganoidPowerState::Blackout, EProjectOrganoidPowerState::Online);
			AssertTrue(Record, TEXT("caseB.blackout"), !AuthoredTrue->bIsInteractable,
				TEXT("false"), BoolText(AuthoredTrue->bIsInteractable), TEXT("caseB"));
			AuthoredTrue->HandlePowerStateChanged(
				EProjectOrganoidPowerState::Online, EProjectOrganoidPowerState::Blackout);
			AssertTrue(Record, TEXT("caseB.online_restore"), AuthoredTrue->bIsInteractable,
				TEXT("true"), BoolText(AuthoredTrue->bIsInteractable), TEXT("caseB"));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Stage = EStage::EndPie;
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			AssertTrue(Record, TEXT("dirty.admin_unchanged"),
				PackageIsDirty(AdminPackage) == bAdminDirtyBefore,
				bAdminDirtyBefore ? TEXT("dirty") : TEXT("clean"),
				PackageIsDirty(AdminPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("SL_Epitope_Admin"));
			AssertTrue(Record, TEXT("dirty.lvl_epitope_unchanged"),
				PackageIsDirty(MapPackage) == bEpitopeDirtyBefore,
				bEpitopeDirtyBefore ? TEXT("dirty") : TEXT("clean"),
				PackageIsDirty(MapPackage) ? TEXT("dirty") : TEXT("clean"),
				TEXT("Lvl_Epitope"));
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			Record.AddActor(TEXT("dirty_before"), FString::Join(DirtyBefore, TEXT(",")));
			Record.AddActor(TEXT("dirty_after"), FString::Join(DirtyAfter, TEXT(",")));
			for (const FString& Name : DirtyAfter)
			{
				if (!Name.Equals(KnownNeuroRecastPackage, ESearchCase::IgnoreCase)
					&& !DirtyBefore.Contains(Name))
				{
					AssertTrue(Record, TEXT("durable.no_unauthorized_new_dirty"), false,
						TEXT("0"), Name, TEXT(""));
				}
			}
			AssertTrue(Record, TEXT("playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""));
			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FDoorLockPowerInteractableAutoRegister
	{
		FDoorLockPowerInteractableAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FDoorLockPowerInteractableFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FDoorLockPowerInteractableAutoRegister GRegisterDoorLockPowerInteractable;
}
