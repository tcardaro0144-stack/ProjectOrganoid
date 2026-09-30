#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataTable.h"
#include "ProjectOrganoidTutorialWidget.generated.h"

class APlayerController;

USTRUCT(BlueprintType)
struct FProjectOrganoidTutorialControlRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FString Control;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FString Keyboard;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial")
	FString Gamepad;
};

/** Hit-test-invisible control card. Does not capture the mouse. */
UCLASS()
class UProjectOrganoidTutorialWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	static void ShowForPlayer(APlayerController* PlayerController);

protected:

	virtual void NativeConstruct() override;

	void BuildLayout();
};
