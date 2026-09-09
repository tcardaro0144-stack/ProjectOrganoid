// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidFacilityLight.h"
#include "ProjectOrganoidPowerAwareComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AProjectOrganoidFacilityLight::AProjectOrganoidFacilityLight()
{
	PrimaryActorTick.bCanEverTick = false;

	FixtureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FixtureMesh"));
	SetRootComponent(FixtureMesh);
	FixtureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FixtureMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.08f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FixtureMesh->SetStaticMesh(CubeMesh.Object);
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(FixtureMesh);
	Light->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	Light->SetIntensity(SectorIntensity);
	Light->SetAttenuationRadius(AttenuationRadius);
	Light->SetLightColor(FLinearColor(0.92f, 0.95f, 1.0f));
	Light->SetCastShadows(false);

	PowerAware = CreateDefaultSubobject<UProjectOrganoidPowerAwareComponent>(TEXT("PowerAware"));
	PowerAware->bIsSectorLight = true;
	PowerAware->NormalLightIntensity = SectorIntensity;
	PowerAware->EmergencyLightIntensity = EmergencyIntensity;
}

void AProjectOrganoidFacilityLight::BeginPlay()
{
	ApplyRoleToComponents();
	Super::BeginPlay();
}

void AProjectOrganoidFacilityLight::ApplyRoleToComponents()
{
	if (PowerAware)
	{
		PowerAware->PowerSector = PowerSector;
		PowerAware->NormalLightIntensity = SectorIntensity;
		PowerAware->EmergencyLightIntensity = EmergencyIntensity;
		PowerAware->bIsEmergencyLight = LightRole == EProjectOrganoidFacilityLightRole::Emergency;
		PowerAware->bIsSectorLight = LightRole == EProjectOrganoidFacilityLightRole::Sector;
	}

	if (!Light)
	{
		return;
	}

	Light->SetAttenuationRadius(AttenuationRadius);

	if (LightRole == EProjectOrganoidFacilityLightRole::Emergency)
	{
		Light->SetLightColor(FLinearColor(1.0f, 0.14f, 0.08f));
		Light->SetIntensity(EmergencyIntensity);
		if (FixtureMesh)
		{
			FixtureMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(0.85f, 0.08f, 0.05f));
		}
	}
	else
	{
		Light->SetLightColor(FLinearColor(0.92f, 0.95f, 1.0f));
		Light->SetIntensity(SectorIntensity);
	}
}
