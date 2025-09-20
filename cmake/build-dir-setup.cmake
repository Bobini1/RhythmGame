add_custom_command(TARGET RhythmGame_exe POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/share ${CMAKE_INSTALL_DATADIR})

if (WIN32)
    set(alsoft_ext "ini")
else ()
    set(alsoft_ext "conf")
endif ()

if (WIN32)
    find_program(TOOL_WINDEPLOYQT NAMES windeployqt)
    find_program(TOOL_WINDEPLOYQT_DEBUG NAMES windeployqt.debug.bat)

    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND $<IF:$<CONFIG:Debug,RelWithDebInfo>,${TOOL_WINDEPLOYQT_DEBUG},${TOOL_WINDEPLOYQT}> --qmldir "${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default" --qmldir "${CMAKE_SOURCE_DIR}/RhythmGameQml"
            $<TARGET_FILE:RhythmGame_exe>
            COMMENT "Running windeployqt..."
            VERBATIM
    )

    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:RhythmGame_exe> $<TARGET_FILE_DIR:RhythmGame_exe>
            COMMAND_EXPAND_LISTS
    )

    # This is only for running in the build directory, before installation.
    add_custom_command(TARGET RhythmGame_exe POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/staticAssets/qt.conf
            ${CMAKE_BINARY_DIR}/bin/$<0:>/qt.conf)
endif ()