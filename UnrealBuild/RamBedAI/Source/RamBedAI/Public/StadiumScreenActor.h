#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RamMqttTypes.h"
#include "StadiumScreenActor.generated.h"

class UWidgetComponent;
class UStadiumStreamWidget;
class URamMqttSubsystem;

UCLASS(Blueprintable, Config = Game, meta = (DisplayName = "Stadium Screen"))
class RAMBEDAI_API AStadiumScreenActor : public AActor
{
	GENERATED_BODY()

public:
	AStadiumScreenActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Stadium")
	void LoadChannel(const FRamStreamChannel& Channel);

	UFUNCTION(BlueprintCallable, Category = "Stadium")
	void SetScreenScale(float UniformScale);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stadium")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stadium")
	TObjectPtr<UStaticMeshComponent> ScreenFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stadium")
	TObjectPtr<UWidgetComponent> ScreenWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stadium")
	FVector2D ScreenDrawSize = FVector2D(1920.0f, 1080.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stadium")
	float DefaultScreenScale = 2.0f;

	/** Optional URL for editor/PIE testing when MQTT has no active channel yet. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Stadium|Debug")
	FString TestStreamUrl;

private:
	UFUNCTION()
	void HandleStreamChannelChanged(const FRamStreamChannel& Channel);

	UPROPERTY()
	TObjectPtr<UStadiumStreamWidget> StreamWidget;

	UPROPERTY()
	TObjectPtr<URamMqttSubsystem> MqttSubsystem;
};