#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HaAnchoredDisplayActor.generated.h"

class UHaDashboardWidget;
class USceneComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS(Blueprintable, Config = Game, meta = (DisplayName = "HA Anchored Display"))
class RAMBEDAI_API AHaAnchoredDisplayActor : public AActor
{
	GENERATED_BODY()

public:
	AHaAnchoredDisplayActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "HA")
	void SetDisplayScale(float UniformScale);

	/** Creates the widget and loads DashboardUrl. Safe to call more than once. */
	UFUNCTION(BlueprintCallable, Category = "HA")
	void InitializeDashboard();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HA")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HA")
	TObjectPtr<UStaticMeshComponent> DisplayFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HA")
	TObjectPtr<UWidgetComponent> DisplayWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HA")
	FVector2D DisplayDrawSize = FVector2D(1280.0f, 720.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HA")
	float DefaultDisplayScale = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "HA")
	FString DashboardUrl = TEXT("http://10.20.25.40:8123/truck-bed/0");

private:
	UPROPERTY()
	TObjectPtr<UHaDashboardWidget> DashboardWidget;
};