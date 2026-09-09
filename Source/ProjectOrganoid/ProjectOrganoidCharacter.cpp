// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "ProjectOrganoidLevelManagerSubsystem.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidInventoryTypes.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidInteractionComponent.h"
#include "ProjectOrganoidFeedbackComponent.h"
#include "ProjectOrganoidLogComponent.h"
#include "ProjectOrganoidPhotoScanComponent.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidStatsSubsystem.h"
#include "ProjectOrganoidDialogueSubsystem.h"
#include "ProjectOrganoidTelemetrySubsystem.h"
#include "ProjectOrganoidAudioSubsystem.h"
#include "ProjectOrganoidAudioAmbienceSubsystem.h"
#include "ProjectOrganoidStatsSubsystem.h"
#include "ProjectOrganoidHazardZone.h"
#include "ProjectOrganoidCheckpoint.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
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
	CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	CameraBoom->TargetArmLength = 320.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 10.0f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f;

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

	// Diegetic heartbeat / post-process / weak-point feedback
	FeedbackComponent = CreateDefaultSubobject<UProjectOrganoidFeedbackComponent>(TEXT("FeedbackComponent"));

	// Facility data-pad lore archive
	LogComponent = CreateDefaultSubobject<UProjectOrganoidLogComponent>(TEXT("LogComponent"));

	// Photography / scanning (DoF framing + lore extract)
	PhotoScanComponent = CreateDefaultSubobject<UProjectOrganoidPhotoScanComponent>(TEXT("PhotoScanComponent"));

	BiologicalAdaptationComponent = CreateDefaultSubobject<UProjectOrganoidBiologicalAdaptationComponent>(TEXT("BiologicalAdaptationComponent"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	auto LoadAction = [](const TCHAR* Path) -> UInputAction*
	{
		return LoadObject<UInputAction>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	};
	if (!JumpAction)
	{
		JumpAction = LoadAction(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump"));
	}
	if (!MoveAction)
	{
		MoveAction = LoadAction(TEXT("/Game/Input/Actions/IA_Move.IA_Move"));
	}
	if (!LookAction)
	{
		LookAction = LoadAction(TEXT("/Game/Input/Actions/IA_Look.IA_Look"));
	}
	if (!MouseLookAction)
	{
		MouseLookAction = LoadAction(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook"));
	}
}

void AProjectOrganoidCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryComponent)
	{
		InventoryComponent->OnItemPickedUp.AddDynamic(this, &AProjectOrganoidCharacter::HandleInventoryItemPickedUp);
	}

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
		{
			AudioSubsystem->BindLocalPlayerCharacter(this);
		}

		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->BindLocalPlayerCharacter(this);
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidTelemetrySubsystem* Telemetry = GI->GetSubsystem<UProjectOrganoidTelemetrySubsystem>())
		{
			Telemetry->ReportGameplayEvent(TEXT("PlayerSpawn"), GetName());
		}
	}

	LastSafeTransform = GetActorTransform();
	bHasLastSafeTransform = true;
	ApplyRuntimeMappingContext();
	ApplyLookLimits();

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		HoldForFacilityGeometry();
	}
}

void AProjectOrganoidCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyRuntimeMappingContext();
	ApplyLookLimits();

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		ApplyCampaignOpeningStart();
	}
}

void AProjectOrganoidCharacter::FellOutOfWorld(const UDamageType& /*DmgType*/)
{
	RecoverFromFall();
}

