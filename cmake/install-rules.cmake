if (NOT WIN32)
    option(USE_SYSTEM_LIBRARIES "Deploy libraries and provide RhythmGame.sh" OFF)
endif ()
if (WIN32 OR NOT USE_SYSTEM_LIBRARIES)
    install(
            TARGETS RhythmGame_exe
            RUNTIME_DEPENDENCY_SET RuntimeDeps
            RUNTIME COMPONENT RhythmGame_Runtime
    )

    qt_generate_deploy_qml_app_script(
            TARGET RhythmGame_exe
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/share/RhythmGame/themes/Default\""
            MACOS_BUNDLE_POST_BUILD
    )
    install(SCRIPT ${deploy_script})

    if (NOT WIN32)
        install(FILES RhythmGame.sh DESTINATION ${CMAKE_INSTALL_BINDIR} PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif ()
else ()
    install(
            TARGETS RhythmGame_exe
            RUNTIME COMPONENT RhythmGame_Runtime
    )
endif ()


install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/profiles/
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/tables/
        COMPONENT RhythmGame_Runtime)

install(FILES share/RhythmGame/avatars/mascot.png DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/avatars
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY share/RhythmGame/themes/Default DESTINATION ${CMAKE_INSTALL_DATADIR}/RhythmGame/themes
        COMPONENT RhythmGame_Runtime)


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