// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidHazardInterface.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "ProjectOrganoidPowerSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

AProjectOrganoidHazardZone::AProjectOrganoidHazardZone()
{
	PrimaryActorTick.bCanEverTick = true;

	HazardVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("HazardVolume"));
	HazardVolume->InitBoxExtent(FVector(200.0f, 200.0f, 150.0f));
	HazardVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	HazardVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	HazardVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HazardVolume->SetGenerateOverlapEvents(true);
	SetRootComponent(HazardVolume);

	HazardVolume->OnComponentBeginOverlap.AddDynamic(this, &AProjectOrganoidHazardZone::OnHazardBeginOverlap);
	HazardVolume->OnComponentEndOverlap.AddDynamic(this, &AProjectOrganoidHazardZone::OnHazardEndOverlap);

	HazardBeacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HazardBeacon"));
	HazardBeacon->SetupAttachment(HazardVolume);
	HazardBeacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HazardBeacon->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
	HazardBeacon->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		HazardBeacon->SetStaticMesh(CubeMesh.Object);
	}

	HazardLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HazardLight"));
	HazardLight->SetupAttachment(HazardVolume);
	HazardLight->SetIntensity(900.0f);
	HazardLight->SetAttenuationRadius(900.0f);
	HazardLight->SetCastShadows(false);

	HazardAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HazardAudio"));
	HazardAudio->SetupAttachment(HazardVolume);
	HazardAudio->bAutoActivate = false;
	HazardAudio->bAllowSpatialization = true;
	HazardAudio->bOverrideAttenuation = true;
	HazardAudio->AttenuationOverrides.bAttenuate = true;
	HazardAudio->AttenuationOverrides.FalloffDistance = 2200.0f;

	HazardLoopSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/Ambient/SW_HazardHiss.SW_HazardHiss")));

	ApplyHazardDefaultsForType();
}

void AProjectOrganoidHazardZone::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidLevelManagerSubsystem* LevelManager = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			LevelManager->RegisterHazardZone(this);
		}

		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->OnSectorPowerChanged.AddDynamic(this, &AProjectOrganoidHazardZone::HandleSectorPowerChanged);
		}
	}

	RefreshPowerGating();
	RefreshPresentation();
}

void AProjectOrganoidHazardZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidLevelManagerSubsystem* LevelManager = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			LevelManager->UnregisterHazardZone(this);
		}

		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			Power->OnSectorPowerChanged.RemoveDynamic(this, &AProjectOrganoidHazardZone::HandleSectorPowerChanged);
		}
	}

	if (HazardAudio)
	{
		HazardAudio->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidHazardZone::ApplySubLevelEnvironmentContext(
	EProjectOrganoidSubLevelTag ActiveTag,
	float DamageMultiplier,
	float ToxicityMultiplier,
	bool bHazardTypeIsAmbient)
{
	EnvironmentDamageMultiplier = DamageMultiplier;
	EnvironmentToxicityMultiplier = ToxicityMultiplier;

	const bool bWasEffective = IsEffectivelyActive();

	if (!bIgnoreSubLevelContext)
	{
		if (AssociatedSubLevelTag == EProjectOrganoidSubLevelTag::None)
		{
			bIsActive = bHazardTypeIsAmbient || ActiveTag == EProjectOrganoidSubLevelTag::None;
		}
		else
		{
			bIsActive = (AssociatedSubLevelTag == ActiveTag) || bHazardTypeIsAmbient;
		}
	}

	if (bWasEffective != IsEffectivelyActive())
	{
		if (IsEffectivelyActive())
		{
			NotifyOccupantsEnter();
		}
		else
		{
			NotifyOccupantsExit();
		}
	}

	RefreshPresentation();
}

void AProjectOrganoidHazardZone::ClearHazardVolume()
{
	NotifyOccupantsExit();

	bIsActive = false;
	OccupyingActors.Reset();
	BP_OnHazardCleared();

	if (HazardVolume)
	{
		HazardVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HazardVolume->SetHiddenInGame(true);
	}

	RefreshPresentation();
}

void AProjectOrganoidHazardZone::ApplyHazardDefaultsForType()
{
	switch (HazardType)
	{
	case EProjectOrganoidHazardType::UVCRadiation:
		DamagePerSecond = 12.0f;
		ToxicityPerSecond = 2.0f;
		HeartRateSpikePerSecond = 6.0f;
		bRequiresSectorOnline = true;
		break;
	case EProjectOrganoidHazardType::LiquidN2Frost:
		DamagePerSecond = 15.0f;
		ToxicityPerSecond = 0.0f;
		HeartRateSpikePerSecond = 8.0f;
		bIntensifyDuringBlackout = true;
		break;
	case EProjectOrganoidHazardType::ToxicGas:
		DamagePerSecond = 4.0f;
		ToxicityPerSecond = 12.0f;
		HeartRateSpikePerSecond = 5.0f;
		break;
	case EProjectOrganoidHazardType::Biohazard:
		DamagePerSecond = 6.0f;
		ToxicityPerSecond = 14.0f;
		HeartRateSpikePerSecond = 7.0f;
		break;
	case EProjectOrganoidHazardType::ExtremeHeat:
		DamagePerSecond = 18.0f;
		ToxicityPerSecond = 0.0f;
		HeartRateSpikePerSecond = 10.0f;
		bRequiresSectorOnline = true;
		break;
	default:
		break;
	}
}

void AProjectOrganoidHazardZone::OnHazardBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		return;
	}

	OccupyingActors.Add(OtherActor);

	if (IsEffectivelyActive())
	{
		IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(OtherActor, HazardType, HazardIntensity * PowerDamageScale);
	}

	RefreshPresentation();
}