void AProjectOrganoidCharacter::EnsureRuntimeInput()
{
	auto MakeAction = [this](UInputAction*& Action, const TCHAR* Name, EInputActionValueType Type)
	{
		if (!Action)
		{
			Action = NewObject<UInputAction>(this, Name, RF_Transient);
			Action->ValueType = Type;
		}
	};

	MakeAction(JumpAction, TEXT("IA_Jump_Runtime"), EInputActionValueType::Boolean);
	MakeAction(MoveAction, TEXT("IA_Move_Runtime"), EInputActionValueType::Axis2D);
	MakeAction(LookAction, TEXT("IA_Look_Runtime"), EInputActionValueType::Axis2D);
	MakeAction(MouseLookAction, TEXT("IA_MouseLook_Runtime"), EInputActionValueType::Axis2D);
	MakeAction(InteractAction, TEXT("IA_Interact_Runtime"), EInputActionValueType::Boolean);
	MakeAction(FireAction, TEXT("IA_Fire_Runtime"), EInputActionValueType::Boolean);
	MakeAction(TacticalAction, TEXT("IA_Tactical_Runtime"), EInputActionValueType::Boolean);
	MakeAction(ReloadAction, TEXT("IA_Reload_Runtime"), EInputActionValueType::Boolean);
	MakeAction(AbilityAction, TEXT("IA_Ability_Runtime"), EInputActionValueType::Boolean);
	MakeAction(UseConsumableAction, TEXT("IA_UseConsumable_Runtime"), EInputActionValueType::Boolean);

	// Content IA_MouseLook can load as a non-Axis2D action. Look() then reads a zero
	// Vector2D while WASD still works from IMC_Default. Force Axis2D in memory only.
	if (LookAction)
	{
		LookAction->ValueType = EInputActionValueType::Axis2D;
	}
	if (MouseLookAction)
	{
		MouseLookAction->ValueType = EInputActionValueType::Axis2D;
	}

	if (RuntimeMappingContext)
	{
		return;
	}

	RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_RuntimeDefault"), RF_Transient);

	auto AddSwizzleY = [this](FEnhancedActionKeyMapping& Mapping)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	};
	auto AddNegate = [this](FEnhancedActionKeyMapping& Mapping)
	{
		Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));
	};

	{
		FEnhancedActionKeyMapping& Mapping = RuntimeMappingContext->MapKey(MoveAction, EKeys::W);
		AddSwizzleY(Mapping);
	}
	{
		FEnhancedActionKeyMapping& Mapping = RuntimeMappingContext->MapKey(MoveAction, EKeys::S);
		AddSwizzleY(Mapping);
		AddNegate(Mapping);
	}
	{
		FEnhancedActionKeyMapping& Mapping = RuntimeMappingContext->MapKey(MoveAction, EKeys::A);
		AddNegate(Mapping);
	}
	RuntimeMappingContext->MapKey(MoveAction, EKeys::D);
	// Mouse2D is not reliable on all UE5 Enhanced Input paths. Also bind the 1D axes
	// onto the same Axis2D action the pawn already listens to for look.
	RuntimeMappingContext->MapKey(MouseLookAction, EKeys::Mouse2D);
	RuntimeMappingContext->MapKey(MouseLookAction, EKeys::MouseX);
	{
		FEnhancedActionKeyMapping& Mapping = RuntimeMappingContext->MapKey(MouseLookAction, EKeys::MouseY);
		AddSwizzleY(Mapping);
	}
	RuntimeMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	RuntimeMappingContext->MapKey(InteractAction, EKeys::E);
	RuntimeMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
	RuntimeMappingContext->MapKey(TacticalAction, EKeys::RightMouseButton);
	RuntimeMappingContext->MapKey(ReloadAction, EKeys::R);
	RuntimeMappingContext->MapKey(AbilityAction, EKeys::Q);
	RuntimeMappingContext->MapKey(UseConsumableAction, EKeys::H);
}

void AProjectOrganoidCharacter::ApplyRuntimeMappingContext()
{
	EnsureRuntimeInput();

	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !RuntimeMappingContext)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		// Priority 1 so mouse-look mappings win over a content IMC that may bind a
		// different IA_MouseLook object (keyboard can still come from IMC_Default).
		Subsystem->AddMappingContext(RuntimeMappingContext, 1);
		UE_LOG(LogProjectOrganoid, Log, TEXT("Runtime Enhanced Input mapping applied (WASD / mouse look / E / LMB / RMB / H)."));
	}
}

