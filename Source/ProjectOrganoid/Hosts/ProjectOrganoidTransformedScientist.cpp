// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidTransformedScientist.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidObjectiveSubsystem.h"

#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"

AProjectOrganoidTransformedScientist::AProjectOrganoidTransformedScientist()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	SetRootComponent(Capsule);
	Capsule->InitCapsuleSize(42.f, 96.f);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionResponseToAllChannels(ECR_Block);
	Capsule->SetCanEverAffectNavigation(false);

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	Mesh->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMesh(
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (BodyMesh.Succeeded())
	{
		Mesh->SetSkeletalMeshAsset(BodyMesh.Object);
	}
}

void AProjectOrganoidTransformedScientist::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

float AProjectOrganoidTransformedScientist::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	(void)DamageEvent;
	(void)EventInstigator;
	ApplyWeaponHit(DamageAmount, DamageCauser);
	return bRevealed && !bDead ? DamageAmount : 0.f;
}

EProjectOrganoidWeakPointType AProjectOrganoidTransformedScientist::ResolveWeakPoint_Implementation(const FHitResult& Hit) const
{
	(void)Hit;
	return EProjectOrganoidWeakPointType::None;
}

void AProjectOrganoidTransformedScientist::ApplyOrganoidHit_Implementation(const FProjectOrganoidBallisticHit& HitInfo, AActor* DamageCauser)
{
	ApplyWeaponHit(HitInfo.FinalDamage, DamageCauser);
}

void AProjectOrganoidTransformedScientist::ApplyWeaponHit(float Damage, AActor* DamageCauser)
{
	if (!bRevealed || bDead || Damage <= 0.f)
	{
		return;
	}
	Health = FMath::Max(0.f, Health - Damage);
	if (Health <= 0.f)
	{
		HandleDeath(DamageCauser);
	}
}

void AProjectOrganoidTransformedScientist::BeginEncounter()
{
	if (bEncounterConsumed)
	{
		return;
	}
	bEncounterConsumed = true;
	bRevealed = true;
	SetActorHiddenInGame(false);
	SetCanBeDamaged(true);

	if (UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")))
	{
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(Walk, true);
		Mesh->SetPlayRate(SlowWalkPlayRate);
	}

	ShowNathanLine();
}

void AProjectOrganoidTransformedScientist::ShowNathanLine()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD)
	{
		HUD->ShowTransientNotification(
			FText::FromString(TEXT("Nathan")),
			FText::FromString(TEXT("It's still wearing the lab coat. Christ.")),
			LineSeconds);
	}
}

void AProjectOrganoidTransformedScientist::HandleDeath(AActor* DamageCauser)
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	SetCanBeDamaged(false);
	if (Mesh)
	{
		Mesh->Stop();
	}
	GrantLyticCharge(DamageCauser);

	UWorld* World = GetWorld();
	UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>()
		: nullptr;
	if (Objectives)
	{
		Objectives->TriggerEvent(TEXT("Event_FirstCombatDefeated"));
	}
}

void AProjectOrganoidTransformedScientist::GrantLyticCharge(AActor* DamageCauser)
{
	DroppedLyticCharges = 1;
	AProjectOrganoidCharacter* Character = Cast<AProjectOrganoidCharacter>(DamageCauser);
	if (!Character)
	{
		UWorld* World = GetWorld();
		Character = World ? Cast<AProjectOrganoidCharacter>(World->GetFirstPlayerController() ? World->GetFirstPlayerController()->GetPawn() : nullptr) : nullptr;
	}
	UProjectOrganoidItemData* Ammo = LoadObject<UProjectOrganoidItemData>(nullptr, TEXT("/Game/Data/Items/DA_Item_PistolAmmo.DA_Item_PistolAmmo"));
	UProjectOrganoidInventoryComponent* Inventory = Character ? Character->GetInventoryComponent() : nullptr;
	if (!Inventory || !Ammo)
	{
		return;
	}
	FGuid InstanceId;
	bLyticChargeGranted = Inventory->TryAddItem(Ammo, InstanceId, 1);
}

AProjectOrganoidFirstCombatTrigger::AProjectOrganoidFirstCombatTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(400.f, 600.f, 250.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetCanEverAffectNavigation(false);
}

void AProjectOrganoidFirstCombatTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	NotifyPlayerOverlap(OtherActor);
}

void AProjectOrganoidFirstCombatTrigger::NotifyPlayerOverlap(AActor* OtherActor)
{
	if (bConsumed || !Cast<AProjectOrganoidCharacter>(OtherActor))
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (TActorIterator<AProjectOrganoidTransformedScientist> It(World); It; ++It)
	{
		bConsumed = true;
		It->BeginEncounter();
		break;
	}
}
