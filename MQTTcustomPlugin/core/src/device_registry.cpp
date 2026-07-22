#include "ramai/device_registry.hpp"

#include <algorithm>
#include <cctype>

namespace ramai {
namespace {

std::string trim_copy(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

}  // namespace

void DeviceRegistry::load_mappings(const std::vector<DeviceSensorMapping>& mappings) {
    topic_to_mapping_.clear();
    device_labels_.clear();

    for (const auto& mapping : mappings) {
        topic_to_mapping_.emplace(mapping.topic, MappingEntry{mapping});
        device_labels_[mapping.device_id] = mapping.device_label;
    }
}

std::optional<SensorReading> DeviceRegistry::parse_message(const MqttMessage& message) const {
    const auto it = topic_to_mapping_.find(message.topic);
    if (it == topic_to_mapping_.end()) {
        return std::nullopt;
    }

    const auto& mapping = it->second.mapping;
    SensorReading reading;
    reading.device_id = mapping.device_id;
    reading.device_label = mapping.device_label;
    reading.sensor_key = mapping.sensor_key;
    reading.sensor_label = mapping.sensor_label;
    reading.value = trim_copy(message.payload);
    reading.unit = mapping.unit;
    reading.received_at = message.received_at;
    return reading;
}

std::vector<std::string> DeviceRegistry::device_ids() const {
    std::vector<std::string> ids;
    ids.reserve(device_labels_.size());
    for (const auto& [device_id, _] : device_labels_) {
        ids.push_back(device_id);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

std::string DeviceRegistry::device_label(const std::string& device_id) const {
    const auto it = device_labels_.find(device_id);
    if (it == device_labels_.end()) {
        return device_id;
    }
    return it->second;
}

}  // namespace ramai