#pragma once

#include "ramai/config.hpp"
#include "ramai/message.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ramai {

class DeviceRegistry {
public:
    void load_mappings(const std::vector<DeviceSensorMapping>& mappings);

    std::optional<SensorReading> parse_message(const MqttMessage& message) const;

    std::vector<std::string> device_ids() const;
    std::string device_label(const std::string& device_id) const;

private:
    struct MappingEntry {
        DeviceSensorMapping mapping;
    };

    std::unordered_map<std::string, MappingEntry> topic_to_mapping_;
    std::unordered_map<std::string, std::string> device_labels_;
};

}  // namespace ramai