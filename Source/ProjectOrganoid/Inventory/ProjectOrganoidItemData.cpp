// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidItemData.h"
#include "UObject/Package.h"

UProjectOrganoidItemData* UProjectOrganoidItemData::CreateTransientPistolAmmo(UObject* Outer)
{
	UProjectOrganoidItemData* Ammo = NewObject<UProjectOrganoidItemData>(
		Outer ? Outer : GetTransientPackage(),
		NAME_None,
		RF_Transient);
	Ammo->ItemName = NSLOCTEXT("ProjectOrganoid", "PistolAmmoName", "Pistol Ammunition");
	Ammo->ItemType = EProjectOrganoidItemType::Ammo;
	Ammo->AmmoType = EProjectOrganoidAmmoType::Pistol;
	Ammo->GridWidth = 1;
	Ammo->GridHeight = 1;
	Ammo->bCanStack = true;
	// PROVISIONAL DESIGN TUNING — not campaign starting-reserve balance.
	Ammo->MaxStackCount = 30;
	Ammo->ItemWeight = 0.05f;
	return Ammo;
}
