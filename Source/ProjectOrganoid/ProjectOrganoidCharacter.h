// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProjectOrganoidCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

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
};
