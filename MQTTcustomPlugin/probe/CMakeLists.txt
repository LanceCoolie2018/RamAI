add_executable(RamMQTTProbe main.cpp)

find_package(PahoMqttCpp CONFIG REQUIRED)

target_link_libraries(RamMQTTProbe PRIVATE
    ramai_mqtt_core
    PahoMqttCpp::paho-mqttpp3-shared
)

include(${CMAKE_SOURCE_DIR}/cmake/StageRuntime.cmake)

add_custom_command(TARGET RamMQTTProbe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:RamMQTTProbe>/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config/default_profile.json
        $<TARGET_FILE_DIR:RamMQTTProbe>/config/default_profile.json
)

rammqtt_stage_probe_to_root(RamMQTTProbe)