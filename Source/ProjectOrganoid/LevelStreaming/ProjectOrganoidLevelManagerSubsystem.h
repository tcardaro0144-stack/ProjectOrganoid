// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectOrganoidLevelTypes.h"
#include "ProjectOrganoidLevelManagerSubsystem.generated.h"

class AProjectOrganoidCharacter;
class AProjectOrganoidHazardZone;
class ULevelStreaming;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidSubLevelChanged, EProjectOrganoidSubLevelTag, NewTag, EProjectOrganoidSubLevelTag, PreviousTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidRegionStreamChanged, FName, StreamingLevelName, bool, bIsLoaded);

/** One streaming partition's outstanding requests and unload grace timer. */
USTRUCT()
struct FProjectOrganoidRegionStreamRecord
{
	GENERATED_BODY()

	/** Volumes currently asking for this partition. Empty means it may unload. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> Requesters;

	/** Last observed residency, used to detect edges worth broadcasting. */
	bool bWasLoaded = false;

	/** World time after which an unrequested partition may unload. 0 = not yet scheduled. */
	double UnloadEligibleTime = 0.0;
};

/**
 *  World subsystem owning Epitope's streaming residency and environmental context.
 *
 *  Epitope is one continuous facility. Partitions are a performance device only, so this
 *  subsystem never travels, teleports, or blocks: volumes raise and lower refcounted
 *  requests, and a reconcile pass drives ULevelStreaming towards the resulting desired set.
 *  Any number of partitions may be in flight at once.
 */
UCLASS()
class UProjectOrganoidLevelManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Registered region definitions (tag → partition name + ambient hazards) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	TArray<FProjectOrganoidSubLevelDefinition> SubLevelDefinitions;

	/**
	 *  Grace period before an unrequested partition unloads. Temporal hysteresis — without it
	 *  a player pacing a seam doorway thrashes the streamer.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Streaming", meta = (ClampMin = "0.0"))
	float UnloadGraceSeconds = 8.0f;

	/** How often the desired set is reconciled against the engine's streaming state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Streaming", meta = (ClampMin = "0.05"))
	float ReconcileIntervalSeconds = 0.25f;

	/** Soft budget. Exceeding it is legal but logged — it means seams are placed too close together. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Streaming", meta = (ClampMin = "1"))
	int32 MaxResidentRegions = 2;

	/**
	 *  Seconds spent easing hazard multipliers when the player crosses into a new region.
	 *  0 snaps. Only worth raising if a seam buffer carries ambient hazards on both sides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level|Hazards", meta = (ClampMin = "0.0"))
	float RegionBlendSeconds = 0.0f;

	/** Fires when the player physically enters a different region. Not a travel event. */
	UPROPERTY(BlueprintAssignable, Category = "Level")
	FOnProjectOrganoidSubLevelChanged OnSubLevelChanged;

	/** Fires when a partition finishes loading or finishes unloading. Diagnostics only. */
	UPROPERTY(BlueprintAssignable, Category = "Level|Streaming")
	FOnProjectOrganoidRegionStreamChanged OnRegionStreamChanged;

	UFUNCTION(BlueprintCallable, Category = "Level")
	void RegisterSubLevelDefinition(const FProjectOrganoidSubLevelDefinition& Definition);

	UFUNCTION(BlueprintPure, Category = "Level")
	bool GetSubLevelDefinition(EProjectOrganoidSubLevelTag Tag, FProjectOrganoidSubLevelDefinition& OutDefinition) const;

	UFUNCTION(BlueprintPure, Category = "Level")
	FName ResolveStreamingLevelName(EProjectOrganoidSubLevelTag Tag) const;

	// -- Residency ---------------------------------------------------------------------

	/** Raise a request for a partition. Idempotent per requester, safe to call every overlap. */
	UFUNCTION(BlueprintCallable, Category = "Level|Streaming")
	void AddStreamRequest(FName StreamingLevelName, AActor* Requester);

	/** Drop a request. The partition unloads only once nothing wants it and the grace elapses. */
	UFUNCTION(BlueprintCallable, Category = "Level|Streaming")
	void RemoveStreamRequest(FName StreamingLevelName, AActor* Requester);

	/** Drop every request held by one actor — call from EndPlay so destroyed volumes let go. */
	UFUNCTION(BlueprintCallable, Category = "Level|Streaming")
	void RemoveAllStreamRequestsFrom(AActor* Requester);

	UFUNCTION(BlueprintPure, Category = "Level|Streaming")
	EProjectOrganoidRegionStreamState GetRegionStreamState(FName StreamingLevelName) const;

	UFUNCTION(BlueprintPure, Category = "Level|Streaming")
	bool IsAnyRegionStreaming() const;

	UFUNCTION(BlueprintPure, Category = "Level|Streaming")
	int32 GetResidentRegionCount() const;

	/** Force a reconcile now rather than waiting for the next tick of the timer. */
	UFUNCTION(BlueprintCallable, Category = "Level|Streaming")
	void ReconcileStreamingNow();

	// -- Player region context ---------------------------------------------------------

	/**
	 *  Declare that the player is standing in a region. Sources stack, so a seam buffer
	 *  overlapping a region volume resolves to whichever was entered most recently.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level")
	void SetPlayerRegion(EProjectOrganoidSubLevelTag Tag, AActor* Source);

	/** Withdraw a region claim. Context reverts to the next valid source on the stack. */
	UFUNCTION(BlueprintCallable, Category = "Level")
	void ClearPlayerRegion(AActor* Source);

	UFUNCTION(BlueprintPure, Category = "Level")
	EProjectOrganoidSubLevelTag GetActiveSubLevelTag() const { return ActiveSubLevelTag; }

	// -- Environmental context ----------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	TArray<EProjectOrganoidHazardType> GetActiveAmbientHazards() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	float GetActiveDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	float GetActiveToxicityMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Level|Hazards")
	bool IsHazardTypeAmbient(EProjectOrganoidHazardType HazardType) const;

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void RegisterHazardZone(AProjectOrganoidHazardZone* HazardZone);

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void UnregisterHazardZone(AProjectOrganoidHazardZone* HazardZone);

	UFUNCTION(BlueprintCallable, Category = "Level|Hazards")
	void RefreshHazardZonesForActiveContext();

	// -- Debug -----------------------------------------------------------------------

	/**
	 *  Hard warp used by debug tooling and the flow manager's sector jump. This is the only
	 *  path that teleports Avery, and normal play must never reach it.
	 */
	UFUNCTION(BlueprintCallable, Category = "Level|Debug")
	bool RequestDebugWarpToRegion(
		AProjectOrganoidCharacter* Character,
		EProjectOrganoidSubLevelTag TargetTag,
		bool bTeleportToDestination,
		FTransform DestinationTransform);