void AProjectOrganoidHazardZone::OnHazardEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	OccupyingActors.Remove(OtherActor);

	if (OtherActor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		IProjectOrganoidHazardInterface::Execute_OnExitedHazard(OtherActor, HazardType);
	}

	RefreshPresentation();
}

void AProjectOrganoidHazardZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsEffectivelyActive() || OccupyingActors.Num() == 0)
	{
		return;
	}

	TArray<AActor*> Actors = OccupyingActors.Array();
	for (AActor* Actor : Actors)
	{
		if (IsValid(Actor))
		{
			ApplyHazardToActor(Actor, DeltaSeconds);
		}
		else
		{
			OccupyingActors.Remove(Actor);
		}
	}
}

float AProjectOrganoidHazardZone::ComputeTickDamageAmount(float DeltaSeconds) const
{
	return DamagePerSecond * EnvironmentDamageMultiplier * HazardIntensity * PowerDamageScale * DeltaSeconds;
}

void AProjectOrganoidHazardZone::ApplyHazardToActor(AActor* Actor, float DeltaSeconds)
{
	if (!Actor || !Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
	{
		return;
	}

	const float TickDamage = ComputeTickDamageAmount(DeltaSeconds);
	IProjectOrganoidHazardInterface::Execute_OnTickHazard(Actor, HazardType, TickDamage, DeltaSeconds);

	if (AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(Actor))
	{
		OnHazardApplied.Broadcast(Character, HazardType);
	}
}

bool AProjectOrganoidHazardZone::IsEffectivelyActive() const
{
	return bIsActive && bPowerAllowsOperation;
}

void AProjectOrganoidHazardZone::HandleSectorPowerChanged(
	EProjectOrganoidPowerSector Sector,
	EProjectOrganoidPowerState NewState,
	EProjectOrganoidPowerState PreviousState)
{
	if (Sector != EProjectOrganoidPowerSector::FacilityWide && Sector != PowerSector)
	{
		return;
	}

	(void)NewState;
	(void)PreviousState;
	RefreshPowerGating();
}

void AProjectOrganoidHazardZone::RefreshPowerGating()
{
	const bool bWasEffective = IsEffectivelyActive();
	EProjectOrganoidPowerState State = EProjectOrganoidPowerState::Online;

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidPowerSubsystem* Power = World->GetSubsystem<UProjectOrganoidPowerSubsystem>())
		{
			State = Power->GetSectorPowerState(PowerSector);
			if (Power->GetSectorPowerState(EProjectOrganoidPowerSector::FacilityWide) == EProjectOrganoidPowerState::Blackout)
			{
				State = EProjectOrganoidPowerState::Blackout;
			}
		}
	}

	bPowerAllowsOperation = !bRequiresSectorOnline || State == EProjectOrganoidPowerState::Online;
	PowerDamageScale = (bIntensifyDuringBlackout && State == EProjectOrganoidPowerState::Blackout)
		? BlackoutIntensityScale
		: 1.0f;

	if (bWasEffective != IsEffectivelyActive())
	{
		if (IsEffectivelyActive())
		{
			NotifyOccupantsEnter();
		}
		else
		{
			NotifyOccupantsExit();
		}
	}

	RefreshPresentation();
}

