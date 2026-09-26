// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptation_LocomotorDisrupt.h"
#include "ProjectOrganoidBiologicalAdaptation_OpticalDisrupt.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AProjectOrganoidResearchStation::AProjectOrganoidResearchStation()
{
	InteractionPrompt = FText::FromString(TEXT("Use Research Station"));
	StationWidgetClass = UProjectOrganoidResearchStationWidget::StaticClass();
	if (InteractionSphere)
	{
		InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	}

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetupAttachment(InteractionSphere.Get());
	StationMesh->SetRelativeScale3D(FVector(0.4f, 0.2f, 0.9f));
	StationMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	StationMesh->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		StationMesh->SetStaticMesh(CubeMesh.Object);
	}
}

bool AProjectOrganoidResearchStation::IsLockedByEncounter() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>())
		{
			return Presence->IsEncounterActive();
		}
	}
	return false;
}

bool AProjectOrganoidResearchStation::CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const
{
	if (!Super::CanInteract_Implementation(Interactor))
	{
		return false;
	}
	return !IsLockedByEncounter();
}

bool AProjectOrganoidResearchStation::Interact_Implementation(AProjectOrganoidCharacter* Interactor)
{
	if (!Super::Interact_Implementation(Interactor))
	{
		return false;
	}

	ApplyCampaignUnlockIfActive(Interactor);
	TryReconcileCampaignIfAlreadyEquipped(Interactor);
	if (OpenResearchStationUI(Interactor) == nullptr)
	{
		return false;
	}

	TryAwardSyringeKitCredit(Interactor);
	TryAwardRespecUseCredit(Interactor);
	return true;
}

FText AProjectOrganoidResearchStation::GetInteractionPrompt() const
{
	if (IsSyringeObjectiveActive() && !SyringePrompt.IsEmpty())
	{
		return SyringePrompt;
	}
	return Super::GetInteractionPrompt();
}

bool AProjectOrganoidResearchStation::IsStationUIOpen() const
{
	return ActiveStationWidget && ActiveStationWidget->IsInViewport();
}

UProjectOrganoidResearchStationWidget* AProjectOrganoidResearchStation::OpenResearchStationUI(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || IsLockedByEncounter())
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return nullptr;
	}

	if (ActiveStationWidget)
	{
		ActiveStationWidget->RemoveFromParent();
		ActiveStationWidget = nullptr;
	}

	TSubclassOf<UProjectOrganoidResearchStationWidget> ClassToSpawn = StationWidgetClass;
	if (!ClassToSpawn)
	{
		ClassToSpawn = UProjectOrganoidResearchStationWidget::StaticClass();
	}

	ActiveStationWidget = CreateWidget<UProjectOrganoidResearchStationWidget>(PC, ClassToSpawn);
	if (!ActiveStationWidget)
	{
		return nullptr;
	}

	ActiveStationWidget->BindToStation(this, Interactor);
	ActiveStationWidget->AddToViewport(50);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(ActiveStationWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;

	return ActiveStationWidget;
}

void AProjectOrganoidResearchStation::CloseResearchStationUI()
{
	if (ActiveStationWidget)
	{
		ActiveStationWidget->CloseStationUI();
	}
}

void AProjectOrganoidResearchStation::NotifyStationUIClosed()
{
	ActiveStationWidget = nullptr;
}

bool AProjectOrganoidResearchStation::TryInstallUnlockedMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData)
{
	if (!Character || !ModData || !Character->IsWeaponModUnlocked(ModData) || IsLockedByEncounter())
	{
		return false;
	}

	UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent();
	AProjectOrganoidWeapon* Weapon = WeaponComp ? WeaponComp->GetEquippedWeapon() : nullptr;
	UProjectOrganoidWeaponModComponent* ModComp = Weapon ? Weapon->GetWeaponModComponent() : nullptr;
	if (!ModComp)
	{
		return false;
	}

	return ModComp->InstallMod(ModData, true);
}

bool AProjectOrganoidResearchStation::TryRemoveInstalledMod(AProjectOrganoidCharacter* Character, EProjectOrganoidWeaponModSlot Slot)
{
	if (!Character || IsLockedByEncounter())
	{
		return false;
	}

	UProjectOrganoidWeaponComponent* WeaponComp = Character->GetWeaponComponent();
	AProjectOrganoidWeapon* Weapon = WeaponComp ? WeaponComp->GetEquippedWeapon() : nullptr;
	UProjectOrganoidWeaponModComponent* ModComp = Weapon ? Weapon->GetWeaponModComponent() : nullptr;
	if (!ModComp)
	{
		return false;
	}

	return ModComp->RemoveModFromSlot(Slot);
}

bool AProjectOrganoidResearchStation::TryEquipUnlockedAdaptation(AProjectOrganoidCharacter* Character, UProjectOrganoidBiologicalAdaptationData* AdaptationData)
{
	if (!Character || !AdaptationData || IsLockedByEncounter())
	{
		return false;
	}

	UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = Character->GetBiologicalAdaptationComponent();
	if (!AdaptComp || !AdaptComp->IsAdaptationUnlocked(AdaptationData))
	{
		return false;
	}

	return AdaptComp->EquipAdaptation(AdaptationData);
}

