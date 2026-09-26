#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Editor.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/PlatformTime.h"

#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_NeuralSlow.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"

namespace SyringeDisruptFunctional
{
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR NeuroPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_NeuroGenetics");
	constexpr TCHAR HostLabel[] = TEXT("Host_Neuro_1");

	bool PackageIsDirty(const TCHAR* Path)
	{
		if (UPackage* Package = FindPackage(nullptr, Path))
		{
			return Package->IsDirty();
		}
		return false;
	}

	void PlaceNearHost(AProjectOrganoidCharacter* Character, AProjectOrganoidHostBase* Host, float Distance)
	{
		if (!Character || !Host)
		{
			return;
		}
		Character->SetActorLocation(Host->GetActorLocation() + FVector(Distance, 0.f, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		OrganoidPlaytestActions::FaceActor(Character, Host);
	}

	void FillPE(AProjectOrganoidCharacter* Character, float Target)
	{
		if (!Character)
		{
			return;
		}
		Character->ApplyPEEnergyDelta(-Character->GetMaxPEEnergy());
		Character->ApplyPEEnergyDelta(Target);
	}

	class FSyringeDisruptCase : public IOrganoidPlaytestCase
	{
	public:
		explicit FSyringeDisruptCase(bool bInOptical)
			: bOptical(bInOptical)
		{
		}

		virtual FString GetTestId() const override
		{
			return bOptical ? TEXT("OpticalDisrupt_Functional") : TEXT("LocomotorDisrupt_Functional");
		}
		virtual FString GetDisplayName() const override
		{
			return bOptical ? TEXT("Optical Disrupt Functional") : TEXT("Locomotor Disrupt Functional");
		}
		virtual FString GetMapPackage() const override { return MapPackage; }
		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.f;
			bAnyAssertFailed = false;
			bRequestedNeuroStream = false;
			bActivated = false;
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
			case EStage::Preflight: TickPreflight(Owner, *Record); break;
			case EStage::StartPie:
				if (!Owner.RequestStartPie(MapPackage))
				{
					Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("RequestPlaySession failed."));
					return;
				}
				WaitSeconds = 0.f;
				bRequestedNeuroStream = false;
				Stage = EStage::WaitReady;
				break;
			case EStage::WaitReady: TickWaitReady(Owner, *Record, DeltaTime); break;
			case EStage::Proof: TickProof(Owner, *Record, DeltaTime); break;
			case EStage::EndPie:
				Owner.RequestEndPieIfStarted();
				WaitSeconds = 0.f;
				Stage = EStage::WaitStopped;
				break;
			case EStage::WaitStopped:
				WaitSeconds += DeltaTime;
				if (!GEditor || !GEditor->IsPlaySessionInProgress() || WaitSeconds > 20.f)
				{
					if (PackageIsDirty(NeuroPackage) || PackageIsDirty(MapPackage))
					{
						bAnyAssertFailed = true;
						if (Record->FailureReason.IsEmpty())
						{
							Record->FailureReason = TEXT("A package was dirty after the adaptation test.");
						}
					}
					Owner.CompleteActive(bAnyAssertFailed ? EOrganoidPlaytestState::Fail : EOrganoidPlaytestState::Pass, Record->FailureReason);
				}
				break;
			}
		}

	private:
		enum class EStage : uint8 { Preflight, StartPie, WaitReady, Proof, EndPie, WaitStopped };

		void FailAndStop(FOrganoidPlaytestRecord& Record, const FString& Reason)
		{
			if (Record.FailureReason.IsEmpty())
			{
				Record.FailureReason = Reason;
			}
			bAnyAssertFailed = true;
			Stage = EStage::EndPie;
		}

