// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace ProjectOrganoidMenuUI
{
	static constexpr float MenuButtonWidth = 300.0f;
	static constexpr float MenuButtonHeight = 80.0f;
	static const FName NewGameButtonName(TEXT("NewGameButton"));
	static const FName RuntimeBackdropName(TEXT("RuntimeMenuDarkImage"));
	static const FName LegacyBackdropName(TEXT("RuntimeMenuBackdrop"));
	static const FName RuntimeSizeBoxName(TEXT("NewGameButton_FixedSizeBox"));
	static const FName RuntimeLabelName(TEXT("NewGameButton_Label"));
	static const FName DesignerMenuBoxName(TEXT("MenuBox"));

	static void CollapseOtherCanvasChildren(UCanvasPanel* Canvas, UWidget* KeepA, UWidget* KeepB)
	{
		if (!Canvas)
		{
			return;
		}
		const TArray<UPanelSlot*>& Slots = Canvas->GetSlots();
		for (UPanelSlot* PanelSlot : Slots)
		{
			if (!PanelSlot || !PanelSlot->Content)
			{
				continue;
			}
			UWidget* Child = PanelSlot->Content;
			if (Child == KeepA || Child == KeepB)
			{
				continue;
			}
			Child->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	static void PinCenteredFixedSlot(UCanvasPanelSlot* Slot, float Width, float Height, float ZOrder)
	{
		if (!Slot)
		{
			return;
		}
		Slot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetAutoSize(false);
		// Left/Top = position relative to center; Right/Bottom = size for point anchors.
		Slot->SetOffsets(FMargin(
			-Width * 0.5f,
			-Height * 0.5f,
			Width,
			Height));
		Slot->SetZOrder(ZOrder);
	}

	static const TArray<FString> GraphicsQualityLabels = {
		TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic")
	};

	static EProjectOrganoidGraphicsQuality QualityFromLabel(const FString& Label)
	{
		const int32 Index = GraphicsQualityLabels.IndexOfByKey(Label);
		return static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(Index, 0, 4));
	}
}

void UProjectOrganoidMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureVisibleMenuLayout();
	BindWidgetCallbacks();
	SyncSettingsWidgets();
	RefreshSaveSlots();
}