void AProjectOrganoidCharacter::RememberSafeGround(float DeltaTime)
{
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	if (!MoveComp || !MoveComp->IsMovingOnGround())
	{
		return;
	}

	const FVector Location = GetActorLocation();
	if (Location.Z < FallResetZ + 800.0f)
	{
		return;
	}

	SafeGroundTimer += DeltaTime;
	if (SafeGroundTimer < 0.35f && bHasLastSafeTransform)
	{
		return;
	}

	SafeGroundTimer = 0.0f;
	LastSafeTransform = GetActorTransform();
	bHasLastSafeTransform = true;
}

void AProjectOrganoidCharacter::RecoverFromFall()
{
	if (bRecoveringFromFall)
	{
		return;
	}

	bRecoveringFromFall = true;

	FTransform RecoverTM = LastSafeTransform;
	if (!bHasLastSafeTransform || RecoverTM.GetLocation().Z < FallResetZ + 400.0f)
	{
		if (UWorld* World = GetWorld())
		{
			if (AActor* Start = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass()))
			{
				RecoverTM = Start->GetActorTransform();
			}
			else
			{
				RecoverTM = FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 200.0f));
			}
		}
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	SetActorTransform(RecoverTM, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PawnController = GetController())
	{
		PawnController->SetControlRotation(RecoverTM.Rotator());
	}

	UE_LOG(LogProjectOrganoid, Warning, TEXT("Fell off the Epitope plate spine — reset to last safe ground."));
	bRecoveringFromFall = false;
}

void AProjectOrganoidCharacter::NotifyCheckpointActivated(AProjectOrganoidCheckpoint* Checkpoint, const FString& SaveSlot)
{
	if (!Checkpoint || SaveSlot.IsEmpty())
	{
		return;
	}

	LastActivatedCheckpoint = Checkpoint;
	LastActivatedCheckpointSlot = SaveSlot;
	bHasActivatedCheckpoint = true;
}

void AProjectOrganoidCharacter::SetPlayerControlEnabled(bool bEnabled)
{
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (bEnabled)
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MoveComp->StopMovementImmediately();
			MoveComp->DisableMovement();
		}
	}

	if (AController* PawnController = GetController())
	{
		PawnController->SetIgnoreMoveInput(!bEnabled);
		PawnController->SetIgnoreLookInput(!bEnabled);
	}

	if (!bEnabled && bIsTacticalModeActive)
	{
		SetTacticalModeActive(false);
	}
}

void AProjectOrganoidCharacter::BeginPlayerDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	Health = 0.0f;
	if (WeaponComponent)
	{
		WeaponComponent->CancelReload();
	}
	SetPlayerControlEnabled(false);

	UE_LOG(LogProjectOrganoid, Warning, TEXT("Nathan reached zero health. Entering death state."));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeathRestartTimer);
		World->GetTimerManager().SetTimer(
			DeathRestartTimer,
			this,
			&AProjectOrganoidCharacter::FinishPlayerDeathRestart,
			DeathRestartDelaySeconds,
			false);
	}
	else
	{
		FinishPlayerDeathRestart();
	}
}

bool AProjectOrganoidCharacter::TryRestartFromActivatedCheckpoint()
{
	if (!bHasActivatedCheckpoint)
	{
		return false;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UProjectOrganoidSaveSubsystem>())
		{
			if (!LastActivatedCheckpointSlot.IsEmpty()
				&& SaveSubsystem->DoesSaveExist(LastActivatedCheckpointSlot)
				&& SaveSubsystem->LoadPlayerProgress(this, LastActivatedCheckpointSlot))
			{
				return true;
			}
		}
	}

	if (AProjectOrganoidCheckpoint* Checkpoint = LastActivatedCheckpoint.Get())
	{
		SetActorTransform(Checkpoint->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
		if (AController* PawnController = GetController())
		{
			PawnController->SetControlRotation(Checkpoint->GetActorRotation());
		}
		Health = MaxHealth;
		return true;
	}

	return false;
}

