#include "ProjectOrganoidOptionsWidget.h"

#include "ProjectOrganoidSettingsSubsystem.h"
#include "ProjectOrganoidSettingsTypes.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"

namespace
{
	UProjectOrganoidSettingsSubsystem* OptionsSettings(const UUserWidget* Widget)
	{
		const UGameInstance* GI = Widget ? Widget->GetGameInstance() : nullptr;
		return GI ? GI->GetSubsystem<UProjectOrganoidSettingsSubsystem>() : nullptr;
	}

	void AddCaption(UWidgetTree* Tree, UVerticalBox* Box, const FString& Label)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetText(FText::FromString(Label));
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.84f, 0.86f, 1.0f)));
		if (UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Text))
		{
			Slot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 2.0f));
		}
	}
}

UProjectOrganoidOptionsWidget* UProjectOrganoidOptionsWidget::ShowForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	for (TObjectIterator<UProjectOrganoidOptionsWidget> It; It; ++It)
	{
		if (It->IsInViewport())
		{
			return *It;
		}
	}

	UClass* WidgetClass = LoadClass<UProjectOrganoidOptionsWidget>(
		nullptr, TEXT("/Game/UI/Menus/WBP_Options.WBP_Options_C"));
	if (!WidgetClass)
	{
		WidgetClass = StaticClass();
	}

	UProjectOrganoidOptionsWidget* Widget = CreateWidget<UProjectOrganoidOptionsWidget>(PlayerController, WidgetClass);
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport(80);
	Widget->SetVisibility(ESlateVisibility::Visible);
	UE_LOG(LogTemp, Log, TEXT("OPTIONS_SCREEN graphics audio controls"));
	return Widget;
}

void UProjectOrganoidOptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	if (UProjectOrganoidSettingsSubsystem* Settings = OptionsSettings(this))
	{
		if (MasterSlider)
		{
			MasterSlider->SetValue(Settings->GetMasterVolume());
		}
		if (SfxSlider)
		{
			SfxSlider->SetValue(Settings->GetSFXVolume());
		}
		if (MusicSlider)
		{
			MusicSlider->SetValue(Settings->GetMusicVolume());
		}
	}
}

void UProjectOrganoidOptionsWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree->FindWidget(TEXT("OptionsTitle")))
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("OptionsRoot"));
		WidgetTree->RootWidget = Root;
	}

	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OptionsPlate"));
	Plate->SetBrushColor(FLinearColor(0.015f, 0.03f, 0.035f, 0.92f));
	Plate->SetPadding(FMargin(28.0f, 22.0f));
	if (UCanvasPanelSlot* PlateSlot = Root->AddChildToCanvas(Plate))
	{
		PlateSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		PlateSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PlateSlot->SetAutoSize(true);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsBox"));
	Plate->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OptionsTitle"));
	Title->SetText(FText::FromString(TEXT("OPTIONS")));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.97f, 1.0f)));
	Box->AddChildToVerticalBox(Title);

	AddCaption(WidgetTree, Box, TEXT("Graphics"));
	QualityCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("GraphicsQuality"));
	QualityCombo->AddOption(TEXT("Low"));
	QualityCombo->AddOption(TEXT("Medium"));
	QualityCombo->AddOption(TEXT("High"));
	QualityCombo->AddOption(TEXT("Epic"));
	QualityCombo->AddOption(TEXT("Cinematic"));
	QualityCombo->SetSelectedOption(TEXT("High"));
	QualityCombo->OnSelectionChanged.AddUniqueDynamic(this, &UProjectOrganoidOptionsWidget::HandleQualityChanged);
	Box->AddChildToVerticalBox(QualityCombo);

	AddCaption(WidgetTree, Box, TEXT("Master volume"));
	MasterSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MasterVolume"));
	MasterSlider->SetValue(1.0f);
	MasterSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidOptionsWidget::HandleMasterChanged);
	Box->AddChildToVerticalBox(MasterSlider);

	AddCaption(WidgetTree, Box, TEXT("Effects volume"));
	SfxSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SfxVolume"));
	SfxSlider->SetValue(1.0f);
	SfxSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidOptionsWidget::HandleSfxChanged);
	Box->AddChildToVerticalBox(SfxSlider);

	AddCaption(WidgetTree, Box, TEXT("Music volume"));
	MusicSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MusicVolume"));
	MusicSlider->SetValue(1.0f);
	MusicSlider->OnValueChanged.AddUniqueDynamic(this, &UProjectOrganoidOptionsWidget::HandleMusicChanged);
	Box->AddChildToVerticalBox(MusicSlider);

	AddCaption(WidgetTree, Box, TEXT("Controls"));
	UTextBlock* Controls = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsText"));
	Controls->SetText(FText::FromString(TEXT("WASD move    Mouse look    LMB / RT fire\nSpace / A jump    E / X interact")));
	Controls->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.90f, 0.92f, 1.0f)));
	Controls->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(Controls);

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseOptions"));
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseOptionsLabel"));
	CloseLabel->SetText(FText::FromString(TEXT("CLOSE")));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseLabel);
	CloseButton->OnClicked.AddUniqueDynamic(this, &UProjectOrganoidOptionsWidget::HandleCloseClicked);
	if (UVerticalBoxSlot* CloseSlot = Box->AddChildToVerticalBox(CloseButton))
	{
		CloseSlot->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));
	}
}

void UProjectOrganoidOptionsWidget::HandleMasterChanged(float Value)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = OptionsSettings(this))
	{
		Settings->SetMasterVolume(Value);
	}
}

void UProjectOrganoidOptionsWidget::HandleSfxChanged(float Value)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = OptionsSettings(this))
	{
		Settings->SetSFXVolume(Value);
	}
}

void UProjectOrganoidOptionsWidget::HandleMusicChanged(float Value)
{
	if (UProjectOrganoidSettingsSubsystem* Settings = OptionsSettings(this))
	{
		Settings->SetMusicVolume(Value);
	}
}

void UProjectOrganoidOptionsWidget::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (SelectionType == ESelectInfo::Direct)
	{
		return;
	}
	if (UProjectOrganoidSettingsSubsystem* Settings = OptionsSettings(this))
	{
		const TArray<FString> Labels = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
		const int32 Index = Labels.IndexOfByKey(SelectedItem);
		if (Index != INDEX_NONE)
		{
			Settings->SetGraphicsQuality(static_cast<EProjectOrganoidGraphicsQuality>(Index));
		}
	}
}

void UProjectOrganoidOptionsWidget::HandleCloseClicked()
{
	RemoveFromParent();
}
