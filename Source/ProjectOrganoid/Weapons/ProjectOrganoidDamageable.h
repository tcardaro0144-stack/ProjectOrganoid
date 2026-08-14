// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidDamageable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UProjectOrganoidDamageable : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Implemented by mutated hosts / organoid actors that react to ballistics
 *  and PE Tactical weak-point targeting.
 */
class IProjectOrganoidDamageable
{
	GENERATED_BODY()

public:

	/** Resolve which weak point (if any) was struck by this hit */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Organoid|Damage")
	EProjectOrganoidWeakPointType ResolveWeakPoint(const FHitResult& Hit) const;

	/** Apply resolved ballistic damage and optional dismember / incap triggers */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Organoid|Damage")
	void ApplyOrganoidHit(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser);
};
