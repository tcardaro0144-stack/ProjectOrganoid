// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidInteractionComponent.h"
#include "ProjectOrganoid.h"

AProjectOrganoidCharacter::AProjectOrganoidCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Grid inventory (default 8x6 — tune on Blueprint defaults)
	InventoryComponent = CreateDefaultSubobject<UProjectOrganoidInventoryComponent>(TEXT("InventoryComponent"));

	// Default firearm component (spawns AProjectOrganoidDefaultWeapon on BeginPlay)
	WeaponComponent = CreateDefaultSubobject<UProjectOrganoidWeaponComponent>(TEXT("WeaponComponent"));
	WeaponComponent->SetupAttachment(RootComponent);

	// Environmental / world interaction scanner
	InteractionComponent = CreateDefaultSubobject<UProjectOrganoidInteractionComponent>(TEXT("InteractionComponent"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AProjectOrganoidCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Use undilated delta so PE drain stays consistent in wall-clock while time is slowed
	float UndilatedDelta = DeltaTime;
	if (UWorld* World = GetWorld())
	{
		if (AWorldSettings* WorldSettings = World->GetWorldSettings())
		{
			const float Dilation = WorldSettings->TimeDilation;
			if (Dilation > KINDA_SMALL_NUMBER)
			{
				UndilatedDelta = DeltaTime / Dilation;
			}
		}
	}

	if (bIsTacticalModeActive)
	{
		PEEnergy = FMath::Max(0.0f, PEEnergy - (PEDrainRate * UndilatedDelta));

		DrawDebugSphere(
			GetWorld(),
			GetActorLocation(),
			TacticalSphereRadius,
			32,
			FColor(0, 255, 200),
			false,
			0.0f,
			0,
			1.5f);

		if (PEEnergy <= 0.0f)
		{
			SetTacticalModeActive(false);
		}
	}
	else
	{
		PEEnergy = FMath::Min(MaxPEEnergy, PEEnergy + (PERechargeRate * UndilatedDelta));
	}
}

void AProjectOrganoidCharacter::SetTacticalModeActive(bool bActive)
{
	if (bIsTacticalModeActive == bActive)
	{
		return;
	}

	bIsTacticalModeActive = bActive;

	const float NewDilation = bActive ? TacticalTimeDilation : 1.0f;
	UGameplayStatics::SetGlobalTimeDilation(this, NewDilation);

	OnTacticalModeChanged.Broadcast(bIsTacticalModeActive);
}

void AProjectOrganoidCharacter::ApplyHealthDelta(float Delta)
{
	Health = FMath::Clamp(Health + Delta, 0.0f, MaxHealth);
}

void AProjectOrganoidCharacter::ApplyToxicityDelta(float Delta)
{
	Toxicity = FMath::Clamp(Toxicity + Delta, 0.0f, MaxToxicity);
}

void AProjectOrganoidCharacter::ApplyHeartRateDelta(float Delta)
{
	HeartRate = FMath::Clamp(HeartRate + Delta, 40.0f, 220.0f);
}

void AProjectOrganoidCharacter::ApplySavedVitals(
	float InHealth,
	float InMaxHealth,
	float InToxicity,
	float InMaxToxicity,
	float InHeartRate,
	float InPEEnergy,
	float InMaxPEEnergy)
{
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	MaxToxicity = FMath::Max(1.0f, InMaxToxicity);
	MaxPEEnergy = FMath::Max(1.0f, InMaxPEEnergy);
	Health = FMath::Clamp(InHealth, 0.0f, MaxHealth);
	Toxicity = FMath::Clamp(InToxicity, 0.0f, MaxToxicity);
	HeartRate = FMath::Clamp(InHeartRate, 40.0f, 220.0f);
	PEEnergy = FMath::Clamp(InPEEnergy, 0.0f, MaxPEEnergy);
}

void AProjectOrganoidCharacter::ApplySavedUpgradeLevels(
	int32 InHealthLvl,
	int32 InToxicityLvl,
	int32 InPELvl,
	int32 InWeaponDmgLvl,
	int32 InWeaponFireRateLvl,
	int32 InWeaponPenLvl)
{
	SuitHealthUpgradeLevel = FMath::Max(0, InHealthLvl);
	SuitToxicityUpgradeLevel = FMath::Max(0, InToxicityLvl);
	SuitPEUpgradeLevel = FMath::Max(0, InPELvl);
	WeaponDamageUpgradeLevel = FMath::Max(0, InWeaponDmgLvl);
	WeaponFireRateUpgradeLevel = FMath::Max(0, InWeaponFireRateLvl);
	WeaponPenetrationUpgradeLevel = FMath::Max(0, InWeaponPenLvl);
}

void AProjectOrganoidCharacter::ApplySavedWeaponStats(float InDamage, float InFireRate, float InPenetration)
{
	if (UProjectOrganoidWeaponComponent* WeaponComp = GetWeaponComponent())
	{
		if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
		{
			Weapon->Damage = InDamage;
			Weapon->FireRate = FMath::Max(0.1f, InFireRate);
			Weapon->Penetration = FMath::Clamp(InPenetration, 0.0f, 1.0f);
		}
	}
}

int32 AProjectOrganoidCharacter::GetUpgradeLevel(EProjectOrganoidUpgradeType UpgradeType) const
{
	switch (UpgradeType)
	{
	case EProjectOrganoidUpgradeType::SuitMaxHealth: return SuitHealthUpgradeLevel;
	case EProjectOrganoidUpgradeType::SuitToxicityThreshold: return SuitToxicityUpgradeLevel;
	case EProjectOrganoidUpgradeType::SuitPEEnergyMax: return SuitPEUpgradeLevel;
	case EProjectOrganoidUpgradeType::WeaponDamage: return WeaponDamageUpgradeLevel;
	case EProjectOrganoidUpgradeType::WeaponFireRate: return WeaponFireRateUpgradeLevel;
	case EProjectOrganoidUpgradeType::WeaponPenetration: return WeaponPenetrationUpgradeLevel;
	default: return 0;
	}
}

bool AProjectOrganoidCharacter::ApplyUpgrade(
	EProjectOrganoidUpgradeType UpgradeType,
	float HealthPerLevel,
	float ToxicityPerLevel,
	float PEPerLevel,
	float WeaponDamagePerLevel,
	float WeaponFireRatePerLevel,
	float WeaponPenetrationPerLevel)
{
	switch (UpgradeType)
	{
	case EProjectOrganoidUpgradeType::SuitMaxHealth:
		++SuitHealthUpgradeLevel;
		MaxHealth += HealthPerLevel;
		Health = FMath::Min(Health + HealthPerLevel, MaxHealth);
		return true;

	case EProjectOrganoidUpgradeType::SuitToxicityThreshold:
		++SuitToxicityUpgradeLevel;
		MaxToxicity += ToxicityPerLevel;
		return true;

	case EProjectOrganoidUpgradeType::SuitPEEnergyMax:
		++SuitPEUpgradeLevel;
		MaxPEEnergy += PEPerLevel;
		PEEnergy = FMath::Min(PEEnergy + PEPerLevel, MaxPEEnergy);
		return true;

	case EProjectOrganoidUpgradeType::WeaponDamage:
		++WeaponDamageUpgradeLevel;
		if (UProjectOrganoidWeaponComponent* WeaponComp = GetWeaponComponent())
		{
			if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
			{
				Weapon->Damage += WeaponDamagePerLevel;
			}
		}
		return true;

	case EProjectOrganoidUpgradeType::WeaponFireRate:
		++WeaponFireRateUpgradeLevel;
		if (UProjectOrganoidWeaponComponent* WeaponComp = GetWeaponComponent())
		{
			if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
			{
				Weapon->FireRate += WeaponFireRatePerLevel;
			}
		}
		return true;

	case EProjectOrganoidUpgradeType::WeaponPenetration:
		++WeaponPenetrationUpgradeLevel;
		if (UProjectOrganoidWeaponComponent* WeaponComp = GetWeaponComponent())
		{
			if (AProjectOrganoidWeapon* Weapon = WeaponComp->GetEquippedWeapon())
			{
				Weapon->Penetration = FMath::Clamp(Weapon->Penetration + WeaponPenetrationPerLevel, 0.0f, 1.0f);
			}
		}
		return true;

	default:
		return false;
	}
}

void AProjectOrganoidCharacter::ToggleTacticalMode()
{
	if (bIsTacticalModeActive)
	{
		SetTacticalModeActive(false);
		return;
	}

	if (PEEnergy <= 0.0f)
	{
		return;
	}

	SetTacticalModeActive(true);
}

void AProjectOrganoidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Look);
	}
	else
	{
		UE_LOG(LogProjectOrganoid, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectOrganoidCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AProjectOrganoidCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AProjectOrganoidCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AProjectOrganoidCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AProjectOrganoidCharacter::DoJumpStart()
{
	// signal the character to jump
	Jump();
}

void AProjectOrganoidCharacter::DoJumpEnd()
{
	// signal the character to stop jumping
	StopJumping();
}
