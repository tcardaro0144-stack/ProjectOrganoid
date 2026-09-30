#include "ProjectOrganoidIntroWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

UProjectOrganoidIntroWidget* UProjectOrganoidIntroWidget::Show(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC)
	{
		return nullptr;
	}

	UProjectOrganoidIntroWidget* Widget = CreateWidget<UProjectOrganoidIntroWidget>(PC, StaticClass());
	if (!Widget)
	{
		return nullptr;
	}

	Widget->AddToViewport(200);
	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(LogTemp, Log, TEXT("INTRO_CINEMATIC EPITOPE | Prepared Immunity"));

	if (UClass* CinematicClass = LoadClass<AActor>(nullptr, TEXT("/Game/Cinematics/BP_IntroCinematic.BP_IntroCinematic_C")))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AActor* Cinematic = World->SpawnActor<AActor>(CinematicClass, FTransform(FVector(0.0f, -400.0f, 180.0f)), Params))
		{
			UE_LOG(LogTemp, Log, TEXT("INTRO_CINEMATIC actor %s"), *Cinematic->GetName());
		}
	}

	return Widget;
}

void UProjectOrganoidIntroWidget::Dismiss()
{
	RemoveFromParent();
}

void UProjectOrganoidIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (WidgetTree->FindWidget(TEXT("IntroTitle")))
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(GetRootWidget());
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("IntroRoot"));
		WidgetTree->RootWidget = Root;
	}

	UBorder* Plate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntroPlate"));
	Plate->SetBrushColor(FLinearColor(0.01f, 0.02f, 0.025f, 0.94f));
	if (UCanvasPanelSlot* PlateSlot = Root->AddChildToCanvas(Plate))
	{
		PlateSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PlateSlot->SetOffsets(FMargin(0.0f));
	}

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("IntroTitle"));
	TitleText->SetText(FText::FromString(TEXT("EPITOPE | Prepared Immunity")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 32));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.95f, 0.96f, 1.0f)));
	if (UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(TitleText))
	{
		TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		TitleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		TitleSlot->SetAutoSize(true);
	}
}

void UProjectOrganoidIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	PanTime += InDeltaTime;
	if (!TitleText)
	{
		return;
	}
	if (UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(TitleText->Slot))
	{
		const float Offset = FMath::Sin(PanTime * 0.7f) * 36.0f;
		TitleSlot->SetPosition(FVector2D(Offset, -8.0f));
	}
}
