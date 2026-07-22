cmake_minimum_required(VERSION 3.20)
project(RamMQTT VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RAMMQTT_BUILD_APP "Build the Qt GUI application" ON)
option(RAMMQTT_BUILD_PROBE "Build the headless MQTT probe CLI" ON)

add_subdirectory(core)

if(RAMMQTT_BUILD_PROBE)
    add_subdirectory(probe)
endif()

if(RAMMQTT_BUILD_APP)
    add_subdirectory(app)
endif()