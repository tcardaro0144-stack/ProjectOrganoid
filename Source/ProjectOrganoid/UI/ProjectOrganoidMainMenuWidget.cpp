// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidMainMenuWidget.h"
#include "ProjectOrganoidSaveSubsystem.h"
#include "ProjectOrganoidSettingsSubsystem.h"
#include "ProjectOrganoidFlowManagerSubsystem.h"
#include "ProjectOrganoidPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

namespace ProjectOrganoidMenuUI
{
	static constexpr float MenuButtonWidth = 320.0f;
	static constexpr float MenuButtonHeight = 56.0f;

	static const TArray<FString> GraphicsQualityLabels = {
		TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic")
	};

	static EProjectOrganoidGraphicsQuality QualityFromLabel(const FString& Label)
	{
		const int32 Index = GraphicsQualityLabels.IndexOfByKey(Label);
		return static_cast<EProjectOrganoidGraphicsQuality>(FMath::Clamp(Index, 0, 4));
	}

	static FSlateBrush MakeSolidBrush(const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(Color);
		Brush.ImageSize = FVector2D(32.0f, 32.0f);
		return Brush;
	}

	static FButtonStyle MakeDarkButtonStyle()
	{
		FButtonStyle Style;
		Style.SetNormal(MakeSolidBrush(FLinearColor(0.10f, 0.11f, 0.13f, 1.0f)));
		Style.SetHovered(MakeSolidBrush(FLinearColor(0.04f, 0.20f, 0.22f, 1.0f)));
		Style.SetPressed(MakeSolidBrush(FLinearColor(0.03f, 0.10f, 0.12f, 1.0f)));
		Style.SetDisabled(MakeSolidBrush(FLinearColor(0.05f, 0.05f, 0.06f, 1.0f)));
		Style.NormalPadding = FMargin(12.0f, 8.0f);
		Style.PressedPadding = FMargin(12.0f, 8.0f);
		return Style;
	}

	static UImage* EnsureFullScreenImage(UWidgetTree* Tree, UCanvasPanel* Canvas, const FName Name, int32 ZOrder)
	{
		UImage* Image = Tree->FindWidget<UImage>(Name);
		if (!Image)
		{
			Image = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
			if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Image))
			{
				CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
				CanvasSlot->SetOffsets(FMargin(0.0f));
				CanvasSlot->SetZOrder(static_cast<float>(ZOrder));
			}
		}
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		return Image;
	}

	static UButton* MakeFixedMenuButton(
		UWidgetTree* Tree,
		UVerticalBox* Parent,
		const FName ButtonName,
		const FName LabelName,
		const FText& Label,
		EHorizontalAlignment ButtonAlign = HAlign_Center)
	{
		USizeBox* SizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(ButtonName.ToString() + TEXT("_SizeBox")));
		SizeBox->SetWidthOverride(MenuButtonWidth);
		SizeBox->SetHeightOverride(MenuButtonHeight);
		SizeBox->SetMinDesiredWidth(MenuButtonWidth);
		SizeBox->SetMinDesiredHeight(MenuButtonHeight);
		SizeBox->SetMaxDesiredWidth(MenuButtonWidth);
		SizeBox->SetMaxDesiredHeight(MenuButtonHeight);

		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		Button->SetStyle(MakeDarkButtonStyle());
		Button->SetColorAndOpacity(FLinearColor::White);

		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
		Text->SetText(Label);
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.90f, 0.92f, 1.0f)));
		Button->SetContent(Text);
		SizeBox->SetContent(Button);

		if (UVerticalBoxSlot* BoxSlot = Parent->AddChildToVerticalBox(SizeBox))
		{
			BoxSlot->SetHorizontalAlignment(ButtonAlign);
			BoxSlot->SetPadding(FMargin(0.0f, 8.0f));
		}
		return Button;
	}
}

