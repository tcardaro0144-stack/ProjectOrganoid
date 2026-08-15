// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidTrapSubsystem.h"
#include "ProjectOrganoidCorridorTrapVolume.h"
#include "ProjectOrganoidLaserTripwire.h"
#include "ProjectOrganoidPressurePlate.h"
#include "ProjectOrganoidHazardEmitterTrap.h"
#include "ProjectOrganoidTrapBase.h"
#include "Engine/World.h"

void UProjectOrganoidTrapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TrapRandom.Initialize(RandomSeed != 0 ? RandomSeed : FMath::Rand());
	EnsureDefaultClasses();
}

void UProjectOrganoidTrapSubsystem::Deinitialize()
{
	RegisteredCorridors.Reset();
	SpawnedTraps.Reset();
	Super::Deinitialize();
}

void UProjectOrganoidTrapSubsystem::EnsureDefaultClasses()
{
	if (!LaserTripwireClass)
	{
		LaserTripwireClass = AProjectOrganoidLaserTripwire::StaticClass();
	}
	if (!PressurePlateClass)
	{
		PressurePlateClass = AProjectOrganoidPressurePlate::StaticClass();
	}
	if (!HazardEmitterClass)
	{
		HazardEmitterClass = AProjectOrganoidHazardEmitterTrap::StaticClass();
	}
}

void UProjectOrganoidTrapSubsystem::RegisterCorridor(AProjectOrganoidCorridorTrapVolume* Corridor)
{
	if (!Corridor)
	{
		return;
	}

	for (const TWeakObjectPtr<AProjectOrganoidCorridorTrapVolume>& Entry : RegisteredCorridors)
	{
		if (Entry.Get() == Corridor)
		{
			return;
		}
	}

	RegisteredCorridors.Add(Corridor);
}

void UProjectOrganoidTrapSubsystem::UnregisterCorridor(AProjectOrganoidCorridorTrapVolume* Corridor)
{
	RegisteredCorridors.RemoveAll([Corridor](const TWeakObjectPtr<AProjectOrganoidCorridorTrapVolume>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Corridor;
	});
}

EProjectOrganoidTrapType UProjectOrganoidTrapSubsystem::RollTrapType(const FProjectOrganoidTrapSpawnWeights& Weights)
{
	const float Total = Weights.LaserTripwireWeight + Weights.PressurePlateWeight + Weights.HazardEmitterWeight;
	if (Total <= KINDA_SMALL_NUMBER)
	{
		return EProjectOrganoidTrapType::LaserTripwire;
	}

	const float Roll = TrapRandom.FRandRange(0.0f, Total);
	if (Roll < Weights.LaserTripwireWeight)
	{
		return EProjectOrganoidTrapType::LaserTripwire;
	}
	if (Roll < Weights.LaserTripwireWeight + Weights.PressurePlateWeight)
	{
		return EProjectOrganoidTrapType::PressurePlate;
	}
	return EProjectOrganoidTrapType::HazardEmitter;
}

TSubclassOf<AProjectOrganoidTrapBase> UProjectOrganoidTrapSubsystem::ResolveClassForType(EProjectOrganoidTrapType Type) const
{
	switch (Type)
	{
	case EProjectOrganoidTrapType::PressurePlate:
		return PressurePlateClass;
	case EProjectOrganoidTrapType::HazardEmitter:
		return HazardEmitterClass;
	case EProjectOrganoidTrapType::LaserTripwire:
	default:
		return LaserTripwireClass;
	}
}

FTransform UProjectOrganoidTrapSubsystem::MakeRandomTransformInBounds(const FBox& Bounds, float EdgePadding, EProjectOrganoidTrapType Type)
{
	const FVector Min = Bounds.Min + FVector(EdgePadding, EdgePadding, 0.0f);
	const FVector Max = Bounds.Max - FVector(EdgePadding, EdgePadding, 0.0f);

	FVector Location;
	Location.X = TrapRandom.FRandRange(FMath::Min(Min.X, Max.X), FMath::Max(Min.X, Max.X));
	Location.Y = TrapRandom.FRandRange(FMath::Min(Min.Y, Max.Y), FMath::Max(Min.Y, Max.Y));
	Location.Z = Bounds.Min.Z + 2.0f;

	FRotator Rotation = FRotator::ZeroRotator;
	if (Type == EProjectOrganoidTrapType::LaserTripwire)
	{
		Rotation.Yaw = TrapRandom.FRandRange(0.0f, 360.0f);
		Location.Z = FMath::Lerp(Bounds.Min.Z, Bounds.Max.Z, 0.45f);
	}
	else if (Type == EProjectOrganoidTrapType::HazardEmitter)
	{
		Location.Z = FMath::Lerp(Bounds.Min.Z, Bounds.Max.Z, 0.35f);
	}

	return FTransform(Rotation, Location);
}