void UProjectOrganoidMainMenuWidget::EnsureVisibleMenuLayout()
{
	if (!WidgetTree)
	{
		UE_LOG(LogTemp, Error, TEXT("MainMenu: WidgetTree is null — cannot build visible menu."));
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	// Collapse designer MenuBox / legacy green-era backdrop so they cannot overpaint.
	if (UWidget* DesignerMenu = WidgetTree->FindWidget(ProjectOrganoidMenuUI::DesignerMenuBoxName))
	{
		DesignerMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWidget* LegacyBackdrop = WidgetTree->FindWidget(ProjectOrganoidMenuUI::LegacyBackdropName))
	{
		LegacyBackdrop->SetVisibility(ESlateVisibility::Collapsed);
		LegacyBackdrop->RemoveFromParent();
	}

	// Dark full-screen Image (not a Border, not green) — never hosts the button.
	UImage* Backdrop = WidgetTree->FindWidget<UImage>(ProjectOrganoidMenuUI::RuntimeBackdropName);
	if (!Backdrop)
	{
		Backdrop = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			ProjectOrganoidMenuUI::RuntimeBackdropName);
	}
	if (UPanelWidget* BackdropParent = Backdrop->GetParent())
	{
		if (BackdropParent != RootCanvas)
		{
			BackdropParent->RemoveChild(Backdrop);
		}
	}
	UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(Backdrop->Slot);
	if (!BackdropSlot)
	{
		BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	}
	if (BackdropSlot)
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		BackdropSlot->SetAutoSize(false);
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetZOrder(0.0f);
	}
	{
		FSlateBrush DarkBrush;
		DarkBrush.DrawAs = ESlateBrushDrawType::Image;
		DarkBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.92f));
		DarkBrush.ImageSize = FVector2D(32.0f, 32.0f);
		Backdrop->SetBrush(DarkBrush);
	}
	Backdrop->SetColorAndOpacity(FLinearColor::White);
	Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Resolve / create NewGameButton (the only green control).
	UButton* PlayButton = NewGameButton.Get();
	if (!PlayButton)
	{
		PlayButton = WidgetTree->FindWidget<UButton>(ProjectOrganoidMenuUI::NewGameButtonName);
	}
	if (!PlayButton)
	{
		PlayButton = WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(),
			ProjectOrganoidMenuUI::NewGameButtonName);
	}
	NewGameButton = PlayButton;

	// Style after parenting so a stretched slot cannot paint green full-screen first.
	PlayButton->SetColorAndOpacity(FLinearColor::White);
	PlayButton->SetVisibility(ESlateVisibility::Visible);

	if (!PlayButton->GetContent())
	{
		UTextBlock* Label = WidgetTree->FindWidget<UTextBlock>(ProjectOrganoidMenuUI::RuntimeLabelName);
		if (!Label)
		{
			Label = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				ProjectOrganoidMenuUI::RuntimeLabelName);
		}
		Label->SetText(FText::FromString(TEXT("NEW GAME")));
		Label->SetJustification(ETextJustify::Center);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		PlayButton->SetContent(Label);
	}

	USizeBox* FixedSizeBox = WidgetTree->FindWidget<USizeBox>(ProjectOrganoidMenuUI::RuntimeSizeBoxName);
	if (!FixedSizeBox)
	{
		FixedSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			ProjectOrganoidMenuUI::RuntimeSizeBoxName);
	}

	if (UPanelWidget* ButtonParent = PlayButton->GetParent())
	{
		if (ButtonParent != FixedSizeBox)
		{
			ButtonParent->RemoveChild(PlayButton);
		}
	}
	if (UPanelWidget* SizeBoxParent = FixedSizeBox->GetParent())
	{
		if (SizeBoxParent != RootCanvas)
		{
			SizeBoxParent->RemoveChild(FixedSizeBox);
		}
	}

	FixedSizeBox->ClearWidthOverride();
	FixedSizeBox->ClearHeightOverride();
	FixedSizeBox->SetWidthOverride(ProjectOrganoidMenuUI::MenuButtonWidth);
	FixedSizeBox->SetHeightOverride(ProjectOrganoidMenuUI::MenuButtonHeight);
	FixedSizeBox->SetMinDesiredWidth(ProjectOrganoidMenuUI::MenuButtonWidth);
	FixedSizeBox->SetMinDesiredHeight(ProjectOrganoidMenuUI::MenuButtonHeight);
	FixedSizeBox->SetMaxDesiredWidth(ProjectOrganoidMenuUI::MenuButtonWidth);
	FixedSizeBox->SetMaxDesiredHeight(ProjectOrganoidMenuUI::MenuButtonHeight);
	FixedSizeBox->SetContent(PlayButton);
	FixedSizeBox->SetVisibility(ESlateVisibility::Visible);

	UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(FixedSizeBox->Slot);
	if (!ButtonSlot || ButtonSlot->Parent != RootCanvas)
	{
		if (FixedSizeBox->GetParent())
		{
			FixedSizeBox->RemoveFromParent();
		}
		ButtonSlot = RootCanvas->AddChildToCanvas(FixedSizeBox);
	}
	ProjectOrganoidMenuUI::PinCenteredFixedSlot(
		ButtonSlot,
		ProjectOrganoidMenuUI::MenuButtonWidth,
		ProjectOrganoidMenuUI::MenuButtonHeight,
		10.0f);

	// Green only on the sized button — never on the backdrop.
	PlayButton->SetBackgroundColor(FLinearColor(0.12f, 0.55f, 0.32f, 1.0f));

	// UUserWidget defaults to non-focusable, which makes FInputModeUIOnly warn and
	// drop focus. SObjectWidget queries this live, so setting it here is honoured.
	SetIsFocusable(true);

	ProjectOrganoidMenuUI::CollapseOtherCanvasChildren(RootCanvas, Backdrop, FixedSizeBox);

	SetVisibility(ESlateVisibility::Visible);
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();

	const FVector2D Desired = FixedSizeBox->GetDesiredSize();
	UE_LOG(LogTemp, Log,
		TEXT("MainMenu: LAYOUT_v2 dark image backdrop + NewGame only (slot %.0fx%.0f, desired %.0fx%.0f)."),
		ProjectOrganoidMenuUI::MenuButtonWidth,
		ProjectOrganoidMenuUI::MenuButtonHeight,
		Desired.X,
		Desired.Y);
}

UWidget* UProjectOrganoidMainMenuWidget::GetDefaultFocusWidget()
{
	if (NewGameButton)
	{
		return NewGameButton;
	}
	return this;
}

void UProjectOrganoidMainMenuWidget::BindWidgetCallbacks()
{
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleNewGameClicked);
	}
	if (LoadGameButton)
	{
		LoadGameButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked);
	}
	if (LoadSlot0Button)
	{
		LoadSlot0Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked);
	}
	if (LoadSlot1Button)
	{
		LoadSlot1Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot1Clicked);
	}
	if (LoadSlot2Button)
	{
		LoadSlot2Button->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleLoadSlot2Clicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleQuitClicked);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleMasterVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleSFXVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleMusicVolumeChanged);
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidMainMenuWidget::HandleGraphicsQualityChanged);
	}
}