bool AProjectOrganoidResearchStation::TryUnequipAdaptation(AProjectOrganoidCharacter* Character)
{
	if (!Character || IsLockedByEncounter())
	{
		return false;
	}

	UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = Character->GetBiologicalAdaptationComponent();
	return AdaptComp && AdaptComp->UnequipAdaptation();
}

bool AProjectOrganoidResearchStation::IsCampaignContractConfigured() const
{
	return !CampaignRequiredActiveObjectiveId.IsNone()
		&& !CampaignUnlockAdaptation.IsNull()
		&& !CampaignCreditAdaptation.IsNull()
		&& !CampaignSuccessObjectiveEventId.IsNone()
		&& !CampaignReplayGuardObjectiveId.IsNone();
}

bool AProjectOrganoidResearchStation::IsCampaignObjectiveInState(
	FName ObjectiveId,
	EProjectOrganoidObjectiveState State) const
{
	if (ObjectiveId.IsNone())
	{
		return false;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	const UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
	if (!Objectives)
	{
		return false;
	}

	FProjectOrganoidObjective Objective;
	return Objectives->GetObjective(ObjectiveId, Objective) && Objective.State == State;
}

bool AProjectOrganoidResearchStation::IsCampaignObjectiveActive() const
{
	return IsCampaignObjectiveInState(CampaignRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Active);
}

bool AProjectOrganoidResearchStation::IsCampaignReplayGuardCompleted() const
{
	return IsCampaignObjectiveInState(CampaignReplayGuardObjectiveId, EProjectOrganoidObjectiveState::Completed);
}

bool AProjectOrganoidResearchStation::AdaptationMatchesSoft(
	const TSoftObjectPtr<UProjectOrganoidBiologicalAdaptationData>& Soft,
	const UProjectOrganoidBiologicalAdaptationData* AdaptationData) const
{
	if (!AdaptationData || Soft.IsNull())
	{
		return false;
	}

	if (Soft.Get() == AdaptationData)
	{
		return true;
	}

	if (Soft.ToSoftObjectPath() == FSoftObjectPath(AdaptationData))
	{
		return true;
	}

	return Soft.LoadSynchronous() == AdaptationData;
}

void AProjectOrganoidResearchStation::ApplyCampaignUnlockIfActive(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || !IsCampaignContractConfigured() || !IsCampaignObjectiveActive() || IsCampaignReplayGuardCompleted())
	{
		return;
	}

	UProjectOrganoidBiologicalAdaptationData* UnlockData = CampaignUnlockAdaptation.LoadSynchronous();
	UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = Interactor->GetBiologicalAdaptationComponent();
	if (!UnlockData || !AdaptComp || AdaptComp->IsAdaptationUnlocked(UnlockData))
	{
		return;
	}

	AdaptComp->UnlockAdaptation(UnlockData);
}

void AProjectOrganoidResearchStation::TryReconcileCampaignIfAlreadyEquipped(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor
		|| !IsCampaignContractConfigured()
		|| CampaignSuccessEventFireCount > 0
		|| !IsCampaignObjectiveActive()
		|| IsCampaignReplayGuardCompleted())
	{
		return;
	}

	UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = Interactor->GetBiologicalAdaptationComponent();
	if (!AdaptComp || !AdaptationMatchesSoft(CampaignCreditAdaptation, AdaptComp->GetEquippedAdaptation()))
	{
		return;
	}

	AwardCampaignEquipCredit(Interactor);
}

void AProjectOrganoidResearchStation::NotifyCampaignAdaptationEquipped(
	AProjectOrganoidCharacter* Character,
	UProjectOrganoidBiologicalAdaptationData* AdaptationData)
{
	if (!Character
		|| !IsCampaignContractConfigured()
		|| CampaignSuccessEventFireCount > 0
		|| !IsCampaignObjectiveActive()
		|| IsCampaignReplayGuardCompleted()
		|| !AdaptationMatchesSoft(CampaignCreditAdaptation, AdaptationData))
	{
		return;
	}

	AwardCampaignEquipCredit(Character);
}

void AProjectOrganoidResearchStation::AwardCampaignEquipCredit(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || CampaignSuccessObjectiveEventId.IsNone() || CampaignSuccessEventFireCount > 0)
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(CampaignSuccessObjectiveEventId);
		}
	}

	++CampaignSuccessEventFireCount;
	PresentCampaignSuccessNotification(Interactor);
}

void AProjectOrganoidResearchStation::PresentCampaignSuccessNotification(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || CampaignSuccessNotificationText.IsEmpty() || CampaignSuccessNotificationDurationSeconds <= 0.0f)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	AProjectOrganoidGameMode* GameMode = World->GetAuthGameMode<AProjectOrganoidGameMode>();
	UProjectOrganoidGameplayHUDController* HUDController = GameMode ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (!HUDController)
	{
		return;
	}

	if (HUDController->ShowTransientNotification(
			CampaignSuccessNotificationSpeaker,
			CampaignSuccessNotificationText,
			CampaignSuccessNotificationDurationSeconds))
	{
		++CampaignSuccessNotificationCount;
	}
}