void AProjectOrganoidHazardZone::RefreshPresentation()
{
	const bool bShow = IsEffectivelyActive();
	const FLinearColor Color = ColorForHazardType();

	if (HazardBeacon)
	{
		HazardBeacon->SetVisibility(bShowHazardBeacon && bShow);
		HazardBeacon->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
	}

	if (HazardLight)
	{
		HazardLight->SetLightColor(Color);
		HazardLight->SetVisibility(bShow);
		HazardLight->SetIntensity(bShow ? 900.0f : 0.0f);
	}

	if (HazardAudio)
	{
		if (USoundBase* Loop = HazardLoopSound.LoadSynchronous())
		{
			if (HazardAudio->GetSound() != Loop)
			{
				HazardAudio->SetSound(Loop);
			}
		}

		HazardAudio->SetVolumeMultiplier(HazardLoopVolume);

		// Local telegraph only. Playing whenever the volume is powered made
		// SW_HazardHiss (dark ventilation bed, 2200uu falloff) audible
		// across Public Admin as a recurring occupancy leak.
		const bool bShouldPlayAudio = bShow && OccupyingActors.Num() > 0;
		if (bShouldPlayAudio)
		{
			if (!HazardAudio->IsPlaying())
			{
				HazardAudio->Play();
			}
		}
		else if (HazardAudio->IsPlaying())
		{
			HazardAudio->FadeOut(0.35f, 0.0f);
		}
	}
}

void AProjectOrganoidHazardZone::NotifyOccupantsEnter()
{
	TArray<AActor*> Occupants = OccupyingActors.Array();
	for (AActor* Actor : Occupants)
	{
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
		{
			IProjectOrganoidHazardInterface::Execute_OnEnteredHazard(Actor, HazardType, HazardIntensity * PowerDamageScale);
		}
	}
}

void AProjectOrganoidHazardZone::NotifyOccupantsExit()
{
	TArray<AActor*> Occupants = OccupyingActors.Array();
	for (AActor* Actor : Occupants)
	{
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(UProjectOrganoidHazardInterface::StaticClass()))
		{
			IProjectOrganoidHazardInterface::Execute_OnExitedHazard(Actor, HazardType);
		}
	}
}

FLinearColor AProjectOrganoidHazardZone::ColorForHazardType() const
{
	switch (HazardType)
	{
	case EProjectOrganoidHazardType::UVCRadiation:
		return FLinearColor(0.55f, 0.15f, 1.0f);
	case EProjectOrganoidHazardType::LiquidN2Frost:
		return FLinearColor(0.35f, 0.85f, 1.0f);
	case EProjectOrganoidHazardType::ToxicGas:
		return FLinearColor(0.25f, 0.9f, 0.2f);
	case EProjectOrganoidHazardType::Biohazard:
		return FLinearColor(0.95f, 0.85f, 0.1f);
	case EProjectOrganoidHazardType::ExtremeHeat:
		return FLinearColor(1.0f, 0.28f, 0.05f);
	default:
		return FLinearColor(0.8f, 0.8f, 0.8f);
	}
}