void UProjectOrganoidMainMenuWidget::SyncSettingsWidgets()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(GetMasterVolume());
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(GetSFXVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(GetMusicVolume());
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->ClearOptions();
		for (const FString& Label : ProjectOrganoidMenuUI::GraphicsQualityLabels)
		{
			GraphicsQualityCombo->AddOption(Label);
		}
		const int32 QualityIndex = static_cast<int32>(GetGraphicsQuality());
		if (ProjectOrganoidMenuUI::GraphicsQualityLabels.IsValidIndex(QualityIndex))
		{
			GraphicsQualityCombo->SetSelectedOption(ProjectOrganoidMenuUI::GraphicsQualityLabels[QualityIndex]);
		}
	}
}

void UProjectOrganoidMainMenuWidget::HandleNewGameClicked()
{
	StartNewGame();
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot0Clicked()
{
	LoadGameFromSlot(0);
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot1Clicked()
{
	LoadGameFromSlot(1);
}

void UProjectOrganoidMainMenuWidget::HandleLoadSlot2Clicked()
{
	LoadGameFromSlot(2);
}

void UProjectOrganoidMainMenuWidget::HandleQuitClicked()
{
	QuitGame();
}

void UProjectOrganoidMainMenuWidget::HandleMasterVolumeChanged(float Value)
{
	SetMasterVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleSFXVolumeChanged(float Value)
{
	SetSFXVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleMusicVolumeChanged(float Value)
{
	SetMusicVolume(Value);
}

void UProjectOrganoidMainMenuWidget::HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	SetGraphicsQuality(ProjectOrganoidMenuUI::QualityFromLabel(SelectedItem));
}

void UProjectOrganoidMainMenuWidget::StartNewGame()
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->ClearPendingLoad();
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidFlowManagerSubsystem* Flow = GI->GetSubsystem<UProjectOrganoidFlowManagerSubsystem>())
		{
			Flow->GameplayLevelName = GameplayLevelName;
			Flow->StartNewGame(GameplayLevelName);
			return;
		}
	}

	UGameplayStatics::OpenLevel(this, GameplayLevelName);
}

bool UProjectOrganoidMainMenuWidget::LoadGameFromSlot(int32 SlotIndex)
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		return LoadGameFromSlotName(SaveSubsystem->GetSlotNameForIndex(SlotIndex));
	}
	return false;
}

bool UProjectOrganoidMainMenuWidget::LoadGameFromSlotName(const FString& SlotName)
{
	UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || !SaveSubsystem->DoesSaveExist(SlotName))
	{
		return false;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidFlowManagerSubsystem* Flow = GI->GetSubsystem<UProjectOrganoidFlowManagerSubsystem>())
		{
			Flow->GameplayLevelName = GameplayLevelName;
			Flow->LoadGameAndTravel(SlotName, GameplayLevelName);
			return true;
		}
	}

	SaveSubsystem->RequestLoadOnNextTravel(SlotName);
	UGameplayStatics::OpenLevel(this, GameplayLevelName);
	return true;
}

TArray<FProjectOrganoidSaveSlotInfo> UProjectOrganoidMainMenuWidget::GetSaveSlotInfos() const
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		return SaveSubsystem->GetSaveSlotInfos(MaxSaveSlots);
	}
	return TArray<FProjectOrganoidSaveSlotInfo>();
}

void UProjectOrganoidMainMenuWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}

void UProjectOrganoidMainMenuWidget::SetMasterVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMasterVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetSFXVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetSFXVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetMusicVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMusicVolume(NewVolume);
	}
}

void UProjectOrganoidMainMenuWidget::SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetGraphicsQuality(NewQuality);
	}
}

float UProjectOrganoidMainMenuWidget::GetMasterVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMasterVolume();
	}
	return 1.0f;
}

float UProjectOrganoidMainMenuWidget::GetSFXVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetSFXVolume();
	}
	return 1.0f;
}

float UProjectOrganoidMainMenuWidget::GetMusicVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMusicVolume();
	}
	return 1.0f;
}

EProjectOrganoidGraphicsQuality UProjectOrganoidMainMenuWidget::GetGraphicsQuality() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetGraphicsQuality();
	}
	return EProjectOrganoidGraphicsQuality::High;
}

void UProjectOrganoidMainMenuWidget::ApplySettings()
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->ApplyAllSettings();
	}
}

UProjectOrganoidSaveSubsystem* UProjectOrganoidMainMenuWidget::GetSaveSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSaveSubsystem>();
	}
	return nullptr;
}

UProjectOrganoidSettingsSubsystem* UProjectOrganoidMainMenuWidget::GetSettingsSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>();
	}
	return nullptr;
}

void UProjectOrganoidMainMenuWidget::RefreshSaveSlots()
{
	OnSaveSlotsRefreshed(GetSaveSlotInfos());
}