bool AProjectOrganoidCharacter::TryRestartFromPlayerStart()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FString MapName = UGameplayStatics::GetCurrentLevelName(this, /*bRemovePrefixString=*/true);
	if (MapName.Contains(TEXT("Lvl_Epitope"), ESearchCase::IgnoreCase))
	{
		if (UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			const FTransform Opening = Levels->GetCampaignOpeningTransform();
			SetActorTransform(Opening, false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* PawnController = GetController())
			{
				PawnController->SetControlRotation(Opening.Rotator());
			}
			Health = MaxHealth;
			return true;
		}
	}

	AActor* Start = UGameplayStatics::GetActorOfClass(World, APlayerStart::StaticClass());
	if (!Start)
	{
		return false;
	}

	SetActorTransform(Start->GetActorTransform(), false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PawnController = GetController())
	{
		PawnController->SetControlRotation(Start->GetActorRotation());
	}
	Health = MaxHealth;
	return true;
}

void AProjectOrganoidCharacter::FinishPlayerDeathRestart()
{
	if (!bIsDead)
	{
		return;
	}

	bool bRestored = TryRestartFromActivatedCheckpoint();
	if (!bRestored)
	{
		bRestored = TryRestartFromPlayerStart();
	}

	if (!bRestored)
	{
		UE_LOG(
			LogProjectOrganoid,
			Error,
			TEXT("Death restart failed closed: no activated checkpoint and no PlayerStart."));
		return;
	}

	bIsDead = false;
	SetPlayerControlEnabled(true);
	LastSafeTransform = GetActorTransform();
	bHasLastSafeTransform = true;

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->NotifyHealthChanged(Health, MaxHealth);
		}
	}
}

void AProjectOrganoidCharacter::HoldForFacilityGeometry()
{
	if (!bSkipOpeningStartSnap)
	{
		ApplyCampaignOpeningStart();
	}

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
		{
			const FName AdminName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel1_Admin);
			if (Levels->IsPartitionReady(AdminName))
			{
				return;
			}
		}
	}

	bWaitingForFacilityGeometry = true;
	GeometryHoldSeconds = 0.0f;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->GravityScale = 0.0f;
		MoveComp->SetMovementMode(MOVE_None);
	}
	SetActorEnableCollision(false);
}

void AProjectOrganoidCharacter::ReleaseFacilityGeometryHold(const FTransform& LandingTransform)
{
	SetActorTransform(LandingTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PawnController = GetController())
	{
		PawnController->SetControlRotation(LandingTransform.Rotator());
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->GravityScale = 1.0f;
		MoveComp->SetMovementMode(MOVE_Walking);
		MoveComp->StopMovementImmediately();
	}

	SetActorEnableCollision(true);
	LastSafeTransform = GetActorTransform();
	bHasLastSafeTransform = true;
	bWaitingForFacilityGeometry = false;
}

void AProjectOrganoidCharacter::ApplyCampaignOpeningStart()
{
	if (bSkipOpeningStartSnap)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>();
	if (!Levels)
	{
		return;
	}

	const FTransform Opening = Levels->GetCampaignOpeningTransform();
	SetActorTransform(Opening, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* PawnController = GetController())
	{
		PawnController->SetControlRotation(Opening.Rotator());
	}
}

void AProjectOrganoidCharacter::NotifyRestoredSavedTransform()
{
	bSkipOpeningStartSnap = true;
	if (AController* PawnController = GetController())
	{
		PawnController->SetControlRotation(GetActorRotation());
	}
}

void AProjectOrganoidCharacter::ApplyLookLimits()
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	PlayerController->PlayerCameraManager->ViewPitchMin = -50.0f;
	PlayerController->PlayerCameraManager->ViewPitchMax = 65.0f;
}

