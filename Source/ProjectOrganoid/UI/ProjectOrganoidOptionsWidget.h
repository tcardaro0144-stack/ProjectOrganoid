#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "ProjectOrganoidOptionsWidget.generated.h"

class UButton;
class UComboBoxString;
class USlider;
class APlayerController;

/** Graphics, audio, and control reference. Spawned from the title screen. */
UCLASS()
class UProjectOrganoidOptionsWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Options")
	static UProjectOrganoidOptionsWidget* ShowForPlayer(APlayerController* PlayerController);

protected:

	virtual void NativeConstruct() override;

	void BuildLayout();

	UPROPERTY()
	TObjectPtr<USlider> MasterSlider;

	UPROPERTY()
	TObjectPtr<USlider> SfxSlider;

	UPROPERTY()
	TObjectPtr<USlider> MusicSlider;

	UPROPERTY()
	TObjectPtr<UComboBoxString> QualityCombo;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	UFUNCTION()
	void HandleMasterChanged(float Value);

	UFUNCTION()
	void HandleSfxChanged(float Value);

	UFUNCTION()
	void HandleMusicChanged(float Value);

	UFUNCTION()
	void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleCloseClicked();
};
