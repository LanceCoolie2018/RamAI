# Stadium Stream Prototype

MQTT channel selection → curved stadium screen in RamBed VR.

## Quick test (no HA changes)

From a machine with MQTT access:

```powershell
# Or use RamMQTTProbe after publishing manually via HA button
```

Publish with mosquitto_pub or tap the button in `MQTTcustomPlugin/config/ha_stream_buttons.yaml`.

## Unreal setup

1. Build `MQTTcustomPlugin` core lib (if not already):
   ```powershell
   cmake --build D:\RamAI\MQTTcustomPlugin\build --config Release
   ```

2. Open `RamBedAI.uproject` and compile C++.

3. **HA dashboard (already in RamBed):** `BP_HA_AnchoredDisplay` shows `HA_Dashboard` at `http://10.20.25.40:8123/truck-bed/0`. Reposition in the level if needed.

4. **Sports screen:** auto-spawns on play via `RamBedSceneSetupSubsystem` (see `Config/DefaultGame.ini`). Or run `Scripts/SetupRamBedScreens.py` from **Tools → Execute Python Script** to place it permanently in the map.

5. PIE in VR — MQTT connects to `10.20.25.40` and subscribes to `tailgate/stream/active`.

6. Publish a channel URL (HA button or mosquitto_pub) — stadium screen loads Tubi.

### Quick PIE test without MQTT

Select the stadium screen actor and set **Test Stream Url** to a Tubi live page, e.g. `https://tubitv.com/live/400000116/the-nba-channel`.

### Quest / Android

MQTT native libs are **Win64-only** for now. Quest builds package successfully using **Test Stream Url** (default NBA Tubi URL in `Config/DefaultGame.ini`). Live MQTT on Quest needs Android Paho binaries (future step).

## HA button

See [`MQTTcustomPlugin/config/ha_stream_buttons.yaml`](../../MQTTcustomPlugin/config/ha_stream_buttons.yaml).

Replace your dashboard button's `url` tap action with `mqtt.publish` to `tailgate/stream/active`.

## Components

| Piece | Location |
|-------|----------|
| `URamMqttSubsystem` | `Plugins/RamMqtt` |
| `AStadiumScreenActor` | `Source/RamBedAI` |
| `UStadiumStreamWidget` | WebBrowser on world widget |

## Next steps (not in prototype)

- MR grab/move/anchor on stadium screen
- Curved mesh + render target material
- Configurable broker credentials (not hardcoded)
- Quest Android Paho binaries