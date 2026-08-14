// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeaponModComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "UObject/SoftObjectPath.h"

UProjectOrganoidWeaponModComponent::UProjectOrganoidWeaponModComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AProjectOrganoidWeapon* UProjectOrganoidWeaponModComponent::GetOwnerWeapon() const
{
	return Cast<AProjectOrganoidWeapon>(GetOwner());
}

bool UProjectOrganoidWeaponModComponent::InstallMod(UProjectOrganoidWeaponModData* ModData, bool bReplaceExisting)
{
	if (!ModData)
	{
		return false;
	}

	const EProjectOrganoidWeaponModSlot Slot = ModData->Slot;
	if (EquippedMods.Contains(Slot))
	{
		if (!bReplaceExisting)
		{
			return false;
		}
		RemoveModFromSlot(Slot);
	}

	EquippedMods.Add(Slot, ModData);
	RecalculateCachedMultipliers();
	OnModInstalled.Broadcast(Slot, ModData);
	return true;
}

bool UProjectOrganoidWeaponModComponent::RemoveModFromSlot(EProjectOrganoidWeaponModSlot Slot)
{
	TObjectPtr<UProjectOrganoidWeaponModData>* Found = EquippedMods.Find(Slot);
	if (!Found || !(*Found))
	{
		return false;
	}

	UProjectOrganoidWeaponModData* Removed = Found->Get();
	EquippedMods.Remove(Slot);
	RecalculateCachedMultipliers();
	OnModRemoved.Broadcast(Slot, Removed);
	return true;
}

void UProjectOrganoidWeaponModComponent::ClearAllMods()
{
	TArray<EProjectOrganoidWeaponModSlot> Slots;
	EquippedMods.GetKeys(Slots);
	for (EProjectOrganoidWeaponModSlot Slot : Slots)
	{
		RemoveModFromSlot(Slot);
	}
}

UProjectOrganoidWeaponModData* UProjectOrganoidWeaponModComponent::GetModInSlot(EProjectOrganoidWeaponModSlot Slot) const
{
	if (const TObjectPtr<UProjectOrganoidWeaponModData>* Found = EquippedMods.Find(Slot))
	{
		return Found->Get();
	}
	return nullptr;
}

TArray<UProjectOrganoidWeaponModData*> UProjectOrganoidWeaponModComponent::GetInstalledMods() const
{
	TArray<UProjectOrganoidWeaponModData*> Result;
	for (const TPair<EProjectOrganoidWeaponModSlot, TObjectPtr<UProjectOrganoidWeaponModData>>& Pair : EquippedMods)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Value.Get());
		}
	}
	return Result;
}

bool UProjectOrganoidWeaponModComponent::HasSuppressor() const
{
	return bCachedHasSuppressor;
}

float UProjectOrganoidWeaponModComponent::GetDamageMultiplier() const
{
	return CachedDamageMultiplier;
}

float UProjectOrganoidWeaponModComponent::GetFireRateMultiplier() const
{
	return CachedFireRateMultiplier;
}

float UProjectOrganoidWeaponModComponent::GetPenetrationMultiplier() const
{
	return CachedPenetrationMultiplier;
}

float UProjectOrganoidWeaponModComponent::GetNoiseLoudnessMultiplier() const
{
	return CachedNoiseLoudnessMultiplier;
}

float UProjectOrganoidWeaponModComponent::GetNoiseRangeMultiplier() const
{
	return CachedNoiseRangeMultiplier;
}

void UProjectOrganoidWeaponModComponent::RecalculateCachedMultipliers()
{
	CachedDamageMultiplier = 1.0f;
	CachedFireRateMultiplier = 1.0f;
	CachedPenetrationMultiplier = 1.0f;
	CachedNoiseLoudnessMultiplier = 1.0f;
	CachedNoiseRangeMultiplier = 1.0f;
	bCachedHasSuppressor = false;

	for (const TPair<EProjectOrganoidWeaponModSlot, TObjectPtr<UProjectOrganoidWeaponModData>>& Pair : EquippedMods)
	{
		UProjectOrganoidWeaponModData* Mod = Pair.Value.Get();
		if (!Mod)
		{
			continue;
		}

		CachedDamageMultiplier *= Mod->DamageMultiplier;
		CachedFireRateMultiplier *= Mod->FireRateMultiplier;
		CachedPenetrationMultiplier *= Mod->PenetrationMultiplier;
		CachedNoiseLoudnessMultiplier *= Mod->NoiseLoudnessMultiplier;
		CachedNoiseRangeMultiplier *= Mod->NoiseRangeMultiplier;

		if (Mod->bSuppressesSoundEmission)
		{
			bCachedHasSuppressor = true;
		}
	}

	// Suppressed barrels hard-cap emission so hosts hear far less gunfire.
	if (bCachedHasSuppressor)
	{
		CachedNoiseLoudnessMultiplier = FMath::Min(CachedNoiseLoudnessMultiplier, SuppressorLoudnessCeiling);
		CachedNoiseRangeMultiplier = FMath::Min(CachedNoiseRangeMultiplier, SuppressorRangeCeiling);
	}
}

TArray<FProjectOrganoidSavedWeaponMod> UProjectOrganoidWeaponModComponent::CaptureModState() const
{
	TArray<FProjectOrganoidSavedWeaponMod> Result;
	for (const TPair<EProjectOrganoidWeaponModSlot, TObjectPtr<UProjectOrganoidWeaponModData>>& Pair : EquippedMods)
	{
		if (!Pair.Value)
		{
			continue;
		}

		FProjectOrganoidSavedWeaponMod Saved;
		Saved.Slot = Pair.Key;
		Saved.ModDataPath = FSoftObjectPath(Pair.Value.Get());
		Saved.ModId = Pair.Value->ModId;
		Result.Add(Saved);
	}
	return Result;
}

void UProjectOrganoidWeaponModComponent::ApplyModState(const TArray<FProjectOrganoidSavedWeaponMod>& SavedMods)
{
	ClearAllMods();

	for (const FProjectOrganoidSavedWeaponMod& Saved : SavedMods)
	{
		UObject* Loaded = Saved.ModDataPath.TryLoad();
		if (UProjectOrganoidWeaponModData* ModData = Cast<UProjectOrganoidWeaponModData>(Loaded))
		{
			InstallMod(ModData, true);
		}
	}
}
