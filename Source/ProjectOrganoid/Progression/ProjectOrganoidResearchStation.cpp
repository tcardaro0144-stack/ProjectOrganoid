// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidResearchStation.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "ProjectOrganoidResearchStationWidget.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
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

	return OpenResearchStationUI(Interactor) != nullptr;
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
