#include "HaDashboardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "WebBrowser.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

void UHaDashboardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildWidgetTree();

	if (!PendingUrl.IsEmpty())
	{
		LoadDashboardUrl(PendingUrl);
	}
}

void UHaDashboardWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Background"));
	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
	BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.08f, 0.12f, 0.08f, 1.0f));
	BackgroundImage->SetBrush(BackgroundBrush);

	UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BackgroundImage);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	WebBrowser = WidgetTree->ConstructWidget<UWebBrowser>(UWebBrowser::StaticClass(), TEXT("HaBrowser"));
	UCanvasPanelSlot* BrowserSlot = RootCanvas->AddChildToCanvas(WebBrowser);
	BrowserSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BrowserSlot->SetOffsets(FMargin(0.0f));
}

void UHaDashboardWidget::LoadDashboardUrl(const FString& Url)
{
	if (Url.IsEmpty())
	{
		return;
	}

	PendingUrl = Url;

	if (!WebBrowser)
	{
		BuildWidgetTree();
	}

	if (!WebBrowser)
	{
		UE_LOG(LogTemp, Warning, TEXT("HA Dashboard widget: WebBrowser not available yet, queued URL %s"), *Url);
		return;
	}

	WebBrowser->LoadURL(Url);
	UE_LOG(LogTemp, Log, TEXT("HA Dashboard widget: LoadURL %s"), *Url);
}

FString UHaDashboardWidget::GetCurrentUrl() const
{
	return WebBrowser ? WebBrowser->GetUrl() : FString();
}