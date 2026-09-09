#include "ProjectOrganoidPlaytestRegistry.h"
#include "ProjectOrganoidPlaytestEditorSubsystem.h"
#include "ProjectOrganoidPlaytestActions.h"
#include "ProjectOrganoidPlaytestReport.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidInteractionTypes.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
	constexpr TCHAR TestId[] = TEXT("DeconAudioIsolation_Functional");
	constexpr TCHAR DisplayName[] = TEXT("Decon Audio Mix/Content A-F Proof");
	constexpr TCHAR MapPackage[] = TEXT("/Game/Maps/Lvl_Epitope");
	constexpr TCHAR AdminPackage[] = TEXT("/Game/Maps/Epitope/SL_Epitope_Admin");
	constexpr TCHAR HazardLabel[] = TEXT("Hazard_DeconUVC");
	constexpr float OutsideWaitSeconds = 1.2f;
	constexpr float EnterObserveSeconds = 3.0f;
	constexpr float StayCSeconds = 4.0f;
	constexpr float CrossObserveSeconds = 0.9f;
	constexpr float ClearSettleSeconds = 2.4f;
	constexpr float HostileDamage = 8.0f;
	constexpr float CombatVolumeQuiet = 0.12f;
	constexpr float SilentVolume = 0.05f;

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
		for (UPackage* Package : WorldDirty)
		{
			if (Package)
			{
				Out.AddUnique(Package->GetName());
			}
		}
		for (UPackage* Package : ContentDirty)
		{
			if (Package)
			{
				Out.AddUnique(Package->GetName());
			}
		}
		Out.Sort();
	}

	class FDeconAudioIsolationFunctional : public IOrganoidPlaytestCase
	{
	public:
		virtual FString GetTestId() const override { return TestId; }
		virtual FString GetDisplayName() const override { return DisplayName; }
		virtual FString GetMapPackage() const override { return MapPackage; }

		virtual void Start(UProjectOrganoidPlaytestEditorSubsystem& Owner) override
		{
			Stage = EStage::Preflight;
			WaitSeconds = 0.0f;
			CaptureIndex = 0;
			CrossIndex = 0;
			bAnyAssertFailed = false;
			HealthAtEntry = 100.0f;
			HealthAfterB = 100.0f;
			Traces.Reset();
			DirtyBefore.Reset();
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
			case EStage::Setup: TickSetup(Owner, *Record); break;
			case EStage::OutsideA: TickOutsideA(Owner, *Record, DeltaTime); break;
			case EStage::EnterB: TickEnter(Owner, *Record, TEXT("B")); break;
			case EStage::ObserveB: TickObserveB(Owner, *Record, DeltaTime); break;
			case EStage::StayC: TickStayC(Owner, *Record, DeltaTime); break;
			case EStage::CrossD: TickCrossD(Owner, *Record, DeltaTime); break;
			case EStage::DrainE: TickDrainE(Owner, *Record, DeltaTime); break;
			case EStage::RestoreF: TickRestoreF(Owner, *Record, DeltaTime); break;
			case EStage::HostileF: TickHostileF(Owner, *Record); break;
			case EStage::ObserveF: TickObserveF(Owner, *Record, DeltaTime); break;
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
			Preflight, StartPie, WaitPieReady, Setup, OutsideA, EnterB, ObserveB, StayC, CrossD,
			DrainE, RestoreF, HostileF, ObserveF, EndPie, WaitPieStopped, AssertDurable, Finalize
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
				if (Record.FailureReason.IsEmpty())
				{
					Record.FailureReason = FString::Printf(TEXT("%s expected=%s actual=%s"), *Id, *Expected, *Actual);
				}
			}
			return bPassed;
		}

		AProjectOrganoidCharacter* GetOrganoid(UWorld* World) const
		{
			return Cast<AProjectOrganoidCharacter>(OrganoidPlaytestActions::GetPlayerPawn(World));
		}

		UAudioComponent* FindComp(UWorld* World, const TCHAR* Name) const
		{
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (Component && Component->GetWorld() == World && Component->GetName().Contains(Name))
				{
					return Component;
				}
			}
			return nullptr;
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

		void PlacePawn(APawn* Pawn, const FVector& XY)
		{
			if (!Pawn)
			{
				return;
			}
			const float CapsuleZ = CapsuleZFor(Pawn);
			if (ACharacter* AsCharacter = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = AsCharacter->GetCharacterMovement())
				{
					Move->SetMovementMode(MOVE_Walking);
					Move->StopMovementImmediately();
				}
			}
			OrganoidPlaytestActions::TeleportNear(Pawn, FVector(XY.X, XY.Y, CapsuleZ), 0.0f, CapsuleZ);
			Pawn->UpdateOverlaps();
		}

		bool CallSetCombatActive(UWorld* World, bool bActive)
		{
			UProjectOrganoidAudioAmbienceSubsystem* Sub = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Sub)
			{
				return false;
			}
			UFunction* Function = Sub->FindFunction(FName(TEXT("SetCombatActive")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Function, TEXT("bActive")))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), bActive);
			}
			Sub->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		bool CallApplyHealthDelta(AProjectOrganoidCharacter* Character, float Delta, EProjectOrganoidHealthDeltaSource Source)
		{
			if (!Character)
			{
				return false;
			}
			UFunction* Function = Character->FindFunction(FName(TEXT("ApplyHealthDelta")));
			if (!Function)
			{
				return false;
			}
			TArray<uint8> Parms;
			Parms.AddZeroed(Function->ParmsSize);
			if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Function, TEXT("Delta")))
			{
				Prop->SetPropertyValue(Prop->ContainerPtrToValuePtr<void>(Parms.GetData()), Delta);
			}
			if (FEnumProperty* EnumProp = FindFProperty<FEnumProperty>(Function, TEXT("Source")))
			{
				EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(
					EnumProp->ContainerPtrToValuePtr<void>(Parms.GetData()),
					static_cast<int64>(Source));
			}
			Character->ProcessEvent(Function, Parms.GetData());
			return true;
		}

		FString DumpPlaying(UWorld* World) const
		{
			TArray<FString> Playing;
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				AActor* Owner = Component->GetOwner();
				Playing.Add(FString::Printf(
					TEXT("%s/%s/%s vol=%.3f pitch=%.3f loop=%s dur=%.2f"),
					Owner ? *OrganoidPlaytestActions::ActorLabel(Owner) : TEXT("none"),
					*Component->GetName(),
					Sound ? *Sound->GetName() : TEXT("none"),
					Component->VolumeMultiplier,
					Component->PitchMultiplier,
					(Sound && Sound->IsLooping()) ? TEXT("true") : TEXT("false"),
					Sound ? Sound->GetDuration() : -1.0f));
			}
			return Playing.Num() ? FString::Join(Playing, TEXT(" | ")) : TEXT("none");
		}

		FString CompLine(UWorld* World, const TCHAR* Name) const
		{
			UAudioComponent* Component = FindComp(World, Name);
			if (!Component)
			{
				return FString::Printf(TEXT("%s=missing"), Name);
			}
			USoundBase* Sound = Component->GetSound();
			return FString::Printf(
				TEXT("%s playing=%s sound=%s vol=%.3f pitch=%.3f dur=%.2f"),
				Name,
				Component->IsPlaying() ? TEXT("true") : TEXT("false"),
				Sound ? *Sound->GetName() : TEXT("none"),
				Component->VolumeMultiplier,
				Component->PitchMultiplier,
				Sound ? Sound->GetDuration() : -1.0f);
		}

		FString Snapshot(UWorld* World, const TCHAR* Label) const
		{
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			const FVector Loc = Character ? Character->GetActorLocation() : FVector::ZeroVector;
			const bool bCriticalHealth = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth;
			return FString::Printf(
				TEXT("%.3f %s loc=(%.0f,%.0f) health=%.1f healthN=%.3f combat=%s hazard=%s criticalHealth=%s state=%s ambVol=%.3f tenVol=%.3f combatVol=%.3f critVol=%.3f | %s | %s | %s | playing=%s"),
				World ? World->GetTimeSeconds() : -1.0f,
				Label,
				Loc.X, Loc.Y,
				Character ? Character->GetHealth() : -1.0f,
				Ambience ? Ambience->GetHealthNormalized() : -1.0f,
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"),
				(Ambience && Ambience->IsInHazard()) ? TEXT("true") : TEXT("false"),
				bCriticalHealth ? TEXT("true") : TEXT("false"),
				Ambience ? *UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"),
				Ambience ? Ambience->GetAmbientLayerVolume() : -1.0f,
				Ambience ? Ambience->GetTensionLayerVolume() : -1.0f,
				Ambience ? Ambience->GetCombatLayerVolume() : -1.0f,
				Ambience ? Ambience->GetCriticalLayerVolume() : -1.0f,
				*CompLine(World, TEXT("HazardAudio")),
				*CompLine(World, TEXT("OrganoidCriticalLayer")),
				*CompLine(World, TEXT("OrganoidCombatLayer")),
				*DumpPlaying(World));
		}

		bool HissPlaying(UWorld* World) const
		{
			UAudioComponent* Component = FindComp(World, TEXT("HazardAudio"));
			USoundBase* Sound = Component ? Component->GetSound() : nullptr;
			return Component && Component->IsPlaying() && Sound && Sound->GetName().Contains(TEXT("HazardHiss")) && Component->VolumeMultiplier > 0.02f;
		}

		bool AlarmPulsePlaying(UWorld* World) const
		{
			for (TObjectIterator<UAudioComponent> It; It; ++It)
			{
				UAudioComponent* Component = *It;
				if (!Component || Component->GetWorld() != World || !Component->IsPlaying())
				{
					continue;
				}
				USoundBase* Sound = Component->GetSound();
				if (Sound && Sound->GetName().Contains(TEXT("AlarmPulse")) && Component->VolumeMultiplier > 0.02f)
				{
					return true;
				}
			}
			return false;
		}

		bool CriticalLayerSilent(UWorld* World) const
		{
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			if (!Ambience || Ambience->GetCriticalLayerVolume() > SilentVolume)
			{
				return false;
			}
			UAudioComponent* Component = FindComp(World, TEXT("OrganoidCriticalLayer"));
			if (Component && Component->IsPlaying() && Component->VolumeMultiplier > 0.02f)
			{
				return false;
			}
			return true;
		}

		bool CombatLayerPlaying(UWorld* World) const
		{
			UAudioComponent* Component = FindComp(World, TEXT("OrganoidCombatLayer"));
			return Component && Component->IsPlaying() && Component->VolumeMultiplier > 0.02f;
		}

		float HissDuration(UWorld* World) const
		{
			UAudioComponent* Component = FindComp(World, TEXT("HazardAudio"));
			USoundBase* Sound = Component ? Component->GetSound() : nullptr;
			return Sound ? Sound->GetDuration() : -1.0f;
		}

		void TickPreflight(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			Owner.SetStage(TEXT("Preflight"));
			if (!GEditor || GEditor->IsPlaySessionInProgress())
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Editor unavailable or PIE already running."));
				return;
			}
			if (PackageIsDirty(MapPackage) || PackageIsDirty(AdminPackage))
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Blocked, TEXT("Lvl_Epitope or SL_Epitope_Admin is dirty."));
				return;
			}
			CollectDirtyPackageNames(DirtyBefore);
			Record.AddActor(TEXT("durable.playtest_mutates_assets"), TEXT("false"));
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
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (World && Pawn)
			{
				CallSetCombatActive(World, false);
				WaitSeconds = 0.0f;
				Stage = EStage::Setup;
				return;
			}
			if (WaitSeconds > 45.0f)
			{
				Owner.CompleteActive(EOrganoidPlaytestState::Fail, TEXT("Timed out waiting for PIE pawn."));
			}
			(void)Record;
		}

		void TickSetup(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AActor* Hazard = OrganoidPlaytestActions::FindUniqueByLabel(World, HazardLabel);
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (!Hazard || !Pawn)
			{
				Record.FailureReason = TEXT("Missing Hazard_DeconUVC or pawn.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			FVector Origin;
			FVector Extent;
			Hazard->GetActorBounds(true, Origin, Extent);
			InsideXY = FVector(Origin.X, Origin.Y, 0.0f);
			OutsideXY = FVector(Origin.X, (Origin - Extent).Y - 200.0f, 0.0f);
			PlacePawn(Pawn, OutsideXY);
			CallSetCombatActive(World, false);
			WaitSeconds = 0.0f;
			Stage = EStage::OutsideA;
			Owner.SetStage(TEXT("A_Outside"));
			(void)Record;
		}

		void TickOutsideA(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			WaitSeconds += DeltaTime;
			if (WaitSeconds < OutsideWaitSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("A.outside"), Snapshot(World, TEXT("outside")));
			AssertTrue(Record, TEXT("A.critical_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("A.hazard_audio_silent"), !HissPlaying(World), TEXT("false"),
				HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			AssertTrue(Record, TEXT("A.no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::EnterB;
			Owner.SetStage(TEXT("B_Enter"));
		}

		void TickEnter(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, const TCHAR* Label)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			HealthAtEntry = Character ? Character->GetHealth() : -1.0f;
			PlacePawn(Pawn, InsideXY);
			Record.AddActor(TEXT("t.entry"), Snapshot(World, TEXT("begin_overlap")));
			WaitSeconds = 0.0f;
			Stage = EStage::ObserveB;
			Owner.SetStage(TEXT("B_Observe"));
			(void)Label;
		}

		void TickObserveB(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			WaitSeconds += DeltaTime;
			static const float Marks[] = { 0.25f, 1.0f, 3.0f };
			static const TCHAR* Names[] = { TEXT("t.plus0_25s"), TEXT("t.plus1s"), TEXT("t.plus3s") };
			if (CaptureIndex < 3 && WaitSeconds + 0.02f >= Marks[CaptureIndex])
			{
				Record.AddActor(Names[CaptureIndex], Snapshot(World, Names[CaptureIndex]));
				++CaptureIndex;
			}
			if (WaitSeconds < EnterObserveSeconds)
			{
				return;
			}

			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			HealthAfterB = Character ? Character->GetHealth() : -1.0f;
			const bool bCombat = Ambience && Ambience->IsInCombat();
			const bool bHazard = Ambience && Ambience->IsInHazard();
			const bool bHazardState = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::Hazard;
			const bool bCriticalHealth = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth;
			const float Dps = (HealthAtEntry >= 0.0f && HealthAfterB >= 0.0f)
				? (HealthAtEntry - HealthAfterB) / EnterObserveSeconds
				: -1.0f;
			const float Dur = HissDuration(World);

			Record.AddActor(TEXT("B.inside"), Snapshot(World, TEXT("inside")));
			Record.AddActor(TEXT("B.hiss_duration"), FString::Printf(TEXT("%.2f"), Dur));

			AssertTrue(Record, TEXT("B.hazard_state"), bHazard && bHazardState, TEXT("Hazard"),
				Ambience ? UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("B.uvc_dps"), FMath::Abs(Dps - 6.0f) <= 1.5f, TEXT("6"),
				FString::Printf(TEXT("%.2f"), Dps), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("B.hiss_playing"), HissPlaying(World), TEXT("true"),
				HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			AssertTrue(Record, TEXT("B.hiss_not_old_3s"), Dur < 0.0f || Dur > 7.0f, TEXT(">=8s or inherited"),
				FString::Printf(TEXT("%.2f"), Dur), TEXT("SW_HazardHiss"), false);
			AssertTrue(Record, TEXT("B.critical_health_false"), !bCriticalHealth, TEXT("false"),
				bCriticalHealth ? TEXT("true") : TEXT("false"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("B.critical_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("B.no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			AssertTrue(Record, TEXT("B.combat_false"), !bCombat, TEXT("false"), bCombat ? TEXT("true") : TEXT("false"), TEXT("combat"), false);

			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::StayC;
			Owner.SetStage(TEXT("C_Stay"));
		}

		void TickStayC(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			WaitSeconds += DeltaTime;
			if (WaitSeconds < StayCSeconds)
			{
				return;
			}
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			const float Health = Character ? Character->GetHealth() : -1.0f;
			const float MaxHealth = Character ? Character->GetMaxHealth() : 100.0f;
			Record.AddActor(TEXT("C.stay"), Snapshot(World, TEXT("stay")));
			AssertTrue(Record, TEXT("C.above_30"), Health > MaxHealth * 0.30f, TEXT(">30%"),
				FString::Printf(TEXT("%.1f"), Health), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("C.no_alarmpulse"), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);
			AssertTrue(Record, TEXT("C.critical_silent"), CriticalLayerSilent(World), TEXT("true"),
				CriticalLayerSilent(World) ? TEXT("true") : TEXT("false"), TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("C.hiss_playing"), HissPlaying(World), TEXT("true"),
				HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			AssertTrue(Record, TEXT("C.combat_false"), Ambience && !Ambience->IsInCombat(), TEXT("false"),
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			CrossIndex = 0;
			Stage = EStage::CrossD;
			Owner.SetStage(TEXT("D_Cross"));
		}

		void TickCrossD(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (WaitSeconds <= 0.0f && Pawn)
			{
				const bool bInside = (CrossIndex % 2) == 1;
				PlacePawn(Pawn, bInside ? InsideXY : OutsideXY);
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < CrossObserveSeconds)
			{
				return;
			}

			const bool bInside = (CrossIndex % 2) == 1;
			const FString Label = bInside ? FString::Printf(TEXT("D.in_%d"), CrossIndex) : FString::Printf(TEXT("D.out_%d"), CrossIndex);
			Record.AddActor(Label, Snapshot(World, *Label));
			if (bInside)
			{
				AssertTrue(Record, FString::Printf(TEXT("D.hiss_on_%d"), CrossIndex), HissPlaying(World), TEXT("true"),
					HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			}
			else
			{
				AssertTrue(Record, FString::Printf(TEXT("D.hiss_off_%d"), CrossIndex), !HissPlaying(World), TEXT("false"),
					HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			}
			AssertTrue(Record, FString::Printf(TEXT("D.no_alarmpulse_%d"), CrossIndex), !AlarmPulsePlaying(World), TEXT("false"),
				AlarmPulsePlaying(World) ? TEXT("true") : TEXT("false"), TEXT("SW_AlarmPulse"), false);

			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}

			++CrossIndex;
			WaitSeconds = 0.0f;
			if (CrossIndex >= 4)
			{
				PlacePawn(Pawn, InsideXY);
				Stage = EStage::DrainE;
				Owner.SetStage(TEXT("E_Drain"));
				return;
			}
			Owner.SetStage(FString::Printf(TEXT("D_Cross_%d"), CrossIndex));
		}

		void TickDrainE(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			WaitSeconds += DeltaTime;
			const float Health = Character ? Character->GetHealth() : -1.0f;
			const float MaxHealth = Character ? Character->GetMaxHealth() : 100.0f;
			const bool bCriticalHp = Health >= 0.0f && Health <= MaxHealth * 0.30f + 0.05f;
			if (!bCriticalHp && WaitSeconds < 22.0f)
			{
				if (Ambience && Ambience->IsInCombat())
				{
					Record.FailureReason = TEXT("Combat rose from UVC drain before CriticalHealth.");
					bAnyAssertFailed = true;
					Stage = EStage::EndPie;
				}
				return;
			}

			Record.AddActor(TEXT("E.after"), Snapshot(World, TEXT("critical")));
			const bool bCriticalState = Ambience && Ambience->GetAmbienceState() == EProjectOrganoidAmbienceState::CriticalHealth;
			AssertTrue(Record, TEXT("E.reached_critical_hp"), bCriticalHp, TEXT("<=30%"),
				FString::Printf(TEXT("%.1f"), Health), TEXT("vitals"), false);
			AssertTrue(Record, TEXT("E.critical_state"), bCriticalState, TEXT("CriticalHealth"),
				Ambience ? UEnum::GetValueAsString(Ambience->GetAmbienceState()) : TEXT("none"), TEXT("ambience"), false);
			AssertTrue(Record, TEXT("E.critical_layer_audible"), Ambience && Ambience->GetCriticalLayerVolume() >= SilentVolume,
				TEXT(">=0.05"), FString::Printf(TEXT("%.3f"), Ambience ? Ambience->GetCriticalLayerVolume() : -1.0f),
				TEXT("OrganoidCriticalLayer"), false);
			AssertTrue(Record, TEXT("E.hiss_still_playing"), HissPlaying(World), TEXT("true"),
				HissPlaying(World) ? TEXT("true") : TEXT("false"), TEXT("HazardAudio"), false);
			AssertTrue(Record, TEXT("E.no_combat_from_hazard"), Ambience && !Ambience->IsInCombat(), TEXT("false"),
				(Ambience && Ambience->IsInCombat()) ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			if (bAnyAssertFailed)
			{
				Stage = EStage::EndPie;
				return;
			}
			WaitSeconds = 0.0f;
			Stage = EStage::RestoreF;
			Owner.SetStage(TEXT("F_Restore"));
		}

		void TickRestoreF(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			APawn* Pawn = OrganoidPlaytestActions::GetPlayerPawn(World);
			if (WaitSeconds <= 0.0f && Character)
			{
				PlacePawn(Pawn, OutsideXY);
				const float Missing = Character->GetMaxHealth() - Character->GetHealth();
				if (Missing > KINDA_SMALL_NUMBER)
				{
					CallApplyHealthDelta(Character, Missing, EProjectOrganoidHealthDeltaSource::Generic);
				}
				CallSetCombatActive(World, false);
			}
			WaitSeconds += DeltaTime;
			if (WaitSeconds < ClearSettleSeconds)
			{
				return;
			}
			Record.AddActor(TEXT("F.before"), Snapshot(World, TEXT("restored")));
			WaitSeconds = 0.0f;
			Stage = EStage::HostileF;
			Owner.SetStage(TEXT("F_Hostile"));
			(void)Record;
		}

		void TickHostileF(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			AProjectOrganoidCharacter* Character = GetOrganoid(World);
			if (!World || !Character)
			{
				Record.FailureReason = TEXT("Lost PIE pawn during hostile damage.");
				bAnyAssertFailed = true;
				Stage = EStage::EndPie;
				return;
			}
			CallApplyHealthDelta(Character, -HostileDamage, EProjectOrganoidHealthDeltaSource::Generic);
			WaitSeconds = 0.0f;
			Stage = EStage::ObserveF;
			Owner.SetStage(TEXT("F_Observe"));
			(void)Record;
		}

		void TickObserveF(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record, float DeltaTime)
		{
			UWorld* World = OrganoidPlaytestActions::GetPieWorld();
			WaitSeconds += DeltaTime;
			if (WaitSeconds < 0.85f)
			{
				return;
			}
			UProjectOrganoidAudioAmbienceSubsystem* Ambience = World ? World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>() : nullptr;
			Record.AddActor(TEXT("F.after"), Snapshot(World, TEXT("hostile")));
			const bool bCombat = Ambience && Ambience->IsInCombat();
			const bool bPulse = AlarmPulsePlaying(World) || CombatLayerPlaying(World)
				|| (Ambience && Ambience->GetCombatLayerVolume() > CombatVolumeQuiet);
			AssertTrue(Record, TEXT("F.hostile_raises_combat"), bCombat, TEXT("true"),
				bCombat ? TEXT("true") : TEXT("false"), TEXT("combat"), false);
			AssertTrue(Record, TEXT("F.combat_alarmpulse"), bPulse, TEXT("true"),
				bPulse ? TEXT("true") : TEXT("false"), TEXT("OrganoidCombatLayer"), false);
			WaitSeconds = 0.0f;
			Stage = EStage::EndPie;
			(void)Owner;
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
		}

		void TickAssertDurable(UProjectOrganoidPlaytestEditorSubsystem& Owner, FOrganoidPlaytestRecord& Record)
		{
			TArray<FString> DirtyAfter;
			CollectDirtyPackageNames(DirtyAfter);
			AssertTrue(Record, TEXT("durable.no_new_dirty_packages"),
				DirtyAfter.Num() == DirtyBefore.Num(),
				FString::FromInt(DirtyBefore.Num()), FString::FromInt(DirtyAfter.Num()), TEXT("packages"), true);
			AssertTrue(Record, TEXT("durable.playtest_mutates_assets"),
				true, TEXT("false"), TEXT("false"), TEXT(""), true);
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
		float HealthAtEntry = 100.0f;
		float HealthAfterB = 100.0f;
		int32 CaptureIndex = 0;
		int32 CrossIndex = 0;
		bool bAnyAssertFailed = false;
		FVector InsideXY = FVector::ZeroVector;
		FVector OutsideXY = FVector::ZeroVector;
		TArray<FString> Traces;
		TArray<FString> DirtyBefore;
	};

	struct FDeconAudioIsolationAutoRegister
	{
		FDeconAudioIsolationAutoRegister()
		{
			FOrganoidPlaytestCatalogEntry Entry;
			Entry.TestId = TestId;
			Entry.DisplayName = DisplayName;
			Entry.MapPackage = MapPackage;
			Entry.Factory = []() -> TSharedRef<IOrganoidPlaytestCase>
			{
				return MakeShared<FDeconAudioIsolationFunctional>();
			};
			FOrganoidPlaytestRegistry::Register(Entry);
		}
	};

	static FDeconAudioIsolationAutoRegister GDeconAudioIsolationAutoRegister;
}
