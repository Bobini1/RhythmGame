if (NOT WIN32)
    option(USE_SYSTEM_LIBRARIES "Do not deploy libraries and do not provide RhythmGame.sh" OFF)
endif ()
if (WIN32 OR NOT USE_SYSTEM_LIBRARIES)
    install(
            TARGETS RhythmGame_exe
            RUNTIME_DEPENDENCY_SET RuntimeDeps
            RUNTIME COMPONENT RhythmGame_Runtime
    )

    if (WIN32)
        qt_generate_deploy_app_script(
                TARGET RhythmGame_exe
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\""
        )
    else ()
        qt_generate_deploy_qml_app_script(
                TARGET RhythmGame_exe
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                POST_EXCLUDE_REGEXES
                    "^/lib/"
                    "^/lib64/"
                    "^/usr/lib/"
                    "^/usr/lib64/"
                DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\""
                MACOS_BUNDLE_POST_BUILD
        )
    endif ()
    install(SCRIPT ${deploy_script} COMPONENT RhythmGame_Runtime)

    if (NOT WIN32)
        install(PROGRAMS RhythmGame.sh DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif ()
else ()
    install(
            TARGETS RhythmGame_exe
            RUNTIME COMPONENT RhythmGame_Runtime
    )
endif ()

install(DIRECTORY share/RhythmGame/themes/Default
        DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/themes
        COMPONENT RhythmGame_Runtime)

foreach (asset_dir avatars bgm soundsets)
    install(DIRECTORY share/RhythmGame/${asset_dir}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame
            COMPONENT RhythmGame_Runtime)
endforeach ()

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

set(CPACK_PACKAGE_VENDOR "Bobini")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RhythmGame")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")
set(CPACK_COMPONENTS_ALL RhythmGame_Runtime)

if (WIN32 AND RhythmGame_USE_MIMALLOC)
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
