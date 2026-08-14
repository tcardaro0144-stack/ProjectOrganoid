// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPowerSubsystem.h"
#include "ProjectOrganoidPowerAwareComponent.h"
#include "ProjectOrganoidDoorLock.h"
#include "ProjectOrganoidTerminal.h"
#include "ProjectOrganoidSecurityGate.h"

void UProjectOrganoidPowerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SeedDefaultSectorStates();
}

void UProjectOrganoidPowerSubsystem::Deinitialize()
{
	PowerAwareComponents.Reset();
	RegisteredDoors.Reset();
	RegisteredTerminals.Reset();
	RegisteredGates.Reset();
	Super::Deinitialize();
}

void UProjectOrganoidPowerSubsystem::SeedDefaultSectorStates()
{
	SectorStates.Reset();
	SectorStates.Add(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
	SectorStates.Add(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
	SectorStates.Add(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
	SectorStates.Add(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
	SectorStates.Add(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
	SectorStates.Add(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Online);
	FacilityPowerState = EProjectOrganoidPowerState::Online;
}

void UProjectOrganoidPowerSubsystem::RegisterPowerAwareComponent(UProjectOrganoidPowerAwareComponent* Component)
{
	if (Component)
	{
		PowerAwareComponents.AddUnique(Component);
	}
}

void UProjectOrganoidPowerSubsystem::UnregisterPowerAwareComponent(UProjectOrganoidPowerAwareComponent* Component)
{
	PowerAwareComponents.Remove(Component);
}

void UProjectOrganoidPowerSubsystem::RegisterDoor(AProjectOrganoidDoorLock* Door)
{
	if (Door)
	{
		RegisteredDoors.AddUnique(Door);
	}
}

void UProjectOrganoidPowerSubsystem::UnregisterDoor(AProjectOrganoidDoorLock* Door)
{
	RegisteredDoors.Remove(Door);
}

void UProjectOrganoidPowerSubsystem::RegisterTerminal(AProjectOrganoidTerminal* Terminal)
{
	if (Terminal)
	{
		RegisteredTerminals.AddUnique(Terminal);
	}
}

void UProjectOrganoidPowerSubsystem::UnregisterTerminal(AProjectOrganoidTerminal* Terminal)
{
	RegisteredTerminals.Remove(Terminal);
}

void UProjectOrganoidPowerSubsystem::RegisterSecurityGate(AProjectOrganoidSecurityGate* Gate)
{
	if (Gate)
	{
		RegisteredGates.AddUnique(Gate);
	}
}

void UProjectOrganoidPowerSubsystem::UnregisterSecurityGate(AProjectOrganoidSecurityGate* Gate)
{
	RegisteredGates.Remove(Gate);
}

EProjectOrganoidPowerState UProjectOrganoidPowerSubsystem::GetSectorPowerState(EProjectOrganoidPowerSector Sector) const
{
	if (const EProjectOrganoidPowerState* Found = SectorStates.Find(Sector))
	{
		return *Found;
	}
	return EProjectOrganoidPowerState::Online;
}

bool UProjectOrganoidPowerSubsystem::IsSectorOnline(EProjectOrganoidPowerSector Sector) const
{
	return GetSectorPowerState(Sector) == EProjectOrganoidPowerState::Online;
}

bool UProjectOrganoidPowerSubsystem::IsSectorInBlackout(EProjectOrganoidPowerSector Sector) const
{
	return GetSectorPowerState(Sector) == EProjectOrganoidPowerState::Blackout
		|| GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Blackout;
}

void UProjectOrganoidPowerSubsystem::SetSectorPowerState(EProjectOrganoidPowerSector Sector, EProjectOrganoidPowerState NewState)
{
	const EProjectOrganoidPowerState Previous = GetSectorPowerState(Sector);
	if (Previous == NewState && SectorStates.Contains(Sector))
	{
		return;
	}

	SectorStates.Add(Sector, NewState);
	RefreshFacilityPowerState();
	ApplyStateToRegisteredDevices(Sector, NewState);
	BroadcastToConnectedInteractables(Sector, NewState, Previous);
	OnSectorPowerChanged.Broadcast(Sector, NewState, Previous);

	if (Sector == EProjectOrganoidPowerSector::FacilityWide)
	{
		OnFacilityPowerChanged.Broadcast(NewState);
	}
}

void UProjectOrganoidPowerSubsystem::TriggerFacilityBlackout()
{
	SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Blackout);
	SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Blackout);
	SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Blackout);
	SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Blackout);
	SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Blackout);
	SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Blackout);
}

void UProjectOrganoidPowerSubsystem::RestoreFacilityPower()
{
	SetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide, EProjectOrganoidPowerState::Online);
	SetSectorPowerState(EProjectOrganoidPowerSector::Admin, EProjectOrganoidPowerState::Online);
	SetSectorPowerState(EProjectOrganoidPowerSector::NeuroGenetics, EProjectOrganoidPowerState::Online);
	SetSectorPowerState(EProjectOrganoidPowerSector::Cryo, EProjectOrganoidPowerState::Online);
	SetSectorPowerState(EProjectOrganoidPowerSector::Compute, EProjectOrganoidPowerState::Online);
	SetSectorPowerState(EProjectOrganoidPowerSector::Reactor, EProjectOrganoidPowerState::Online);
}

