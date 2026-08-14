// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidWeaponModComponent.generated.h"

class AProjectOrganoidWeapon;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProjectOrganoidWeaponModChanged, EProjectOrganoidWeaponModSlot, Slot, UProjectOrganoidWeaponModData*, ModData);

/**
 *  Manages firearm attachments and aggregates damage / noise multipliers.
 *  Owned by AProjectOrganoidWeapon.
 */
UCLASS(ClassGroup = (ProjectOrganoid), meta = (BlueprintSpawnableComponent))
class UProjectOrganoidWeaponModComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UProjectOrganoidWeaponModComponent();

	/** When any installed mod has bSuppressesSoundEmission, loudness is at most this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons|Mods|Audio", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SuppressorLoudnessCeiling = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapons|Mods|Audio", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float SuppressorRangeCeiling = 0.45f;

	UPROPERTY(BlueprintAssignable, Category = "Weapons|Mods")
	FOnProjectOrganoidWeaponModChanged OnModInstalled;

	UPROPERTY(BlueprintAssignable, Category = "Weapons|Mods")
	FOnProjectOrganoidWeaponModChanged OnModRemoved;

	UFUNCTION(BlueprintCallable, Category = "Weapons|Mods")
	bool InstallMod(UProjectOrganoidWeaponModData* ModData, bool bReplaceExisting = true);

	UFUNCTION(BlueprintCallable, Category = "Weapons|Mods")
	bool RemoveModFromSlot(EProjectOrganoidWeaponModSlot Slot);

	UFUNCTION(BlueprintCallable, Category = "Weapons|Mods")
	void ClearAllMods();

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	UProjectOrganoidWeaponModData* GetModInSlot(EProjectOrganoidWeaponModSlot Slot) const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	TArray<UProjectOrganoidWeaponModData*> GetInstalledMods() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	bool HasSuppressor() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	float GetDamageMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	float GetFireRateMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	float GetPenetrationMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	float GetNoiseLoudnessMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "Weapons|Mods")
	float GetNoiseRangeMultiplier() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons|Mods|Save")
	TArray<FProjectOrganoidSavedWeaponMod> CaptureModState() const;

	UFUNCTION(BlueprintCallable, Category = "Weapons|Mods|Save")
	void ApplyModState(const TArray<FProjectOrganoidSavedWeaponMod>& SavedMods);

protected:

	/** One mod per slot */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapons|Mods")
	TMap<EProjectOrganoidWeaponModSlot, TObjectPtr<UProjectOrganoidWeaponModData>> EquippedMods;

	AProjectOrganoidWeapon* GetOwnerWeapon() const;
	void RecalculateCachedMultipliers();

	float CachedDamageMultiplier = 1.0f;
	float CachedFireRateMultiplier = 1.0f;
	float CachedPenetrationMultiplier = 1.0f;
	float CachedNoiseLoudnessMultiplier = 1.0f;
	float CachedNoiseRangeMultiplier = 1.0f;
	bool bCachedHasSuppressor = false;
};
