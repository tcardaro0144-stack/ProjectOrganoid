// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ProjectOrganoidHostCombatTypes.h"
#include "ProjectOrganoidHostAIController.generated.h"

class AProjectOrganoidHostBase;
class AProjectOrganoidCharacter;

/**
 *  Reusable Host combat controller.
 *  Idle → Investigate (sound/glimpse) → Pursue (confirmed sight) → Attack
 *  → Search (last known) → Return → Idle.
 *  Does not track Nathan without perception. Not a StateTree.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidHostAIController : public AAIController
{
	GENERATED_BODY()

public:

	AProjectOrganoidHostAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	EProjectOrganoidHostCombatState GetCombatState() const { return CombatState; }

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	FVector GetMoveTargetLocation() const { return MoveTargetLocation; }

	UFUNCTION(BlueprintPure, Category = "Host|AI")
	bool HasMoveTarget() const { return bHasMoveTarget; }

	/** Host vitals/preempt: stagger, blind, incap, death. */
	void HandleHostPreempt();

	/** Test/debug: investigate a world location without inventing pawn knowledge. */
	UFUNCTION(BlueprintCallable, Category = "Host|AI")
	void RequestInvestigateAt(const FVector& WorldLocation);

	/** Sets combat state without changing Think() rules. Used by tests and presence notify. */
	UFUNCTION(BlueprintCallable, Category = "Host|AI")
	void ApplyCombatState(EProjectOrganoidHostCombatState NewState);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Host|AI")
	EProjectOrganoidHostCombatState CombatState = EProjectOrganoidHostCombatState::Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "0.5"))
	float SearchDwellSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "0.5"))
	float ReturnTimeoutSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "20.0"))
	float ArrivalAcceptanceRadius = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Host|AI", meta = (ClampMin = "0.05"))
	float ThinkInterval = 0.12f;

	FTransform SpawnTransform = FTransform::Identity;
	FVector MoveTargetLocation = FVector::ZeroVector;
	FVector LastKnownLocation = FVector::ZeroVector;
	bool bHasMoveTarget = false;
	bool bHasLastKnownLocation = false;
	float SearchAge = 0.0f;
	float ReturnAge = 0.0f;
	float ThinkAccumulator = 0.0f;

	AProjectOrganoidHostBase* GetHost() const;
	AProjectOrganoidCharacter* GetPerceivedPlayer() const;
	void SetCombatState(EProjectOrganoidHostCombatState NewState);
	void Think();
	void StopHostMovement();
	bool MoveToWorldLocation(const FVector& WorldLocation);
	void FaceWorldLocation(const FVector& WorldLocation);
	bool IsAtLocation(const FVector& WorldLocation) const;
	bool HasNavigablePoint(const FVector& WorldLocation, FVector& OutProjected) const;
	void EnterIdle();
	void EnterInvestigate(const FVector& Location);
	void EnterPursue();
	void EnterAttack();
	void EnterSearch();
	void EnterReturn();
	void EnterDead();
};