bool AProjectOrganoidResearchStation::IsRespecContractConfigured() const
{
	return !RespecRequiredActiveObjectiveId.IsNone()
		&& !RespecSuccessObjectiveEventId.IsNone()
		&& !RespecReplayGuardObjectiveId.IsNone()
		&& !RespecNotificationText.IsEmpty()
		&& RespecNotificationDurationSeconds > 0.0f;
}

bool AProjectOrganoidResearchStation::IsRespecObjectiveActive() const
{
	return IsCampaignObjectiveInState(RespecRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Active);
}

bool AProjectOrganoidResearchStation::IsRespecReplayGuardCompleted() const
{
	return IsCampaignObjectiveInState(RespecReplayGuardObjectiveId, EProjectOrganoidObjectiveState::Completed);
}

void AProjectOrganoidResearchStation::TryAwardRespecUseCredit(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor
		|| !IsRespecContractConfigured()
		|| RespecEventFireCount > 0
		|| !IsRespecObjectiveActive()
		|| IsRespecReplayGuardCompleted())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(RespecSuccessObjectiveEventId);
		}
	}

	++RespecEventFireCount;
	PresentRespecNotification(Interactor);
}

void AProjectOrganoidResearchStation::PresentRespecNotification(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || RespecNotificationText.IsEmpty() || RespecNotificationDurationSeconds <= 0.0f)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	AProjectOrganoidGameMode* GameMode = World->GetAuthGameMode<AProjectOrganoidGameMode>();
	UProjectOrganoidGameplayHUDController* HUDController = GameMode ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (!HUDController)
	{
		return;
	}

	if (HUDController->ShowTransientNotification(
			RespecNotificationSpeaker,
			RespecNotificationText,
			RespecNotificationDurationSeconds))
	{
		++RespecNotificationCount;
	}
}

bool AProjectOrganoidResearchStation::IsSyringeContractConfigured() const
{
	return !SyringeRequiredActiveObjectiveId.IsNone()
		&& !SyringeSuccessObjectiveEventId.IsNone()
		&& !SyringeReplayGuardObjectiveId.IsNone()
		&& !SyringePrompt.IsEmpty()
		&& !SyringeNotificationText.IsEmpty()
		&& SyringeNotificationDurationSeconds > 0.0f;
}

bool AProjectOrganoidResearchStation::IsSyringeObjectiveActive() const
{
	return IsCampaignObjectiveInState(SyringeRequiredActiveObjectiveId, EProjectOrganoidObjectiveState::Active);
}

bool AProjectOrganoidResearchStation::IsSyringeReplayGuardCompleted() const
{
	return IsCampaignObjectiveInState(SyringeReplayGuardObjectiveId, EProjectOrganoidObjectiveState::Completed);
}

void AProjectOrganoidResearchStation::TryAwardSyringeKitCredit(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor
		|| !IsSyringeContractConfigured()
		|| SyringeEventFireCount >= 2
		|| !IsSyringeObjectiveActive()
		|| IsSyringeReplayGuardCompleted())
	{
		return;
	}

	UProjectOrganoidBiologicalAdaptationComponent* Adapt = Interactor->GetBiologicalAdaptationComponent();
	UProjectOrganoidBiologicalAdaptationData* Locomotor = UProjectOrganoidBiologicalAdaptation_LocomotorDisrupt::Resolve();
	UProjectOrganoidBiologicalAdaptationData* Optical = UProjectOrganoidBiologicalAdaptation_OpticalDisrupt::Resolve();
	if (!Adapt || !Locomotor || !Optical)
	{
		return;
	}

	const bool bFirstSyringe = SyringeEventFireCount == 0;
	UProjectOrganoidBiologicalAdaptationData* ToUnlock = bFirstSyringe ? Locomotor : Optical;
	if (!Adapt->UnlockAdaptation(ToUnlock))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UProjectOrganoidObjectiveSubsystem* Objectives = GameInstance->GetSubsystem<UProjectOrganoidObjectiveSubsystem>())
		{
			Objectives->TriggerEvent(SyringeSuccessObjectiveEventId);
		}
	}

	++SyringeEventFireCount;
	if (bFirstSyringe)
	{
		PresentSyringeNotification(Interactor);
	}
}

void AProjectOrganoidResearchStation::PresentSyringeNotification(AProjectOrganoidCharacter* Interactor)
{
	if (!Interactor || SyringeNotificationText.IsEmpty() || SyringeNotificationDurationSeconds <= 0.0f)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Interactor->GetController());
	UWorld* World = GetWorld();
	if (!PC || !World)
	{
		return;
	}

	AProjectOrganoidGameMode* GameMode = World->GetAuthGameMode<AProjectOrganoidGameMode>();
	UProjectOrganoidGameplayHUDController* HUDController = GameMode ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (!HUDController)
	{
		return;
	}

	if (HUDController->ShowTransientNotification(
			SyringeNotificationSpeaker,
			SyringeNotificationText,
			SyringeNotificationDurationSeconds))
	{
		++SyringeNotificationCount;
	}
}
