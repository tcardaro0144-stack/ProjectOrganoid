// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Engine/TimerHandle.h"
#include "ProjectOrganoidUpgradeTypes.h"
#include "ProjectOrganoidHazardInterface.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UDamageType;
class UProjectOrganoidInventoryComponent;
class UProjectOrganoidItemData;
class UProjectOrganoidWeaponComponent;
class UProjectOrganoidInteractionComponent;
class UProjectOrganoidFeedbackComponent;
class UProjectOrganoidLogComponent;
class UProjectOrganoidPhotoScanComponent;
class UProjectOrganoidBiologicalAdaptationComponent;
class AProjectOrganoidCheckpoint;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidTacticalModeChanged, bool, bIsTacticalModeActive);

/**
 *  Avery Vance — ProjectOrganoid player character.
 *  Suit vitals, PE Energy, and Parasite Eve-style Tactical Sphere targeting.
 */
UCLASS()
class PROJECTORGANOID_API AProjectOrganoidCharacter : public ACharacter, public IProjectOrganoidHazardInterface
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Grid inventory for weapons, ammo, and survival items */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidInventoryComponent* InventoryComponent;

	/** Equipped firearm manager (default P226-style sidearm) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidWeaponComponent* WeaponComponent;

	/** World interaction scanner (doors, terminals, locks) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidInteractionComponent* InteractionComponent;

	/** Diegetic audio / post-process / weak-point feedback */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidFeedbackComponent* FeedbackComponent;

	/** Facility lore / data-pad archive */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidLogComponent* LogComponent;

	/** Photography / scanning mode (DoF + lore extract + hi-res capture) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidPhotoScanComponent* PhotoScanComponent;

	/** Biological Adaptation ownership, loadout, and activation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectOrganoidBiologicalAdaptationComponent* BiologicalAdaptationComponent;
	
protected:

	/** Maximum suit health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float MaxHealth = 100.0f;

	/** Maximum toxicity before critical contamination */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float MaxToxicity = 100.0f;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Toggle photography / scanning mode */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PhotoModeAction;

	/** Extract lore from focused scan target (photo mode) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PhotoScanAction;

	/** Capture high-res screenshot (photo mode) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PhotoCaptureAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FireAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* TacticalAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ReloadAction;

	/** Activate the currently equipped Biological Adaptation */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AbilityAction;

	/** Use the first healing consumable in inventory (no inventory grid UI yet) */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* UseConsumableAction;

	/** Current suit integrity / health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float Health = 100.0f;

	/** Bio-contamination buildup (0–100+) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float Toxicity = 0.0f;

	/** Diegetic heart rate in BPM */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float HeartRate = 72.0f;

	/** Current Parasite Eve energy pool */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float PEEnergy = 100.0f;

	/** Maximum PE Energy capacity */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float MaxPEEnergy = 100.0f;

	/** PE Energy restored per second while Tactical Mode is inactive */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float PERechargeRate = 2.5f;

	/** PE Energy drained per second while Tactical Mode is active */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float PEDrainRate = 10.0f;

	/** True while the Tactical Target Sphere is engaged */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	bool bIsTacticalModeActive = false;

	/** Wireframe targeting sphere radius (uu) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float TacticalSphereRadius = 800.0f;

	/** Global time dilation applied during Tactical Mode */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float TacticalTimeDilation = 0.2f;

	/** Sterling terminal upgrade levels */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 SuitHealthUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 SuitToxicityUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 SuitPEUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 WeaponDamageUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 WeaponFireRateUpgradeLevel = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Upgrade")
	int32 WeaponPenetrationUpgradeLevel = 0;

	/**
	 *  Owned / unlocked weapon modifications. Distinct from installed loadout.
	 *  Removing a mod at a Research Station never clears an entry here.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression|WeaponMods")
	TArray<FSoftObjectPath> UnlockedWeaponMods;

public:

	/** Constructor */
	AProjectOrganoidCharacter();

	virtual void Tick(float DeltaTime) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void FellOutOfWorld(const UDamageType& DmgType) override;

	/** Apply or clear Tactical Mode time dilation and state */
	void SetTacticalModeActive(bool bActive);

	UFUNCTION()
	void HandleInventoryItemPickedUp(UProjectOrganoidItemData* ItemData, int32 Quantity);

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	void HandleInteract();
	void HandleFire();
	void HandleReload();
	void HandleTacticalToggle();
	void HandleAbilityActivate();
	void HandleUseConsumable();

	void EnsureRuntimeInput();
	void ApplyRuntimeMappingContext();
	void RecoverFromFall();
	void BeginPlayerDeath();
	void FinishPlayerDeathRestart();
	void SetPlayerControlEnabled(bool bEnabled);
	bool TryRestartFromActivatedCheckpoint();
	bool TryRestartFromPlayerStart();
	void RememberSafeGround(float DeltaTime);
	void HoldForFacilityGeometry();
	void ReleaseFacilityGeometryHold(const FTransform& LandingTransform);
	void ApplyLookLimits();

	UPROPERTY()
	TObjectPtr<UInputMappingContext> RuntimeMappingContext;

	FTransform LastSafeTransform = FTransform::Identity;
	bool bHasLastSafeTransform = false;
	bool bIsDead = false;
	bool bHasActivatedCheckpoint = false;
	FString LastActivatedCheckpointSlot;
	TWeakObjectPtr<AProjectOrganoidCheckpoint> LastActivatedCheckpoint;
	FTimerHandle DeathRestartTimer;
	float DeathRestartDelaySeconds = 1.0f;
	float SafeGroundTimer = 0.0f;
	bool bRecoveringFromFall = false;
	bool bWaitingForFacilityGeometry = false;
	float GeometryHoldSeconds = 0.0f;
	bool bSkipOpeningStartSnap = false;

	FString LastResourceFeedback;
	int32 ResourceFeedbackCount = 0;
	bool bHasShownFirstResourceHint = false;

	/** World Z below the Reactor plate where Avery is teleported back. */
	UPROPERTY(EditAnywhere, Category = "World")
	float FallResetZ = -7200.0f;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float LookYawScale = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Input", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float LookPitchScale = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bInvertLookY = false;

