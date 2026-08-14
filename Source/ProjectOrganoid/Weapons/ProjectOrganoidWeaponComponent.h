// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ProjectOrganoidWeaponComponent.generated.h"

class AProjectOrganoidWeapon;
class AProjectOrganoidCharacter;

/**
 *  Equips and fires Avery's active weapon. Attach to AProjectOrganoidCharacter.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidWeaponComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidWeaponComponent();

	virtual void BeginPlay() override;

	/** Default weapon class spawned for Avery (P226-style hitscan by default) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AProjectOrganoidWeapon> DefaultWeaponClass;

	/** Socket on the character mesh used for attachment */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponAttachSocketName = TEXT("hand_r");

	/** Currently equipped weapon instance */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<AProjectOrganoidWeapon> EquippedWeapon;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AProjectOrganoidWeapon* EquipWeaponClass(TSubclassOf<AProjectOrganoidWeapon> WeaponClass);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool FireEquippedWeapon();

	/** Secondary overcharged pulse (strip bio-shields / clear toxic gas) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool FireOverchargedPulse();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AProjectOrganoidWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

protected:

	void SpawnDefaultWeapon();
};
