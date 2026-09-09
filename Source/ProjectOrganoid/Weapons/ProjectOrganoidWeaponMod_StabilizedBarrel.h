// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidWeaponMod_StabilizedBarrel.generated.h"

/**
 *  Provisional pistol Barrel mod — campaign-compatible C++ definition.
 *  Soft path: /Script/ProjectOrganoid.ProjectOrganoidWeaponMod_StabilizedBarrel
 *  PROVISIONAL DESIGN TUNING: +5% damage. No magazine / ammo / reload change.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidWeaponMod_StabilizedBarrel : public UProjectOrganoidWeaponModData
{
	GENERATED_BODY()

public:

	UProjectOrganoidWeaponMod_StabilizedBarrel();

	static const TCHAR* ContentPath();
	static UProjectOrganoidWeaponModData* Resolve();
};
