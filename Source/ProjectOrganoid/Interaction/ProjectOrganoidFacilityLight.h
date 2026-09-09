// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidPowerTypes.h"
#include "ProjectOrganoidFacilityLight.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UProjectOrganoidPowerAwareComponent;

/**
 *  Power-aware room fixture. Sector lights dim under emergency and die in a blackout;
 *  emergency fixtures only ignite while their sector is on backup power.
 */
UCLASS(Blueprintable)
class AProjectOrganoidFacilityLight : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidFacilityLight();

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FixtureMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> Light;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Power")
	TObjectPtr<UProjectOrganoidPowerAwareComponent> PowerAware;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidPowerSector PowerSector = EProjectOrganoidPowerSector::Admin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power")
	EProjectOrganoidFacilityLightRole LightRole = EProjectOrganoidFacilityLightRole::Sector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Lighting", meta = (ClampMin = "0.0"))
	float SectorIntensity = 4500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Lighting", meta = (ClampMin = "0.0"))
	float EmergencyIntensity = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Power|Lighting", meta = (ClampMin = "100.0"))
	float AttenuationRadius = 2200.0f;

protected:

	void ApplyRoleToComponents();
};
