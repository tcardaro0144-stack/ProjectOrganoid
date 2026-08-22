// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHazardZone.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOrganoidStreaming, Log, All);

void UProjectOrganoidLevelManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (SubLevelDefinitions.Num() == 0)
	{
		// Default Epitope region profiles. These describe places, not destinations — what the
		// air is like and which breaker feeds them. Streaming names match the partitions
		// registered on the persistent Lvl_Epitope spine.
		auto AddDef = [this](EProjectOrganoidSubLevelTag Tag, FName LevelName, EProjectOrganoidHazardType Hazard, float DmgMul, float ToxMul)
		{
			FProjectOrganoidSubLevelDefinition Def;
			Def.Tag = Tag;
			Def.StreamingLevelName = LevelName;
			Def.AmbientHazardTypes.Add(Hazard);
			Def.AmbientDamageMultiplier = DmgMul;
			Def.AmbientToxicityMultiplier = ToxMul;
			SubLevelDefinitions.Add(Def);
		};

		AddDef(EProjectOrganoidSubLevelTag::SubLevel1_Admin, TEXT("SL_Epitope_Admin"), EProjectOrganoidHazardType::UVCRadiation, 1.0f, 1.0f);
		AddDef(EProjectOrganoidSubLevelTag::SubLevel2_NeuroGenetics, TEXT("SL_Epitope_NeuroGenetics"), EProjectOrganoidHazardType::ToxicGas, 1.1f, 1.25f);
		AddDef(EProjectOrganoidSubLevelTag::SubLevel3_Cryo, TEXT("SL_Epitope_Cryo"), EProjectOrganoidHazardType::LiquidN2Frost, 1.35f, 0.5f);
		AddDef(EProjectOrganoidSubLevelTag::SubLevel4_Compute, TEXT("SL_Epitope_Compute"), EProjectOrganoidHazardType::ToxicGas, 1.0f, 1.15f);
		AddDef(EProjectOrganoidSubLevelTag::SubLevel5_Reactor, TEXT("SL_Epitope_Reactor"), EProjectOrganoidHazardType::UVCRadiation, 1.5f, 1.4f);
		SubLevelDefinitions.Last().AmbientHazardTypes.Add(EProjectOrganoidHazardType::ToxicGas);
	}
}

void UProjectOrganoidLevelManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	InWorld.GetTimerManager().SetTimer(
		ReconcileTimerHandle,
		FTimerDelegate::CreateUObject(this, &UProjectOrganoidLevelManagerSubsystem::ReconcileStreaming),
		FMath::Max(0.05f, ReconcileIntervalSeconds),
		true);
}

void UProjectOrganoidLevelManagerSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReconcileTimerHandle);
		World->GetTimerManager().ClearTimer(BlendRefreshTimerHandle);
	}

	RegionStreams.Reset();
	RegionContextSources.Reset();
	RegionContextTags.Reset();
	RegisteredHazardZones.Reset();
	DebugWarpHeldLevels.Reset();

	Super::Deinitialize();
}

// -- Definitions -------------------------------------------------------------------------

void UProjectOrganoidLevelManagerSubsystem::RegisterSubLevelDefinition(const FProjectOrganoidSubLevelDefinition& Definition)
{
	for (FProjectOrganoidSubLevelDefinition& Existing : SubLevelDefinitions)
	{
		if (Existing.Tag == Definition.Tag)
		{
			Existing = Definition;
			return;
		}
	}
	SubLevelDefinitions.Add(Definition);
}

bool UProjectOrganoidLevelManagerSubsystem::GetSubLevelDefinition(EProjectOrganoidSubLevelTag Tag, FProjectOrganoidSubLevelDefinition& OutDefinition) const
{
	for (const FProjectOrganoidSubLevelDefinition& Def : SubLevelDefinitions)
	{
		if (Def.Tag == Tag)
		{
			OutDefinition = Def;
			return true;
		}
	}
	return false;
}

