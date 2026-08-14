// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UProjectOrganoidLevelManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (SubLevelDefinitions.Num() == 0)
	{
		// Default Epitope facility profiles (streaming names assigned in editor / BP)
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

void UProjectOrganoidLevelManagerSubsystem::Deinitialize()
{
	RegisteredHazardZones.Reset();
	Super::Deinitialize();
}

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
	return GetSubLevelDefinition(ActiveSubLevelTag, Def) ? Def.AmbientDamageMultiplier : 1.0f;
}

float UProjectOrganoidLevelManagerSubsystem::GetActiveToxicityMultiplier() const
{
	FProjectOrganoidSubLevelDefinition Def;
	return GetSubLevelDefinition(ActiveSubLevelTag, Def) ? Def.AmbientToxicityMultiplier : 1.0f;
}

bool UProjectOrganoidLevelManagerSubsystem::IsHazardTypeAmbient(EProjectOrganoidHazardType HazardType) const
{
	return GetActiveAmbientHazards().Contains(HazardType);
}

bool UProjectOrganoidLevelManagerSubsystem::AutoSaveCharacter(AProjectOrganoidCharacter* Character)
{
	if (!bAutoSaveOnTransition || !Character)
	{
		return false;
	}

	if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
		{
			return SaveSubsystem->SavePlayerProgress(Character, TransitionSaveSlot);
		}
	}
	return false;
}

void UProjectOrganoidLevelManagerSubsystem::SetActiveSubLevelTag(EProjectOrganoidSubLevelTag NewTag)
{
	if (ActiveSubLevelTag == NewTag)
	{
		RefreshHazardZonesForActiveContext();
		return;
	}

	const EProjectOrganoidSubLevelTag Previous = ActiveSubLevelTag;
	ActiveSubLevelTag = NewTag;
	OnSubLevelChanged.Broadcast(ActiveSubLevelTag, Previous);
	RefreshHazardZonesForActiveContext();
}

bool UProjectOrganoidLevelManagerSubsystem::RequestSubLevelTransition(
	AProjectOrganoidCharacter* Character,
	EProjectOrganoidSubLevelTag TargetTag,
	FName TargetStreamingLevelName,
	const TArray<FName>& StreamingLevelsToUnload,
	bool bMakeVisibleAfterLoad,
	bool bTeleportToDestination,
	FTransform DestinationTransform)
{
	UWorld* World = GetWorld();
	if (!World || bIsTransitioning)
	{
		return false;
	}

	FName LevelToLoad = TargetStreamingLevelName;
	if (LevelToLoad.IsNone())
	{
		FProjectOrganoidSubLevelDefinition Def;
		if (GetSubLevelDefinition(TargetTag, Def))
		{
			LevelToLoad = Def.StreamingLevelName;
		}
	}

	if (LevelToLoad.IsNone())
	{
		return false;
	}

	AutoSaveCharacter(Character);

	bIsTransitioning = true;
	PendingTargetTag = TargetTag;
	PendingUnloadLevels = StreamingLevelsToUnload;
	bPendingTeleport = bTeleportToDestination;
	PendingTeleportTransform = DestinationTransform;
	PendingTeleportCharacter = Character;

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName(TEXT("OnStreamLevelLoaded"));
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = ++PendingLoadCallbackId;

	UGameplayStatics::LoadStreamLevel(World, LevelToLoad, bMakeVisibleAfterLoad, false, LatentInfo);
	return true;
}

void UProjectOrganoidLevelManagerSubsystem::OnStreamLevelLoaded()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bIsTransitioning = false;
		return;
	}

	for (const FName& LevelName : PendingUnloadLevels)
	{
		if (LevelName.IsNone())
		{
			continue;
		}

		FLatentActionInfo UnloadInfo;
		UnloadInfo.CallbackTarget = this;
		UnloadInfo.ExecutionFunction = NAME_None;
		UnloadInfo.Linkage = 0;
		UnloadInfo.UUID = ++PendingLoadCallbackId;
		UGameplayStatics::UnloadStreamLevel(World, LevelName, UnloadInfo, false);
	}

	FinishTransition();
}

void UProjectOrganoidLevelManagerSubsystem::FinishTransition()
{
	SetActiveSubLevelTag(PendingTargetTag);

	if (bPendingTeleport && PendingTeleportCharacter.IsValid())
	{
		PendingTeleportCharacter->SetActorTransform(PendingTeleportTransform, false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* Controller = PendingTeleportCharacter->GetController())
		{
			Controller->SetControlRotation(PendingTeleportTransform.Rotator());
		}
	}

	bPendingTeleport = false;
	PendingTeleportCharacter.Reset();
	PendingUnloadLevels.Reset();
	bIsTransitioning = false;

	OnLevelTransitionFinished.Broadcast(ActiveSubLevelTag);
}

void UProjectOrganoidLevelManagerSubsystem::RegisterHazardZone(AProjectOrganoidHazardZone* HazardZone)
{
	if (!HazardZone)
	{
		return;
	}

	RegisteredHazardZones.AddUnique(HazardZone);
	HazardZone->ApplySubLevelEnvironmentContext(ActiveSubLevelTag, GetActiveDamageMultiplier(), GetActiveToxicityMultiplier(), IsHazardTypeAmbient(HazardZone->HazardType));
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

		const bool bAmbient = IsHazardTypeAmbient(Zone->HazardType);
		Zone->ApplySubLevelEnvironmentContext(ActiveSubLevelTag, DamageMul, ToxMul, bAmbient);
	}
}
