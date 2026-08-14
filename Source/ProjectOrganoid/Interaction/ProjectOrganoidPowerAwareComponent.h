// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidPowerAwareComponent.generated.h"

class ULightComponent;
class UPrimitiveComponent;

/**
 *  Registers with UProjectOrganoidPowerSubsystem and reacts to sector power changes.
 *  Use for emergency lights, security cameras, and laser grids.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidPowerAwareComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidPowerAwareComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerSector PowerSector = EProjectOrganoidPowerSector::Admin;

	/** Emergency backup light — activates in Emergency, off in Online/Blackout by default */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Role")
	bool bIsEmergencyLight = false;

	/** Security camera feed — dims / disables under Emergency / Blackout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Role")
	bool bIsSecurityCamera = false;

	/** Laser tripwire / grid — disabled during Blackout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Role")
	bool bIsLaserGrid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Lighting", meta = (ClampMin = "0.0"))
	float NormalLightIntensity = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Lighting", meta = (ClampMin = "0.0"))
	float EmergencyLightIntensity = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Camera", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EmergencyCameraDimScale = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Power")
	EProjectOrganoidPowerState CachedPowerState = EProjectOrganoidPowerState::Online;

	UFUNCTION(BlueprintCallable, Category = "Power")
	void ApplyPowerState(EProjectOrganoidPowerState NewState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Power")
	void BP_OnPowerStateApplied(EProjectOrganoidPowerState NewState);

protected:

	UPROPERTY()
	TArray<TObjectPtr<ULightComponent>> CachedLights;

	UPROPERTY()
	TArray<TObjectPtr<UPrimitiveComponent>> CachedPrimitives;

	void CacheLinkedComponents();
	void ApplyEmergencyLighting(EProjectOrganoidPowerState State);
	void ApplySecurityCamera(EProjectOrganoidPowerState State);
	void ApplyLaserGrid(EProjectOrganoidPowerState State);
};
