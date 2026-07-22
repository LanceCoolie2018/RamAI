#pragma once

#include "CoreMinimal.h"
#include "RamMqttTypes.generated.h"

USTRUCT(BlueprintType)
struct FRamStreamChannel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RamMQTT")
	FString Id;

	UPROPERTY(BlueprintReadOnly, Category = "RamMQTT")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "RamMQTT")
	FString Url;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRamStreamChannelChanged, const FRamStreamChannel&, Channel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRamMqttConnectionStateChanged, bool, bConnected);