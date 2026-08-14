// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPhotoScanComponent.h"
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
	BindPhotoScanEvents();

	if (BoundCharacter->IsTacticalModeActive())
	{
		OnTacticalModeActivated();
	}
}

void UProjectOrganoidHUDWidget::UnbindFromCharacter()
{
	UnbindPhotoScanEvents();

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
		BoundObjectiveSubsystem->OnJournalUpdated.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleJournalUpdated);
		BoundObjectiveSubsystem->OnJournalStageChanged.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleJournalStageChanged);
		RefreshActiveObjectiveList();
		HandleJournalUpdated();
	}
}

void UProjectOrganoidHUDWidget::UnbindFromObjectiveSubsystem()
{
	if (BoundObjectiveSubsystem)
	{
		BoundObjectiveSubsystem->OnObjectivePopupRequested.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleObjectivePopup);
		BoundObjectiveSubsystem->OnJournalUpdated.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleJournalUpdated);
		BoundObjectiveSubsystem->OnJournalStageChanged.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleJournalStageChanged);
		BoundObjectiveSubsystem = nullptr;
	}
}

void UProjectOrganoidHUDWidget::BindPhotoScanEvents()
{
	UnbindPhotoScanEvents();
	if (!BoundCharacter)
	{
		return;
	}

	if (UProjectOrganoidPhotoScanComponent* Photo = BoundCharacter->GetPhotoScanComponent())
	{
		Photo->OnPhotoModeChanged.AddDynamic(this, &UProjectOrganoidHUDWidget::HandlePhotoModeChanged);
		Photo->OnScanFocusChanged.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleScanFocusChanged);
		Photo->OnScanCompleted.AddDynamic(this, &UProjectOrganoidHUDWidget::HandleScanCompleted);
		Photo->OnPhotoCaptured.AddDynamic(this, &UProjectOrganoidHUDWidget::HandlePhotoCaptured);
	}
}

void UProjectOrganoidHUDWidget::UnbindPhotoScanEvents()
{
	if (!BoundCharacter)
	{
		return;
	}

	if (UProjectOrganoidPhotoScanComponent* Photo = BoundCharacter->GetPhotoScanComponent())
	{
		Photo->OnPhotoModeChanged.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandlePhotoModeChanged);
		Photo->OnScanFocusChanged.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleScanFocusChanged);
		Photo->OnScanCompleted.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandleScanCompleted);
		Photo->OnPhotoCaptured.RemoveDynamic(this, &UProjectOrganoidHUDWidget::HandlePhotoCaptured);
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

void UProjectOrganoidHUDWidget::HandleJournalUpdated()
{
	if (BoundObjectiveSubsystem)
	{
		RefreshJournal(BoundObjectiveSubsystem->GetJournalEntries());
	}
}

void UProjectOrganoidHUDWidget::HandleJournalStageChanged(int32 StageIndex, const TArray<FProjectOrganoidObjective>& StageObjectives)
{
	ShowJournalStage(StageIndex, StageObjectives);
}

void UProjectOrganoidHUDWidget::HandlePhotoModeChanged(bool bActive)
{
	OnPhotoModeChanged(bActive);
}

void UProjectOrganoidHUDWidget::HandleScanFocusChanged(AActor* FocusedActor, FText DisplayName)
{
	OnScanFocusChanged(FocusedActor, DisplayName);
}

void UProjectOrganoidHUDWidget::HandleScanCompleted(AActor* /*ScannedActor*/, const FProjectOrganoidLogEntry& LoreEntry, bool bNewLore)
{
	OnScanLoreExtracted(LoreEntry, bNewLore);
}

void UProjectOrganoidHUDWidget::HandlePhotoCaptured(const FString& ScreenshotPath)
{
	OnPhotoCaptured(ScreenshotPath);
}

void UProjectOrganoidHUDWidget::RefreshActiveObjectiveList()
{
	if (BoundObjectiveSubsystem)
	{
		RefreshObjectiveList(BoundObjectiveSubsystem->GetActiveObjectives());
	}
}
