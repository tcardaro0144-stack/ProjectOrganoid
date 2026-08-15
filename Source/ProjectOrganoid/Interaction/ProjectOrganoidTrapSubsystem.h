// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidTrapTypes.h"
#include "ProjectOrganoidTrapSubsystem.generated.h"

class AProjectOrganoidTrapBase;
class AProjectOrganoidCorridorTrapVolume;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidTrapSpawned, AProjectOrganoidTrapBase*, Trap, FName, CorridorId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidCorridorPopulated, FName, CorridorId, int32, TrapCount);

/**
 *  Procedural hazard / trap director — fills registered corridor volumes with
 *  laser tripwires, pressure plates, and hazard emitters.
 */
UCLASS()
class UProjectOrganoidTrapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintAssignable, Category = "Trap")
	FOnProjectOrganoidTrapSpawned OnTrapSpawned;

	UPROPERTY(BlueprintAssignable, Category = "Trap")
	FOnProjectOrganoidCorridorPopulated OnCorridorPopulated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Classes")
	TSubclassOf<AProjectOrganoidTrapBase> LaserTripwireClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Classes")
	TSubclassOf<AProjectOrganoidTrapBase> PressurePlateClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Classes")
	TSubclassOf<AProjectOrganoidTrapBase> HazardEmitterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap", meta = (ClampMin = "0"))
	int32 RandomSeed = 0;

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void RegisterCorridor(AProjectOrganoidCorridorTrapVolume* Corridor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void UnregisterCorridor(AProjectOrganoidCorridorTrapVolume* Corridor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	int32 PopulateCorridor(AProjectOrganoidCorridorTrapVolume* Corridor);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	int32 PopulateAllCorridors();

	UFUNCTION(BlueprintCallable, Category = "Trap")
	AProjectOrganoidTrapBase* SpawnTrap(const FProjectOrganoidTrapSpawnRequest& Request);

	UFUNCTION(BlueprintCallable, Category = "Trap")
	void ClearTrapsForCorridor(FName CorridorId);

	UFUNCTION(BlueprintPure, Category = "Trap")
	TArray<AProjectOrganoidTrapBase*> GetSpawnedTraps() const;

protected:

	UPROPERTY()
	TArray<TWeakObjectPtr<AProjectOrganoidCorridorTrapVolume>> RegisteredCorridors;

	UPROPERTY()
	TArray<TWeakObjectPtr<AProjectOrganoidTrapBase>> SpawnedTraps;

	FRandomStream TrapRandom;

	void EnsureDefaultClasses();
	EProjectOrganoidTrapType RollTrapType(const FProjectOrganoidTrapSpawnWeights& Weights);
	TSubclassOf<AProjectOrganoidTrapBase> ResolveClassForType(EProjectOrganoidTrapType Type) const;
	FTransform MakeRandomTransformInBounds(const FBox& Bounds, float EdgePadding, EProjectOrganoidTrapType Type);
};
