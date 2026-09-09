// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidPauseWidget.h"
#include "ProjectOrganoidPlayerController.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

namespace ProjectOrganoidPauseUI
{
	static const TArray<FString> GraphicsQualityLabels = {
		TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic")
	};

	static const TArray<FString> WindowModeLabels = {
		TEXT("Fullscreen"), TEXT("Borderless Window"), TEXT("Windowed")
	};

	static EProjectOrganoidGraphicsQuality QualityFromLabel(const FString& Label)
	{
		const int32 Index = GraphicsQualityLabels.IndexOfByKey(Label);
		return static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(Index, 0, 4));
	}

	static float ResolutionPercentFromNormalized(float Normalized)
	{
		return FMath::Lerp(50.0f, 100.0f, FMath::Clamp(Normalized, 0.0f, 1.0f));
	}

	static float ResolutionNormalizedFromPercent(float Percent)
	{
		return FMath::GetMappedRangeValueClamped(FVector2D(50.0f, 100.0f), FVector2D(0.0f, 1.0f), Percent);
	}
}

void UProjectOrganoidPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureVisiblePauseLayout();
	BindWidgetCallbacks();
	SyncSettingsWidgets();
}

void UProjectOrganoidPauseWidget::EnsureVisiblePauseLayout()
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

	if (!WidgetTree->FindWidget(TEXT("PauseDimmer")))
	{
		UImage* Dimmer = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("PauseDimmer"));
		if (UCanvasPanelSlot* DimmerSlot = RootCanvas->AddChildToCanvas(Dimmer))
		{
			DimmerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			DimmerSlot->SetOffsets(FMargin(0.0f));
			DimmerSlot->SetZOrder(0);
		}
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		Dimmer->SetBrush(Brush);
		Dimmer->SetVisibility(ESlateVisibility::Visible);
	}

	if (ResumeButton && ReturnToMainMenuButton && QuitButton)
	{
		return;
	}

	UVerticalBox* MenuBox = WidgetTree->FindWidget<UVerticalBox>(TEXT("PauseBox"));
	if (!MenuBox)
	{
		MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseBox"));
		if (UCanvasPanelSlot* BoxSlot = RootCanvas->AddChildToCanvas(MenuBox))
		{
			BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			BoxSlot->SetAutoSize(true);
			BoxSlot->SetZOrder(1);
		}
	}

	auto MakeLabeledButton = [this, MenuBox](TObjectPtr<UButton>& Target, const TCHAR* Name, const TCHAR* Label)
	{
		if (Target)
		{
			return;
		}
		Target = WidgetTree->FindWidget<UButton>(Name);
		if (!Target)
		{
			Target = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Text->SetText(FText::FromString(Label));
			Target->SetContent(Text);
			MenuBox->AddChildToVerticalBox(Target);
		}
	};

	if (!TitleText)
	{
		TitleText = WidgetTree->FindWidget<UTextBlock>(TEXT("TitleText"));
		if (!TitleText)
		{
			TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
			TitleText->SetText(FText::FromString(TEXT("PAUSED")));
			MenuBox->AddChildToVerticalBox(TitleText);
		}
	}

	MakeLabeledButton(ResumeButton, TEXT("ResumeButton"), TEXT("RESUME"));
	MakeLabeledButton(ReturnToMainMenuButton, TEXT("ReturnToMainMenuButton"), TEXT("MAIN MENU"));
	MakeLabeledButton(QuitButton, TEXT("QuitButton"), TEXT("QUIT"));
}

void UProjectOrganoidPauseWidget::BindWidgetCallbacks()
{
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleResumeClicked);
	}
	if (ReturnToMainMenuButton)
	{
		ReturnToMainMenuButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleReturnToMainMenuClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleQuitClicked);
	}
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleMasterVolumeChanged);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleSFXVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleMusicVolumeChanged);
	}
	if (GraphicsQualityCombo)
	{
		GraphicsQualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleGraphicsQualityChanged);
	}
	if (WindowModeCombo)
	{
		WindowModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleWindowModeChanged);
	}
	if (ResolutionScaleSlider)
	{
		ResolutionScaleSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidPauseWidget::HandleResolutionScaleChanged);
	}
}

void UProjectOrganoidPauseWidget::SyncSettingsWidgets()
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
		for (const FString& Label : ProjectOrganoidPauseUI::GraphicsQualityLabels)
		{
			GraphicsQualityCombo->AddOption(Label);
		}
		const int32 QualityIndex = static_cast<int32>(GetGraphicsQuality());
		if (ProjectOrganoidPauseUI::GraphicsQualityLabels.IsValidIndex(QualityIndex))
		{
			GraphicsQualityCombo->SetSelectedOption(ProjectOrganoidPauseUI::GraphicsQualityLabels[QualityIndex]);
		}
	}
	if (WindowModeCombo)
	{
		WindowModeCombo->ClearOptions();
		for (const FString& Label : ProjectOrganoidPauseUI::WindowModeLabels)
		{
			WindowModeCombo->AddOption(Label);
		}
		WindowModeCombo->SetSelectedOption(UProjectOrganoidSettingsSubsystem::WindowModeToLabel(GetWindowMode()));
	}
	if (ResolutionScaleSlider)
	{
		ResolutionScaleSlider->SetValue(ProjectOrganoidPauseUI::ResolutionNormalizedFromPercent(GetResolutionScalePercent()));
	}
}

