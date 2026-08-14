// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDefaultWeapon.h"

AProjectOrganoidDefaultWeapon::AProjectOrganoidDefaultWeapon()
{
	Damage = 28.0f;
	Penetration = 0.25f;
	MaxPenetrations = 0;
	AmmoType = EProjectOrganoidAmmoType::Pistol;
	FireRate = 4.5f;
	BallisticsMode = EProjectOrganoidBallisticsMode::Hitscan;
	HitscanRange = 10000.0f;
	TacticalWeakPointDamageMultiplier = 2.5f;
}
