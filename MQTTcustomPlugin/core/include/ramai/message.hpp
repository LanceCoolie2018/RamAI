#pragma once

#include <chrono>
#include <string>

namespace ramai {

struct MqttMessage {
    std::string topic;
    std::string payload;
    int qos = 0;
    bool retained = false;
    std::chrono::system_clock::time_point received_at =
        std::chrono::system_clock::now();
};

struct SensorReading {
    std::string device_id;
    std::string device_label;
    std::string sensor_key;
    std::string sensor_label;
    std::string value;
    std::string unit;
    std::chrono::system_clock::time_point received_at =
        std::chrono::system_clock::now();
};

}  // namespace ramai