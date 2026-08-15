// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ProjectOrganoidLoadingScreenWidget.generated.h"

/**
 *  Full-screen loading overlay driven by UProjectOrganoidFlowManagerSubsystem.
 */
UCLASS()
class UProjectOrganoidLoadingScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "UI|Loading")
	void SetStatus(const FText& StatusText, float Progress01);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Loading")
	void OnLoadingStatusChanged(const FText& StatusText, float Progress01);

	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Loading")
	void OnLoadingFinished();

protected:

	UPROPERTY(BlueprintReadOnly, Category = "UI|Loading")
	FText CurrentStatus;

	UPROPERTY(BlueprintReadOnly, Category = "UI|Loading")
	float CurrentProgress = 0.0f;
};
