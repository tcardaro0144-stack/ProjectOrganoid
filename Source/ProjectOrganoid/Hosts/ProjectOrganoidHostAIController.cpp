// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHostAIController.h"
#include "ProjectOrganoidHostBase.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidEncounterPresenceSubsystem.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

AProjectOrganoidHostAIController::AProjectOrganoidHostAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bAttachToPawn = true;
	bSetControlRotationFromPawnOrientation = true;
}

void AProjectOrganoidHostAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (InPawn)
	{
		SpawnTransform = InPawn->GetActorTransform();
	}

	if (AProjectOrganoidHostBase* Host = Cast<AProjectOrganoidHostBase>(InPawn))
	{
		Host->BindHostPerceptionToController();
	}

	EnterIdle();
}

void AProjectOrganoidHostAIController::OnUnPossess()
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>())
		{
			Presence->SetEncounterSourceActive(FName(*GetName()), false);
		}
	}
	StopHostMovement();
	Super::OnUnPossess();
}

void AProjectOrganoidHostAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>())
		{
			Presence->SetEncounterSourceActive(FName(*GetName()), false);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidHostAIController::ApplyCombatState(EProjectOrganoidHostCombatState NewState)
{
	SetCombatState(NewState);
}

void AProjectOrganoidHostAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ThinkAccumulator += DeltaTime;
	if (CombatState == EProjectOrganoidHostCombatState::Search)
	{
		SearchAge += DeltaTime;
	}
	if (CombatState == EProjectOrganoidHostCombatState::Return)
	{
		ReturnAge += DeltaTime;
	}

	if (ThinkAccumulator >= ThinkInterval)
	{
		ThinkAccumulator = 0.0f;
		Think();
	}
}

void AProjectOrganoidHostAIController::HandleHostPreempt()
{
	AProjectOrganoidHostBase* Host = GetHost();
	if (!Host)
	{
		return;
	}

	if (Host->bIsDead || Host->bIsIncapacitated)
	{
		EnterDead();
		return;
	}

	Host->CancelMeleeAttack();
	StopHostMovement();

	if (Host->bIsBlinded
		&& (CombatState == EProjectOrganoidHostCombatState::Pursue
			|| CombatState == EProjectOrganoidHostCombatState::Attack))
	{
		EnterSearch();
	}
}

void AProjectOrganoidHostAIController::RequestInvestigateAt(const FVector& WorldLocation)
{
	if (const AProjectOrganoidHostBase* Host = GetHost())
	{
		if (!Host->IsEncounterActivated())
		{
			return;
		}
	}

	if (CombatState == EProjectOrganoidHostCombatState::Dead
		|| CombatState == EProjectOrganoidHostCombatState::Pursue
		|| CombatState == EProjectOrganoidHostCombatState::Attack)
	{
		return;
	}

	EnterInvestigate(WorldLocation);
}

AProjectOrganoidHostBase* AProjectOrganoidHostAIController::GetHost() const
{
	return Cast<AProjectOrganoidHostBase>(GetPawn());
}

AProjectOrganoidCharacter* AProjectOrganoidHostAIController::GetPerceivedPlayer() const
{
	AProjectOrganoidHostBase* Host = GetHost();
	if (!Host || !Host->HasSightOnPlayer())
	{
		return nullptr;
	}

	return Cast<AProjectOrganoidCharacter>(Host->GetCurrentSightTarget());
}

void AProjectOrganoidHostAIController::SetCombatState(EProjectOrganoidHostCombatState NewState)
{
	if (CombatState == NewState)
	{
		return;
	}

	CombatState = NewState;
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidEncounterPresenceSubsystem* Presence = World->GetSubsystem<UProjectOrganoidEncounterPresenceSubsystem>())
		{
			Presence->NotifySourceCombatState(FName(*GetName()), NewState);
		}
	}
	if (AProjectOrganoidHostBase* Host = GetHost())
	{
		Host->BroadcastCombatState(NewState);
	}
}

void AProjectOrganoidHostAIController::StopHostMovement()
{
	StopMovement();
	bHasMoveTarget = false;
	if (AProjectOrganoidHostBase* Host = GetHost())
	{
		if (UCharacterMovementComponent* MoveComp = Host->GetCharacterMovement())
		{
			MoveComp->StopMovementImmediately();
		}
	}
}

bool AProjectOrganoidHostAIController::HasNavigablePoint(const FVector& WorldLocation, FVector& OutProjected) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	FNavLocation Projected;
	if (!NavSys->ProjectPointToNavigation(WorldLocation, Projected, FVector(200.0f, 200.0f, 400.0f)))
	{
		return false;
	}

	OutProjected = Projected.Location;
	return true;
}

bool AProjectOrganoidHostAIController::MoveToWorldLocation(const FVector& WorldLocation)
{
	FVector Projected = WorldLocation;
	if (!HasNavigablePoint(WorldLocation, Projected))
	{
		return false;
	}

	MoveTargetLocation = Projected;
	bHasMoveTarget = true;
	FaceWorldLocation(Projected);
	const FAIRequestID Request = MoveToLocation(
		Projected,
		ArrivalAcceptanceRadius,
		true,
		true,
		false,
		true,
		nullptr,
		true);
	return Request.IsValid();
}

