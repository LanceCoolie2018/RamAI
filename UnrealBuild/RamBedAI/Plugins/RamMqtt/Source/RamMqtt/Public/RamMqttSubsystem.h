#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RamMqttTypes.h"
#include "RamMqttSubsystem.generated.h"

UCLASS()
class RAMMQTT_API URamMqttSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	UFUNCTION(BlueprintCallable, Category = "RamMQTT")
	void ConnectToBroker();

	UFUNCTION(BlueprintCallable, Category = "RamMQTT")
	void DisconnectFromBroker();

	UFUNCTION(BlueprintCallable, Category = "RamMQTT")
	bool IsConnected() const;

	UFUNCTION(BlueprintCallable, Category = "RamMQTT")
	FRamStreamChannel GetActiveChannel() const { return ActiveChannel; }

	UFUNCTION(BlueprintCallable, Category = "RamMQTT")
	bool HasActiveChannel() const { return !ActiveChannel.Url.IsEmpty(); }

	UPROPERTY(BlueprintAssignable, Category = "RamMQTT")
	FOnRamStreamChannelChanged OnStreamChannelChanged;

	UPROPERTY(BlueprintAssignable, Category = "RamMQTT")
	FOnRamMqttConnectionStateChanged OnConnectionStateChanged;

private:
	struct FRamMqttClientHolder;

	void HandleMqttMessage(const FString& Topic, const FString& Payload);
	void ParseStreamPayload(const FString& Payload);
	void BroadcastChannel();

	FRamMqttClientHolder* ClientHolder = nullptr;
	FRamStreamChannel ActiveChannel;
	bool bConnected = false;
};