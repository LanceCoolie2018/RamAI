#include "ramai/config.hpp"
#include "ramai/device_registry.hpp"
#include "ramai/mqtt_client.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <thread>

namespace {

std::atomic<bool> g_running{true};

void on_signal(int) {
    g_running.store(false);
}

void print_usage() {
    std::cout << "RamMQTTProbe - connect to the lab broker and print device-grouped sensor data\n"
              << "Usage: RamMQTTProbe [config_path] [seconds_to_run]\n";
}

std::string config_path(int argc, char** argv) {
    if (argc > 1) {
        return argv[1];
    }
    return "config/default_profile.json";
}

int run_seconds(int argc, char** argv) {
    if (argc > 2) {
        return std::max(1, std::stoi(argv[2]));
    }
    return 30;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--help") {
        print_usage();
        return 0;
    }

    std::signal(SIGINT, on_signal);

    ramai::AppConfig app_config;
    if (!ramai::load_app_config(config_path(argc, argv), app_config) || app_config.profiles.empty()) {
        std::cerr << "Failed to load config/default_profile.json\n";
        return 1;
    }

    ramai::DeviceRegistry registry;
    registry.load_mappings(app_config.device_mappings);

    ramai::MqttClient client;
    std::map<std::string, std::map<std::string, ramai::SensorReading>> latest_by_device;

    client.set_message_handler([&](const ramai::MqttMessage& message) {
        std::cout << "[MQTT] " << message.topic << " = " << message.payload << '\n';

        if (const auto reading = registry.parse_message(message)) {
            latest_by_device[reading->device_id][reading->sensor_key] = *reading;
        }
    });

    const auto& profile = app_config.profiles.front();
    std::cout << "Connecting to " << profile.host << ":" << profile.port << "...\n";

    client.connect(profile, [&](bool connected, const std::string& detail) {
        std::cout << (connected ? "[OK] " : "[ERR] ") << detail << '\n';
    });

    if (!client.is_connected()) {
        std::cerr << "Could not connect. Check network/VPN and broker credentials in the config.\n";
        return 1;
    }

    for (const auto& topic : profile.preset_subscriptions) {
        client.subscribe(topic, 0);
        std::cout << "Subscribed: " << topic << '\n';
    }

    const int duration = run_seconds(argc, argv);
    std::cout << "Listening for " << duration << " seconds. Press Ctrl+C to stop.\n\n";

    const auto started = std::chrono::steady_clock::now();
    while (g_running.load()) {
        client.drain_incoming_messages();

        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - started);
        if (elapsed.count() >= duration) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "\n=== Sensors by Device ===\n";
    if (latest_by_device.empty()) {
        std::cout << "No mapped sensor messages received.\n";
        std::cout << "If MQTT traffic appeared above, add mappings in config/default_profile.json.\n";
    }

    for (const auto& [device_id, sensors] : latest_by_device) {
        const auto label = registry.device_label(device_id);
        std::cout << "\n" << label << " (" << device_id << ")\n";
        for (const auto& [sensor_key, reading] : sensors) {
            std::cout << "  " << reading.sensor_label << ": " << reading.value;
            if (!reading.unit.empty()) {
                std::cout << ' ' << reading.unit;
            }
            std::cout << '\n';
        }
    }

    client.disconnect();
    return 0;
}