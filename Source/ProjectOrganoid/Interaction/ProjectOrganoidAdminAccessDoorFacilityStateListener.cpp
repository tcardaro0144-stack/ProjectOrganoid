// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidAdminAccessDoorFacilityStateListener.h"

#include "GameFramework/Actor.h"
#include "ProjectOrganoidAdminFacilityStateSubsystem.h"
#include "UObject/UnrealType.h"

UProjectOrganoidAdminAccessDoorFacilityStateListener::UProjectOrganoidAdminAccessDoorFacilityStateListener()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::BeginPlay()
{
	Super::BeginPlay();
	BindAndSynchronize();
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::HandleAdminFacilityStateChanged(
	EProjectOrganoidAdminFacilityState NewState,
	EProjectOrganoidAdminFacilityState PreviousState)
{
	(void)PreviousState;
	ApplyAdminFacilityState(NewState);
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::BindAndSynchronize()
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
		&UProjectOrganoidAdminAccessDoorFacilityStateListener::HandleAdminFacilityStateChanged);
	ApplyAdminFacilityState(State->GetAdminFacilityState());
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::Unbind()
{
	if (UProjectOrganoidAdminFacilityStateSubsystem* State = BoundSubsystem.Get())
	{
		State->OnAdminFacilityStateChanged.RemoveDynamic(
			this,
			&UProjectOrganoidAdminAccessDoorFacilityStateListener::HandleAdminFacilityStateChanged);
	}
	BoundSubsystem.Reset();
}

void UProjectOrganoidAdminAccessDoorFacilityStateListener::ApplyAdminFacilityState(EProjectOrganoidAdminFacilityState State)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	if (State == EProjectOrganoidAdminFacilityState::Lockdown)
	{
		if (!bFacilityLockOverrideActive)
		{
			ReadOwnerLocked(Owner, bBaselineLocked);
			bBaselineCaptured = true;
			bFacilityLockOverrideActive = true;
		}
		WriteOwnerLocked(Owner, true);
		return;
	}

	if (!bBaselineCaptured)
	{
		ReadOwnerLocked(Owner, bBaselineLocked);
		bBaselineCaptured = true;
	}
	bFacilityLockOverrideActive = false;
	WriteOwnerLocked(Owner, bBaselineLocked);
}

bool UProjectOrganoidAdminAccessDoorFacilityStateListener::ReadOwnerLocked(AActor* Owner, bool& OutLocked)
{
	OutLocked = false;
	if (!Owner)
	{
		return false;
	}
	const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Owner->GetClass(), TEXT("bLocked"));
	if (!Prop)
	{
		return false;
	}
	OutLocked = Prop->GetPropertyValue_InContainer(Owner);
	return true;
}

bool UProjectOrganoidAdminAccessDoorFacilityStateListener::WriteOwnerLocked(AActor* Owner, bool bLocked)
{
	if (!Owner)
	{
		return false;
	}
	FBoolProperty* Prop = FindFProperty<FBoolProperty>(Owner->GetClass(), TEXT("bLocked"));
	if (!Prop)
	{
		return false;
	}
	Prop->SetPropertyValue_InContainer(Owner, bLocked);
	return true;
}
