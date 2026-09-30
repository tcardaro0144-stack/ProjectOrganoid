#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidIntroWidget.generated.h"

class UTextBlock;

/** Short title card: EPITOPE | Prepared Immunity. Plays on New Game before travel. */
UCLASS()
class UProjectOrganoidIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	static UProjectOrganoidIntroWidget* Show(UWorld* World);

	void Dismiss();

protected:

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	float PanTime = 0.0f;
};
