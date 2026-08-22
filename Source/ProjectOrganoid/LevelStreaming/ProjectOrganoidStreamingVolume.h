// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidStreamingVolume.generated.h"

class UBoxComponent;
class AProjectOrganoidCharacter;
class UProjectOrganoidLevelManagerSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidStreamingVolumeChanged, AProjectOrganoidCharacter*, Character);

/**
 *  Re-entrant volume that keeps Epitope's streaming partitions resident around the player.
 *
 *  Two authoring roles, same actor:
 *    Region volume — RegionContextTag set, requesting its own partition. Wraps a whole region
 *      and tells the world where Avery is. Must live on the persistent spine, or it would
 *      unload itself out from under the player.
 *    Seam band — RegionContextTag left as None, requesting the neighbouring partition. Placed
 *      roughly a room ahead of a seam so the neighbour is resident before it can be seen.
 *
 *  Overlapping raises a request and leaving lowers it; nothing here travels, teleports, fades,
 *  or fires once. Walking back and forth across a seam is free.
 */
UCLASS(Blueprintable)
class AProjectOrganoidStreamingVolume : public AActor
{
	GENERATED_BODY()

public:

	AProjectOrganoidStreamingVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** Partitions to keep resident while the player is inside, resolved via region definitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TArray<EProjectOrganoidSubLevelTag> RequestedRegions;

	/** Extra partitions by raw name, for anything without a region definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	TArray<FName> RequestedStreamingLevels;

	/**
	 *  If set, the player counts as standing in this region while overlapping — this is what
	 *  drives ambient hazard multipliers and the HUD location name. Leave as None for seam bands.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Streaming")
	EProjectOrganoidSubLevelTag RegionContextTag = EProjectOrganoidSubLevelTag::None;

	UPROPERTY(BlueprintAssignable, Category = "Streaming")
	FOnProjectOrganoidStreamingVolumeChanged OnPlayerEntered;

	UPROPERTY(BlueprintAssignable, Category = "Streaming")
	FOnProjectOrganoidStreamingVolumeChanged OnPlayerExited;

	/** Every partition this volume asks for, regions and raw names combined. */
	UFUNCTION(BlueprintPure, Category = "Streaming")
	TArray<FName> GetRequestedPartitions() const;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UProjectOrganoidLevelManagerSubsystem* GetLevelManager() const;

	void AcquireRequests(AProjectOrganoidCharacter* Character);
	void ReleaseRequests(AProjectOrganoidCharacter* Character);

	/** Guards against duplicate overlap pairs from multi-capsule pawns. */
	bool bPlayerInside = false;
};
