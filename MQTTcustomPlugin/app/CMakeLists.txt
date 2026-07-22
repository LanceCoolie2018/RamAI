find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

set(RAMMQTT_APP_SOURCES
    main.cpp
    main_window.cpp
    connection_panel.cpp
    subscription_panel.cpp
    message_log.cpp
    publish_panel.cpp
    device_dashboard.cpp
)

qt_add_executable(RamMQTT ${RAMMQTT_APP_SOURCES})

target_include_directories(RamMQTT PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(RamMQTT
    PRIVATE
        ramai_mqtt_core
        Qt6::Widgets
)

if(WIN32)
    set_target_properties(RamMQTT PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/StageRuntime.cmake)

add_custom_command(TARGET RamMQTT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:RamMQTT>/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config/default_profile.json
        $<TARGET_FILE_DIR:RamMQTT>/config/default_profile.json
)

rammqtt_stage_gui_to_root(RamMQTT)