#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidSaveGame.h"
#include "ProjectOrganoidSaveSubsystem.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("CheckpointHealth_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Checkpoint Health Functional");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR KnownNeuroRecastPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR TraumaPath[] = TEXT("/Game/Data/Items/DA_Item_TraumaStabilizer.DA_Item_TraumaStabilizer");
	constexpr TCHAR CheckpointLabel[] = TEXT("Checkpoint_ReceptionAtrium");
	constexpr TCHAR SaveSlot[] = TEXT("OrganoidCheckpointHealthTest");
	constexpr float HealthTolerance = 0.05f;

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

	class FCheckpointHealthFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			Proof = EProof::World;
			WaitSeconds = 0.0f;
			bAnyAssertFailed = false;
			bAdminDirtyBefore = false;
			bEpitopeDirtyBefore = false;
			DirtyBefore.Reset();
			Owner.SetStage(TEXT("Preflight"));
		}

		virtual void Abort(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			DeleteTestSave();
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
				DeleteTestSave();
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
			World,
			FloorCases,
			DeathFortyKill,
			DeathFortyWait,
			DeathFloorKill,
			DeathFloorWait,
			Trauma,
			Done
		};

		EStage Stage = EStage::Preflight;
		EProof Proof = EProof::World;
		float WaitSeconds = 0.0f;
		bool bAnyAssertFailed = false;
		bool bAdminDirtyBefore = false;
		bool bEpitopeDirtyBefore = false;
		TArray<FString> DirtyBefore;
		TWeakObjectPtr<AProjectOrganoidCharacter> Player;
		TWeakObjectPtr<AProjectOrganoidCheckpoint> ActivatedCheckpoint;

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

		void DeleteTestSave() const
		{
			UGameplayStatics::DeleteGameInSlot(SaveSlot, 0);
		}

		UProjectOrganoidSaveSubsystem* GetSaves(AProjectOrganoidCharacter* Character) const
		{
			UGameInstance* GI = Character ? Character->GetGameInstance() : nullptr;
			return GI ? GI->GetSubsystem<UProjectOrganoidSaveSubsystem>() : nullptr;
		}

		void SetHealthExact(AProjectOrganoidCharacter* Character, float Target) const
		{
			if (!Character)
			{
				return;
			}
			Character->ApplyHealthDelta(Target - Character->GetHealth());
		}

		bool HealthIs(AProjectOrganoidCharacter* Character, float Expected) const
		{
			return Character && FMath::IsNearlyEqual(Character->GetHealth(), Expected, HealthTolerance);
		}

		float ReadSavedHealth() const
		{
			USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlot, 0);
			if (const UProjectOrganoidSaveGame* SaveGame = Cast<UProjectOrganoidSaveGame>(Loaded))
			{
				return SaveGame->Health;
			}
			return -1.0f;
		}

		bool FloorIsTwentyFive(const AProjectOrganoidCheckpoint* Checkpoint) const
		{
			return Checkpoint && FMath::IsNearlyEqual(Checkpoint->HealthStabilizationFloorPercent, 0.25f);
		}

		AProjectOrganoidCheckpoint* FindReceptionCheckpoint(UWorld* World) const
		{
			return Cast<AProjectOrganoidCheckpoint>(OrganoidPlaytestActions::FindUniqueByLabel(World, CheckpointLabel));
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
				Player = Character;
				WaitSeconds = 0.0f;
				Proof = EProof::World;
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

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Player.Get();
			if (!Character)
			{
				Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
				Player = Character;
			}
			if (!World || !Character)
			{
				FailAndStop(Owner, Record, TEXT("Lost PIE world or Nathan during CheckpointHealth proof."));
				return;
			}

			switch (Proof)
			{
			case EProof::World:
				TickWorld(Owner, Record, World, Character);
				break;
			case EProof::FloorCases:
				TickFloorCases(Owner, Record, World, Character);
				break;
			case EProof::DeathFortyKill:
				TickDeathKill(Owner, Record, Character, 40.0f, TEXT("death40"));
				Proof = EProof::DeathFortyWait;
				WaitSeconds = 0.0f;
				break;
			case EProof::DeathFortyWait:
				TickDeathWait(Owner, Record, Character, DeltaTime, 40.0f, TEXT("death40"), EProof::DeathFloorKill);
				break;
			case EProof::DeathFloorKill:
				TickDeathKill(Owner, Record, Character, 10.0f, TEXT("death25"));
				Proof = EProof::DeathFloorWait;
				WaitSeconds = 0.0f;
				break;
			case EProof::DeathFloorWait:
				TickDeathWait(Owner, Record, Character, DeltaTime, 25.0f, TEXT("death25"), EProof::Trauma);
				break;
			case EProof::Trauma:
				TickTrauma(Owner, Record, Character);
				break;
			case EProof::Done:
				Stage = EStage::EndPie;
				break;
			}

			if (bAnyAssertFailed && Stage == EStage::Proof)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
			}
		}

		void TickWorld(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidCheckpoint* AdminCheckpoint = FindReceptionCheckpoint(World);
			AssertTrue(Record, TEXT("pie.Checkpoint_ReceptionAtrium.present"),
				AdminCheckpoint && FVector::Dist(AdminCheckpoint->GetActorLocation(), FVector(-1535.0f, 0.0f, 60.0f)) <= 5.0f,
				TEXT("(-1535,0,60)"),
				AdminCheckpoint ? AdminCheckpoint->GetActorLocation().ToCompactString() : TEXT("missing"),
				CheckpointLabel);
			AssertTrue(Record, TEXT("pie.Checkpoint_ReceptionAtrium.floor_percent"),
				FloorIsTwentyFive(AdminCheckpoint),
				TEXT("0.25"),
				AdminCheckpoint ? FString::SanitizeFloat(AdminCheckpoint->HealthStabilizationFloorPercent) : TEXT("missing"),
				CheckpointLabel);
			AssertTrue(Record, TEXT("fresh.health"),
				HealthIs(Character, 100.0f) && FMath::IsNearlyEqual(Character->GetMaxHealth(), 100.0f),
				TEXT("100/100"),
				FString::Printf(TEXT("%.0f/%.0f"), Character->GetHealth(), Character->GetMaxHealth()),
				TEXT("player"));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::FloorCases;
			Owner.SetStage(TEXT("FloorCases"));
		}

		bool RunFloorCase(
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCheckpoint* Checkpoint,
			AProjectOrganoidCharacter* Character,
			float StartHealth,
			float ExpectedHealth,
			const TCHAR* CaseId)
		{
			SetHealthExact(Character, StartHealth);
			AssertTrue(Record, FString::Printf(TEXT("case.%s.start"), CaseId),
				HealthIs(Character, StartHealth),
				FString::SanitizeFloat(StartHealth),
				FString::SanitizeFloat(Character->GetHealth()),
				CheckpointLabel);
			const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
			AssertTrue(Record, FString::Printf(TEXT("case.%s.saved"), CaseId),
				bSaved && Character->HasActivatedCheckpoint(),
				TEXT("true"),
				bSaved ? TEXT("true") : TEXT("false"),
				CheckpointLabel);
			AssertTrue(Record, FString::Printf(TEXT("case.%s.live_health"), CaseId),
				HealthIs(Character, ExpectedHealth),
				FString::SanitizeFloat(ExpectedHealth),
				FString::SanitizeFloat(Character->GetHealth()),
				CheckpointLabel);
			const float SavedHealth = ReadSavedHealth();
			AssertTrue(Record, FString::Printf(TEXT("case.%s.saved_health"), CaseId),
				FMath::IsNearlyEqual(SavedHealth, ExpectedHealth, HealthTolerance),
				FString::SanitizeFloat(ExpectedHealth),
				FString::SanitizeFloat(SavedHealth),
				SaveSlot);
			return !bAnyAssertFailed;
		}

		void TickFloorCases(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			UWorld* World,
			AProjectOrganoidCharacter* Character)
		{
			AProjectOrganoidCheckpoint* Checkpoint = FindReceptionCheckpoint(World);
			ActivatedCheckpoint = Checkpoint;
			if (!Checkpoint)
			{
				FailAndStop(Owner, Record, TEXT("Checkpoint_ReceptionAtrium missing in PIE."));
				return;
			}

			Checkpoint->SaveSlotOverride = SaveSlot;
			DeleteTestSave();

			const TArray<TPair<float, float>> Cases = {
				{10.0f, 25.0f},
				{20.0f, 25.0f},
				{24.0f, 25.0f},
				{25.0f, 25.0f},
				{40.0f, 40.0f},
				{90.0f, 90.0f},
				{100.0f, 100.0f},
			};
			const TCHAR* CaseIds[] = {
				TEXT("10"), TEXT("20"), TEXT("24"), TEXT("25"), TEXT("40"), TEXT("90"), TEXT("100")
			};
			for (int32 Index = 0; Index < Cases.Num(); ++Index)
			{
				if (!RunFloorCase(Record, Checkpoint, Character, Cases[Index].Key, Cases[Index].Value, CaseIds[Index]))
				{
					FailAndStop(Owner, Record, Record.FailureReason);
					return;
				}
			}

			SetHealthExact(Character, 10.0f);
			Checkpoint->TriggerCheckpointSave(Character);
			AssertTrue(Record, TEXT("farm.first"), HealthIs(Character, 25.0f), TEXT("25"), FString::SanitizeFloat(Character->GetHealth()), CheckpointLabel);
			Checkpoint->TriggerCheckpointSave(Character);
			AssertTrue(Record, TEXT("farm.second"), HealthIs(Character, 25.0f), TEXT("25"), FString::SanitizeFloat(Character->GetHealth()), CheckpointLabel);
			Checkpoint->TriggerCheckpointSave(Character);
			AssertTrue(Record, TEXT("farm.third"), HealthIs(Character, 25.0f), TEXT("25"), FString::SanitizeFloat(Character->GetHealth()), CheckpointLabel);
			AssertTrue(Record, TEXT("farm.not_50"),
				!HealthIs(Character, 50.0f) && !HealthIs(Character, 75.0f) && !HealthIs(Character, 100.0f),
				TEXT("25"),
				FString::SanitizeFloat(Character->GetHealth()),
				CheckpointLabel);

			SetHealthExact(Character, 10.0f);
			Checkpoint->TriggerCheckpointSave(Character);
			SetHealthExact(Character, 5.0f);
			UProjectOrganoidSaveSubsystem* Saves = GetSaves(Character);
			const bool bLoaded = Saves && Saves->LoadPlayerProgress(Character, SaveSlot);
			AssertTrue(Record, TEXT("reload.loaded"), bLoaded, TEXT("true"), bLoaded ? TEXT("true") : TEXT("false"), SaveSlot);
			AssertTrue(Record, TEXT("reload.health_25"),
				HealthIs(Character, 25.0f),
				TEXT("25"),
				FString::SanitizeFloat(Character->GetHealth()),
				TEXT("save"));

			AssertTrue(Record, TEXT("anchor.activated"),
				Character->HasActivatedCheckpoint() && Character->GetLastActivatedCheckpoint() == Checkpoint,
				TEXT("ReceptionAtrium"),
				Character->GetLastActivatedCheckpoint() ? OrganoidPlaytestActions::ActorLabel(Character->GetLastActivatedCheckpoint()) : TEXT("null"),
				CheckpointLabel);

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::DeathFortyKill;
			Owner.SetStage(TEXT("DeathFortyKill"));
		}

		void TickDeathKill(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character,
			float StartHealth,
			const TCHAR* CaseId)
		{
			AProjectOrganoidCheckpoint* Checkpoint = ActivatedCheckpoint.Get();
			if (!Checkpoint)
			{
				FailAndStop(Owner, Record, TEXT("Checkpoint lost before death case."));
				return;
			}

			const float ExpectedSaved = StartHealth < 25.0f ? 25.0f : StartHealth;
			SetHealthExact(Character, StartHealth);
			const bool bSaved = Checkpoint->TriggerCheckpointSave(Character);
			AssertTrue(Record, FString::Printf(TEXT("%s.pre_death_saved"), CaseId),
				bSaved && HealthIs(Character, ExpectedSaved),
				FString::SanitizeFloat(ExpectedSaved),
				FString::SanitizeFloat(Character->GetHealth()),
				CheckpointLabel);
			Character->ApplyHealthDelta(-Character->GetMaxHealth());
			AssertTrue(Record, FString::Printf(TEXT("%s.entered_death"), CaseId),
				Character->IsPlayerDead(),
				TEXT("true"),
				Character->IsPlayerDead() ? TEXT("true") : TEXT("false"),
				TEXT("player"));
			Owner.SetStage(CaseId);
		}

		void TickDeathWait(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character,
			float DeltaTime,
			float ExpectedHealth,
			const TCHAR* CaseId,
			EProof Next)
		{
			WaitSeconds += DeltaTime;
			if (!Character->IsPlayerDead() && Character->GetHealth() > 0.0f)
			{
				AProjectOrganoidCheckpoint* Checkpoint = Character->GetLastActivatedCheckpoint();
				const float Dist = Checkpoint ? FVector::Dist2D(Character->GetActorLocation(), Checkpoint->GetActorLocation()) : 99999.0f;
				AssertTrue(Record, FString::Printf(TEXT("%s.restarted"), CaseId),
					true, TEXT("alive"), TEXT("alive"), TEXT("player"));
				AssertTrue(Record, FString::Printf(TEXT("%s.anchor"), CaseId),
					Dist < 280.0f, TEXT("<280"), FString::SanitizeFloat(Dist), CheckpointLabel);
				AssertTrue(Record, FString::Printf(TEXT("%s.health"), CaseId),
					HealthIs(Character, ExpectedHealth),
					FString::SanitizeFloat(ExpectedHealth),
					FString::SanitizeFloat(Character->GetHealth()),
					TEXT("save"));
				AssertTrue(Record, FString::Printf(TEXT("%s.not_full_heal"), CaseId),
					!FMath::IsNearlyEqual(Character->GetHealth(), Character->GetMaxHealth(), HealthTolerance)
						|| FMath::IsNearlyEqual(ExpectedHealth, Character->GetMaxHealth(), HealthTolerance),
					TEXT("saved health"),
					FString::SanitizeFloat(Character->GetHealth()),
					TEXT("death"));
				Proof = Next;
				Owner.SetStage(TEXT("Proof"));
				WaitSeconds = 0.0f;
				return;
			}
			if (WaitSeconds > 4.0f)
			{
				FailAndStop(Owner, Record, FString::Printf(TEXT("%s death restart timed out."), CaseId));
			}
		}

		void TickTrauma(
			UProjectOrganoidPlaytestEditorSubsystem& Owner,
			FOrganoidPlaytestRecord& Record,
			AProjectOrganoidCharacter* Character)
		{
			UProjectOrganoidInventoryComponent* Inventory = Character->GetInventoryComponent();
			UProjectOrganoidItemData* Trauma = LoadObject<UProjectOrganoidItemData>(nullptr, TraumaPath);
			AssertTrue(Record, TEXT("trauma.asset"),
				Trauma && FMath::IsNearlyEqual(Trauma->HealAmount, 35.0f),
				TEXT("35"),
				Trauma ? FString::SanitizeFloat(Trauma->HealAmount) : TEXT("missing"),
				TEXT("DA_Item_TraumaStabilizer"));
			if (!Inventory || !Trauma)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}

			FGuid InstanceId;
			const bool bAdded = Inventory->TryAddItem(Trauma, InstanceId, 1);
			AssertTrue(Record, TEXT("trauma.added"),
				bAdded && Inventory->CountItem(Trauma) == 1,
				TEXT("1"),
				FString::FromInt(Inventory->CountItem(Trauma)),
				TEXT("inventory"));
			SetHealthExact(Character, 50.0f);
			const bool bUsed = Character->TryUseFirstHealingConsumable();
			AssertTrue(Record, TEXT("trauma.used"), bUsed, TEXT("true"), bUsed ? TEXT("true") : TEXT("false"), TEXT("H"));
			AssertTrue(Record, TEXT("trauma.plus_35"),
				HealthIs(Character, 85.0f),
				TEXT("85"),
				FString::SanitizeFloat(Character->GetHealth()),
				TEXT("player"));
			AssertTrue(Record, TEXT("trauma.consumed"),
				Inventory->CountItem(Trauma) == 0,
				TEXT("0"),
				FString::FromInt(Inventory->CountItem(Trauma)),
				TEXT("inventory"));

			SetHealthExact(Character, 100.0f);
			Inventory->TryAddItem(Trauma, InstanceId, 1);
			const bool bRefused = Character->TryUseFirstHealingConsumable();
			AssertTrue(Record, TEXT("trauma.full_health_refuse"),
				!bRefused && Inventory->CountItem(Trauma) == 1 && HealthIs(Character, 100.0f),
				TEXT("refuse"),
				bRefused ? TEXT("used") : TEXT("refuse"),
				TEXT("H"));

			if (bAnyAssertFailed)
			{
				FailAndStop(Owner, Record, Record.FailureReason);
				return;
			}
			Proof = EProof::Done;
			Owner.SetStage(TEXT("Done"));
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* EditorWorld = (GEditor && GEditor->PlayWorld == nullptr)
				? GEditor->GetEditorWorldContext().World()
				: (GEditor ? GEditor->GetEditorWorldContext().World() : nullptr);
			const TArray<TPair<const TCHAR*, FVector>> Checkpoints = {
				{TEXT("Checkpoint_ReceptionAtrium"), FVector(-1535.0f, 0.0f, 60.0f)},
				{TEXT("Checkpoint_NeuroAirlock"), FVector(1950.0f, 0.0f, -1140.0f)},
				{TEXT("Checkpoint_FreightAirlock"), FVector(1950.0f, 0.0f, -2340.0f)},
				{TEXT("Checkpoint_InterfaceChamber"), FVector(-2425.0f, -1650.0f, -3540.0f)},
				{TEXT("Checkpoint_BasinRim"), FVector(25.0f, 0.0f, -4740.0f)},
			};
			for (const TPair<const TCHAR*, FVector>& Entry : Checkpoints)
			{
				AProjectOrganoidCheckpoint* Checkpoint = Cast<AProjectOrganoidCheckpoint>(
					OrganoidPlaytestActions::FindUniqueByLabel(EditorWorld, Entry.Key));
				AssertTrue(Record, FString::Printf(TEXT("durable.checkpoint.%s.present"), Entry.Key),
					Checkpoint && FVector::Dist(Checkpoint->GetActorLocation(), Entry.Value) <= 5.0f,
					Entry.Value.ToCompactString(),
					Checkpoint ? Checkpoint->GetActorLocation().ToCompactString() : TEXT("missing"),
					Entry.Key);
				AssertTrue(Record, FString::Printf(TEXT("durable.checkpoint.%s.floor_percent"), Entry.Key),
					FloorIsTwentyFive(Checkpoint),
					TEXT("0.25"),
					Checkpoint ? FString::SanitizeFloat(Checkpoint->HealthStabilizationFloorPercent) : TEXT("missing"),
					Entry.Key);
			}

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
			AssertTrue(
				Record, TEXT("dirty_unchanged"),
				DirtyAfter == DirtyBefore,
				FString::Join(DirtyBefore, TEXT(",")),
				FString::Join(DirtyAfter, TEXT(",")),
				TEXT("packages"));
			Record.AddActor(TEXT("dirty_before"), FString::Join(DirtyBefore, TEXT(",")));
			Record.AddActor(TEXT("dirty_after"), FString::Join(DirtyAfter, TEXT(",")));
			AssertTrue(Record, TEXT("playtest_mutates_assets"), true, TEXT("false"), TEXT("false"), TEXT(""));
			Stage = EStage::Finalize;
			(void)Owner;
		}
	};

	struct FCheckpointHealthAutoRegister
	{
		FCheckpointHealthAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FCheckpointHealthFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FCheckpointHealthAutoRegister AutoRegister;
}
