// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ProjectOrganoidInteractable.h"
#include "ProjectOrganoidWeaponModTypes.h"
#include "ProjectOrganoidResearchStation.generated.h"

class AProjectOrganoidCharacter;
class UProjectOrganoidResearchStationWidget;
class UProjectOrganoidWeaponModData;
class UProjectOrganoidBiologicalAdaptationData;
class UStaticMeshComponent;

/**
 *  Research Station — free remount of already-unlocked build elements.
 *  Unlock and install are separate. No SOT charge. No heal. No checkpoint.
 *  Interaction is locked only by encounter presence (Pursue / Attack).
 */
UCLASS(Blueprintable)
class PROJECTORGANOID_API AProjectOrganoidResearchStation : public AProjectOrganoidInteractable
{
	GENERATED_BODY()

public:

	AProjectOrganoidResearchStation();

	/** Provisional blockout console. Not final Research Station art. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ResearchStation")
	TObjectPtr<UStaticMeshComponent> StationMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ResearchStation|UI")
	TSubclassOf<UProjectOrganoidResearchStationWidget> StationWidgetClass;

	virtual bool CanInteract_Implementation(AProjectOrganoidCharacter* Interactor) const override;
	virtual bool Interact_Implementation(AProjectOrganoidCharacter* Interactor) override;

	UFUNCTION(BlueprintPure, Category = "ResearchStation")
	bool IsLockedByEncounter() const;

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	bool IsStationUIOpen() const;

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	UProjectOrganoidResearchStationWidget* GetActiveStationWidget() const { return ActiveStationWidget; }

	UFUNCTION(BlueprintPure, Category = "ResearchStation")
	int32 GetConfigurationSOTCost() const { return 0; }

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	UProjectOrganoidResearchStationWidget* OpenResearchStationUI(AProjectOrganoidCharacter* Interactor);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void CloseResearchStationUI();

	void NotifyStationUIClosed();

	/** Install an already-unlocked mod. Costs 0 SOT. Does not consume items. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Loadout")
	bool TryInstallUnlockedMod(AProjectOrganoidCharacter* Character, UProjectOrganoidWeaponModData* ModData);

	/** Remove an installed mod. Ownership is retained. Costs 0 SOT. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Loadout")
	bool TryRemoveInstalledMod(AProjectOrganoidCharacter* Character, EProjectOrganoidWeaponModSlot Slot);

	/** Equip an already-unlocked adaptation. Costs 0 SOT. Ownership is retained on unequip. */
	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool TryEquipUnlockedAdaptation(AProjectOrganoidCharacter* Character, UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool TryUnequipAdaptation(AProjectOrganoidCharacter* Character);

protected:

	UPROPERTY()
	TObjectPtr<UProjectOrganoidResearchStationWidget> ActiveStationWidget;
};
