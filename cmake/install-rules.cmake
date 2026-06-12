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
        set(deploy_script "${CMAKE_CURRENT_BINARY_DIR}/.qt/deploy_qml_app_RhythmGame_exe_installed.cmake")
        file(GENERATE OUTPUT "${deploy_script}" CONTENT
"include(\"${CMAKE_CURRENT_BINARY_DIR}/.qt/QtDeploySupport-\${CMAKE_INSTALL_CONFIG_NAME}.cmake\")
include(\"${CMAKE_CURRENT_BINARY_DIR}/.qt/RhythmGame_exe-plugins-\${CMAKE_INSTALL_CONFIG_NAME}.cmake\" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS \"qtbase;qtdeclarative;qtserialport;qtwebsockets;qtmultimedia\")

qt6_deploy_runtime_dependencies(
    EXECUTABLE \"\${QT_DEPLOY_PREFIX}/\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:RhythmGame_exe>\"
    GENERATE_QT_CONF
    DEPLOY_TOOL_OPTIONS \"--qmldir\" \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\"
)
")
        install(SCRIPT ${deploy_script})
    else ()
        qt_generate_deploy_qml_app_script(
                TARGET RhythmGame_exe
                OUTPUT_SCRIPT deploy_script
                NO_UNSUPPORTED_PLATFORM_ERROR
                DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\""
                MACOS_BUNDLE_POST_BUILD
        )
        install(SCRIPT ${deploy_script})
    endif ()

    if (NOT WIN32)
        install(PROGRAMS RhythmGame.sh DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif ()
else ()
    install(
            TARGETS RhythmGame_exe
            RUNTIME COMPONENT RhythmGame_Runtime
    )
endif ()

install(DIRECTORY
        share/RhythmGame/avatars
        share/RhythmGame/bgm
        share/RhythmGame/soundsets
        DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY share/RhythmGame/themes/Default
        DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/themes
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

set(CPACK_PACKAGE_VENDOR "Bobini")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RhythmGame")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")

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