void AProjectOrganoidCharacter::HandleInteract()
{
	if (bIsDead)
	{
		return;
	}

	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AProjectOrganoidCharacter::HandleFire()
{
	if (bIsDead)
	{
		return;
	}

	if (WeaponComponent)
	{
		WeaponComponent->FireEquippedWeapon();
	}
}

void AProjectOrganoidCharacter::HandleReload()
{
	if (bIsDead)
	{
		return;
	}

	if (WeaponComponent)
	{
		WeaponComponent->ReloadEquippedWeapon();
	}
}

void AProjectOrganoidCharacter::HandleTacticalToggle()
{
	if (bIsDead)
	{
		return;
	}

	ToggleTacticalMode();
}

void AProjectOrganoidCharacter::HandleAbilityActivate()
{
	if (bIsDead)
	{
		return;
	}

	if (BiologicalAdaptationComponent)
	{
		BiologicalAdaptationComponent->TryActivateEquipped();
	}
}

void AProjectOrganoidCharacter::HandleUseConsumable()
{
	if (bIsDead)
	{
		return;
	}

	TryUseFirstHealingConsumable();
}

bool AProjectOrganoidCharacter::TryUseConsumable(UProjectOrganoidItemData* ItemData)
{
	if (!ItemData || !InventoryComponent)
	{
		return false;
	}

	if (ItemData->ItemType != EProjectOrganoidItemType::Consumable)
	{
		return false;
	}

	if (ItemData->HealAmount <= 0.0f)
	{
		return false;
	}

	if (InventoryComponent->CountItem(ItemData) <= 0)
	{
		return false;
	}

	if (Health >= MaxHealth)
	{
		return false;
	}

	if (!InventoryComponent->ConsumeItem(ItemData, 1))
	{
		return false;
	}

	ApplyHealthDelta(ItemData->HealAmount);
	return true;
}

bool AProjectOrganoidCharacter::TryUseFirstHealingConsumable()
{
	if (!InventoryComponent)
	{
		return false;
	}

	for (const FProjectOrganoidPlacedItem& Placed : InventoryComponent->GetAllItems())
	{
		if (Placed.IsValid()
			&& Placed.ItemData
			&& Placed.ItemData->ItemType == EProjectOrganoidItemType::Consumable
			&& Placed.ItemData->HealAmount > 0.0f)
		{
			return TryUseConsumable(Placed.ItemData);
		}
	}

	return false;
}

void AProjectOrganoidCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnItemPickedUp.RemoveDynamic(this, &AProjectOrganoidCharacter::HandleInventoryItemPickedUp);
	}

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
		{
			AudioSubsystem->UnbindLocalPlayerCharacter(this);
		}

		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->UnbindLocalPlayerCharacter(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AProjectOrganoidCharacter::HandleInventoryItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity)
{
	if (Quantity <= 0)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
		{
			Stats->RecordItemPickup(Quantity);
		}
	}

	if (!ItemData)
	{
		return;
	}

	FString Name = ItemData->ItemName.ToString();
	if (Name.IsEmpty())
	{
		Name = ItemData->GetName();
	}

	FString Line;
	if (ItemData->ItemType == EProjectOrganoidItemType::Ammo)
	{
		const int32 Reserve = InventoryComponent
			? InventoryComponent->CountAmmoOfType(ItemData->AmmoType)
			: Quantity;
		Line = FString::Printf(TEXT("%s +%d. Pistol reserve %d."), *Name, Quantity, Reserve);
	}
	else
	{
		Line = FString::Printf(TEXT("%s acquired."), *Name);
		if (ItemData->ItemType == EProjectOrganoidItemType::Consumable && ItemData->HealAmount > 0.0f)
		{
			Line += TEXT(" Press H to use.");
		}
	}

	if (!bHasShownFirstResourceHint)
	{
		bHasShownFirstResourceHint = true;
		Line += TEXT(" Supplies are stored in your inventory.");
	}

	LastResourceFeedback = Line;
	++ResourceFeedbackCount;
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

	if (bWaitingForFacilityGeometry)
	{
		GeometryHoldSeconds += DeltaTime;
		FName AdminName = NAME_None;
		FTransform Landing = GetActorTransform();
		bool bAdminReady = false;
		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidLevelManagerSubsystem* Levels = World->GetSubsystem<UProjectOrganoidLevelManagerSubsystem>())
			{
				AdminName = Levels->ResolveStreamingLevelName(EProjectOrganoidSubLevelTag::SubLevel1_Admin);
				bAdminReady = Levels->IsPartitionReady(AdminName);
				if (!bSkipOpeningStartSnap)
				{
					Landing = Levels->GetCampaignOpeningTransform();
				}
			}
		}

		if (bAdminReady)
		{
			ReleaseFacilityGeometryHold(Landing);
		}
		else if (GeometryHoldSeconds >= 8.0f)
		{
			UE_LOG(LogProjectOrganoid, Warning, TEXT("Admin partition did not become visible — using campaign opening transform."));
			ReleaseFacilityGeometryHold(Landing);
		}
		return;
	}

	RememberSafeGround(DeltaTime);
	if (GetActorLocation().Z < FallResetZ)
	{
		RecoverFromFall();
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

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
		{
			AudioSubsystem->NotifyTacticalModeChanged(bIsTacticalModeActive);
		}
	}

	OnTacticalModeChanged.Broadcast(bIsTacticalModeActive);
}

