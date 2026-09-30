#include "ProjectOrganoidTutorialWidget.h"
#include "ProjectOrganoidGameMode.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* TutorialTablePath = TEXT("/Game/UI/Tutorial/DT_Tutorial_Controls.DT_Tutorial_Controls");

	struct FFallbackRow
	{
		const TCHAR* Control;
		const TCHAR* Keyboard;
		const TCHAR* Gamepad;
	};

	const FFallbackRow FallbackRows[] = {
		{ TEXT("Move"), TEXT("WASD"), TEXT("Left stick") },
		{ TEXT("Look"), TEXT("Mouse"), TEXT("Right stick") },
		{ TEXT("Fire"), TEXT("Left mouse"), TEXT("Right trigger") },
		{ TEXT("Jump"), TEXT("Space"), TEXT("A") },
		{ TEXT("Interact"), TEXT("E"), TEXT("X") },
	};
}

void UProjectOrganoidTutorialWidget::ShowForPlayer(APlayerController* PlayerController)
{
	if (!PlayerController || AProjectOrganoidGameMode::IsAutomatedPlaytestActive())
	{
		return;
	}

	for (TObjectIterator<UProjectOrganoidTutorialWidget> It; It; ++It)
	{
		if (It->IsInViewport())
		{
			return;
		}
	}

	UClass* WidgetClass = LoadClass<UProjectOrganoidTutorialWidget>(
		nullptr, TEXT("/Game/UI/Menus/WBP_Tutorial.WBP_Tutorial_C"));
	if (!WidgetClass)
	{
		WidgetClass = StaticClass();
	}

	UProjectOrganoidTutorialWidget* Widget = CreateWidget<UProjectOrganoidTutorialWidget>(PlayerController, WidgetClass);
	if (!Widget)
	{
		return;
	}

	Widget->AddToViewport(40);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(LogTemp, Log, TEXT("TUTORIAL_CONTROLS shown WASD mouse gamepad"));

	TWeakObjectPtr<UProjectOrganoidTutorialWidget> WeakWidget(Widget);
	if (UWorld* World = PlayerController->GetWorld())
	{
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, [WeakWidget]()
		{
			if (WeakWidget.IsValid())
			{
				WeakWidget->SetVisibility(ESlateVisibility::Collapsed);
				WeakWidget->RemoveFromParent();
			}
		}, 12.0f, false);
	}
}

void UProjectOrganoidTutorialWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildLayout();
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UProjectOrganoidTutorialWidget::BuildLayout()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree->FindWidget(TEXT("TutorialTitle")))
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TutorialRoot"));
		WidgetTree->RootWidget = Root;
	}

	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TutorialPlate"));
	Plate->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.05f, 0.82f));
	Plate->SetPadding(FMargin(18.0f, 14.0f));
	if (UCanvasPanelSlot* PlateSlot = Root->AddChildToCanvas(Plate))
	{
		PlateSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		PlateSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		PlateSlot->SetPosition(FVector2D(28.0f, 28.0f));
		PlateSlot->SetAutoSize(true);
	}

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TutorialBox"));
	Plate->SetContent(Box);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TutorialTitle"));
	Title->SetText(FText::FromString(TEXT("CONTROLS")));
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.86f, 0.84f, 1.0f)));
	Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 16));
	Box->AddChildToVerticalBox(Title);

	UDataTable* Table = LoadObject<UDataTable>(nullptr, TutorialTablePath);
	if (Table && Table->GetRowStruct() == FProjectOrganoidTutorialControlRow::StaticStruct())
	{
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			const FProjectOrganoidTutorialControlRow* Row = reinterpret_cast<const FProjectOrganoidTutorialControlRow*>(Pair.Value);
			if (!Row)
			{
				continue;
			}
			UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			Line->SetText(FText::FromString(FString::Printf(TEXT("%s    %s    %s"), *Row->Control, *Row->Keyboard, *Row->Gamepad)));
			Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.92f, 0.94f, 1.0f)));
			if (UVerticalBoxSlot* LineSlot = Box->AddChildToVerticalBox(Line))
			{
				LineSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			}
		}
		return;
	}

	for (const FFallbackRow& Row : FallbackRows)
	{
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Line->SetText(FText::FromString(FString::Printf(TEXT("%s    %s    %s"), Row.Control, Row.Keyboard, Row.Gamepad)));
		Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.92f, 0.94f, 1.0f)));
		if (UVerticalBoxSlot* LineSlot = Box->AddChildToVerticalBox(Line))
		{
			LineSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
	}
}
