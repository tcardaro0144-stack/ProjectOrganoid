// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidPowerSubsystem.generated.h"

class UProjectOrganoidPowerAwareComponent;
class AProjectOrganoidDoorLock;
class AProjectOrganoidTerminal;
class AProjectOrganoidSecurityGate;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidSectorPowerChanged, EProjectOrganoidPowerSector, Sector, EProjectOrganoidPowerState, NewState, EProjectOrganoidPowerState, PreviousState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidFacilityPowerChanged, EProjectOrganoidPowerState, FacilityState);

/**
 *  Facility power grid director:
 *  - Tracks per-sector Online / Emergency / Blackout states
 *  - Toggles emergency backup lights
 *  - Dims security cameras and disables laser grids during blackouts
 *  - Broadcasts power events for doors, terminals, and gates
 */
UCLASS()
class UProjectOrganoidPowerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Power")
	FOnProjectOrganoidSectorPowerChanged OnSectorPowerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Power")
	FOnProjectOrganoidFacilityPowerChanged OnFacilityPowerChanged;

	UFUNCTION(BlueprintCallable, Category = "Power")
	void RegisterPowerAwareComponent(UProjectOrganoidPowerAwareComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Power")
	void UnregisterPowerAwareComponent(UProjectOrganoidPowerAwareComponent* Component);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void RegisterDoor(AProjectOrganoidDoorLock* Door);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void UnregisterDoor(AProjectOrganoidDoorLock* Door);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void RegisterTerminal(AProjectOrganoidTerminal* Terminal);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void UnregisterTerminal(AProjectOrganoidTerminal* Terminal);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void RegisterSecurityGate(AProjectOrganoidSecurityGate* Gate);

	UFUNCTION(BlueprintCallable, Category = "Power|Listeners")
	void UnregisterSecurityGate(AProjectOrganoidSecurityGate* Gate);

	UFUNCTION(BlueprintPure, Category = "Power")
	EProjectOrganoidPowerState GetSectorPowerState(EProjectOrganoidPowerSector Sector) const;

	UFUNCTION(BlueprintPure, Category = "Power")
	EProjectOrganoidPowerState GetFacilityPowerState() const { return FacilityPowerState; }

	UFUNCTION(BlueprintPure, Category = "Power")
	bool IsSectorOnline(EProjectOrganoidPowerSector Sector) const;

	UFUNCTION(BlueprintPure, Category = "Power")
	bool IsSectorInBlackout(EProjectOrganoidPowerSector Sector) const;

	UFUNCTION(BlueprintCallable, Category = "Power")
	void SetSectorPowerState(EProjectOrganoidPowerSector Sector, EProjectOrganoidPowerState NewState);

	/** Push every sector (including FacilityWide) into Blackout */
	UFUNCTION(BlueprintCallable, Category = "Power")
	void TriggerFacilityBlackout();

	/** Restore all sectors to Online */
	UFUNCTION(BlueprintCallable, Category = "Power")
	void RestoreFacilityPower();

	/** Drop a sector to Emergency (backup lighting on, cameras dimmed) */
	UFUNCTION(BlueprintCallable, Category = "Power")
	void EngageEmergencyPower(EProjectOrganoidPowerSector Sector);

	UFUNCTION(BlueprintPure, Category = "Power")
	TArray<FProjectOrganoidSectorPowerStatus> GetAllSectorStatuses() const;

protected:

	UPROPERTY()
	TMap<EProjectOrganoidPowerSector, EProjectOrganoidPowerState> SectorStates;

	UPROPERTY(BlueprintReadOnly, Category = "Power")
	EProjectOrganoidPowerState FacilityPowerState = EProjectOrganoidPowerState::Online;

	UPROPERTY()
	TArray<TObjectPtr<UProjectOrganoidPowerAwareComponent>> PowerAwareComponents;

	UPROPERTY()
	TArray<TObjectPtr<AProjectOrganoidDoorLock>> RegisteredDoors;

	UPROPERTY()
	TArray<TObjectPtr<AProjectOrganoidTerminal>> RegisteredTerminals;

	UPROPERTY()
	TArray<TObjectPtr<AProjectOrganoidSecurityGate>> RegisteredGates;

	void SeedDefaultSectorStates();
	void PruneInvalidRegistrations();
	void ApplyStateToRegisteredDevices(EProjectOrganoidPowerSector Sector, EProjectOrganoidPowerState NewState);
	void BroadcastToConnectedInteractables(EProjectOrganoidPowerSector Sector, EProjectOrganoidPowerState NewState, EProjectOrganoidPowerState PreviousState);
	void RefreshFacilityPowerState();
	static bool SectorMatches(EProjectOrganoidPowerSector ListenerSector, EProjectOrganoidPowerSector ChangedSector);
};