TSharedRef<SWidget> UProjectOrganoidMainMenuWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	EnsureVisibleMenuLayout();
	return Super::RebuildWidget();
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
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	UImage* Vista = ProjectOrganoidMenuUI::EnsureFullScreenImage(
		WidgetTree, RootCanvas, TEXT("RuntimeMenuVista"), 0);
	if (UTexture2D* BackdropTex = ResolveBackdropTexture())
	{
		Vista->SetBrushFromTexture(BackdropTex, true);
	}
	else
	{
		Vista->SetBrush(ProjectOrganoidMenuUI::MakeSolidBrush(FLinearColor(0.02f, 0.03f, 0.04f, 1.0f)));
	}

	UImage* Vignette = ProjectOrganoidMenuUI::EnsureFullScreenImage(
		WidgetTree, RootCanvas, TEXT("RuntimeMenuVignette"), 1);
	Vignette->SetBrush(ProjectOrganoidMenuUI::MakeSolidBrush(FLinearColor(0.0f, 0.0f, 0.0f, 0.26f)));

	UImage* HazardBar = WidgetTree->FindWidget<UImage>(TEXT("RuntimeHazardBar"));
	if (!HazardBar)
	{
		HazardBar = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeHazardBar"));
		if (UCanvasPanelSlot* BarSlot = RootCanvas->AddChildToCanvas(HazardBar))
		{
			BarSlot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
			BarSlot->SetAlignment(FVector2D(0.0f, 1.0f));
			BarSlot->SetOffsets(FMargin(0.0f, -8.0f, 0.0f, 8.0f));
			BarSlot->SetZOrder(2);
		}
	}
	HazardBar->SetBrush(ProjectOrganoidMenuUI::MakeSolidBrush(FLinearColor(0.92f, 0.62f, 0.04f, 0.92f)));
	HazardBar->SetVisibility(ESlateVisibility::HitTestInvisible);

	UBorder* Plate = WidgetTree->FindWidget<UBorder>(TEXT("RuntimeMenuPlate"));
	if (!Plate)
	{
		Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RuntimeMenuPlate"));
		if (UCanvasPanelSlot* PlateSlot = RootCanvas->AddChildToCanvas(Plate))
		{
			PlateSlot->SetAnchors(FAnchors(0.08f, 0.5f, 0.08f, 0.5f));
			PlateSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			PlateSlot->SetAutoSize(true);
			PlateSlot->SetZOrder(10);
		}
	}
	Plate->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.03f, 0.72f));
	Plate->SetPadding(FMargin(40.0f, 36.0f));
	Plate->SetVisibility(ESlateVisibility::Visible);

	UImage* Accent = WidgetTree->FindWidget<UImage>(TEXT("RuntimeMenuAccent"));
	if (!Accent)
	{
		Accent = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RuntimeMenuAccent"));
		if (UCanvasPanelSlot* AccentSlot = RootCanvas->AddChildToCanvas(Accent))
		{
			AccentSlot->SetAnchors(FAnchors(0.08f, 0.5f, 0.08f, 0.5f));
			AccentSlot->SetAlignment(FVector2D(1.0f, 0.5f));
			AccentSlot->SetSize(FVector2D(4.0f, 420.0f));
			AccentSlot->SetZOrder(11);
		}
	}
	Accent->SetBrush(ProjectOrganoidMenuUI::MakeSolidBrush(FLinearColor(0.10f, 0.68f, 0.66f, 0.88f)));
	Accent->SetVisibility(ESlateVisibility::HitTestInvisible);

	UVerticalBox* MenuBox = WidgetTree->FindWidget<UVerticalBox>(TEXT("RuntimeMenuBox"));
	if (!MenuBox)
	{
		MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RuntimeMenuBox"));
		Plate->SetContent(MenuBox);
	}

	if (!WidgetTree->FindWidget(TEXT("LockdownBanner")))
	{
		UTextBlock* Banner = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LockdownBanner"));
		Banner->SetText(FText::FromString(TEXT("BSL-4  //  SITE LOCKDOWN")));
		Banner->SetJustification(ETextJustify::Left);
		Banner->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.42f, 0.12f, 1.0f)));
		if (UVerticalBoxSlot* BannerSlot = MenuBox->AddChildToVerticalBox(Banner))
		{
			BannerSlot->SetHorizontalAlignment(HAlign_Left);
			BannerSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}
	}

	if (!TitleText)
	{
		TitleText = WidgetTree->FindWidget<UTextBlock>(TEXT("TitleText"));
	}
	if (!TitleText)
	{
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(FText::FromString(TEXT("PROJECT ORGANOID")));
		TitleText->SetJustification(ETextJustify::Left);
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.97f, 1.0f)));
		TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 36));
		if (UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Left);
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
	}

	if (!WidgetTree->FindWidget(TEXT("SubtitleText")))
	{
		UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SubtitleText"));
		Subtitle->SetText(FText::FromString(TEXT("Epitope Subterranean Complex")));
		Subtitle->SetJustification(ETextJustify::Left);
		Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.72f, 0.74f, 1.0f)));
		if (UVerticalBoxSlot* SubSlot = MenuBox->AddChildToVerticalBox(Subtitle))
		{
			SubSlot->SetHorizontalAlignment(HAlign_Left);
			SubSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
		}
	}

	if (!NewGameButton)
	{
		NewGameButton = WidgetTree->FindWidget<UButton>(TEXT("NewGameButton"));
	}
	if (!NewGameButton)
	{
		NewGameButton = ProjectOrganoidMenuUI::MakeFixedMenuButton(
			WidgetTree,
			MenuBox,
			TEXT("NewGameButton"),
			TEXT("NewGameButton_Label"),
			FText::FromString(TEXT("NEW GAME")),
			HAlign_Left);
	}

	if (!QuitButton)
	{
		QuitButton = WidgetTree->FindWidget<UButton>(TEXT("QuitButton"));
	}
	if (!QuitButton)
	{
		QuitButton = ProjectOrganoidMenuUI::MakeFixedMenuButton(
			WidgetTree,
			MenuBox,
			TEXT("QuitButton"),
			TEXT("QuitButton_Label"),
			FText::FromString(TEXT("QUIT")),
			HAlign_Left);
	}

	if (!WidgetTree->FindWidget(TEXT("AuditorFooter")))
	{
		UTextBlock* Footer = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AuditorFooter"));
		Footer->SetText(FText::FromString(TEXT("AVERY VANCE  —  BIO-HAZARD AUDITOR")));
		Footer->SetJustification(ETextJustify::Left);
		Footer->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.48f, 0.50f, 1.0f)));
		if (UVerticalBoxSlot* FootSlot = MenuBox->AddChildToVerticalBox(Footer))
		{
			FootSlot->SetHorizontalAlignment(HAlign_Left);
			FootSlot->SetPadding(FMargin(0.0f, 28.0f, 0.0f, 0.0f));
		}
	}

	SetColorAndOpacity(FLinearColor::White);
	SetForegroundColor(FSlateColor(FLinearColor::White));
	SetIsFocusable(true);
	SetIsEnabled(true);
	SetVisibility(ESlateVisibility::Visible);
	InvalidateLayoutAndVolatility();

	UE_LOG(LogTemp, Log, TEXT("MainMenu: C++ atmospheric title layout ready (vista=%s)."),
		RuntimeBackdropTexture ? TEXT("yes") : TEXT("procedural/fallback"));
}