void AProjectOrganoidHostAIController::FaceWorldLocation(const FVector& WorldLocation)
{
	APawn* HostPawn = GetPawn();
	if (!HostPawn)
	{
		return;
	}

	FVector Delta = WorldLocation - HostPawn->GetActorLocation();
	Delta.Z = 0.0f;
	if (Delta.SizeSquared() < 1.0f)
	{
		return;
	}

	const FRotator Yaw(0.0f, Delta.Rotation().Yaw, 0.0f);
	SetControlRotation(Yaw);
	HostPawn->SetActorRotation(Yaw);
}

bool AProjectOrganoidHostAIController::IsAtLocation(const FVector& WorldLocation) const
{
	const APawn* HostPawn = GetPawn();
	if (!HostPawn)
	{
		return false;
	}

	const FVector Delta = HostPawn->GetActorLocation() - WorldLocation;
	return Delta.Size2D() <= (ArrivalAcceptanceRadius + 40.0f);
}

void AProjectOrganoidHostAIController::EnterIdle()
{
	StopHostMovement();
	SetCombatState(EProjectOrganoidHostCombatState::Idle);
}

void AProjectOrganoidHostAIController::EnterInvestigate(const FVector& Location)
{
	SetCombatState(EProjectOrganoidHostCombatState::Investigate);
	MoveToWorldLocation(Location);
}

void AProjectOrganoidHostAIController::EnterPursue()
{
	SetCombatState(EProjectOrganoidHostCombatState::Pursue);
}

void AProjectOrganoidHostAIController::EnterAttack()
{
	StopHostMovement();
	SetCombatState(EProjectOrganoidHostCombatState::Attack);
}

void AProjectOrganoidHostAIController::EnterSearch()
{
	SearchAge = 0.0f;
	SetCombatState(EProjectOrganoidHostCombatState::Search);
	if (bHasLastKnownLocation)
	{
		MoveToWorldLocation(LastKnownLocation);
	}
}

void AProjectOrganoidHostAIController::EnterReturn()
{
	ReturnAge = 0.0f;
	SetCombatState(EProjectOrganoidHostCombatState::Return);
	MoveToWorldLocation(SpawnTransform.GetLocation());
}

void AProjectOrganoidHostAIController::EnterDead()
{
	if (AProjectOrganoidHostBase* Host = GetHost())
	{
		Host->CancelMeleeAttack();
	}
	StopHostMovement();
	SetCombatState(EProjectOrganoidHostCombatState::Dead);
}

void AProjectOrganoidHostAIController::Think()
{
	AProjectOrganoidHostBase* Host = GetHost();
	if (!Host)
	{
		return;
	}

	if (Host->bIsDead || Host->bIsIncapacitated)
	{
		EnterDead();
		return;
	}

	if (CombatState == EProjectOrganoidHostCombatState::Dead)
	{
		return;
	}

	AProjectOrganoidCharacter* SeenPlayer = GetPerceivedPlayer();
	if (!Host->IsEncounterActivated())
	{
		if (SeenPlayer && !Host->bIsBlinded)
		{
			Host->TryActivateEncounterFromProximity(SeenPlayer);
		}

		if (!Host->IsEncounterActivated())
		{
			if (CombatState != EProjectOrganoidHostCombatState::Idle || bHasMoveTarget)
			{
				EnterIdle();
			}
			return;
		}
	}

	if (Host->bIsStaggered)
	{
		Host->CancelMeleeAttack();
		StopHostMovement();
		return;
	}

	if (SeenPlayer && !Host->bIsBlinded)
	{
		LastKnownLocation = SeenPlayer->GetActorLocation();
		bHasLastKnownLocation = true;
		Host->RememberPerceivedPlayerLocation(LastKnownLocation);

		if (CombatState != EProjectOrganoidHostCombatState::Pursue
			&& CombatState != EProjectOrganoidHostCombatState::Attack)
		{
			EnterPursue();
		}

		if (Host->IsTargetInMeleeRange(SeenPlayer) && Host->HasValidMeleeLineOfSight(SeenPlayer))
		{
			if (CombatState != EProjectOrganoidHostCombatState::Attack)
			{
				EnterAttack();
			}
			Host->TryBeginMeleeAttack(SeenPlayer);
			return;
		}

		if (CombatState == EProjectOrganoidHostCombatState::Attack)
		{
			EnterPursue();
		}

		FaceWorldLocation(SeenPlayer->GetActorLocation());
		MoveToWorldLocation(SeenPlayer->GetActorLocation());
		return;
	}

	if (CombatState == EProjectOrganoidHostCombatState::Pursue
		|| CombatState == EProjectOrganoidHostCombatState::Attack)
	{
		EnterSearch();
		return;
	}

	if (Host->HasRecentNoiseStimulus())
	{
		const FVector NoiseLocation = Host->GetLastHeardNoiseLocation();
		if (CombatState != EProjectOrganoidHostCombatState::Investigate
			|| !IsAtLocation(NoiseLocation))
		{
			EnterInvestigate(NoiseLocation);
		}
		return;
	}

	if (CombatState == EProjectOrganoidHostCombatState::Investigate)
	{
		if (!bHasMoveTarget || IsAtLocation(MoveTargetLocation))
		{
			EnterSearch();
		}
		return;
	}

	if (CombatState == EProjectOrganoidHostCombatState::Search)
	{
		if (SearchAge >= SearchDwellSeconds)
		{
			EnterReturn();
		}
		return;
	}

	if (CombatState == EProjectOrganoidHostCombatState::Return)
	{
		if (IsAtLocation(SpawnTransform.GetLocation()) || ReturnAge >= ReturnTimeoutSeconds)
		{
			EnterIdle();
		}
		return;
	}
}
