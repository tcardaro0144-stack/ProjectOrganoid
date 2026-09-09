// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectOrganoidLoadingScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UProjectOrganoidLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureVisibleLoadingLayout();
}

void UProjectOrganoidLoadingScreenWidget::EnsureVisibleLoadingLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("LoadingRootCanvas"));
		WidgetTree->RootWidget = RootCanvas;
	}

	UImage* Backdrop = WidgetTree->FindWidget<UImage>(TEXT("LoadingBackdrop"));
	if (!Backdrop)
	{
		Backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoadingBackdrop"));
		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Backdrop))
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetZOrder(0);
		}
	}

	FSlateBrush DarkBrush;
	DarkBrush.DrawAs = ESlateBrushDrawType::Image;
	DarkBrush.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
	Backdrop->SetBrush(DarkBrush);
	Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);

	UTextBlock* Status = WidgetTree->FindWidget<UTextBlock>(TEXT("LoadingStatusText"));
	if (!Status)
	{
		Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadingStatusText"));
		if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Status))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetZOrder(1);
		}
	}
	Status->SetJustification(ETextJustify::Center);
	Status->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.9f, 1.0f)));
	if (Status->GetText().IsEmpty())
	{
		Status->SetText(CurrentStatus.IsEmpty() ? FText::FromString(TEXT("Loading...")) : CurrentStatus);
	}

	SetColorAndOpacity(FLinearColor::White);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UProjectOrganoidLoadingScreenWidget::SetStatus(const FText& StatusText, float Progress01)
{
	CurrentStatus = StatusText;
	CurrentProgress = FMath::Clamp(Progress01, 0.0f, 1.0f);
	OnLoadingStatusChanged(CurrentStatus, CurrentProgress);

	if (WidgetTree)
	{
		if (UTextBlock* Status = WidgetTree->FindWidget<UTextBlock>(TEXT("LoadingStatusText")))
		{
			Status->SetText(CurrentStatus);
		}
	}
}
