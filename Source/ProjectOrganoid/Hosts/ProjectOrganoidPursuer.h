// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectOrganoidPursuer.generated.h"

class UBoxComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;

/** First authored pursuer appearance. Invulnerable, no weapon drop, slow, no NavMesh chase. */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidPursuer : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidPursuer();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void BeginEncounter();

	bool IsRevealed() const { return bRevealed; }
	bool HasPlayedBang() const { return bBangPlayed; }
	bool DropsWeapon() const { return false; }
	float GetSlowPlayRate() const { return SlowWalkPlayRate; }

protected:
	virtual void BeginPlay() override;

private:
	void PlayBang();
	void ShowNathanLine();
	void CompleteEncounterObjective();
	void Despawn();

	UPROPERTY(VisibleAnywhere, Category = "Pursuer")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Pursuer")
	TObjectPtr<USkeletalMeshComponent> Mesh;

	bool bRevealed = false;
	bool bBangPlayed = false;
	bool bEncounterConsumed = false;

	static constexpr float SlowWalkPlayRate = 0.45f;
	static constexpr float DespawnSeconds = 12.f;
	static constexpr float LineSeconds = 7.f;
};

/** Overlap in the Reactor viewing area. Fires the pursuer intro once. */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidPursuerTrigger : public AActor
{
	GENERATED_BODY()

public:
	AProjectOrganoidPursuerTrigger();

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

public:
	void NotifyPlayerOverlap(AActor* OtherActor);

private:
	UPROPERTY(VisibleAnywhere, Category = "Pursuer")
	TObjectPtr<UBoxComponent> Trigger;

	bool bConsumed = false;
};
