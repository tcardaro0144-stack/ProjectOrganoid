// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProjectOrganoidCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UProjectOrganoidInventoryComponent;
class UProjectOrganoidWeaponComponent;
class UProjectOrganoidInteractionComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProjectOrganoidTacticalModeChanged, bool, bIsTacticalModeActive);

/**
 *  Avery Vance — ProjectOrganoid player character.
 *  Suit vitals, PE Energy, and Parasite Eve-style Tactical Sphere targeting.
 */
UCLASS(abstract)
class AProjectOrganoidCharacter : public ACharacter
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

	/** Maximum suit health */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float MaxHealth = 100.0f;

	/** Maximum toxicity before critical contamination */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Suit Vitals")
	float MaxToxicity = 100.0f;
	
protected:

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

public:

	/** Constructor */
	AProjectOrganoidCharacter();

	virtual void Tick(float DeltaTime) override;

protected:

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Apply or clear Tactical Mode time dilation and state */
	void SetTacticalModeActive(bool bActive);

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

public:

	/** Toggle PE Tactical Mode (time dilation + targeting sphere) */
	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void ToggleTacticalMode();

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

	/** Apply a signed health change (hazards pass negative values) */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyHealthDelta(float Delta);

	/** Apply a signed toxicity change */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyToxicityDelta(float Delta);

	/** Apply a signed heart-rate change (clamped to a survivable BPM band) */
	UFUNCTION(BlueprintCallable, Category = "Suit Vitals")
	void ApplyHeartRateDelta(float Delta);

	UFUNCTION(BlueprintPure, Category = "Tactical")
	bool IsTacticalModeActive() const { return bIsTacticalModeActive; }

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
};