FName UProjectOrganoidLevelManagerSubsystem::ResolveStreamingLevelName(EProjectOrganoidSubLevelTag Tag) const
{
	FProjectOrganoidSubLevelDefinition Def;
	return GetSubLevelDefinition(Tag, Def) ? Def.StreamingLevelName : NAME_None;
}

// -- Residency ---------------------------------------------------------------------------

ULevelStreaming* UProjectOrganoidLevelManagerSubsystem::FindStreamingLevel(FName StreamingLevelName) const
{
	UWorld* World = GetWorld();
	if (!World || StreamingLevelName.IsNone())
	{
		return nullptr;
	}

	const FString Target = StreamingLevelName.ToString();
	for (ULevelStreaming* Streaming : World->GetStreamingLevels())
	{
		if (!Streaming)
		{
			continue;
		}

		const FString PackageName = Streaming->GetWorldAssetPackageName();
		if (PackageName == Target || FPackageName::GetShortName(PackageName) == Target)
		{
			return Streaming;
		}
	}

	return nullptr;
}

FProjectOrganoidRegionStreamRecord* UProjectOrganoidLevelManagerSubsystem::FindOrAddRecord(FName StreamingLevelName)
{
	if (StreamingLevelName.IsNone())
	{
		return nullptr;
	}
	return &RegionStreams.FindOrAdd(StreamingLevelName);
}

void UProjectOrganoidLevelManagerSubsystem::AddStreamRequest(FName StreamingLevelName, AActor* Requester)
{
	FProjectOrganoidRegionStreamRecord* Record = FindOrAddRecord(StreamingLevelName);
	if (!Record || !Requester)
	{
		return;
	}

	Record->Requesters.AddUnique(Requester);
	Record->UnloadEligibleTime = 0.0;

	ReconcileStreamingNow();
}

void UProjectOrganoidLevelManagerSubsystem::RemoveStreamRequest(FName StreamingLevelName, AActor* Requester)
{
	FProjectOrganoidRegionStreamRecord* Record = RegionStreams.Find(StreamingLevelName);
	if (!Record)
	{
		return;
	}

	Record->Requesters.RemoveAll([Requester](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Requester;
	});

	// Leave the grace timer to ReconcileStreaming so a re-entry within the window is free.
	ReconcileStreamingNow();
}

void UProjectOrganoidLevelManagerSubsystem::RemoveAllStreamRequestsFrom(AActor* Requester)
{
	for (TPair<FName, FProjectOrganoidRegionStreamRecord>& Pair : RegionStreams)
	{
		Pair.Value.Requesters.RemoveAll([Requester](const TWeakObjectPtr<AActor>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == Requester;
		});
	}

	ReconcileStreamingNow();
}

EProjectOrganoidRegionStreamState UProjectOrganoidLevelManagerSubsystem::GetRegionStreamState(FName StreamingLevelName) const
{
	ULevelStreaming* Streaming = FindStreamingLevel(StreamingLevelName);
	if (!Streaming)
	{
		return EProjectOrganoidRegionStreamState::Missing;
	}

	const bool bWantsLoaded = Streaming->ShouldBeLoaded();
	const bool bVisible = Streaming->IsLevelVisible();
	const bool bLoaded = Streaming->IsLevelLoaded();

	if (bWantsLoaded)
	{
		return (bLoaded && bVisible) ? EProjectOrganoidRegionStreamState::Loaded : EProjectOrganoidRegionStreamState::Loading;
	}

	return bLoaded ? EProjectOrganoidRegionStreamState::Unloading : EProjectOrganoidRegionStreamState::Unloaded;
}

bool UProjectOrganoidLevelManagerSubsystem::IsAnyRegionStreaming() const
{
	for (const TPair<FName, FProjectOrganoidRegionStreamRecord>& Pair : RegionStreams)
	{
		const EProjectOrganoidRegionStreamState State = GetRegionStreamState(Pair.Key);
		if (State == EProjectOrganoidRegionStreamState::Loading || State == EProjectOrganoidRegionStreamState::Unloading)
		{
			return true;
		}
	}
	return false;
}

