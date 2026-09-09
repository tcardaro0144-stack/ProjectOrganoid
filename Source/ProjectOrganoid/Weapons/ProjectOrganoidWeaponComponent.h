// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ProjectOrganoidWeaponTypes.h"
#include "ProjectOrganoidWeaponComponent.generated.h"

class AProjectOrganoidWeapon;
class AProjectOrganoidCharacter;

/**
 *  Equips and fires Avery's active weapon. Attach to AProjectOrganoidCharacter.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class PROJECTORGANOID_API UProjectOrganoidWeaponComponent : public USceneComponent
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

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	bool ReloadEquippedWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	void CancelReload();

	/** Secondary overcharged pulse (strip bio-shields / clear toxic gas) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool FireOverchargedPulse();

	UFUNCTION(BlueprintPure, Category = "Weapon")
	AProjectOrganoidWeapon* GetEquippedWeapon() const { return EquippedWeapon; }

	/**
	 *  Equipped + holstered magazine snapshots. Future weapon switching
	 *  upserts by WeaponClass without changing fire/reload/inventory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo|Save")
	TArray<FProjectOrganoidWeaponMagazineState> CaptureMagazineStates() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo|Save")
	void ApplyMagazineStates(const TArray<FProjectOrganoidWeaponMagazineState>& States);

	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	int32 GetHolsteredMagazineCount(TSubclassOf<AProjectOrganoidWeapon> WeaponClass) const;

protected:

	void SpawnDefaultWeapon();
	void StoreEquippedMagazineState();
	void RestoreMagazineStateFor(AProjectOrganoidWeapon* Weapon);
	void UpsertMagazineState(const FProjectOrganoidWeaponMagazineState& State);

	/** Holstered / unequipped magazine snapshots keyed by weapon class path. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Ammo")
	TArray<FProjectOrganoidWeaponMagazineState> HolsteredMagazineStates;
};
