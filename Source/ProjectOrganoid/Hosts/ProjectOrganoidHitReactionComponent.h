// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidHitReactionComponent.generated.h"

class USkeletalMeshComponent;
class UPhysicalMaterial;
class UMaterialInterface;
class UParticleSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidHitReaction, const FProjectOrganoidBallisticHit&, HitInfo, FName, ReactionTag, FVector, Impulse);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnProjectOrganoidDismemberment, EProjectOrganoidWeakPointType, Limb, FName, BoneName, FVector, Impulse);

USTRUCT(BlueprintType)
struct FProjectOrganoidGoreMaterialMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore")
	TObjectPtr<UPhysicalMaterial> PhysicalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore")
	TObjectPtr<UMaterialInterface> GoreDecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore")
	TObjectPtr<UParticleSystem> GoreFx = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gore", meta = (ClampMin = "0.1"))
	float ImpulseScale = 1.0f;
};

/**
 *  Combat hit-reaction + dismemberment director for organoid hosts.
 *  Applies directional impulses, picks material-based gore FX, and broadcasts limb events.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidHitReactionComponent();

	UPROPERTY(BlueprintAssignable, Category = "Combat|HitReaction")
	FOnProjectOrganoidHitReaction OnHitReaction;

	UPROPERTY(BlueprintAssignable, Category = "Combat|HitReaction")
	FOnProjectOrganoidDismemberment OnDismemberment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float BaseImpulseStrength = 40000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float DamageImpulseScale = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float TacticalImpulseMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction", meta = (ClampMin = "0.0"))
	float DismemberImpulseMultiplier = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|HitReaction")
	bool bApplyPhysicsImpulse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gore")
	TArray<FProjectOrganoidGoreMaterialMapping> GoreMappings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gore")
	TObjectPtr<UMaterialInterface> DefaultGoreDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Gore")
	TObjectPtr<UParticleSystem> DefaultGoreFx;

	/** Bone names preferred when dismembering LocomotorNerves */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dismember")
	TArray<FName> LocomotorBoneNames = { TEXT("thigh_l"), TEXT("thigh_r"), TEXT("calf_l"), TEXT("calf_r") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dismember")
	TArray<FName> OpticalBoneNames = { TEXT("head"), TEXT("neck_01") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Dismember")
	TArray<FName> CoreBoneNames = { TEXT("spine_03"), TEXT("spine_02") };

	UFUNCTION(BlueprintCallable, Category = "Combat|HitReaction")
	void ProcessBallisticHit(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser);

	UFUNCTION(BlueprintPure, Category = "Combat|HitReaction")
	FVector ComputeImpactImpulse(const FProjectOrganoidBallisticHit& HitInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|HitReaction")
	FName ResolveReactionTag(const FProjectOrganoidBallisticHit& HitInfo) const;

	UFUNCTION(BlueprintPure, Category = "Combat|Dismember")
	FName ResolveDismemberBone(EProjectOrganoidWeakPointType WeakPoint, FName PreferredBone) const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Gore")
	bool ResolveGoreForHit(const FProjectOrganoidBallisticHit& HitInfo, UMaterialInterface*& OutDecal, UParticleSystem*& OutFx, float& OutImpulseScale) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|HitReaction")
	void BP_OnHitReaction(const FProjectOrganoidBallisticHit& HitInfo, FName ReactionTag, FVector Impulse);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat|Dismember")
	void BP_OnDismemberment(EProjectOrganoidWeakPointType Limb, FName BoneName, FVector Impulse, UMaterialInterface* GoreDecal, UParticleSystem* GoreFx);

protected:

	USkeletalMeshComponent* ResolveMesh() const;
	void ApplyDirectionalImpulse(const FProjectOrganoidBallisticHit& HitInfo, const FVector& Impulse) const;
};
