// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHitReactionComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInterface.h"
#include "Particles/ParticleSystem.h"

UProjectOrganoidHitReactionComponent::UProjectOrganoidHitReactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UProjectOrganoidHitReactionComponent::ProcessBallisticHit(const FProjectOrganoidBallisticHit& HitInfo, AActor* /*DamageCauser*/)
{
	FVector Impulse = ComputeImpactImpulse(HitInfo);
	float GoreImpulseScale = 1.0f;
	UMaterialInterface* GoreDecal = nullptr;
	UParticleSystem* GoreFx = nullptr;
	ResolveGoreForHit(HitInfo, GoreDecal, GoreFx, GoreImpulseScale);
	Impulse *= GoreImpulseScale;

	const FName ReactionTag = ResolveReactionTag(HitInfo);
	ApplyDirectionalImpulse(HitInfo, Impulse);
	OnHitReaction.Broadcast(HitInfo, ReactionTag, Impulse);
	BP_OnHitReaction(HitInfo, ReactionTag, Impulse);

	if (HitInfo.bTriggeredDismemberment || HitInfo.WeakPoint != EProjectOrganoidWeakPointType::None)
	{
		const EProjectOrganoidWeakPointType Limb = HitInfo.bTriggeredDismemberment
			? (HitInfo.WeakPoint != EProjectOrganoidWeakPointType::None
				? HitInfo.WeakPoint
				: EProjectOrganoidWeakPointType::LocomotorNerves)
			: HitInfo.WeakPoint;

		if (HitInfo.bTriggeredDismemberment || HitInfo.bTacticalModeHit)
		{
			const FName Bone = ResolveDismemberBone(Limb, HitInfo.HitBoneName);
			const FVector DismemberImpulse = Impulse * DismemberImpulseMultiplier;
			OnDismemberment.Broadcast(Limb, Bone, DismemberImpulse);
			BP_OnDismemberment(Limb, Bone, DismemberImpulse, GoreDecal, GoreFx);
		}
	}
}

FVector UProjectOrganoidHitReactionComponent::ComputeImpactImpulse(const FProjectOrganoidBallisticHit& HitInfo) const
{
	FVector Direction = HitInfo.ImpactDirection;
	if (Direction.IsNearlyZero())
	{
		Direction = -HitInfo.ImpactNormal;
	}
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}
	Direction.Normalize();

	float Strength = BaseImpulseStrength + HitInfo.FinalDamage * DamageImpulseScale;
	if (HitInfo.ImpulseStrength > KINDA_SMALL_NUMBER)
	{
		Strength = HitInfo.ImpulseStrength;
	}
	if (HitInfo.bTacticalModeHit)
	{
		Strength *= TacticalImpulseMultiplier;
	}
	if (HitInfo.bTriggeredDismemberment)
	{
		Strength *= DismemberImpulseMultiplier;
	}

	return Direction * Strength;
}

FName UProjectOrganoidHitReactionComponent::ResolveReactionTag(const FProjectOrganoidBallisticHit& HitInfo) const
{
	if (HitInfo.bTriggeredDismemberment)
	{
		return TEXT("Dismember");
	}
	if (HitInfo.bTriggeredIncapacitation)
	{
		return TEXT("Incapacitate");
	}

	switch (HitInfo.WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		return TEXT("Stagger_Locomotor");
	case EProjectOrganoidWeakPointType::OpticalNodes:
		return TEXT("Flinch_Optical");
	case EProjectOrganoidWeakPointType::OrganoidCore:
		return TEXT("Critical_Core");
	default:
		break;
	}

	const FVector Dir = HitInfo.ImpactDirection.GetSafeNormal();
	if (!Dir.IsNearlyZero())
	{
		if (FMath::Abs(Dir.Z) > 0.65f)
		{
			return Dir.Z > 0.0f ? TEXT("Flinch_Up") : TEXT("Flinch_Down");
		}
		if (FMath::Abs(Dir.Y) > FMath::Abs(Dir.X))
		{
			return Dir.Y > 0.0f ? TEXT("Flinch_Right") : TEXT("Flinch_Left");
		}
		return Dir.X > 0.0f ? TEXT("Flinch_Front") : TEXT("Flinch_Back");
	}

	return TEXT("Flinch");
}

FName UProjectOrganoidHitReactionComponent::ResolveDismemberBone(EProjectOrganoidWeakPointType WeakPoint, FName PreferredBone) const
{
	if (PreferredBone != NAME_None)
	{
		return PreferredBone;
	}

	const TArray<FName>* Pool = nullptr;
	switch (WeakPoint)
	{
	case EProjectOrganoidWeakPointType::LocomotorNerves:
		Pool = &LocomotorBoneNames;
		break;
	case EProjectOrganoidWeakPointType::OpticalNodes:
		Pool = &OpticalBoneNames;
		break;
	case EProjectOrganoidWeakPointType::OrganoidCore:
		Pool = &CoreBoneNames;
		break;
	default:
		Pool = &LocomotorBoneNames;
		break;
	}

	if (Pool && Pool->Num() > 0)
	{
		return (*Pool)[FMath::RandRange(0, Pool->Num() - 1)];
	}

	return NAME_None;
}

bool UProjectOrganoidHitReactionComponent::ResolveGoreForHit(
	const FProjectOrganoidBallisticHit& HitInfo,
	UMaterialInterface*& OutDecal,
	UParticleSystem*& OutFx,
	float& OutImpulseScale) const
{
	OutDecal = DefaultGoreDecalMaterial;
	OutFx = DefaultGoreFx;
	OutImpulseScale = 1.0f;

	UPhysicalMaterial* PhysMat = nullptr;
	if (HitInfo.HitComponent)
	{
		PhysMat = HitInfo.HitComponent->GetBodyInstance()
			? HitInfo.HitComponent->GetBodyInstance()->GetSimplePhysicalMaterial()
			: nullptr;
	}

	for (const FProjectOrganoidGoreMaterialMapping& Mapping : GoreMappings)
	{
		if (Mapping.PhysicalMaterial && Mapping.PhysicalMaterial == PhysMat)
		{
			OutDecal = Mapping.GoreDecalMaterial ? Mapping.GoreDecalMaterial.Get() : OutDecal;
			OutFx = Mapping.GoreFx ? Mapping.GoreFx.Get() : OutFx;
			OutImpulseScale = Mapping.ImpulseScale;
			return true;
		}
	}

	return OutDecal != nullptr || OutFx != nullptr;
}

USkeletalMeshComponent* UProjectOrganoidHitReactionComponent::ResolveMesh() const
{
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		return Character->GetMesh();
	}
	return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

void UProjectOrganoidHitReactionComponent::ApplyDirectionalImpulse(const FProjectOrganoidBallisticHit& HitInfo, const FVector& Impulse) const
{
	if (!bApplyPhysicsImpulse || Impulse.IsNearlyZero())
	{
		return;
	}

	USkeletalMeshComponent* Mesh = ResolveMesh();
	if (!Mesh)
	{
		return;
	}

	if (HitInfo.HitBoneName != NAME_None)
	{
		Mesh->AddImpulseAtLocation(Impulse, HitInfo.ImpactPoint, HitInfo.HitBoneName);
	}
	else
	{
		Mesh->AddImpulseAtLocation(Impulse, HitInfo.ImpactPoint);
	}
}
