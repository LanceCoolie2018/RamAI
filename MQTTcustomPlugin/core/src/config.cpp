#include "ramai/config.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace ramai {
namespace {

BrokerProfile parse_profile(const nlohmann::json& json) {
    BrokerProfile profile;
    profile.name = json.value("name", "Default");
    profile.host = json.value("host", "127.0.0.1");
    profile.port = json.value("port", 1883);
    profile.client_id = json.value("client_id", "RamMQTT");
    profile.username = json.value("username", "");
    profile.password = json.value("password", "");

    if (json.contains("preset_subscriptions") && json["preset_subscriptions"].is_array()) {
        for (const auto& topic : json["preset_subscriptions"]) {
            profile.preset_subscriptions.push_back(topic.get<std::string>());
        }
    }

    return profile;
}

DeviceSensorMapping parse_mapping(const nlohmann::json& json) {
    DeviceSensorMapping mapping;
    mapping.topic = json.at("topic").get<std::string>();
    mapping.device_id = json.at("device_id").get<std::string>();
    mapping.device_label = json.value("device_label", mapping.device_id);
    mapping.sensor_key = json.at("sensor_key").get<std::string>();
    mapping.sensor_label = json.value("sensor_label", mapping.sensor_key);
    mapping.unit = json.value("unit", "");
    return mapping;
}

}  // namespace

bool load_app_config(const std::string& path, AppConfig& out_config) {
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    nlohmann::json json;
    input >> json;

    out_config.profiles.clear();
    out_config.device_mappings.clear();

    if (json.contains("profiles") && json["profiles"].is_array()) {
        for (const auto& profile_json : json["profiles"]) {
            out_config.profiles.push_back(parse_profile(profile_json));
        }
    } else if (json.contains("host")) {
        out_config.profiles.push_back(parse_profile(json));
    }

    if (json.contains("device_mappings") && json["device_mappings"].is_array()) {
        for (const auto& mapping_json : json["device_mappings"]) {
            out_config.device_mappings.push_back(parse_mapping(mapping_json));
        }
    }

    return true;
}

bool save_broker_profile(const std::string& path, const BrokerProfile& profile) {
    nlohmann::json json;
    json["name"] = profile.name;
    json["host"] = profile.host;
    json["port"] = profile.port;
    json["client_id"] = profile.client_id;
    json["username"] = profile.username;
    json["password"] = profile.password;
    json["preset_subscriptions"] = profile.preset_subscriptions;

    std::ofstream output(path);
    if (!output.is_open()) {
        return false;
    }

    output << json.dump(2);
    return true;
}

}  // namespace ramai