protected:

	UPROPERTY(VisibleAnywhere, Category = "Level")
	EProjectOrganoidSubLevelTag ActiveSubLevelTag = EProjectOrganoidSubLevelTag::None;

	UPROPERTY()
	TMap<FName, FProjectOrganoidRegionStreamRecord> RegionStreams;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> RegionContextSources;

	UPROPERTY()
	TArray<EProjectOrganoidSubLevelTag> RegionContextTags;

	UPROPERTY()
	TArray<TWeakObjectPtr<AProjectOrganoidHazardZone>> RegisteredHazardZones;

	/** Warp requests hold a permanent request so a debug jump cannot unload itself. */
	UPROPERTY()
	TArray<FName> DebugWarpHeldLevels;

	UPROPERTY()
	TWeakObjectPtr<AProjectOrganoidCharacter> PendingWarpCharacter;

	FTransform PendingWarpTransform = FTransform::Identity;
	FName PendingWarpLevel = NAME_None;
	bool bPendingWarpTeleport = false;

	FTimerHandle ReconcileTimerHandle;
	FTimerHandle BlendRefreshTimerHandle;
	TSet<FName> WarnedMissingLevels;

	double RegionBlendStartTime = 0.0;
	float BlendFromDamageMultiplier = 1.0f;
	float BlendFromToxicityMultiplier = 1.0f;

	ULevelStreaming* FindStreamingLevel(FName StreamingLevelName) const;
	FProjectOrganoidRegionStreamRecord* FindOrAddRecord(FName StreamingLevelName);

	void SetActiveSubLevelTag(EProjectOrganoidSubLevelTag NewTag);
	void ResolveRegionContext();
	float GetBlendAlpha() const;
	void TickRegionBlend();

	void ReconcileStreaming();
	bool IsPartitionProtected(FName StreamingLevelName) const;
};
