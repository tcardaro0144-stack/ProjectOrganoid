// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidTrapTypes.h"
#include "ProjectOrganoidCorridorTrapVolume.generated.h"

class UBoxComponent;

/**
 *  Marks a sector corridor for procedural trap population by UProjectOrganoidTrapSubsystem.
 */
UCLASS(Blueprintable)
class AProjectOrganoidCorridorTrapVolume : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidCorridorTrapVolume();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CorridorBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor")
	FName CorridorId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor", meta = (ClampMin = "0", ClampMax = "32"))
	int32 DesiredTrapCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor")
	FProjectOrganoidTrapSpawnWeights SpawnWeights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor")
	bool bPopulateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor")
	bool bClearExistingOnRepopulate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trap|Corridor", meta = (ClampMin = "50.0"))
	float EdgePadding = 80.0f;

	UFUNCTION(BlueprintCallable, Category = "Trap|Corridor")
	int32 PopulateTraps();

	UFUNCTION(BlueprintPure, Category = "Trap|Corridor")
	FBox GetCorridorWorldBounds() const;
};
