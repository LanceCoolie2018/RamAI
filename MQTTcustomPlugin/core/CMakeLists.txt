add_library(ramai_mqtt_core STATIC
    src/mqtt_client.cpp
    src/config.cpp
    src/device_registry.cpp
)

target_include_directories(ramai_mqtt_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(nlohmann_json CONFIG QUIET)

if(NOT nlohmann_json_FOUND)
    find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
    if(NLOHMANN_JSON_INCLUDE_DIR)
        add_library(nlohmann_json INTERFACE)
        target_include_directories(nlohmann_json INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR})
        add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    else()
        message(FATAL_ERROR "nlohmann_json not found. Install via vcpkg or libnlohmann-json3-dev.")
    endif()
endif()

find_package(PahoMqttCpp CONFIG QUIET)

if(NOT PahoMqttCpp_FOUND)
    find_library(PAHO_MQTT_CPP_LIB NAMES paho-mqttpp3)
    find_library(PAHO_MQTT_C_LIB NAMES paho-mqtt3as paho-mqtt3a)
    find_path(PAHO_MQTT_INCLUDE_DIR mqtt/async_client.h)

    if(PAHO_MQTT_CPP_LIB AND PAHO_MQTT_C_LIB AND PAHO_MQTT_INCLUDE_DIR)
        add_library(PahoMqttCpp::paho-mqttpp3 UNKNOWN IMPORTED)
        set_target_properties(PahoMqttCpp::paho-mqttpp3 PROPERTIES
            IMPORTED_LOCATION ${PAHO_MQTT_CPP_LIB}
            INTERFACE_INCLUDE_DIRECTORIES ${PAHO_MQTT_INCLUDE_DIR}
            INTERFACE_LINK_LIBRARIES ${PAHO_MQTT_C_LIB}
        )
    else()
        message(FATAL_ERROR "Paho MQTT C++ not found. Install via vcpkg or libpaho-mqttpp-dev.")
    endif()
endif()

target_link_libraries(ramai_mqtt_core
    PUBLIC
        PahoMqttCpp::paho-mqttpp3
        nlohmann_json::nlohmann_json
)

if(WIN32)
    target_compile_definitions(ramai_mqtt_core PUBLIC _WIN32_WINNT=0x0601)
    if(TARGET PahoMqttCpp::paho-mqttpp3-shared)
        target_compile_definitions(ramai_mqtt_core PUBLIC PAHO_MQTTPP_IMPORTS)
    endif()
endif()