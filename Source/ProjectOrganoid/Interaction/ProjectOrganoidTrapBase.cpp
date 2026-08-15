// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidTrapBase.h"
#include "ProjectOrganoidHazardInterface.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidTelemetrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AProjectOrganoidTrapBase::AProjectOrganoidTrapBase()
{
	PrimaryActorTick.bCanEverTick = false;
	TrapRoot = CreateDefaultSubobject<USceneComponent>(TEXT("TrapRoot"));
	SetRootComponent(TrapRoot);
}

void AProjectOrganoidTrapBase::SetArmed(bool bNewArmed)
{
	if (bArmed == bNewArmed)
	{
		return;
	}

	bArmed = bNewArmed;
	OnTrapArmedChanged.Broadcast(bArmed);
}

bool AProjectOrganoidTrapBase::CanTrigger() const
{
	if (!bArmed)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	return (World->GetTimeSeconds() - LastTriggerTime) >= RetriggerCooldown;
}

bool AProjectOrganoidTrapBase::TriggerTrap(AActor* TriggeringActor)
{
	if (!CanTrigger() || !TriggeringActor)
	{
		return false;
	}

	LastTriggerTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	ApplyTrapEffects(TriggeringActor);
	OnTrapTriggered.Broadcast(this, TriggeringActor);
	BP_OnTrapTriggered(TriggeringActor);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidTelemetrySubsystem* Telemetry = GI->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
		{
			Telemetry->ReportGameplayEvent(
				TEXT("TrapTriggered"),
				FString::Printf(TEXT("%s corridor=%s actor=%s"),
					*UEnum::GetValueAsString(TrapType),
					*CorridorId.ToString(),
					*GetNameSafe(TriggeringActor)));
		}
	}

	if (bSingleUse)
	{
		SetArmed(false);
	}

	return true;
}

void AProjectOrganoidTrapBase::ApplyTrapEffects(AActor* TriggeringActor)
{
	ApplyHazardToActor(TriggeringActor, TriggerDamage);
}

void AProjectOrganoidTrapBase::ApplyHazardToActor(AActor* Target, float DamageOverride) const
{
	if (!Target || LinkedHazard == EProjectOrganoidHazardType::None)
	{
		return;
	}

	const float Damage = DamageOverride >= 0.0f ? DamageOverride : TriggerDamage;

	if (Target->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(Target, LinkedHazard, TriggerIntensity);
		IProjectOrganoidHazardInterface::Execute_OnTickHazard(Target, LinkedHazard, Damage, 1.0f);
	}
	else if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Target))
	{
		IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(Character, LinkedHazard, TriggerIntensity);
		IProjectOrganoidHazardInterface::Execute_OnTickHazard(Character, LinkedHazard, Damage, 1.0f);
	}
}
