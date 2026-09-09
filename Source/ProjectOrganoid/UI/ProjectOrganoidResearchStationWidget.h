// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidResearchStationWidget.generated.h"

class AProjectOrganoidResearchStation;
class AProjectOrganoidCharacter;
class UProjectOrganoidWeaponModData;
class UProjectOrganoidBiologicalAdaptationData;
class UTextBlock;
class UButton;
class UVerticalBox;

/**
 *  Smallest functional Research Station UI.
 *  Proves open/close, unlocked vs installed, free remount. No final visual design.
 */
UCLASS()
class PROJECTORGANOID_API UProjectOrganoidResearchStationWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void BindToStation(AProjectOrganoidResearchStation* InStation, AProjectOrganoidCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void UnbindFromStation();

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	AProjectOrganoidResearchStation* GetBoundStation() const { return BoundStation; }

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	AProjectOrganoidCharacter* GetBoundCharacter() const { return BoundCharacter; }

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	bool InstallUnlockedMod(UProjectOrganoidWeaponModData* ModData);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	bool RemoveInstalledMod();

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool EquipUnlockedAdaptation(UProjectOrganoidBiologicalAdaptationData* AdaptationData);

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|Adaptations")
	bool UnequipAdaptation();

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void CloseStationUI();

	UFUNCTION(BlueprintCallable, Category = "ResearchStation|UI")
	void RefreshPresentation();

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	int32 GetConfigurationSOTCost() const { return 0; }

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	FText GetStatusText() const;

	UFUNCTION(BlueprintPure, Category = "ResearchStation|UI")
	bool IsStationUIOpen() const;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleInstallClicked();

	UFUNCTION()
	void HandleRemoveClicked();

	UFUNCTION()
	void HandleEquipAdaptationClicked();

	UFUNCTION()
	void HandleUnequipAdaptationClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void EnsureFunctionalLayout();

	UPROPERTY(BlueprintReadOnly, Category = "ResearchStation|UI")
	TObjectPtr<AProjectOrganoidResearchStation> BoundStation;

	UPROPERTY(BlueprintReadOnly, Category = "ResearchStation|UI")
	TObjectPtr<AProjectOrganoidCharacter> BoundCharacter;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusLabel;

	UPROPERTY()
	TObjectPtr<UButton> InstallButton;

	UPROPERTY()
	TObjectPtr<UButton> RemoveButton;

	UPROPERTY()
	TObjectPtr<UButton> EquipAdaptationButton;

	UPROPERTY()
	TObjectPtr<UButton> UnequipAdaptationButton;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;
};