AProjectOrganoidTrapBase* UProjectOrganoidTrapSubsystem::SpawnTrap(const FProjectOrganoidTrapSpawnRequest& Request)
{
	UWorld* World = GetWorld();
	EnsureDefaultClasses();
	TSubclassOf<AProjectOrganoidTrapBase> ClassToSpawn = ResolveClassForType(Request.TrapType);
	if (!World || !*ClassToSpawn)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AProjectOrganoidTrapBase* Trap = World->SpawnActor<AProjectOrganoidTrapBase>(ClassToSpawn, Request.SpawnTransform, Params);
	if (!Trap)
	{
		return nullptr;
	}

	Trap->TrapType = Request.TrapType;
	Trap->CorridorId = Request.CorridorId;
	Trap->LinkedHazard = Request.LinkedHazard;
	Trap->TriggerDamage = Request.Damage;
	Trap->TriggerIntensity = Request.Intensity;
	Trap->SetArmed(true);

	SpawnedTraps.Add(Trap);
	OnTrapSpawned.Broadcast(Trap, Request.CorridorId);
	return Trap;
}

void UProjectOrganoidTrapSubsystem::ClearTrapsForCorridor(FName CorridorId)
{
	for (int32 Index = SpawnedTraps.Num() - 1; Index >= 0; --Index)
	{
		AProjectOrganoidTrapBase* Trap = SpawnedTraps[Index].Get();
		if (!Trap)
		{
			SpawnedTraps.RemoveAt(Index);
			continue;
		}

		if (CorridorId.IsNone() || Trap->CorridorId == CorridorId)
		{
			Trap->Destroy();
			SpawnedTraps.RemoveAt(Index);
		}
	}
}

int32 UProjectOrganoidTrapSubsystem::PopulateCorridor(AProjectOrganoidCorridorTrapVolume* Corridor)
{
	if (!Corridor)
	{
		return 0;
	}

	RegisterCorridor(Corridor);

	if (Corridor->CorridorId.IsNone())
	{
		Corridor->CorridorId = Corridor->GetFName();
	}

	if (Corridor->bClearExistingOnRepopulate)
	{
		ClearTrapsForCorridor(Corridor->CorridorId);
	}

	const FBox Bounds = Corridor->GetCorridorWorldBounds();
	int32 Spawned = 0;

	for (int32 Index = 0; Index < Corridor->DesiredTrapCount; ++Index)
	{
		FProjectOrganoidTrapSpawnRequest Request;
		Request.TrapType = RollTrapType(Corridor->SpawnWeights);
		Request.SpawnTransform = MakeRandomTransformInBounds(Bounds, Corridor->EdgePadding, Request.TrapType);
		Request.CorridorId = Corridor->CorridorId;

		switch (Request.TrapType)
		{
		case EProjectOrganoidTrapType::PressurePlate:
			Request.LinkedHazard = EProjectOrganoidHazardType::ExtremeHeat;
			Request.Damage = 30.0f;
			break;
		case EProjectOrganoidTrapType::HazardEmitter:
			Request.LinkedHazard = EProjectOrganoidHazardType::ToxicGas;
			Request.Damage = 12.0f;
			break;
		case EProjectOrganoidTrapType::LaserTripwire:
		default:
			Request.LinkedHazard = EProjectOrganoidHazardType::UVCRadiation;
			Request.Damage = 18.0f;
			break;
		}

		if (SpawnTrap(Request))
		{
			++Spawned;
		}
	}

	OnCorridorPopulated.Broadcast(Corridor->CorridorId, Spawned);
	return Spawned;
}

int32 UProjectOrganoidTrapSubsystem::PopulateAllCorridors()
{
	int32 Total = 0;
	for (const TWeakObjectPtr<AProjectOrganoidCorridorTrapVolume>& Entry : RegisteredCorridors)
	{
		if (AProjectOrganoidCorridorTrapVolume* Corridor = Entry.Get())
		{
			Total += PopulateCorridor(Corridor);
		}
	}
	return Total;
}

TArray<AProjectOrganoidTrapBase*> UProjectOrganoidTrapSubsystem::GetSpawnedTraps() const
{
	TArray<AProjectOrganoidTrapBase*> Result;
	for (const TWeakObjectPtr<AProjectOrganoidTrapBase>& Entry : SpawnedTraps)
	{
		if (AProjectOrganoidTrapBase* Trap = Entry.Get())
		{
			Result.Add(Trap);
		}
	}
	return Result;
}
