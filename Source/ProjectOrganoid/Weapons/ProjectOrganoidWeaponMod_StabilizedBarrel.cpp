// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeaponMod_StabilizedBarrel.h"

UProjectOrganoidWeaponMod_StabilizedBarrel::UProjectOrganoidWeaponMod_StabilizedBarrel()
{
	ModId = TEXT("StabilizedBarrel");
	DisplayName = FText::FromString(TEXT("Stabilized Barrel"));
	Slot = EProjectOrganoidWeaponModSlot::Barrel;
	// PROVISIONAL DESIGN TUNING — conservative pistol benefit. Not final balance.
	DamageMultiplier = 1.05f;
	FireRateMultiplier = 1.0f;
	PenetrationMultiplier = 1.0f;
	NoiseLoudnessMultiplier = 1.0f;
	NoiseRangeMultiplier = 1.0f;
	bSuppressesSoundEmission = false;
	SOTInstallCost = 2;
}

const TCHAR* UProjectOrganoidWeaponMod_StabilizedBarrel::ContentPath()
{
	return TEXT("/Game/Data/Weapons/Mods/DA_Mod_StabilizedBarrel.DA_Mod_StabilizedBarrel");
}

UProjectOrganoidWeaponModData* UProjectOrganoidWeaponMod_StabilizedBarrel::Resolve()
{
	if (UProjectOrganoidWeaponModData* Asset = LoadObject<UProjectOrganoidWeaponModData>(nullptr, ContentPath()))
	{
		return Asset;
	}
	return GetMutableDefault<UProjectOrganoidWeaponMod_StabilizedBarrel>();
}
