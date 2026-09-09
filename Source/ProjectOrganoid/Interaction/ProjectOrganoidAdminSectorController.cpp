// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminSectorController.h"
#include "ProjectOrganoidSecuritySubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AProjectOrganoidAdminSectorController::AProjectOrganoidAdminSectorController()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	SetActorEnableCollision(false);
}

void AProjectOrganoidAdminSectorController::BeginPlay()
{
	Super::BeginPlay();
	BindSecuritySubsystem();
	InitializeAdmin();
}

void AProjectOrganoidAdminSectorController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSecuritySubsystem();
	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidAdminSectorController::InitializeAdmin()
{
	FacilityState = TEXT("Normal");
	bAdminInitialized = true;
	ApplyFacilityStateFromSubsystems();
	OnAdminInitialized.Broadcast(this);
}

void AProjectOrganoidAdminSectorController::SetCurrentRoom(FName NewRoom)
{
	CurrentRoom = NewRoom;

	if (NewRoom == TEXT("Security"))
	{
		bSecurityScanned = true;
	}
	else if (NewRoom == TEXT("Records"))
	{
		bRecordsAccessed = true;
	}
	else if (NewRoom == TEXT("DirectorSuite"))
	{
		bDirectorOfficeVisited = true;
	}
	else if (NewRoom == TEXT("Operations"))
	{
		bOperationsActivated = true;
	}
}

void AProjectOrganoidAdminSectorController::ApplyFacilityStateFromSubsystems()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
	{
		const EProjectOrganoidPowerState AdminPower = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin);
		if (AdminPower == EProjectOrganoidPowerState::Emergency || AdminPower == EProjectOrganoidPowerState::Blackout)
		{
			FacilityState = TEXT("Emergency");
			return;
		}
	}

	if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
	{
		if (Security->IsFacilityLockdownActive())
		{
			FacilityState = TEXT("Lockdown");
			return;
		}
	}

	FacilityState = TEXT("Normal");
}

void AProjectOrganoidAdminSectorController::HandleFacilityLockdownChanged(bool bIsLockdownActive, FName /*LockdownId*/)
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			const EProjectOrganoidPowerState AdminPower = Power->GetSectorPowerState(EProjectOrganoidPowerSector::Admin);
			if (AdminPower == EProjectOrganoidPowerState::Emergency || AdminPower == EProjectOrganoidPowerState::Blackout)
			{
				FacilityState = TEXT("Emergency");
				return;
			}
		}
	}

	FacilityState = bIsLockdownActive ? TEXT("Lockdown") : TEXT("Normal");
}

void AProjectOrganoidAdminSectorController::BindSecuritySubsystem()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
	{
		Security->OnFacilityLockdownChanged.AddDynamic(this, &AProjectOrganoidAdminSectorController::HandleFacilityLockdownChanged);
	}
}

void AProjectOrganoidAdminSectorController::UnbindSecuritySubsystem()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UProjectOrganoidSecuritySubsystem* Security = World->GetSubsystem<UProjectOrganoidSecuritySubsystem>())
	{
		Security->OnFacilityLockdownChanged.RemoveDynamic(this, &AProjectOrganoidAdminSectorController::HandleFacilityLockdownChanged);
	}
}
