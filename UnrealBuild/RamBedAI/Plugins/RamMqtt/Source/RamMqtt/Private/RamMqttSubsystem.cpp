#include "RamMqttSubsystem.h"

#if RAMMQTT_SUPPORTED
#include "ramai/mqtt_client.hpp"
#endif

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr const TCHAR* StreamTopic = TEXT("tailgate/stream/active");

#if RAMMQTT_SUPPORTED
// Broker credentials are NOT hard-coded. Prefer (in order):
// 1) Project Config/mqtt_local.json  (gitignored — copy from mqtt_local.example.json)
// 2) Empty defaults (connect will fail until you add mqtt_local.json)
ramai::BrokerProfile MakeBrokerProfile()
{
	ramai::BrokerProfile Profile;
	Profile.name = "RamBedAI";
	Profile.host = "127.0.0.1";
	Profile.port = 1883;
	Profile.client_id = "RamBedAI";
	Profile.username = "";
	Profile.password = "";
	Profile.preset_subscriptions = {"tailgate/stream/active"};

	const FString LocalConfigPath = FPaths::ProjectConfigDir() / TEXT("mqtt_local.json");
	if (FPaths::FileExists(LocalConfigPath))
	{
		FString JsonText;
		if (FFileHelper::LoadFileToString(JsonText, *LocalConfigPath))
		{
			TSharedPtr<FJsonObject> Root;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
			if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
			{
				FString Value;
				if (Root->TryGetStringField(TEXT("host"), Value))
				{
					Profile.host = TCHAR_TO_UTF8(*Value);
				}
				if (Root->TryGetStringField(TEXT("client_id"), Value))
				{
					Profile.client_id = TCHAR_TO_UTF8(*Value);
				}
				if (Root->TryGetStringField(TEXT("username"), Value))
				{
					Profile.username = TCHAR_TO_UTF8(*Value);
				}
				if (Root->TryGetStringField(TEXT("password"), Value))
				{
					Profile.password = TCHAR_TO_UTF8(*Value);
				}
				double Port = Profile.port;
				if (Root->TryGetNumberField(TEXT("port"), Port))
				{
					Profile.port = static_cast<int>(Port);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("RamMQTT: missing Config/mqtt_local.json — copy mqtt_local.example.json and add broker credentials."));
	}
	return Profile;
}
#endif
}  // namespace

#if RAMMQTT_SUPPORTED
struct URamMqttSubsystem::FRamMqttClientHolder
{
	ramai::MqttClient Client;
};
#endif

void URamMqttSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if RAMMQTT_SUPPORTED
	ClientHolder = new FRamMqttClientHolder();
	ClientHolder->Client.set_message_handler([this](const ramai::MqttMessage& Message) {
		const FString Topic = UTF8_TO_TCHAR(Message.topic.c_str());
		const FString Payload = UTF8_TO_TCHAR(Message.payload.c_str());
		HandleMqttMessage(Topic, Payload);
	});

	ConnectToBroker();
#else
	UE_LOG(LogTemp, Warning, TEXT("RamMQTT: native client is Win64-only for now; use Test Stream Url on stadium screen"));
#endif
}

void URamMqttSubsystem::Deinitialize()
{
#if RAMMQTT_SUPPORTED
	DisconnectFromBroker();
	delete ClientHolder;
	ClientHolder = nullptr;
#endif
	Super::Deinitialize();
}

void URamMqttSubsystem::Tick(float DeltaTime)
{
#if RAMMQTT_SUPPORTED
	if (ClientHolder)
	{
		ClientHolder->Client.drain_incoming_messages();
	}
#endif
}

TStatId URamMqttSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URamMqttSubsystem, STATGROUP_Tickables);
}

bool URamMqttSubsystem::IsTickable() const
{
#if RAMMQTT_SUPPORTED
	return true;
#else
	return false;
#endif
}

void URamMqttSubsystem::ConnectToBroker()
{
#if RAMMQTT_SUPPORTED
	if (!ClientHolder)
	{
		return;
	}

	const ramai::BrokerProfile Profile = MakeBrokerProfile();
	ClientHolder->Client.connect(Profile, [this, Profile](bool Connected, const std::string& Detail) {
		AsyncTask(ENamedThreads::GameThread, [this, Connected, Profile, Detail]()
		{
			bConnected = Connected;
			OnConnectionStateChanged.Broadcast(Connected);

			if (Connected)
			{
				for (const auto& Topic : Profile.preset_subscriptions)
				{
					ClientHolder->Client.subscribe(Topic, 0);
					UE_LOG(LogTemp, Log, TEXT("RamMQTT: subscribed to %s"), UTF8_TO_TCHAR(Topic.c_str()));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("RamMQTT: broker connect failed: %s"), UTF8_TO_TCHAR(Detail.c_str()));
			}
		});
	});

	if (ClientHolder->Client.is_connected())
	{
		bConnected = true;
		for (const auto& Topic : Profile.preset_subscriptions)
		{
			ClientHolder->Client.subscribe(Topic, 0);
			UE_LOG(LogTemp, Log, TEXT("RamMQTT: subscribed to %s"), UTF8_TO_TCHAR(Topic.c_str()));
		}
	}
#endif
}

void URamMqttSubsystem::DisconnectFromBroker()
{
#if RAMMQTT_SUPPORTED
	if (ClientHolder)
	{
		ClientHolder->Client.disconnect();
	}
#endif
	bConnected = false;
}

bool URamMqttSubsystem::IsConnected() const
{
#if RAMMQTT_SUPPORTED
	return ClientHolder && ClientHolder->Client.is_connected();
#else
	return false;
#endif
}

void URamMqttSubsystem::HandleMqttMessage(const FString& Topic, const FString& Payload)
{
	if (Topic != StreamTopic)
	{
		return;
	}

	AsyncTask(ENamedThreads::GameThread, [this, Payload]()
	{
		ParseStreamPayload(Payload);
	});
}

void URamMqttSubsystem::ParseStreamPayload(const FString& Payload)
{
	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Payload);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("RamMQTT: invalid stream payload: %s"), *Payload);
		return;
	}

	FRamStreamChannel Channel;
	Channel.Id = JsonObject->GetStringField(TEXT("id"));
	Channel.Name = JsonObject->GetStringField(TEXT("name"));
	Channel.Url = JsonObject->GetStringField(TEXT("url"));

	if (Channel.Url.IsEmpty())
	{
		return;
	}

	ActiveChannel = Channel;
	BroadcastChannel();
}

void URamMqttSubsystem::BroadcastChannel()
{
	OnStreamChannelChanged.Broadcast(ActiveChannel);
	UE_LOG(LogTemp, Log, TEXT("RamMQTT: active stream -> %s (%s)"), *ActiveChannel.Name, *ActiveChannel.Url);
}