void AProjectOrganoidCharacter::ApplyHealthDelta(float Delta, EProjectOrganoidHealthDeltaSource Source)
{
	if (Delta < 0.0f)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UProjectOrganoidStatsSubsystem* Stats = GI->GetSubsystem<UProjectOrganoidStatsSubsystem>())
			{
				Stats->RecordDamageTaken(-Delta);
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
			{
				if (Source == EProjectOrganoidHealthDeltaSource::EnvironmentalHazard)
				{
					UE_LOG(LogTemp, Warning, TEXT("OrganoidHealthHazard t=%.3f delta=%.3f health=%.1f/%.1f player=%s"),
						World->GetTimeSeconds(),
						Delta,
						Health,
						MaxHealth,
						IsPlayerControlled() ? TEXT("true") : TEXT("false"));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("OrganoidHealthCombat t=%.3f delta=%.3f health=%.1f/%.1f player=%s"),
						World->GetTimeSeconds(),
						Delta,
						Health,
						MaxHealth,
						IsPlayerControlled() ? TEXT("true") : TEXT("false"));
					Ambience->NotifyCombatStimulus(0.45f);
				}
			}
		}
	}

	if (bIsDead && Delta < 0.0f)
	{
		return;
	}

	Health = FMath::Clamp(Health + Delta, 0.0f, MaxHealth);

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->NotifyHealthChanged(Health, MaxHealth);
		}
	}

	if (!bIsDead && Health <= 0.0f)
	{
		BeginPlayerDeath();
	}
}

void AProjectOrganoidCharacter::ApplyToxicityDelta(float Delta)
{
	Toxicity = FMath::Clamp(Toxicity + Delta, 0.0f, MaxToxicity);
}

void AProjectOrganoidCharacter::ApplyHeartRateDelta(float Delta)
{
	HeartRate = FMath::Clamp(HeartRate + Delta, 40.0f, 220.0f);
}

void AProjectOrganoidCharacter::ApplyPEEnergyDelta(float Delta)
{
	PEEnergy = FMath::Clamp(PEEnergy + Delta, 0.0f, MaxPEEnergy);
}

void AProjectOrganoidCharacter::OnEnteredHazard_Implementation(EProjectOrganoidHazardType HazardType, float Intensity)
{
	const float ClampedIntensity = FMath::Max(0.0f, Intensity);
	ApplyHeartRateDelta(8.0f * ClampedIntensity);

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->NotifyHazardEntered(HazardType, ClampedIntensity);
		}

		if (HazardType == EProjectOrganoidHazardType::ToxicGas || HazardType == EProjectOrganoidHazardType::Biohazard)
		{
			if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
			{
				AudioSubsystem->SetToxicGasDistortion(FMath::Clamp(ClampedIntensity, 0.0f, 1.0f));
			}
		}
	}
}

