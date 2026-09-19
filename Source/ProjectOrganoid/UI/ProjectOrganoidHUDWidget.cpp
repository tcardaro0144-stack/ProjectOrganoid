// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidHUDWidget.h"
#include "ProjectOrganoidCharacter.h"
#include "ProjectOrganoidObjectiveSubsystem.h"
#include "ProjectOrganoidPhotoScanComponent.h"
#include "ProjectOrganoidGameplayHUDController.h"
#include "ProjectOrganoidInventoryComponent.h"
#include "ProjectOrganoidItemData.h"
#include "ProjectOrganoidWeapon.h"
#include "ProjectOrganoidWeaponComponent.h"
#include "ProjectOrganoidBiologicalAdaptationComponent.h"
#include "ProjectOrganoidBiologicalAdaptationTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"

void UProjectOrganoidHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureMinimalResourcePresentation();
	BindToObjectiveSubsystem();
}

void UProjectOrganoidHUDWidget::EnsureMinimalResourcePresentation()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	if (!AmmoReadoutText)
	{
		AmmoReadoutText = WidgetTree->FindWidget<UTextBlock>(TEXT("AmmoReadoutText"));
	}
	if (!AmmoReadoutText)
	{
		AmmoReadoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoReadoutText"));
		if (UCanvasPanelSlot* AmmoSlot = RootCanvas->AddChildToCanvas(AmmoReadoutText))
		{
			AmmoSlot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
			AmmoSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			AmmoSlot->SetAutoSize(true);
			AmmoSlot->SetOffsets(FMargin(0.0f, 0.0f, 36.0f, 28.0f));
			AmmoSlot->SetZOrder(20);
		}
		AmmoReadoutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.9f, 0.95f, 0.95f)));
	}

	if (!ResourceNotificationText)
	{
		ResourceNotificationText = WidgetTree->FindWidget<UTextBlock>(TEXT("ResourceNotificationText"));
	}
	if (!ResourceNotificationText)
	{
		ResourceNotificationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResourceNotificationText"));
		if (UCanvasPanelSlot* NotifySlot = RootCanvas->AddChildToCanvas(ResourceNotificationText))
		{
			NotifySlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			NotifySlot->SetAlignment(FVector2D(0.5f, 0.0f));
			NotifySlot->SetAutoSize(true);
			NotifySlot->SetOffsets(FMargin(0.0f, 72.0f, 0.0f, 0.0f));
			NotifySlot->SetZOrder(21);
		}
		ResourceNotificationText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.94f, 0.88f, 0.95f)));
		ResourceNotificationText->SetJustification(ETextJustify::Center);
		ResourceNotificationText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UProjectOrganoidHUDWidget::NotifyResourceAcquired(UProjectOrganoidItemData* ItemData, int32 Quantity)
{
	if (!ItemData || Quantity <= 0)
	{
		return;
	}

	if (BoundCharacter)
	{
		LastResourceNotification = FText::FromString(BoundCharacter->GetLastResourceFeedback());
	}

	if (LastResourceNotification.IsEmpty())
	{
		FString Name = ItemData->ItemName.ToString();
		if (Name.IsEmpty())
		{
			Name = ItemData->GetName();
		}
		LastResourceNotification = FText::FromString(FString::Printf(TEXT("%s acquired."), *Name));
	}

	if (ResourceNotificationText)
	{
		ResourceNotificationText->SetText(LastResourceNotification);
		ResourceNotificationText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	ResourceNotificationSecondsRemaining = 4.0f;
}

void UProjectOrganoidHUDWidget::ShowTransientNotification(const FText& SpeakerLabel, const FText& Line, float Seconds)
{
	const FString Speaker = SpeakerLabel.ToString().TrimStartAndEnd();
	const FString LineText = Line.ToString();
	if (Speaker.IsEmpty())
	{
		LastResourceNotification = FText::FromString(LineText);
	}
	else
	{
		LastResourceNotification = FText::FromString(FString::Printf(TEXT("%s: %s"), *Speaker, *LineText));
	}

	if (ResourceNotificationText)
	{
		ResourceNotificationText->SetText(LastResourceNotification);
		ResourceNotificationText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	ResourceNotificationSecondsRemaining = FMath::Max(0.0f, Seconds);
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
	UpdateAmmoFromCharacter();
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
	UpdateAdaptationFromCharacter();
}

void UProjectOrganoidHUDWidget::UpdateAdaptationFromCharacter()
{
	if (!BoundCharacter)
	{
		SetEquippedAdaptation(FText::FromString(TEXT("None")), 0.0f, 0.0f, 0.0f, false, TEXT("Unequipped"));
		return;
	}

	UProjectOrganoidBiologicalAdaptationComponent* AdaptComp = BoundCharacter->GetBiologicalAdaptationComponent();
	UProjectOrganoidBiologicalAdaptationData* Equipped = AdaptComp ? AdaptComp->GetEquippedAdaptation() : nullptr;
	if (!Equipped)
	{
		SetEquippedAdaptation(FText::FromString(TEXT("None")), 0.0f, 0.0f, 0.0f, false, TEXT("Unequipped"));
		return;
	}

	const FName Unavailable = AdaptComp->GetUnavailableReason();
	SetEquippedAdaptation(
		Equipped->DisplayName,
		Equipped->PECost,
		AdaptComp->GetCooldownRemaining(),
		AdaptComp->GetCooldownDuration(),
		Unavailable.IsNone(),
		Unavailable);
}

void UProjectOrganoidHUDWidget::UpdateAmmoFromCharacter()
{
	if (!BoundCharacter)
	{
		SetWeaponAmmo(0, 0, 0);
		return;
	}

	AProjectOrganoidWeapon* Weapon = BoundCharacter->GetWeaponComponent()
		? BoundCharacter->GetWeaponComponent()->GetEquippedWeapon()
		: nullptr;
	UProjectOrganoidInventoryComponent* Inventory = BoundCharacter->GetInventoryComponent();

	const int32 Current = Weapon ? Weapon->GetCurrentMagazine() : 0;
	const int32 Capacity = Weapon ? Weapon->GetMagazineCapacity() : 0;
	const int32 Reserve = (Weapon && Inventory)
		? Inventory->CountAmmoOfType(Weapon->AmmoType)
		: 0;
	SetWeaponAmmo(Current, Reserve, Capacity);

	LastAmmoText = FText::FromString(FString::Printf(TEXT("%d  |  Reserve %d"), Current, Reserve));
	if (AmmoReadoutText)
	{
		AmmoReadoutText->SetText(LastAmmoText);
		AmmoReadoutText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UProjectOrganoidHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BoundCharacter)
	{
		UpdateVitalsFromCharacter();
		UpdateAmmoFromCharacter();
	}

	if (ResourceNotificationSecondsRemaining > 0.0f)
	{
		ResourceNotificationSecondsRemaining = FMath::Max(0.0f, ResourceNotificationSecondsRemaining - InDeltaTime);
		if (ResourceNotificationSecondsRemaining <= 0.0f && ResourceNotificationText)
		{
			ResourceNotificationText->SetVisibility(ESlateVisibility::Hidden);
		}
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

void UProjectOrganoidHUDWidget::BindGameplayLoopController(UProjectOrganoidGameplayHUDController* Controller)
{
	GameplayLoopController = Controller;
}
