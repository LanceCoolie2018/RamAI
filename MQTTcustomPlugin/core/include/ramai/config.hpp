#pragma once

#include <string>
#include <vector>

namespace ramai {

struct BrokerProfile {
    std::string name;
    std::string host;
    int port = 1883;
    std::string client_id;
    std::string username;
    std::string password;
    std::vector<std::string> preset_subscriptions;
};

struct DeviceSensorMapping {
    std::string topic;
    std::string device_id;
    std::string device_label;
    std::string sensor_key;
    std::string sensor_label;
    std::string unit;
};

struct AppConfig {
    std::vector<BrokerProfile> profiles;
    std::vector<DeviceSensorMapping> device_mappings;
};

bool load_app_config(const std::string& path, AppConfig& out_config);
bool save_broker_profile(const std::string& path, const BrokerProfile& profile);

}  // namespace ramai