public:

	/** Toggle PE Tactical Mode (time dilation + targeting sphere) */
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void ToggleTacticalMode();

	/** Toggle photography / scanning mode */
	UFUNCTION(BlueprintCallable, Category = "Photo")
	void TogglePhotoMode();

	UFUNCTION(BlueprintCallable, Category = "Photo")
	void PerformPhotoScan();

	UFUNCTION(BlueprintCallable, Category = "Photo")
	void CapturePhotoScreenshot();

	UFUNCTION(BlueprintPure, Category = "Photo")
	bool IsPhotoModeActive() const;

	/** Dialogue helpers (route through UProjectOrganoidDialogueSubsystem) */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool SelectDialogueChoice(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndActiveDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const;

	/** Broadcast when Tactical Mode engages or disengages */
	UPROPERTY(BlueprintAssignable, Category = "Tactical")
	FOnProjectOrganoidTacticalModeChanged OnTacticalModeChanged;

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetToxicity() const { return Toxicity; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetHeartRate() const { return HeartRate; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetPEEnergy() const { return PEEnergy; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetMaxPEEnergy() const { return MaxPEEnergy; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetMaxToxicity() const { return MaxToxicity; }

	/** Apply a signed health change. Environmental hazard ticks must not raise Combat solely from HP loss. */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyHealthDelta(float Delta, EProjectOrganoidHealthDeltaSource Source = EProjectOrganoidHealthDeltaSource::Generic);

	/**
	 *  Narrow consumable-use path. Verifies Consumable + HealAmount > 0 + valid stack.
	 *  Refuses at full health without consuming. On success consumes 1 and applies HealAmount.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool TryUseConsumable(UProjectOrganoidItemData* ItemData);

	/** Player H-key path: use the first healing consumable in the grid. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool TryUseFirstHealingConsumable();

	UFUNCTION(BlueprintPure, Category = "HUD|Resources")
	FString GetLastResourceFeedback() const { return LastResourceFeedback; }

	UFUNCTION(BlueprintPure, Category = "HUD|Resources")
	int32 GetResourceFeedbackCount() const { return ResourceFeedbackCount; }

	UFUNCTION(BlueprintPure, Category = "HUD|Resources")
	bool HasShownFirstResourceHint() const { return bHasShownFirstResourceHint; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	bool IsPlayerDead() const { return bIsDead; }

	/** Called only after a successful checkpoint save. Stores the actor + slot, never a sector name. */
	UFUNCTION(BlueprintCallable, Category = "Checkpoint")
	void NotifyCheckpointActivated(AProjectOrganoidCheckpoint* Checkpoint, const FString& SaveSlot);

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	bool HasActivatedCheckpoint() const { return bHasActivatedCheckpoint; }

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	AProjectOrganoidCheckpoint* GetLastActivatedCheckpoint() const { return LastActivatedCheckpoint.Get(); }

	UFUNCTION(BlueprintPure, Category = "Checkpoint")
	FString GetLastActivatedCheckpointSlot() const { return LastActivatedCheckpointSlot; }

	/** Apply a signed toxicity change */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyToxicityDelta(float Delta);

	/** Apply a signed heart-rate change (clamped to a survivable BPM band) */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyHeartRateDelta(float Delta);

	/** Apply a signed PE Energy change */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyPEEnergyDelta(float Delta);

	// IProjectOrganoidHazardInterface
	virtual void OnEnteredHazard_Implementation(EProjectOrganoidHazardType HazardType, float Intensity) override;
	virtual void OnTickHazard_Implementation(EProjectOrganoidHazardType HazardType, float DamageAmount, float DeltaTime) override;
	virtual void OnExitedHazard_Implementation(EProjectOrganoidHazardType HazardType) override;

	/** New Game / first possess / geometry-hold / timeout Vestibule landing. No-ops after a save restore. */
	UFUNCTION(BlueprintCallable, Category = "World")
	void ApplyCampaignOpeningStart();

	/** Save-load restored PlayerTransform. Geometry hold must not snap to the New Game Vestibule. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void NotifyRestoredSavedTransform();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySavedVitals(float InHealth, float InMaxHealth, float InToxicity, float InMaxToxicity, float InHeartRate, float InPEEnergy, float InMaxPEEnergy);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySavedUpgradeLevels(int32 InHealthLvl, int32 InToxicityLvl, int32 InPELvl, int32 InWeaponDmgLvl, int32 InWeaponFireRateLvl, int32 InWeaponPenLvl);

	UFUNCTION(BlueprintCallable, Category = "Save")
	void ApplySavedWeaponStats(float InDamage, float InFireRate, float InPenetration);

	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	bool ApplyUpgrade(EProjectOrganoidUpgradeType UpgradeType, float HealthPerLevel, float ToxicityPerLevel, float PEPerLevel, float WeaponDamagePerLevel, float WeaponFireRatePerLevel, float WeaponPenetrationPerLevel);

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetUpgradeLevel(EProjectOrganoidUpgradeType UpgradeType) const;

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetSuitHealthUpgradeLevel() const { return SuitHealthUpgradeLevel; }

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetSuitToxicityUpgradeLevel() const { return SuitToxicityUpgradeLevel; }

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetSuitPEUpgradeLevel() const { return SuitPEUpgradeLevel; }

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetWeaponDamageUpgradeLevel() const { return WeaponDamageUpgradeLevel; }

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetWeaponFireRateUpgradeLevel() const { return WeaponFireRateUpgradeLevel; }

	UFUNCTION(BlueprintPure, Category = "Upgrade")
	int32 GetWeaponPenetrationUpgradeLevel() const { return WeaponPenetrationUpgradeLevel; }

	UFUNCTION(BlueprintCallable, Category = "Progression|WeaponMods")
	bool UnlockWeaponMod(UProjectOrganoidWeaponModData* ModData);

	UFUNCTION(BlueprintPure, Category = "Progression|WeaponMods")
	bool IsWeaponModUnlocked(const UProjectOrganoidWeaponModData* ModData) const;

	UFUNCTION(BlueprintPure, Category = "Progression|WeaponMods")
	TArray<UProjectOrganoidWeaponModData*> GetUnlockedWeaponMods() const;

	UFUNCTION(BlueprintPure, Category = "Progression|WeaponMods")
	TArray<FSoftObjectPath> GetUnlockedWeaponModPaths() const { return UnlockedWeaponMods; }

	UFUNCTION(BlueprintCallable, Category = "Progression|WeaponMods|Save")
	void ApplyUnlockedWeaponMods(const TArray<FSoftObjectPath>& Paths);

	UFUNCTION(BlueprintPure, Category = "Tactical")
	bool IsTacticalModeActive() const { return bIsTacticalModeActive; }

	UFUNCTION(BlueprintPure, Category = "Tactical")
	float GetTacticalSphereRadius() const { return TacticalSphereRadius; }

	UFUNCTION(BlueprintPure, Category = "Tactical")
	float GetTacticalTimeDilation() const { return TacticalTimeDilation; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetPERechargeRate() const { return PERechargeRate; }

	UFUNCTION(BlueprintPure, Category = "Suit Vitals")
	float GetPEDrainRate() const { return PEDrainRate; }

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

public:

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns grid inventory component **/
	FORCEINLINE UProjectOrganoidInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Returns weapon component **/
	FORCEINLINE UProjectOrganoidWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	/** Returns interaction component **/
	FORCEINLINE UProjectOrganoidInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

	/** Returns feedback component **/
	FORCEINLINE UProjectOrganoidFeedbackComponent* GetFeedbackComponent() const { return FeedbackComponent; }

	/** Returns lore log component **/
	FORCEINLINE UProjectOrganoidLogComponent* GetLogComponent() const { return LogComponent; }

	/** Returns photography / scanning component **/
	FORCEINLINE UProjectOrganoidPhotoScanComponent* GetPhotoScanComponent() const { return PhotoScanComponent; }

	/** Returns Biological Adaptation component **/
	FORCEINLINE UProjectOrganoidBiologicalAdaptationComponent* GetBiologicalAdaptationComponent() const { return BiologicalAdaptationComponent; }
};
