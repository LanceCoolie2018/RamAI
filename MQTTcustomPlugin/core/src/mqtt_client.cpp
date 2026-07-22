#include "ramai/mqtt_client.hpp"

#include <mqtt/async_client.h>

#include <utility>

namespace ramai {
namespace {

std::string make_server_uri(const BrokerProfile& profile) {
    return "tcp://" + profile.host + ":" + std::to_string(profile.port);
}

}  // namespace

struct MqttClient::Impl {
    std::unique_ptr<mqtt::async_client> client;
    mqtt::connect_options connect_options;
    StateCallback state_callback;
    MessageCallback message_callback;
    std::mutex queue_mutex;
    std::queue<MqttMessage> incoming_messages;
    std::atomic<bool> connected{false};
};

MqttClient::MqttClient() : impl_(std::make_unique<Impl>()) {}

MqttClient::~MqttClient() {
    disconnect();
}

void MqttClient::connect(const BrokerProfile& profile, StateCallback on_state) {
    disconnect();

    impl_->state_callback = std::move(on_state);
    const auto server_uri = make_server_uri(profile);

    mqtt::create_options create_opts(MQTTVERSION_3_1_1);
    impl_->client = std::make_unique<mqtt::async_client>(server_uri, profile.client_id, create_opts);

    impl_->connect_options = mqtt::connect_options_builder()
                                 .user_name(profile.username)
                                 .password(profile.password)
                                 .clean_session(true)
                                 .keep_alive_interval(std::chrono::seconds(30))
                                 .finalize();

    impl_->client->set_connection_lost_handler([this](const std::string& cause) {
        impl_->connected.store(false);
        if (impl_->state_callback) {
            impl_->state_callback(false, cause.empty() ? "Connection lost" : cause);
        }
    });

    impl_->client->set_message_callback([this](mqtt::const_message_ptr msg) {
        MqttMessage message;
        message.topic = msg->get_topic();
        message.payload = msg->to_string();
        message.qos = msg->get_qos();
        message.retained = msg->is_retained();
        message.received_at = std::chrono::system_clock::now();

        {
            std::lock_guard<std::mutex> lock(impl_->queue_mutex);
            impl_->incoming_messages.push(std::move(message));
        }
    });

    try {
        auto token = impl_->client->connect(impl_->connect_options);
        token->wait();
        impl_->connected.store(true);
        if (impl_->state_callback) {
            impl_->state_callback(true, "Connected to " + profile.host);
        }
    } catch (const mqtt::exception& ex) {
        impl_->connected.store(false);
        if (impl_->state_callback) {
            impl_->state_callback(false, ex.what());
        }
    }
}

void MqttClient::disconnect() {
    if (!impl_->client) {
        return;
    }

    try {
        if (impl_->client->is_connected()) {
            auto token = impl_->client->disconnect();
            token->wait();
        }
    } catch (const mqtt::exception&) {
    }

    impl_->connected.store(false);
    impl_->client.reset();
}

void MqttClient::subscribe(const std::string& topic, int qos) {
    if (!impl_->client || !impl_->connected.load()) {
        return;
    }

    try {
        auto token = impl_->client->subscribe(topic, qos);
        token->wait();
    } catch (const mqtt::exception&) {
    }
}

void MqttClient::unsubscribe(const std::string& topic) {
    if (!impl_->client || !impl_->connected.load()) {
        return;
    }

    try {
        auto token = impl_->client->unsubscribe(topic);
        token->wait();
    } catch (const mqtt::exception&) {
    }
}

void MqttClient::publish(const MqttMessage& message) {
    if (!impl_->client || !impl_->connected.load()) {
        return;
    }

    try {
        auto msg = mqtt::make_message(message.topic, message.payload, message.qos, message.retained);
        auto token = impl_->client->publish(msg);
        token->wait();
    } catch (const mqtt::exception&) {
    }
}

void MqttClient::set_message_handler(MessageCallback handler) {
    impl_->message_callback = std::move(handler);
}

bool MqttClient::is_connected() const {
    return impl_->connected.load();
}

void MqttClient::drain_incoming_messages() {
    if (!impl_->message_callback) {
        return;
    }

    std::queue<MqttMessage> pending;
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        pending.swap(impl_->incoming_messages);
    }

    while (!pending.empty()) {
        impl_->message_callback(pending.front());
        pending.pop();
    }
}

}  // namespace ramai