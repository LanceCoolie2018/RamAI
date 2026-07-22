#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StadiumStreamWidget.generated.h"

class UWebBrowser;

UCLASS()
class RAMBEDAI_API UStadiumStreamWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Stadium")
	void LoadStreamUrl(const FString& Url);

	UFUNCTION(BlueprintCallable, Category = "Stadium")
	FString GetCurrentUrl() const;

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TObjectPtr<UWebBrowser> WebBrowser;
};