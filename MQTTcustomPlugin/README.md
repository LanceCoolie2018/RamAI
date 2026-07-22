# RamMQTT (MQTTcustomPlugin)

Standalone Windows MQTT client for the RamAI Smart Bed lab. The `core/` library is shared with the upcoming Unreal plugin.

## Quick start

Double-click **`RunRamMQTT.bat`** or run **`RamMQTT.exe`** from this folder (after building once).

1. Click **Connect**
2. Click **Smart Bed Presets**
3. Watch **Sensors by Device** (FunHouse vs Rain ESP32)

Credentials: copy `config/default_profile.json` → `config/local_profile.json` and add broker username/password. The app prefers `local_profile.json` automatically.

## Project layout

```
MQTTcustomPlugin/
├── RamMQTT.exe          # staged here after build (gitignored)
├── RamMQTTProbe.exe     # optional CLI probe (gitignored)
├── RunRamMQTT.bat       # launcher
├── config/              # broker + device-to-topic mappings
├── core/                # ramai_mqtt_core — used by GUI and future UE plugin
├── app/                 # Qt GUI sources
├── probe/               # headless CLI sources
├── cmake/               # build helpers (staging, deploy)
└── build/               # CMake output (gitignored)
```

## Rebuild

```powershell
cd D:\RamAI\MQTTcustomPlugin

cmake --build build --config Release
```

A full rebuild stages `RamMQTT.exe`, Qt/Paho DLLs, and plugin folders to this directory automatically.

### First-time setup

```powershell
# Dependencies (once)
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install paho-mqttpp3 nlohmann-json --triplet x64-windows

# Qt 6.8 (once)
python -m venv .qtvenv
.\.qtvenv\Scripts\pip install aqtinstall
.\.qtvenv\Scripts\aqt install-qt windows desktop 6.8.1 win64_msvc2022_64 -O Qt

# Configure (once)
cmake -B build -S . `
  -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DCMAKE_PREFIX_PATH=D:/RamAI/MQTTcustomPlugin/Qt/6.8.1/msvc2022_64 `
  -DRAMMQTT_BUILD_APP=ON `
  -DRAMMQTT_BUILD_PROBE=ON `
  -G "Visual Studio 17 2022" -A x64
```

## Device mapping

Edit `config/default_profile.json` → `device_mappings` to attribute topics to devices. Example: FunHouse temp vs a future basement sensor.

## Next: Unreal plugin

`core/` will be wrapped as a UE 5.6 Runtime module (`RamMqttSubsystem`) for RamBedAI, replacing the experimental Epic MQTT plugin and legacy MqttUtilities.