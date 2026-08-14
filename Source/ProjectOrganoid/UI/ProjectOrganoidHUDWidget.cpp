// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UProjectOrganoidHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindToObjectiveSubsystem();
}

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
	BindToObjectiveSubsystem();

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

void UProjectOrganoidHUDWidget::BindToObjectiveSubsystem()
{
	UnbindFromObjectiveSubsystem();

	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		BoundObjectiveSubsystem = GI->GetSubsystem<UProjectOrganoidObjectiveSubsystem>();
	}

	if (BoundObjectiveSubsystem)
	{
		BoundObjectiveSubsystem->OnObjectivePopupRequested.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleObjectivePopup);
		RefreshActiveObjectiveList();
	}
}

void UProjectOrganoidHUDWidget::UnbindFromObjectiveSubsystem()
{
	if (BoundObjectiveSubsystem)
	{
		BoundObjectiveSubsystem->OnObjectivePopupRequested.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleObjectivePopup);
		BoundObjectiveSubsystem = nullptr;
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
	UnbindFromObjectiveSubsystem();
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

void UProjectOrganoidHUDWidget::HandleObjectivePopup(const FProjectOrganoidObjective& Objective, FName PopupReason)
{
	ShowObjectivePopup(Objective, PopupReason);
	RefreshActiveObjectiveList();
}

void UProjectOrganoidHUDWidget::RefreshActiveObjectiveList()
{
	if (BoundObjectiveSubsystem)
	{
		RefreshObjectiveList(BoundObjectiveSubsystem->GetActiveObjectives());
	}
}
