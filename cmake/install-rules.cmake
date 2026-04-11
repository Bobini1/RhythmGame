if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # ---- macOS bundle install ----
    install(
            TARGETS RhythmGame_exe
            BUNDLE DESTINATION . COMPONENT RhythmGame_Runtime
    )

    set(deploy_tool_options "-qmldir=${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default")

    qt_generate_deploy_qml_app_script(
            TARGET RhythmGame_exe
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS ${deploy_tool_options}
    )
    install(SCRIPT ${deploy_script})

    get_target_property(_svg_plugin_file Qt6::QSvgPlugin LOCATION)
    install(FILES "${_svg_plugin_file}"
            DESTINATION "RhythmGame.app/Contents/PlugIns/imageformats"
            COMPONENT RhythmGame_Runtime)
else ()
    # ---- Windows / Linux install ----
    option(USE_SYSTEM_LIBRARIES "Do not deploy libraries and do not provide RhythmGame.sh" OFF)

    if (WIN32 OR NOT USE_SYSTEM_LIBRARIES)
        install(
                TARGETS RhythmGame_exe
                RUNTIME_DEPENDENCY_SET RuntimeDeps
                RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT RhythmGame_Runtime
        )

        set(deploy_tool_options "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\"")
        qt_generate_deploy_qml_app_script(
                TARGET RhythmGame_exe
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                DEPLOY_TOOL_OPTIONS ${deploy_tool_options}
        )
        install(SCRIPT ${deploy_script})

        if (NOT WIN32)
            install(PROGRAMS RhythmGame.sh DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
        endif ()
    else ()
        install(
                TARGETS RhythmGame_exe
                RUNTIME COMPONENT RhythmGame_Runtime
        )
    endif ()

    install(DIRECTORY share/RhythmGame
            DESTINATION ${CMAKE_INSTALL_DATADIR}
            COMPONENT RhythmGame_Runtime)

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/profiles/
            COMPONENT RhythmGame_Runtime)

    install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/tables/
            COMPONENT RhythmGame_Runtime)

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

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # The DragNDrop CPack generator mishandles macOS app bundles by placing
    # extra files into Contents/Resources/bin.  Build the DMG ourselves from
    # the correctly-staged install tree produced by "cmake --install".
    set(_dmg_stage "${CMAKE_BINARY_DIR}/dmg_stage")
    set(_dmg_out   "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-Darwin.dmg")
    add_custom_target(dmg
            COMMENT "Creating DMG..."
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${_dmg_stage}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${_dmg_stage}"
            COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}"
                    --prefix "${_dmg_stage}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink /Applications
                    "${_dmg_stage}/Applications"
            COMMAND hdiutil create -volname "${PROJECT_NAME}"
                    -srcfolder "${_dmg_stage}"
                    -ov -format UDZO
                    "${_dmg_out}"
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${_dmg_stage}"
            VERBATIM
    )
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()