		bool AssertTrue(FOrganoidPlaytestRecord& Record, const FString& Id, bool bPassed, const FString& Expected, const FString& Actual)
		{
			Record.AddAssertion(Id, bPassed, Expected, Actual, GetTestId(), false);
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

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UProjectOrganoidBiologicalAdaptation_NeuralSlow* Neural = LoadObject<UProjectOrganoidBiologicalAdaptation_NeuralSlow>(nullptr, UProjectOrganoidBiologicalAdaptation_NeuralSlow::ContentPath());
			if (bOptical)
			{
				UProjectOrganoidBiologicalAdaptation_OpticalDisrupt* Optical = LoadObject<UProjectOrganoidBiologicalAdaptation_OpticalDisrupt>(nullptr, UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::ContentPath());
				AssertTrue(Record, TEXT("asset.id"), Optical && Optical->AdaptationId == TEXT("Adaptation_OpticalDisrupt"), TEXT("Adaptation_OpticalDisrupt"), Optical ? Optical->AdaptationId.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.title"), Optical && Optical->DisplayName.ToString() == TEXT("Optical Disrupt"), TEXT("Optical Disrupt"), Optical ? Optical->DisplayName.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.description"), Optical && Optical->EffectDescription.ToString().Contains(TEXT("optical nodes")), TEXT("optical nodes"), Optical ? Optical->EffectDescription.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.pe_20"), Optical && FMath::IsNearlyEqual(Optical->PECost, 20.f), TEXT("20"), Optical ? FString::SanitizeFloat(Optical->PECost) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.cooldown_8"), Optical && FMath::IsNearlyEqual(Optical->CooldownSeconds, 8.f), TEXT("8"), Optical ? FString::SanitizeFloat(Optical->CooldownSeconds) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.range_800"), Optical && FMath::IsNearlyEqual(Optical->MaxTargetRange, 800.f), TEXT("800"), Optical ? FString::SanitizeFloat(Optical->MaxTargetRange) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.duration_5"), Optical && FMath::IsNearlyEqual(Optical->DurationSeconds, 5.f), TEXT("5"), Optical ? FString::SanitizeFloat(Optical->DurationSeconds) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.distinct_from_neural_slow"), Optical && Neural && Optical->GetClass() != Neural->GetClass() && !FMath::IsNearlyEqual(Neural->DurationSeconds, Optical->DurationSeconds), TEXT("distinct"), Neural ? FString::SanitizeFloat(Neural->DurationSeconds) : TEXT("missing"));
			}
			else
			{
				UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt* Locomotor = LoadObject<UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt>(nullptr, UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::ContentPath());
				AssertTrue(Record, TEXT("asset.id"), Locomotor && Locomotor->AdaptationId == TEXT("Adaptation_LocomotorDisrupt"), TEXT("Adaptation_LocomotorDisrupt"), Locomotor ? Locomotor->AdaptationId.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.title"), Locomotor && Locomotor->DisplayName.ToString() == TEXT("Locomotor Disrupt"), TEXT("Locomotor Disrupt"), Locomotor ? Locomotor->DisplayName.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.description"), Locomotor && Locomotor->EffectDescription.ToString().Contains(TEXT("locomotor nerve")), TEXT("locomotor nerve"), Locomotor ? Locomotor->EffectDescription.ToString() : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.pe_20"), Locomotor && FMath::IsNearlyEqual(Locomotor->PECost, 20.f), TEXT("20"), Locomotor ? FString::SanitizeFloat(Locomotor->PECost) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.cooldown_8"), Locomotor && FMath::IsNearlyEqual(Locomotor->CooldownSeconds, 8.f), TEXT("8"), Locomotor ? FString::SanitizeFloat(Locomotor->CooldownSeconds) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.range_800"), Locomotor && FMath::IsNearlyEqual(Locomotor->MaxTargetRange, 800.f), TEXT("800"), Locomotor ? FString::SanitizeFloat(Locomotor->MaxTargetRange) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.duration_5"), Locomotor && FMath::IsNearlyEqual(Locomotor->DurationSeconds, 5.f), TEXT("5"), Locomotor ? FString::SanitizeFloat(Locomotor->DurationSeconds) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.speed_50"), Locomotor && FMath::IsNearlyEqual(Locomotor->LocomotorSpeedMultiplier, 0.5f), TEXT("0.5"), Locomotor ? FString::SanitizeFloat(Locomotor->LocomotorSpeedMultiplier) : TEXT("missing"));
				AssertTrue(Record, TEXT("asset.distinct_from_neural_slow"), Locomotor && Neural && !FMath::IsNearlyEqual(Locomotor->LocomotorSpeedMultiplier, Neural->LocomotorSpeedMultiplier) && !FMath::IsNearlyEqual(Locomotor->DurationSeconds, Neural->DurationSeconds), TEXT("distinct"), Neural ? FString::SanitizeFloat(Neural->LocomotorSpeedMultiplier) : TEXT("missing"));
			}
			if (bAnyAssertFailed)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, Record.FailureReason);
				return;
			}
			Stage = EStage::StartPie;
		}

		void TickWaitReady(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			(void)Owner;
			WaitSeconds += DeltaTime;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			UProjectOrganoidLevelManagerSubsystem* Levels = World ? World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>() : nullptr;
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
			AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, HostLabel));
			if (Character && Host && bRequestedNeuroStream)
			{
				Stage = EStage::Proof;
				WaitSeconds = 0.f;
				return;
			}
			if (WaitSeconds > 30.f)
			{
				FailAndStop(Record, TEXT("PIE did not become ready with Host_Neuro_1."));
			}
		}

		void TickProof(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			(void)Owner;
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerCharacter(World));
			AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(OrganoidPlaytestActions::FindUniqueByLabel(World, HostLabel));
			UProjectOrganoidBiologicalAdaptationComponent* Adapt = Character ? Character->GetBiologicalAdaptationComponent() : nullptr;
			if (!Character || !Host || !Adapt)
			{
				FailAndStop(Record, TEXT("Character, host, or adaptation component missing."));
				return;
			}
			if (!bActivated)
			{
				if (!bAimStarted)
				{
					if (AProjectOrganoidHostAIController* AI = Cast<AProjectOrganoidHostAIController>(Host->GetController()))
					{
						AI->ApplyCombatState(EProjectOrganoidHostCombatState::Idle);
					}
					Host->ClearStatusEffects();
					Host->bLocomotorNervesDestroyed = false;
					Host->bOpticalNodesDestroyed = false;
					Host->bIsDead = false;
					Host->bIsIncapacitated = false;
					if (USpringArmComponent* Boom = Character->GetCameraBoom())
					{
						Boom->bEnableCameraLag = false;
					}
					UProjectOrganoidBiologicalAdaptationData* Data = bOptical
						? static_cast<UProjectOrganoidBiologicalAdaptationData*>(UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve())
						: static_cast<UProjectOrganoidBiologicalAdaptationData*>(UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve());
					if (!Data || !Adapt->UnlockAdaptation(Data) || !Adapt->EquipAdaptation(Data))
					{
						FailAndStop(Record, TEXT("Could not unlock and equip the syringe adaptation."));
						return;
					}
					FillPE(Character, 100.f);
					PlaceNearHost(Character, Host, 250.f);
					bAimStarted = true;
					WaitSeconds = 0.f;
					return;
				}
				// GetPlayerViewPoint uses the camera cache. Same-tick teleport still sees the previous view.
				WaitSeconds += DeltaTime;
				if (WaitSeconds < 0.35f)
				{
					PlaceNearHost(Character, Host, 250.f);
					return;
				}
				BaselineSpeed = Host->GetCharacterMovement() ? Host->GetCharacterMovement()->MaxWalkSpeed : 0.f;
				HealthBefore = Host->Health;
				const float PEBefore = Character->GetPEEnergy();
				const bool bDidActivate = Adapt->TryActivateEquipped();
				const float Speed = Host->GetCharacterMovement() ? Host->GetCharacterMovement()->MaxWalkSpeed : 0.f;
				const FString ActivationActual = bDidActivate
					? TEXT("true")
					: FString::Printf(TEXT("reason=%d dist=%.0f dead=%d"), static_cast<int32>(Adapt->GetLastFailReason()), FVector::Dist(Character->GetActorLocation(), Host->GetActorLocation()), Host->bIsDead ? 1 : 0);
				AssertTrue(Record, TEXT("valid_activation"), bDidActivate, TEXT("true"), ActivationActual);
				AssertTrue(Record, TEXT("consumes_20_pe"), FMath::IsNearlyEqual(Character->GetPEEnergy(), PEBefore - 20.f, 0.05f), TEXT("20"), FString::SanitizeFloat(PEBefore - Character->GetPEEnergy()));
				AssertTrue(Record, TEXT("zero_direct_damage"), FMath::IsNearlyEqual(Host->Health, HealthBefore), FString::SanitizeFloat(HealthBefore), FString::SanitizeFloat(Host->Health));
				AssertTrue(Record, TEXT("cooldown_rejects"), !Adapt->TryActivateEquipped() && Adapt->GetLastFailReason() == EProjectOrganoidBiologicalAdaptationFailReason::Cooldown, TEXT("Cooldown"), TEXT("repeat"));
				if (bOptical)
				{
					AssertTrue(Record, TEXT("impairs_vision"), Host->bIsBlinded && Host->IsBiologicalOpticalBlindActive() && !Host->bIsStaggered && !Host->bOpticalNodesDestroyed, TEXT("blind"), Host->bIsBlinded ? TEXT("blind") : TEXT("sighted"));
					AssertTrue(Record, TEXT("does_not_slow"), !Host->IsBiologicalLocomotorSlowActive() && FMath::Abs(Speed - BaselineSpeed) <= 8.f, FString::SanitizeFloat(BaselineSpeed), FString::SanitizeFloat(Speed));
				}
				else
				{
					const float Expected = BaselineSpeed * 0.5f;
					AssertTrue(Record, TEXT("impairs_movement"), Host->IsBiologicalLocomotorSlowActive() && FMath::IsNearlyEqual(Host->GetBiologicalLocomotorSlowMultiplier(), 0.5f) && FMath::Abs(Speed - Expected) <= 8.f, FString::SanitizeFloat(Expected), FString::SanitizeFloat(Speed));
					AssertTrue(Record, TEXT("does_not_blind"), !Host->bIsBlinded && !Host->bLocomotorNervesDestroyed, TEXT("sighted"), Host->bIsBlinded ? TEXT("blind") : TEXT("sighted"));
				}
				if (bAnyAssertFailed)
				{
					Stage = EStage::EndPie;
					return;
				}
				bActivated = true;
				ActivationRealTime = FPlatformTime::Seconds();
				return;
			}
			if (FPlatformTime::Seconds() - ActivationRealTime < 5.6)
			{
				return;
			}
			const float Speed = Host->GetCharacterMovement() ? Host->GetCharacterMovement()->MaxWalkSpeed : 0.f;
			if (bOptical)
			{
				AssertTrue(Record, TEXT("vision_restored"), !Host->bIsBlinded && !Host->IsBiologicalOpticalBlindActive() && !Host->bOpticalNodesDestroyed, TEXT("sighted"), Host->bIsBlinded ? TEXT("blind") : TEXT("sighted"));
			}
			else
			{
				AssertTrue(Record, TEXT("movement_restored"), !Host->IsBiologicalLocomotorSlowActive() && FMath::Abs(Speed - BaselineSpeed) <= 8.f, FString::SanitizeFloat(BaselineSpeed), FString::SanitizeFloat(Speed));
			}
			AssertTrue(Record, TEXT("still_no_damage"), FMath::IsNearlyEqual(Host->Health, HealthBefore), FString::SanitizeFloat(HealthBefore), FString::SanitizeFloat(Host->Health));
			Stage = EStage::EndPie;
		}

		bool bOptical = false;
		EStage Stage = EStage::Preflight;
		float WaitSeconds = 0.f;
		bool bAnyAssertFailed = false;
		bool bRequestedNeuroStream = false;
		bool bAimStarted = false;
		bool bActivated = false;
		double ActivationRealTime = 0.0;
		float BaselineSpeed = 0.f;
		float HealthBefore = 0.f;
	};

	struct FRegister
	{
		FRegister()
		{
			FOrganoidPlaytestCatalogEntry Locomotor;
			Locomotor.TestId = TEXT("LocomotorDisrupt_Functional");
			Locomotor.DisplayName = TEXT("Locomotor Disrupt Functional");
			Locomotor.MapPackage = MapPackage;
			Locomotor.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FSyringeDisruptCase>(false); };
			FOrganoidPlaytestRegistry::Register(Locomotor);

			FOrganoidPlaytestCatalogEntry Optical;
			Optical.TestId = TEXT("OpticalDisrupt_Functional");
			Optical.DisplayName = TEXT("Optical Disrupt Functional");
			Optical.MapPackage = MapPackage;
			Optical.Factory = []() -> TSharedRef<IOrganoidPlaytestCase> { return MakeShared<FSyringeDisruptCase>(true); };
			FOrganoidPlaytestRegistry::Register(Optical);
		}
	};
	static FRegister RegisterSyringeDisruptFunctional;
}