int32 UProjectOrganoidLevelManagerSubsystem::GetResidentRegionCount() const
{
	int32 Count = 0;
	for (const TPair<FName, FProjectOrganoidRegionStreamRecord>& Pair : RegionStreams)
	{
		if (GetRegionStreamState(Pair.Key) == EProjectOrganoidRegionStreamState::Loaded)
		{
			++Count;
		}
	}
	return Count;
}

bool UProjectOrganoidLevelManagerSubsystem::IsPartitionProtected(FName StreamingLevelName) const
{
	// The region Avery is standing in can never be released, whatever the volumes say.
	if (StreamingLevelName == ResolveStreamingLevelName(ActiveSubLevelTag))
	{
		return true;
	}

	return DebugWarpHeldLevels.Contains(StreamingLevelName);
}

void UProjectOrganoidLevelManagerSubsystem::ReconcileStreamingNow()
{
	ReconcileStreaming();
}

void UProjectOrganoidLevelManagerSubsystem::ReconcileStreaming()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	int32 DesiredResident = 0;
	TArray<FName> Retired;

	for (TPair<FName, FProjectOrganoidRegionStreamRecord>& Pair : RegionStreams)
	{
		const FName LevelName = Pair.Key;
		FProjectOrganoidRegionStreamRecord& Record = Pair.Value;

		Record.Requesters.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

		const bool bDesired = Record.Requesters.Num() > 0 || IsPartitionProtected(LevelName);
		if (bDesired)
		{
			++DesiredResident;
			Record.UnloadEligibleTime = 0.0;
		}
		else if (Record.UnloadEligibleTime == 0.0)
		{
			Record.UnloadEligibleTime = Now + FMath::Max(0.0f, UnloadGraceSeconds);
		}

		ULevelStreaming* Streaming = FindStreamingLevel(LevelName);
		if (!Streaming)
		{
			if (bDesired && !WarnedMissingLevels.Contains(LevelName))
			{
				WarnedMissingLevels.Add(LevelName);
				UE_LOG(LogOrganoidStreaming, Warning,
					TEXT("Partition '%s' was requested but is not registered on the persistent level. Add it in Window > Levels."),
					*LevelName.ToString());
			}
			continue;
		}

		if (bDesired)
		{
			if (!Streaming->ShouldBeLoaded())
			{
				Streaming->SetShouldBeLoaded(true);
			}
			if (!Streaming->ShouldBeVisible())
			{
				Streaming->SetShouldBeVisible(true);
			}
		}
		else if (Now >= Record.UnloadEligibleTime)
		{
			if (Streaming->ShouldBeVisible())
			{
				Streaming->SetShouldBeVisible(false);
			}
			if (Streaming->ShouldBeLoaded())
			{
				Streaming->SetShouldBeLoaded(false);
			}
		}

		const bool bLoadedNow = GetRegionStreamState(LevelName) == EProjectOrganoidRegionStreamState::Loaded;
		if (bLoadedNow != Record.bWasLoaded)
		{
			Record.bWasLoaded = bLoadedNow;
			OnRegionStreamChanged.Broadcast(LevelName, bLoadedNow);

			if (bLoadedNow && bPendingWarpTeleport && LevelName == PendingWarpLevel)
			{
				if (AProjectOrganoidCharacter* Character = PendingWarpCharacter.Get())
				{
					Character->SetActorTransform(PendingWarpTransform, false, nullptr, ETeleportType::TeleportPhysics);
					if (AController* Controller = Character->GetController())
					{
						Controller->SetControlRotation(PendingWarpTransform.Rotator());
					}
				}

				bPendingWarpTeleport = false;
				PendingWarpCharacter.Reset();
				PendingWarpLevel = NAME_None;
			}
		}

		if (!bDesired && !bLoadedNow && Record.Requesters.Num() == 0)
		{
			Retired.Add(LevelName);
		}
	}

	for (const FName& LevelName : Retired)
	{
		RegionStreams.Remove(LevelName);
	}

	if (DesiredResident > MaxResidentRegions)
	{
		UE_LOG(LogOrganoidStreaming, Warning,
			TEXT("%d partitions requested at once (budget %d). Seam volumes are overlapping too heavily."),
			DesiredResident, MaxResidentRegions);
	}
}

