// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLoadingScreenWidget.h"

void UProjectOrganoidLoadingScreenWidget::SetStatus(const FText& StatusText, float Progress01)
{
	CurrentStatus = StatusText;
	CurrentProgress = FMath::Clamp(Progress01, 0.0f, 1.0f);
	OnLoadingStatusChanged(CurrentStatus, CurrentProgress);
}
