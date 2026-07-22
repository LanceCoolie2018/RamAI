#pragma once

#include "ramai/config.hpp"
#include "ramai/message.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace ramai {

class MqttClient {
public:
    using MessageCallback = std::function<void(const MqttMessage&)>;
    using StateCallback = std::function<void(bool connected, const std::string& detail)>;

    MqttClient();
    ~MqttClient();

    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;

    void connect(const BrokerProfile& profile, StateCallback on_state);
    void disconnect();
    void subscribe(const std::string& topic, int qos = 0);
    void unsubscribe(const std::string& topic);
    void publish(const MqttMessage& message);
    void set_message_handler(MessageCallback handler);

    bool is_connected() const;
    void drain_incoming_messages();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ramai