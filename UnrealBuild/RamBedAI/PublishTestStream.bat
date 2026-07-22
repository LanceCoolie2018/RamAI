@echo off
setlocal

set BROKER=10.20.25.40
set TOPIC=tailgate/stream/active
set PAYLOAD={"id":"nba","name":"NBA Channel","url":"https://tubitv.com/live/400000116/the-nba-channel"}

echo Publishing test stream to %BROKER% topic %TOPIC%
echo Payload: %PAYLOAD%
echo.
echo Use your MQTT credentials when prompted, or run the same publish from RamMQTT.
echo.

where mosquitto_pub >nul 2>&1
if errorlevel 1 (
    echo mosquitto_pub not found in PATH.
    echo Install Mosquitto clients or publish from Home Assistant / RamMQTT instead.
    pause
    exit /b 1
)

set /p MQTT_USER=MQTT username:
set /p MQTT_PASS=MQTT password:

mosquitto_pub -h %BROKER% -u %MQTT_USER% -P %MQTT_PASS% -t %TOPIC% -r -q 1 -m %PAYLOAD%
if errorlevel 1 (
    echo Publish failed.
    pause
    exit /b 1
)

echo Publish succeeded. Check the stadium screen in VR.
pause