void UProjectOrganoidPowerSubsystem::EngageEmergencyPower(EProjectOrganoidPowerSector Sector)
{
	SetSectorPowerState(Sector, EProjectOrganoidPowerState::Emergency);
}

TArray<FProjectOrganoidSectorPowerStatus> UProjectOrganoidPowerSubsystem::GetAllSectorStatuses() const
{
	TArray<FProjectOrganoidSectorPowerStatus> Result;
	Result.Reserve(SectorStates.Num());
	for (const TPair<EProjectOrganoidPowerSector, EProjectOrganoidPowerState>& Pair : SectorStates)
	{
		FProjectOrganoidSectorPowerStatus Status;
		Status.Sector = Pair.Key;
		Status.State = Pair.Value;
		Result.Add(Status);
	}
	return Result;
}

void UProjectOrganoidPowerSubsystem::PruneInvalidRegistrations()
{
	PowerAwareComponents.RemoveAll([](const TObjectPtr<UProjectOrganoidPowerAwareComponent>& Comp)
	{
		return !IsValid(Comp);
	});
	RegisteredDoors.RemoveAll([](const TObjectPtr<AProjectOrganoidDoorLock>& Door)
	{
		return !IsValid(Door);
	});
	RegisteredTerminals.RemoveAll([](const TObjectPtr<AProjectOrganoidTerminal>& Terminal)
	{
		return !IsValid(Terminal);
	});
	RegisteredGates.RemoveAll([](const TObjectPtr<AProjectOrganoidSecurityGate>& Gate)
	{
		return !IsValid(Gate);
	});
}

void UProjectOrganoidPowerSubsystem::ApplyStateToRegisteredDevices(EProjectOrganoidPowerSector Sector, EProjectOrganoidPowerState NewState)
{
	PruneInvalidRegistrations();

	for (UProjectOrganoidPowerAwareComponent* Component : PowerAwareComponents)
	{
		if (!Component)
		{
			continue;
		}

		if (SectorMatches(Component->PowerSector, Sector))
		{
			Component->ApplyPowerState(NewState);
		}
	}
}

void UProjectOrganoidPowerSubsystem::BroadcastToConnectedInteractables(
	EProjectOrganoidPowerSector Sector,
	EProjectOrganoidPowerState NewState,
	EProjectOrganoidPowerState PreviousState)
{
	PruneInvalidRegistrations();

	for (AProjectOrganoidDoorLock* Door : RegisteredDoors)
	{
		if (Door && SectorMatches(Door->PowerSector, Sector))
		{
			Door->HandlePowerStateChanged(NewState, PreviousState);
		}
	}

	for (AProjectOrganoidTerminal* Terminal : RegisteredTerminals)
	{
		if (Terminal && SectorMatches(Terminal->PowerSector, Sector))
		{
			Terminal->HandlePowerStateChanged(NewState, PreviousState);
		}
	}

	for (AProjectOrganoidSecurityGate* Gate : RegisteredGates)
	{
		if (Gate && SectorMatches(Gate->PowerSector, Sector))
		{
			Gate->HandlePowerStateChanged(NewState, PreviousState);
		}
	}
}

void UProjectOrganoidPowerSubsystem::RefreshFacilityPowerState()
{
	bool bAnyBlackout = false;
	bool bAnyEmergency = false;
	bool bAllOnline = true;

	for (const TPair<EProjectOrganoidPowerSector, EProjectOrganoidPowerState>& Pair : SectorStates)
	{
		if (Pair.Key == EProjectOrganoidPowerSector::FacilityWide)
		{
			continue;
		}

		if (Pair.Value == EProjectOrganoidPowerState::Blackout)
		{
			bAnyBlackout = true;
			bAllOnline = false;
		}
		else if (Pair.Value == EProjectOrganoidPowerState::Emergency)
		{
			bAnyEmergency = true;
			bAllOnline = false;
		}
	}

	if (GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Blackout)
	{
		bAnyBlackout = true;
		bAllOnline = false;
	}

	const EProjectOrganoidPowerState PreviousFacility = FacilityPowerState;
	if (bAnyBlackout)
	{
		FacilityPowerState = EProjectOrganoidPowerState::Blackout;
	}
	else if (bAnyEmergency)
	{
		FacilityPowerState = EProjectOrganoidPowerState::Emergency;
	}
	else if (bAllOnline)
	{
		FacilityPowerState = EProjectOrganoidPowerState::Online;
	}

	if (PreviousFacility != FacilityPowerState)
	{
		OnFacilityPowerChanged.Broadcast(FacilityPowerState);
	}
}

bool UProjectOrganoidPowerSubsystem::SectorMatches(EProjectOrganoidPowerSector ListenerSector, EProjectOrganoidPowerSector ChangedSector)
{
	return ChangedSector == EProjectOrganoidPowerSector::FacilityWide
		|| ListenerSector == EProjectOrganoidPowerSector::FacilityWide
		|| ListenerSector == ChangedSector;
}
