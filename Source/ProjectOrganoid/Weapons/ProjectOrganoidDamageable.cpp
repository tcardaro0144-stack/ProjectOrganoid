// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidDamageable.h"

EProjectOrganoidWeakPointType IProjectOrganoidDamageable::ResolveWeakPoint_Implementation(const FHitResult& Hit) const
{
	// Component tags / bone names can be remapped in Blueprint overrides.
	if (Hit.Component.IsValid())
	{
		if (Hit.Component->ComponentHasTag(TEXT("OrganoidCore")) || Hit.Component->ComponentHasTag(TEXT("BioCore")))
		{
			return EProjectOrganoidWeakPointType::OrganoidCore;
		}
		if (Hit.Component->ComponentHasTag(TEXT("LocomotorNerves")))
		{
			return EProjectOrganoidWeakPointType::LocomotorNerves;
		}
		if (Hit.Component->ComponentHasTag(TEXT("OpticalNodes")))
		{
			return EProjectOrganoidWeakPointType::OpticalNodes;
		}
	}

	if (Hit.BoneName == TEXT("OrganoidCore") || Hit.BoneName == TEXT("core"))
	{
		return EProjectOrganoidWeakPointType::OrganoidCore;
	}
	if (Hit.BoneName == TEXT("LocomotorNerves") || Hit.BoneName == TEXT("spine_01"))
	{
		return EProjectOrganoidWeakPointType::LocomotorNerves;
	}
	if (Hit.BoneName == TEXT("OpticalNodes") || Hit.BoneName == TEXT("head"))
	{
		return EProjectOrganoidWeakPointType::OpticalNodes;
	}

	return EProjectOrganoidWeakPointType::None;
}

void IProjectOrganoidDamageable::ApplyOrganoidHit_Implementation(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser)
{
	// Default no-op — host Blueprints / C++ subclasses implement HP, VFX, and gibs.
}