void AProjectOrganoidCharacter::OnTickHazard_Implementation(EProjectOrganoidHazardType HazardType, float DamageAmount, float DeltaTime)
{
	if (DamageAmount <= KINDA_SMALL_NUMBER || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// DamageAmount is already scaled by zone DPS * intensity * delta from the volume.
	ApplyHealthDelta(-DamageAmount, EProjectOrganoidHealthDeltaSource::EnvironmentalHazard);

	switch (HazardType)
	{
	case EProjectOrganoidHazardType::ToxicGas:
	case EProjectOrganoidHazardType::Biohazard:
		ApplyToxicityDelta(DamageAmount * 1.25f);
		ApplyHeartRateDelta(DamageAmount * 0.35f);
		break;
	case EProjectOrganoidHazardType::UVCRadiation:
		ApplyToxicityDelta(DamageAmount * 0.15f);
		ApplyHeartRateDelta(DamageAmount * 0.45f);
		break;
	case EProjectOrganoidHazardType::LiquidN2Frost:
	case EProjectOrganoidHazardType::ExtremeHeat:
		ApplyHeartRateDelta(DamageAmount * 0.55f);
		break;
	default:
		ApplyHeartRateDelta(DamageAmount * 0.25f);
		break;
	}
}

void AProjectOrganoidCharacter::OnExitedHazard_Implementation(EProjectOrganoidHazardType HazardType)
{
	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->NotifyHazardExited(HazardType);
		}

		if (HazardType == EProjectOrganoidHazardType::ToxicGas || HazardType == EProjectOrganoidHazardType::Biohazard)
		{
			if (UProjectOrganoidAudioSubsystem* AudioSubsystem = World->GetSubsystem<UProjectOrganoidAudioSubsystem>())
			{
				bool bStillInToxicHazard = false;
				TArray<AActor*> HazardZones;
				UGameplayStatics::GetAllActorsOfClass(World, AProjectOrganoidHazardZone::StaticClass(), HazardZones);
				for (AActor* ZoneActor : HazardZones)
				{
					AProjectOrganoidHazardZone* Zone = Cast<AProjectOrganoidHazardZone>(ZoneActor);
					if (Zone && Zone->bIsActive && Zone->HazardVolume && Zone->HazardVolume->IsOverlappingActor(this)
						&& (Zone->HazardType == EProjectOrganoidHazardType::ToxicGas || Zone->HazardType == EProjectOrganoidHazardType::Biohazard))
					{
						bStillInToxicHazard = true;
						break;
					}
				}

				if (!bStillInToxicHazard)
				{
					AudioSubsystem->SetToxicGasDistortion(0.0f);
				}
			}
		}
	}
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

	if (UWorld* World = GetWorld())
	{
		if (UProjectOrganoidAudioAmbienceSubsystem* Ambience = World->GetSubsystem<UProjectOrganoidAudioAmbienceSubsystem>())
		{
			Ambience->NotifyHealthChanged(Health, MaxHealth);
		}
	}
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

bool AProjectOrganoidCharacter::UnlockWeaponMod(UProjectOrganoidWeaponModData* ModData)
{
	if (!ModData)
	{
		return false;
	}

	const FSoftObjectPath Path(ModData);
	if (Path.IsNull() || UnlockedWeaponMods.Contains(Path))
	{
		return UnlockedWeaponMods.Contains(Path);
	}

	UnlockedWeaponMods.Add(Path);
	return true;
}

bool AProjectOrganoidCharacter::IsWeaponModUnlocked(const UProjectOrganoidWeaponModData* ModData) const
{
	if (!ModData)
	{
		return false;
	}

	const FSoftObjectPath Path(ModData);
	if (UnlockedWeaponMods.Contains(Path))
	{
		return true;
	}

	for (const FSoftObjectPath& Existing : UnlockedWeaponMods)
	{
		if (Existing.TryLoad() == ModData)
		{
			return true;
		}
	}
	return false;
}

TArray<UProjectOrganoidWeaponModData*> AProjectOrganoidCharacter::GetUnlockedWeaponMods() const
{
	TArray<UProjectOrganoidWeaponModData*> Result;
	for (const FSoftObjectPath& Path : UnlockedWeaponMods)
	{
		if (UProjectOrganoidWeaponModData* Mod = Cast<UProjectOrganoidWeaponModData>(Path.TryLoad()))
		{
			Result.AddUnique(Mod);
		}
	}
	return Result;
}

