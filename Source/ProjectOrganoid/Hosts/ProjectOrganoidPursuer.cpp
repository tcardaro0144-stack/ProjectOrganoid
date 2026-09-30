// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPursuer.h"

#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidGameMode.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidSaveSubsystem.h"

#include "Animation/AnimSequence.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWaveProcedural.h"
#include "TimerManager.h"

AProjectOrganoidPursuer::AProjectOrganoidPursuer()
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

void AProjectOrganoidPursuer::BeginPlay()
{
	Super::BeginPlay();
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
}

float AProjectOrganoidPursuer::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	(void)DamageAmount;
	(void)DamageEvent;
	(void)EventInstigator;
	(void)DamageCauser;
	return 0.f;
}

void AProjectOrganoidPursuer::BeginEncounter()
{
	if (bEncounterConsumed)
	{
		return;
	}
	bEncounterConsumed = true;
	bRevealed = true;
	SetActorHiddenInGame(false);

	if (UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd")))
	{
		float PlayRate = SlowWalkPlayRate;
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (const UProjectOrganoidSaveSubsystem* Saves = GameInstance->GetSubsystem<UProjectOrganoidSaveSubsystem>())
				{
					if (Saves->HasNewGamePlus())
					{
						PlayRate = FMath::Min(1.f, SlowWalkPlayRate * 1.35f);
					}
				}
			}
		}
		Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Mesh->PlayAnimation(Walk, true);
		Mesh->SetPlayRate(PlayRate);
	}

	PlayBang();
	ShowNathanLine();
	CompleteEncounterObjective();

	FTimerHandle DespawnHandle;
	GetWorldTimerManager().SetTimer(DespawnHandle, this, &AProjectOrganoidPursuer::Despawn, DespawnSeconds, false);
}

void AProjectOrganoidPursuer::PlayBang()
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	if (!Wave)
	{
		return;
	}
	constexpr int32 SampleRate = 22050;
	constexpr int32 SampleCount = 4410;
	Wave->SetSampleRate(SampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = static_cast<float>(SampleCount) / static_cast<float>(SampleRate);
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = false;

	TArray<uint8> Bytes;
	Bytes.SetNumUninitialized(SampleCount * sizeof(int16));
	int16* Samples = reinterpret_cast<int16*>(Bytes.GetData());
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Envelope = 1.f - (static_cast<float>(Index) / static_cast<float>(SampleCount));
		const float Tone = FMath::Sin(2.f * PI * 70.f * static_cast<float>(Index) / static_cast<float>(SampleRate));
		Samples[Index] = static_cast<int16>(Tone * Envelope * 22000.f);
	}
	Wave->QueueAudio(Bytes.GetData(), Bytes.Num());
	UGameplayStatics::PlaySoundAtLocation(this, Wave, GetActorLocation());
	bBangPlayed = true;
}

void AProjectOrganoidPursuer::ShowNathanLine()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	AProjectOrganoidGameMode* GameMode = World ? World->GetAuthGameMode<AProjectOrganoidGameMode>() : nullptr;
	UProjectOrganoidGameplayHUDController* HUD = GameMode && PC ? GameMode->GetHUDControllerForPlayer(PC) : nullptr;
	if (HUD)
	{
		HUD->ShowTransientNotification(
			FText::FromString(TEXT("Nathan")),
			FText::FromString(TEXT("That was in a person. It's still walking.")),
			LineSeconds);
	}
}

void AProjectOrganoidPursuer::CompleteEncounterObjective()
{
	UWorld* World = GetWorld();
	UProjectOrganoidObjectiveSubsystem* Objectives = World && World->GetGameInstance()
		? World->GetGameInstance()->GetSubsystem<UProjectOrganoidObjectiveSubsystem>()
		: nullptr;
	if (Objectives)
	{
		Objectives->TriggerEvent(TEXT("Event_PursuerEncountered"));
	}
}

void AProjectOrganoidPursuer::Despawn()
{
	Destroy();
}

void AProjectOrganoidPursuerTrigger::BeginPlay()
{
	Super::BeginPlay();
	if (Trigger)
	{
		Trigger->SetBoxExtent(FVector(500.f, 500.f, 300.f), true);
	}
}

AProjectOrganoidPursuerTrigger::AProjectOrganoidPursuerTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(500.f, 500.f, 300.f));
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->SetCanEverAffectNavigation(false);
}

void AProjectOrganoidPursuerTrigger::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	NotifyPlayerOverlap(OtherActor);
}

void AProjectOrganoidPursuerTrigger::NotifyPlayerOverlap(AActor* OtherActor)
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
	for (TActorIterator<AProjectOrganoidPursuer> It(World); It; ++It)
	{
		bConsumed = true;
		It->BeginEncounter();
		break;
	}
}