UTexture2D* UProjectOrganoidMainMenuWidget::ResolveBackdropTexture()
{
	if (RuntimeBackdropTexture)
	{
		return RuntimeBackdropTexture;
	}

	// Loose PNG first — never opens a .uasset the editor might have locked.
	const FString PngPath = FPaths::ProjectContentDir() / TEXT("UI/Menus/T_TitleVista.png");
	if (FPaths::FileExists(PngPath))
	{
		RuntimeBackdropTexture = FImageUtils::ImportFileAsTexture2D(PngPath);
		if (RuntimeBackdropTexture)
		{
			RuntimeBackdropTexture->NeverStream = true;
			return RuntimeBackdropTexture;
		}
	}

	if (FPackageName::DoesPackageExist(TEXT("/Game/UI/Menus/T_TitleVista")))
	{
		if (UTexture2D* Imported = LoadObject<UTexture2D>(
			nullptr,
			TEXT("/Game/UI/Menus/T_TitleVista.T_TitleVista"),
			nullptr,
			LOAD_NoWarn | LOAD_Quiet))
		{
			RuntimeBackdropTexture = Imported;
			return RuntimeBackdropTexture;
		}
	}

	RuntimeBackdropTexture = CreateProceduralBackdropTexture();
	return RuntimeBackdropTexture;
}

UTexture2D* UProjectOrganoidMainMenuWidget::CreateProceduralBackdropTexture()
{
	const int32 Width = 1024;
	const int32 Height = 576;
	TArray64<uint8> Bytes;
	Bytes.SetNumUninitialized(static_cast<int64>(Width) * Height * 4);
	FColor* Pixels = reinterpret_cast<FColor*>(Bytes.GetData());

	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(Height - 1);
		for (int32 X = 0; X < Width; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(Width - 1);
			FLinearColor Color = FMath::Lerp(
				FLinearColor(0.015f, 0.04f, 0.055f),
				FLinearColor(0.02f, 0.012f, 0.018f),
				V);

			const float CyanGlow = FMath::Exp(-(((U - 0.18f) * (U - 0.18f)) + ((V - 0.22f) * (V - 0.22f))) * 7.0f);
			Color += FLinearColor(0.02f, 0.16f, 0.18f) * CyanGlow * 0.4f;

			const float AmberWash = FMath::Max(0.0f, V - 0.62f);
			Color += FLinearColor(0.22f, 0.08f, 0.02f) * AmberWash * 0.55f;

			if ((Y % 4) == 0)
			{
				Color *= 0.84f;
			}
			if ((X % 64) == 0 || (Y % 64) == 0)
			{
				Color += FLinearColor(0.015f, 0.04f, 0.045f);
			}

			const float Dx = U - 0.5f;
			const float Dy = V - 0.5f;
			const float Vignette = FMath::Clamp(1.0f - ((Dx * Dx) + (Dy * Dy)) * 1.55f, 0.22f, 1.0f);
			Color *= Vignette;
			Pixels[Y * Width + X] = Color.ToFColor(true);
		}
	}

	UTexture2D* Texture = UTexture2D::CreateTransient(
		Width,
		Height,
		PF_B8G8R8A8,
		NAME_None,
		Bytes);
	if (!Texture)
	{
		return nullptr;
	}

	Texture->SRGB = true;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->NeverStream = true;
	return Texture;
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

void UProjectOrganoidMainMenuWidget::DismissFromViewport()
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsEnabled(false);
	RemoveFromParent();
}

void UProjectOrganoidMainMenuWidget::StartNewGame()
{
	if (UProjectOrganoidSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->ClearPendingLoad();
	}

	if (AProjectOrganoidPlayerController* OrganoidPC = Cast<AProjectOrganoidPlayerController>(GetOwningPlayer()))
	{
		OrganoidPC->DismissTitleMainMenu();
	}
	else
	{
		DismissFromViewport();
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