void UProjectOrganoidPauseWidget::HandleResumeClicked()
{
	ResumeGame();
}

void UProjectOrganoidPauseWidget::HandleReturnToMainMenuClicked()
{
	ReturnToMainMenu();
}

void UProjectOrganoidPauseWidget::HandleQuitClicked()
{
	QuitGame();
}

void UProjectOrganoidPauseWidget::HandleMasterVolumeChanged(float Value)
{
	SetMasterVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleSFXVolumeChanged(float Value)
{
	SetSFXVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleMusicVolumeChanged(float Value)
{
	SetMusicVolume(Value);
}

void UProjectOrganoidPauseWidget::HandleGraphicsQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	SetGraphicsQuality(ProjectOrganoidPauseUI::QualityFromLabel(SelectedItem));
}

void UProjectOrganoidPauseWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	SetWindowMode(UProjectOrganoidSettingsSubsystem::WindowModeFromLabel(SelectedItem));
}

void UProjectOrganoidPauseWidget::HandleResolutionScaleChanged(float Value)
{
	SetResolutionScalePercent(ProjectOrganoidPauseUI::ResolutionPercentFromNormalized(Value));
}

void UProjectOrganoidPauseWidget::ResumeGame()
{
	if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
	else if (APlayerController* FallbackPC = GetOwningPlayer())
	{
		FallbackPC->SetPause(false);
		RemoveFromParent();
	}
}

void UProjectOrganoidPauseWidget::ReturnToMainMenu()
{
	if (AProjectOrganoidPlayerController* PC = Cast<AProjectOrganoidPlayerController>(GetOwningPlayer()))
	{
		PC->ClosePauseMenu();
	}
	else if (APlayerController* FallbackPC = GetOwningPlayer())
	{
		FallbackPC->SetPause(false);
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UProjectOrganoidFlowManagerSubsystem* Flow = GI->GetSubsystem<UProjectOrganoidFlowManagerSubsystem>())
		{
			Flow->TitleLevelName = MainMenuLevelName;
			Flow->ReturnToTitle();
			return;
		}
	}

	UGameplayStatics::OpenLevel(this, MainMenuLevelName);
}

void UProjectOrganoidPauseWidget::QuitGame()
{
	APlayerController* PC = GetOwningPlayer();
	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(PC))
	{
		OrganoidPC->ClosePauseMenu();
	}
	else if (PC)
	{
		PC->SetPause(false);
	}

	UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
}

void UProjectOrganoidPauseWidget::SetMasterVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMasterVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetSFXVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetSFXVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetMusicVolume(float NewVolume)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetMusicVolume(NewVolume);
	}
}

void UProjectOrganoidPauseWidget::SetGraphicsQuality(EProjectOrganoidGraphicsQuality NewQuality)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetGraphicsQuality(NewQuality);
	}
}

void UProjectOrganoidPauseWidget::SetWindowMode(EProjectOrganoidWindowMode NewMode)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetWindowMode(NewMode);
	}
}

void UProjectOrganoidPauseWidget::SetResolutionScalePercent(float NewPercent)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->SetResolutionScalePercent(NewPercent);
	}
}

float UProjectOrganoidPauseWidget::GetMasterVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMasterVolume();
	}
	return 1.0f;
}

float UProjectOrganoidPauseWidget::GetSFXVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetSFXVolume();
	}
	return 1.0f;
}

float UProjectOrganoidPauseWidget::GetMusicVolume() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetMusicVolume();
	}
	return 1.0f;
}

EProjectOrganoidGraphicsQuality UProjectOrganoidPauseWidget::GetGraphicsQuality() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetGraphicsQuality();
	}
	return EProjectOrganoidGraphicsQuality::High;
}

EProjectOrganoidWindowMode UProjectOrganoidPauseWidget::GetWindowMode() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetWindowMode();
	}
	return EProjectOrganoidWindowMode::Fullscreen;
}

float UProjectOrganoidPauseWidget::GetResolutionScalePercent() const
{
	if (const UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		return Settings->GetResolutionScalePercent();
	}
	return 100.0f;
}

void UProjectOrganoidPauseWidget::ApplySettings()
{
	if (UProjectOrganoidSettingsSubsystem* Settings = GetSettingsSubsystem())
	{
		Settings->ApplyAllSettings();
	}
}

UProjectOrganoidSettingsSubsystem* UProjectOrganoidPauseWidget::GetSettingsSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>();
	}
	return nullptr;
}
