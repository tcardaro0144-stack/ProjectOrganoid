// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPowerAwareComponent.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"

UProjectOrganoidPowerAwareComponent::UProjectOrganoidPowerAwareComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProjectOrganoidPowerAwareComponent::BeginPlay()
{
	Super::BeginPlay();
	CacheLinkedComponents();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->RegisterPowerAwareComponent(this);
			ApplyPowerState(Power->GetSectorPowerState(PowerSector));
		}
	}
}

void UProjectOrganoidPowerAwareComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->UnregisterPowerAwareComponent(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void UProjectOrganoidPowerAwareComponent::CacheLinkedComponents()
{
	CachedLights.Reset();
	CachedPrimitives.Reset();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<ULightComponent*> Lights;
	Owner->GetComponents<ULightComponent>(Lights);
	for (ULightComponent* Light : Lights)
	{
		CachedLights.Add(Light);
	}

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive && Primitive->ComponentHasTag(TEXT("Laser")))
		{
			CachedPrimitives.Add(Primitive);
		}
	}
}

void UProjectOrganoidPowerAwareComponent::ApplyPowerState(EProjectOrganoidPowerState NewState)
{
	CachedPowerState = NewState;

	if (bIsEmergencyLight)
	{
		ApplyEmergencyLighting(NewState);
	}
	if (bIsSecurityCamera)
	{
		ApplySecurityCamera(NewState);
	}
	if (bIsLaserGrid)
	{
		ApplyLaserGrid(NewState);
	}

	BP_OnPowerStateApplied(NewState);
}

void UProjectOrganoidPowerAwareComponent::ApplyEmergencyLighting(EProjectOrganoidPowerState State)
{
	for (ULightComponent* Light : CachedLights)
	{
		if (!Light)
		{
			continue;
		}

		switch (State)
		{
		case EProjectOrganoidPowerState::Online:
			Light->SetVisibility(false);
			Light->SetIntensity(0.0f);
			break;
		case EProjectOrganoidPowerState::Emergency:
			Light->SetVisibility(true);
			Light->SetIntensity(EmergencyLightIntensity);
			break;
		case EProjectOrganoidPowerState::Blackout:
			Light->SetVisibility(false);
			Light->SetIntensity(0.0f);
			break;
		}
	}
}

void UProjectOrganoidPowerAwareComponent::ApplySecurityCamera(EProjectOrganoidPowerState State)
{
	const bool bActive = State == EProjectOrganoidPowerState::Online;
	const float Dim = State == EProjectOrganoidPowerState::Emergency ? EmergencyCameraDimScale : 0.0f;

	for (ULightComponent* Light : CachedLights)
	{
		if (!Light)
		{
			continue;
		}

		if (bActive)
		{
			Light->SetVisibility(true);
			Light->SetIntensity(NormalLightIntensity);
		}
		else if (State == EProjectOrganoidPowerState::Emergency)
		{
			Light->SetVisibility(true);
			Light->SetIntensity(NormalLightIntensity * Dim);
		}
		else
		{
			Light->SetVisibility(false);
			Light->SetIntensity(0.0f);
		}
	}

	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorHiddenInGame(State == EProjectOrganoidPowerState::Blackout);
		Owner->SetActorTickEnabled(bActive);
	}
}

void UProjectOrganoidPowerAwareComponent::ApplyLaserGrid(EProjectOrganoidPowerState State)
{
	const bool bEnabled = State == EProjectOrganoidPowerState::Online;

	for (UPrimitiveComponent* Primitive : CachedPrimitives)
	{
		if (!Primitive)
		{
			continue;
		}

		Primitive->SetVisibility(bEnabled, true);
		Primitive->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	// Fallback: all owner primitives tagged or named with Laser if none cached
	if (CachedPrimitives.Num() == 0)
	{
		AActor* Owner = GetOwner();
		if (!Owner)
		{
			return;
		}

		TArray<UPrimitiveComponent*> Primitives;
		Owner->GetComponents<UPrimitiveComponent>(Primitives);
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive)
			{
				continue;
			}
			Primitive->SetVisibility(bEnabled, true);
			if (!bEnabled)
			{
				Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}
