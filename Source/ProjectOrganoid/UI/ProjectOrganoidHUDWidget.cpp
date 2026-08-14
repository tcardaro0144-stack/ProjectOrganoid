// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidCharacter.h"

void UProjectOrganoidHUDWidget::BindToCharacter(AProjectOrganoidCharacter* InCharacter)
{
	if (BoundCharacter == InCharacter)
	{
		return;
	}

	UnbindFromCharacter();

	BoundCharacter = InCharacter;
	if (!BoundCharacter)
	{
		return;
	}

	BoundCharacter->OnTacticalModeChanged.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleTacticalModeChanged);
	UpdateVitalsFromCharacter();

	if (BoundCharacter->IsTacticalModeActive())
	{
		OnTacticalModeActivated();
	}
}

void UProjectOrganoidHUDWidget::UnbindFromCharacter()
{
	if (BoundCharacter)
	{
		BoundCharacter->OnTacticalModeChanged.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleTacticalModeChanged);
		BoundCharacter = nullptr;
	}
}

void UProjectOrganoidHUDWidget::UpdateVitalsFromCharacter()
{
	if (!BoundCharacter)
	{
		return;
	}

	SetHeartRateBPM(BoundCharacter->GetHeartRate());
	SetToxicityPercent(BoundCharacter->GetToxicity());
	SetHealth(BoundCharacter->GetHealth());
	SetPEEnergy(BoundCharacter->GetPEEnergy(), BoundCharacter->GetMaxPEEnergy());
}

void UProjectOrganoidHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundCharacter)
	{
		UpdateVitalsFromCharacter();
	}
}

void UProjectOrganoidHUDWidget::NativeDestruct()
{
	UnbindFromCharacter();
	Super::NativeDestruct();
}

void UProjectOrganoidHUDWidget::HandleTacticalModeChanged(bool bIsActive)
{
	if (bIsActive)
	{
		OnTacticalModeActivated();
	}
	else
	{
		OnTacticalModeDeactivated();
	}
}