void AProjectOrganoidCharacter::ApplyUnlockedWeaponMods(const TArray<FSoftObjectPath>& Paths)
{
	UnlockedWeaponMods.Reset();
	for (const FSoftObjectPath& Path : Paths)
	{
		if (!Path.IsNull())
		{
			UnlockedWeaponMods.AddUnique(Path);
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
	if (bIsDead)
	{
		return;
	}

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

void AProjectOrganoidCharacter::TogglePhotoMode()
{
	if (PhotoScanComponent)
	{
		// Exit tactical when framing photos
		if (!PhotoScanComponent->IsPhotoModeActive() && bIsTacticalModeActive)
		{
			SetTacticalModeActive(false);
		}
		PhotoScanComponent->TogglePhotoMode();
	}
}

void AProjectOrganoidCharacter::PerformPhotoScan()
{
	if (PhotoScanComponent && PhotoScanComponent->IsPhotoModeActive())
	{
		PhotoScanComponent->TryScanFocusedTarget();
	}
}

void AProjectOrganoidCharacter::CapturePhotoScreenshot()
{
	if (PhotoScanComponent && PhotoScanComponent->IsPhotoModeActive())
	{
		PhotoScanComponent->CaptureHighResScreenshot();
	}
}

bool AProjectOrganoidCharacter::IsPhotoModeActive() const
{
	return PhotoScanComponent && PhotoScanComponent->IsPhotoModeActive();
}

bool AProjectOrganoidCharacter::SelectDialogueChoice(int32 ChoiceIndex)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			return Dialogue->SelectChoice(ChoiceIndex);
		}
	}
	return false;
}

void AProjectOrganoidCharacter::EndActiveDialogue()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			Dialogue->EndDialogue();
		}
	}
}

bool AProjectOrganoidCharacter::IsInDialogue() const
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidDialogueSubsystem* Dialogue = GI->GetSubsystem<UProjectOrganoidDialogueSubsystem>())
		{
			return Dialogue->IsDialogueActive();
		}
	}
	return false;
}

void AProjectOrganoidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	EnsureRuntimeInput();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Move);
		}
		if (MouseLookAction)
		{
			EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Look);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectOrganoidCharacter::Look);
		}
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleInteract);
		}
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleFire);
		}
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleReload);
		}
		if (TacticalAction)
		{
			EnhancedInputComponent->BindAction(TacticalAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleTacticalToggle);
		}
		if (AbilityAction)
		{
			EnhancedInputComponent->BindAction(AbilityAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleAbilityActivate);
		}
		if (UseConsumableAction)
		{
			EnhancedInputComponent->BindAction(UseConsumableAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::HandleUseConsumable);
		}
		if (PhotoModeAction)
		{
			EnhancedInputComponent->BindAction(PhotoModeAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::TogglePhotoMode);
		}
		if (PhotoScanAction)
		{
			EnhancedInputComponent->BindAction(PhotoScanAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::PerformPhotoScan);
		}
		if (PhotoCaptureAction)
		{
			EnhancedInputComponent->BindAction(PhotoCaptureAction, ETriggerEvent::Started, this, &AProjectOrganoidCharacter::CapturePhotoScreenshot);
		}

		ApplyRuntimeMappingContext();
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
	if (Value.GetValueType() == EInputActionValueType::Axis1D)
	{
		DoLook(Value.Get<float>(), 0.0f);
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AProjectOrganoidCharacter::DoMove(float Right, float Forward)
{
	if (bIsDead)
	{
		return;
	}

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
	if (GetController() == nullptr)
	{
		return;
	}

	const float PitchSign = bInvertLookY ? 1.0f : -1.0f;
	AddControllerYawInput(Yaw * LookYawScale);
	AddControllerPitchInput(Pitch * PitchSign * LookPitchScale);
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
