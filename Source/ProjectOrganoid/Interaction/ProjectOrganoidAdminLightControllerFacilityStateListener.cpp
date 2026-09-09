// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminLightControllerFacilityStateListener.h"

#include "GameFramework/Actor.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "ProjectOrganoidAdminLightingLibrary.h"
#include "UObject/UnrealType.h"

UProjectOrganoidAdminLightControllerFacilityStateListener::UProjectOrganoidAdminLightControllerFacilityStateListener()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::BeginPlay()
{
	Super::BeginPlay();
	BindAndSynchronize();
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::HandleAdminFacilityStateChanged(
	EProjectOrganoidAdminFacilityState NewState,
	EProjectOrganoidAdminFacilityState PreviousState)
{
	(void)NewState;
	(void)PreviousState;
	ReapplyOwnerLighting();
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::BindAndSynchronize()
{
	Unbind();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UProjectOrganoidAdminFacilityStateSubsystem* State = World->GetSubsystem<UProjectOrganoidAdminFacilityStateSubsystem>();
	if (!State)
	{
		return;
	}

	BoundSubsystem = State;
	State->OnAdminFacilityStateChanged.AddDynamic(
		this,
		&UProjectOrganoidAdminLightControllerFacilityStateListener::HandleAdminFacilityStateChanged);
	ReapplyOwnerLighting();
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::Unbind()
{
	if (UProjectOrganoidAdminFacilityStateSubsystem* State = BoundSubsystem.Get())
	{
		State->OnAdminFacilityStateChanged.RemoveDynamic(
			this,
			&UProjectOrganoidAdminLightControllerFacilityStateListener::HandleAdminFacilityStateChanged);
	}
	BoundSubsystem.Reset();
}

void UProjectOrganoidAdminLightControllerFacilityStateListener::ReapplyOwnerLighting()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FName Zone = NAME_None;
	float AuthoredIntensity = 5000.0f;
	float InactiveMultiplier = 0.25f;

	if (const FNameProperty* ZoneProp = FindFProperty<FNameProperty>(Owner->GetClass(), TEXT("CurrentLightingZone")))
	{
		Zone = ZoneProp->GetPropertyValue_InContainer(Owner);
	}
	if (const FFloatProperty* AuthoredProp = FindFProperty<FFloatProperty>(Owner->GetClass(), TEXT("AuthoredIntensity")))
	{
		AuthoredIntensity = AuthoredProp->GetPropertyValue_InContainer(Owner);
	}
	if (const FFloatProperty* InactiveProp = FindFProperty<FFloatProperty>(Owner->GetClass(), TEXT("InactiveMultiplier")))
	{
		InactiveMultiplier = InactiveProp->GetPropertyValue_InContainer(Owner);
	}

	UProjectOrganoidAdminLightingLibrary::ApplyAdminZoneLighting(Owner, Zone, AuthoredIntensity, InactiveMultiplier);
}
