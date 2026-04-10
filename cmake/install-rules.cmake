if (NOT WIN32)
    option(USE_SYSTEM_LIBRARIES "Do not deploy libraries and do not provide RhythmGame.sh" OFF)
endif ()
if (WIN32 OR NOT USE_SYSTEM_LIBRARIES)
    install(
            TARGETS RhythmGame_exe
            RUNTIME_DEPENDENCY_SET RuntimeDeps
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  COMPONENT RhythmGame_Runtime
            BUNDLE  DESTINATION .                         COMPONENT RhythmGame_Runtime
    )

    if (APPLE)
        set(deploy_tool_options "-qmldir=${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default")
    else ()
        set(deploy_tool_options "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\"")
    endif ()
    qt_generate_deploy_qml_app_script(
            TARGET RhythmGame_exe
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS ${deploy_tool_options}
            MACOS_BUNDLE_POST_BUILD
    )
    install(SCRIPT ${deploy_script})

    if (NOT WIN32 AND NOT APPLE)
        install(PROGRAMS RhythmGame.sh DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif ()
else ()
    install(
            TARGETS RhythmGame_exe
            RUNTIME COMPONENT RhythmGame_Runtime
    )
endif ()

if (APPLE)
    # install(TARGETS BUNDLE ...) copies the bundle verbatim, preserving the
    # build-time symlink Resources/RhythmGame -> <source>/share/RhythmGame.
    # Remove that symlink and copy the real directory tree instead.
    install(CODE "
        set(_res_dir \"\${CMAKE_INSTALL_PREFIX}/RhythmGame.app/Contents/Resources\")
        file(REMOVE \"\${_res_dir}/RhythmGame\")
        file(COPY \"${CMAKE_SOURCE_DIR}/share/RhythmGame\"
             DESTINATION \"\${_res_dir}\")
    " COMPONENT RhythmGame_Runtime)

    # macdeployqt does not auto-deploy imageformats/libqsvg.dylib even when
    # Qt6Svg is linked.  Copy it manually and rewrite its @rpath references to
    # the @loader_path-relative form that macdeployqt uses for other plugins.
    if (NOT USE_SYSTEM_LIBRARIES)
        include(${Qt6_DIR}/../Qt6Gui/Qt6QSvgPluginConfig.cmake OPTIONAL)
        get_target_property(_svg_plugin_src Qt6::QSvgPlugin IMPORTED_LOCATION)
        if (_svg_plugin_src)
            install(CODE "
                set(_src \"${_svg_plugin_src}\")
                set(_dst \"\${CMAKE_INSTALL_PREFIX}/RhythmGame.app/Contents/PlugIns/imageformats\")
                file(MAKE_DIRECTORY \"\${_dst}\")
                file(COPY \"\${_src}\" DESTINATION \"\${_dst}\")
                set(_plugin \"\${_dst}/libqsvg.dylib\")
                execute_process(COMMAND otool -L \"\${_plugin}\" OUTPUT_VARIABLE _otool)
                string(REGEX MATCHALL \"@rpath/[^ ]+\" _refs \"\${_otool}\")
                foreach(_old IN LISTS _refs)
                    string(REGEX REPLACE
                        \"@rpath/(libQt6[A-Za-z0-9]+)[.][0-9][0-9.]*[.]dylib\"
                        \"@loader_path/../../Frameworks/\\1.6.dylib\"
                        _new \"\${_old}\")
                    if(NOT _old STREQUAL _new)
                        execute_process(COMMAND install_name_tool
                            -change \"\${_old}\" \"\${_new}\" \"\${_plugin}\")
                    endif()
                endforeach()
            " COMPONENT RhythmGame_Runtime)
        endif ()
    endif ()
else ()
    install(DIRECTORY share/RhythmGame
            DESTINATION ${CMAKE_INSTALL_DATADIR}
            COMPONENT RhythmGame_Runtime)

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/profiles/
            COMPONENT RhythmGame_Runtime)

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/tables/
            COMPONENT RhythmGame_Runtime)
endif ()

if (LINUX)
    install(
            PROGRAMS staticAssets/RhythmGame.desktop
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
            COMPONENT RhythmGame_Runtime
    )
    install(
            FILES staticAssets/icon.svg
            DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps"
            RENAME RhythmGame.svg
            COMPONENT RhythmGame_Runtime
    )
    install(
            FILES LICENSE.md
            DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/rhythmgame"
            RENAME LICENSE
            COMPONENT RhythmGame_Runtime
    )
endif ()

set(CPACK_PACKAGE_VENDOR "Bobini")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RhythmGame")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")

if (WIN32)
    install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc$<$<CONFIG:Debug>:-debug>.dll
            DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT RhythmGame_Runtime)
    install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc-redirect.dll DESTINATION ${CMAKE_INSTALL_BINDIR}
            COMPONENT RhythmGame_Runtime)

    set(CPACK_PACKAGE_EXECUTABLES "RhythmGame" "RhythmGame")
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()
