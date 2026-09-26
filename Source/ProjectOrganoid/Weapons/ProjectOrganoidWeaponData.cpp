// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidWeaponData.h"

namespace
{
	struct FRosterPath
	{
		const TCHAR* Id;
		const TCHAR* Path;
	};

	const FRosterPath RosterPaths[] = {
		{ TEXT("Weapon_BioStabilizerPistol"), TEXT("/Game/Data/Weapons/DA_Weapon_BioStabilizerPistol.DA_Weapon_BioStabilizerPistol") },
		{ TEXT("Weapon_PulseCarbine"), TEXT("/Game/Data/Weapons/DA_Weapon_PulseCarbine.DA_Weapon_PulseCarbine") },
		{ TEXT("Weapon_CryoInjector"), TEXT("/Game/Data/Weapons/DA_Weapon_CryoInjector.DA_Weapon_CryoInjector") },
		{ TEXT("Weapon_DenaturingShotgun"), TEXT("/Game/Data/Weapons/DA_Weapon_DenaturingShotgun.DA_Weapon_DenaturingShotgun") },
		{ TEXT("Weapon_IncineratorLance"), TEXT("/Game/Data/Weapons/DA_Weapon_IncineratorLance.DA_Weapon_IncineratorLance") }
	};
}

UProjectOrganoidWeaponData* UProjectOrganoidWeaponData::ResolveById(FName Id)
{
	const FString IdString = Id.ToString();
	for (const FRosterPath& Entry : RosterPaths)
	{
		if (IdString.Equals(Entry.Id, ESearchCase::CaseSensitive))
		{
			return LoadObject<UProjectOrganoidWeaponData>(nullptr, Entry.Path);
		}
	}
	return nullptr;
}
