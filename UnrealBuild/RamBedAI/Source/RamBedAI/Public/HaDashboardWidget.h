#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HaDashboardWidget.generated.h"

class UWebBrowser;

UCLASS()
class RAMBEDAI_API UHaDashboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HA")
	void LoadDashboardUrl(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "HA")
	FString GetCurrentUrl() const;

protected:
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();

	UPROPERTY()
	TObjectPtr<class UImage> BackgroundImage;

	UPROPERTY()
	TObjectPtr<UWebBrowser> WebBrowser;

	FString PendingUrl;
};