// -- Player region context ----------------------------------------------------------------

void UProjectOrganoidLevelManagerSubsystem::SetPlayerRegion(EProjectOrganoidSubLevelTag Tag, AActor* Source)
{
	if (!Source || Tag == EProjectOrganoidSubLevelTag::None)
	{
		return;
	}

	const int32 Existing = RegionContextSources.IndexOfByKey(Source);
	if (Existing != INDEX_NONE)
	{
		RegionContextSources.RemoveAt(Existing);
		RegionContextTags.RemoveAt(Existing);
	}

	// Most recently entered claim wins, so a seam buffer inside a region resolves predictably.
	RegionContextSources.Add(Source);
	RegionContextTags.Add(Tag);

	ResolveRegionContext();
}

void UProjectOrganoidLevelManagerSubsystem::ClearPlayerRegion(AActor* Source)
{
	for (int32 Index = RegionContextSources.Num() - 1; Index >= 0; --Index)
	{
		if (!RegionContextSources[Index].IsValid() || RegionContextSources[Index].Get() == Source)
		{
			RegionContextSources.RemoveAt(Index);
			RegionContextTags.RemoveAt(Index);
		}
	}

	ResolveRegionContext();
}

void UProjectOrganoidLevelManagerSubsystem::ResolveRegionContext()
{
	for (int32 Index = RegionContextSources.Num() - 1; Index >= 0; --Index)
	{
		if (RegionContextSources[Index].IsValid())
		{
			SetActiveSubLevelTag(RegionContextTags[Index]);
			return;
		}

		RegionContextSources.RemoveAt(Index);
		RegionContextTags.RemoveAt(Index);
	}

	// Nothing claims the player — hold the last known region rather than snapping to None,
	// which would drop hazard context while crossing an unauthored gap.
}

void UProjectOrganoidLevelManagerSubsystem::SetActiveSubLevelTag(EProjectOrganoidSubLevelTag NewTag)
{
	if (ActiveSubLevelTag == NewTag)
	{
		return;
	}

	const EProjectOrganoidSubLevelTag Previous = ActiveSubLevelTag;

	BlendFromDamageMultiplier = GetActiveDamageMultiplier();
	BlendFromToxicityMultiplier = GetActiveToxicityMultiplier();
	ActiveSubLevelTag = NewTag;

	UWorld* World = GetWorld();
	RegionBlendStartTime = World ? World->GetTimeSeconds() : 0.0;

	OnSubLevelChanged.Broadcast(ActiveSubLevelTag, Previous);
	RefreshHazardZonesForActiveContext();

	if (World && RegionBlendSeconds > KINDA_SMALL_NUMBER)
	{
		World->GetTimerManager().SetTimer(
			BlendRefreshTimerHandle,
			FTimerDelegate::CreateUObject(this, &UProjectOrganoidLevelManagerSubsystem::TickRegionBlend),
			0.1f,
			true);
	}

	ReconcileStreamingNow();
}

float UProjectOrganoidLevelManagerSubsystem::GetBlendAlpha() const
{
	if (RegionBlendSeconds <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return 1.0f;
	}

	const double Elapsed = World->GetTimeSeconds() - RegionBlendStartTime;
	return FMath::Clamp(static_cast<float>(Elapsed) / RegionBlendSeconds, 0.0f, 1.0f);
}

void UProjectOrganoidLevelManagerSubsystem::TickRegionBlend()
{
	RefreshHazardZonesForActiveContext();

	if (GetBlendAlpha() >= 1.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(BlendRefreshTimerHandle);
		}
	}
}

// -- Environmental context ------------------------------------------------------------------

