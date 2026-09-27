// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidDamageable.h"
#include "ProjectOrganoidTransformedScientist.generated.h"

class UBoxComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;

/** First combat enemy. Slow, 100 health, no pursuer AI, vulnerable to Lytic and the roster. */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidTransformedScientist : public AActor, public IProjectOrganoidDamageable
{
	GENERATED_BODY()

public:
	AProjectOrganoidTransformedScientist();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;
	virtual EProjectOrganoidWeakPointType ResolveWeakPoint_Implementation(const FHitResult& Hit) const override;
	virtual void ApplyOrganoidHit_Implementation(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser) override;

	void BeginEncounter();
	void ApplyWeaponHit(float Damage, AActor* DamageCauser);

	bool IsRevealed() const { return bRevealed; }
	bool IsDead() const { return bDead; }
	bool UsesPursuerAI() const { return false; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetSlowPlayRate() const { return SlowWalkPlayRate; }
	int32 GetDroppedLyticCharges() const { return DroppedLyticCharges; }
	bool WasLyticChargeGranted() const { return bLyticChargeGranted; }

protected:
	virtual void BeginPlay() override;

private:
	void ShowNathanLine();
	void HandleDeath(AActor* DamageCauser);
	void GrantLyticCharge(AActor* DamageCauser);

	UPROPERTY(VisibleAnywhere, Category = "Transformed")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Transformed")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	float Health = 100.f;
	float MaxHealth = 100.f;
	int32 DroppedLyticCharges = 0;
	bool bRevealed = false;
	bool bDead = false;
	bool bEncounterConsumed = false;
	bool bLyticChargeGranted = false;

	static constexpr float SlowWalkPlayRate = 0.45f;
	static constexpr float LineSeconds = 7.f;
};

/** Overlap that reveals the transformed scientist once. */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidFirstCombatTrigger : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidFirstCombatTrigger();

	void NotifyPlayerOverlap(AActor* OtherActor);

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Transformed")
	TObjectPtr<UBoxComponent> Trigger;

	bool bConsumed = false;
};
