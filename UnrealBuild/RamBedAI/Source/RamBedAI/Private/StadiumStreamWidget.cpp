#include "StadiumStreamWidget.h"

#include "Blueprint/WidgetTree.h"
#include "WebBrowser.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void UStadiumStreamWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	WebBrowser = WidgetTree->ConstructWidget<UWebBrowser>(UWebBrowser::StaticClass(), TEXT("StreamBrowser"));
	UCanvasPanelSlot* BrowserSlot = RootCanvas->AddChildToCanvas(WebBrowser);
	BrowserSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BrowserSlot->SetOffsets(FMargin(0.0f));
}

void UStadiumStreamWidget::LoadStreamUrl(const FString& Url)
{
	if (!WebBrowser || Url.IsEmpty())
	{
		return;
	}

	WebBrowser->LoadURL(Url);
}

FString UStadiumStreamWidget::GetCurrentUrl() const
{
	return WebBrowser ? WebBrowser->GetUrl() : FString();
}