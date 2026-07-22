function(rammqtt_find_windeployqt out_var)
    if(NOT WIN32 OR NOT TARGET Qt6::qmake)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    get_target_property(_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(_windeployqt_executable windeployqt HINTS ${_qt_bin_dir})
    set(${out_var} "${_windeployqt_executable}" PARENT_SCOPE)
endfunction()

function(rammqtt_stage_gui_to_root target_name)
    if(NOT WIN32)
        return()
    endif()

    rammqtt_find_windeployqt(_windeployqt)
    if(NOT _windeployqt)
        message(WARNING "windeployqt not found; ${target_name} will not be staged to project root.")
        return()
    endif()

    set(_stage_root "${CMAKE_SOURCE_DIR}")
    set(_qt_plugin_dirs platforms styles imageformats iconengines generic networkinformation tls)

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND "${_windeployqt}" --no-translations "$<TARGET_FILE:${target_name}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${target_name}>"
            "${_stage_root}/RamMQTT.exe"
        COMMAND powershell -NoProfile -Command
            "Copy-Item -Force '$<TARGET_FILE_DIR:${target_name}>\\*.dll' '${_stage_root}'"
        COMMENT "Staging RamMQTT runtime to project root"
    )

    foreach(_plugin_dir IN LISTS _qt_plugin_dirs)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND powershell -NoProfile -Command
                "if (Test-Path '$<TARGET_FILE_DIR:${target_name}>\\${_plugin_dir}') { Copy-Item -Recurse -Force '$<TARGET_FILE_DIR:${target_name}>\\${_plugin_dir}' '${_stage_root}' }"
        )
    endforeach()
endfunction()

function(rammqtt_stage_probe_to_root target_name)
    if(NOT WIN32)
        return()
    endif()

    set(_stage_root "${CMAKE_SOURCE_DIR}")

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${target_name}>"
            "${_stage_root}/RamMQTTProbe.exe"
        COMMENT "Staging RamMQTTProbe to project root"
    )
endfunction()