TArray<EProjectOrganoidHazardType> UProjectOrganoidLevelManagerSubsystem::GetActiveAmbientHazards() const
{
	FProjectOrganoidSubLevelDefinition Def;
	if (GetSubLevelDefinition(ActiveSubLevelTag, Def))
	{
		return Def.AmbientHazardTypes;
	}
	return TArray<EProjectOrganoidHazardType>();
}

float UProjectOrganoidLevelManagerSubsystem::GetActiveDamageMultiplier() const
{
	FProjectOrganoidSubLevelDefinition Def;
	const float Target = GetSubLevelDefinition(ActiveSubLevelTag, Def) ? Def.AmbientDamageMultiplier : 1.0f;
	return FMath::Lerp(BlendFromDamageMultiplier, Target, GetBlendAlpha());
}

float UProjectOrganoidLevelManagerSubsystem::GetActiveToxicityMultiplier() const
{
	FProjectOrganoidSubLevelDefinition Def;
	const float Target = GetSubLevelDefinition(ActiveSubLevelTag, Def) ? Def.AmbientToxicityMultiplier : 1.0f;
	return FMath::Lerp(BlendFromToxicityMultiplier, Target, GetBlendAlpha());
}

bool UProjectOrganoidLevelManagerSubsystem::IsHazardTypeAmbient(EProjectOrganoidHazardType HazardType) const
{
	return GetActiveAmbientHazards().Contains(HazardType);
}

void UProjectOrganoidLevelManagerSubsystem::RegisterHazardZone(AProjectOrganoidHazardZone* HazardZone)
{
	if (!HazardZone)
	{
		return;
	}

	RegisteredHazardZones.AddUnique(HazardZone);
	HazardZone->ApplySubLevelEnvironmentContext(
		ActiveSubLevelTag,
		GetActiveDamageMultiplier(),
		GetActiveToxicityMultiplier(),
		IsHazardTypeAmbient(HazardZone->HazardType));
}

void UProjectOrganoidLevelManagerSubsystem::UnregisterHazardZone(AProjectOrganoidHazardZone* HazardZone)
{
	RegisteredHazardZones.RemoveAll([HazardZone](const TWeakObjectPtr<AProjectOrganoidHazardZone>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == HazardZone;
	});
}

void UProjectOrganoidLevelManagerSubsystem::RefreshHazardZonesForActiveContext()
{
	const float DamageMul = GetActiveDamageMultiplier();
	const float ToxMul = GetActiveToxicityMultiplier();

	for (int32 Index = RegisteredHazardZones.Num() - 1; Index >= 0; --Index)
	{
		AProjectOrganoidHazardZone* Zone = RegisteredHazardZones[Index].Get();
		if (!Zone)
		{
			RegisteredHazardZones.RemoveAt(Index);
			continue;
		}

		Zone->ApplySubLevelEnvironmentContext(ActiveSubLevelTag, DamageMul, ToxMul, IsHazardTypeAmbient(Zone->HazardType));
	}
}

// -- Debug ------------------------------------------------------------------------------------

bool UProjectOrganoidLevelManagerSubsystem::RequestDebugWarpToRegion(
	AProjectOrganoidCharacter* Character,
	EProjectOrganoidSubLevelTag TargetTag,
	bool bTeleportToDestination,
	FTransform DestinationTransform)
{
	const FName LevelToLoad = ResolveStreamingLevelName(TargetTag);
	if (!GetWorld() || LevelToLoad.IsNone())
	{
		return false;
	}

	// A warp has no volume behind it, so hold the partition explicitly or the reconcile pass
	// would release it the moment the player's previous region volume lets go.
	DebugWarpHeldLevels.Empty();
	DebugWarpHeldLevels.Add(LevelToLoad);
	FindOrAddRecord(LevelToLoad);

	if (bTeleportToDestination && Character)
	{
		PendingWarpCharacter = Character;
		PendingWarpTransform = DestinationTransform;
		PendingWarpLevel = LevelToLoad;
		bPendingWarpTeleport = true;
	}

	SetActiveSubLevelTag(TargetTag);
	ReconcileStreamingNow();
	return true;
}
