option(PORTABLE_BUILD "Deploy libraries and provide RhythmGame.sh" ON)
if (PORTABLE_BUILD OR WIN32)
    install(
            TARGETS RhythmGame_exe
            RUNTIME_DEPENDENCY_SET RuntimeDeps
            RUNTIME COMPONENT RhythmGame_Runtime
    )

    qt_generate_deploy_qml_app_script(
            TARGET RhythmGame_exe
            OUTPUT_SCRIPT deploy_script
            NO_UNSUPPORTED_PLATFORM_ERROR
            DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/data/themes/Default\""
            MACOS_BUNDLE_POST_BUILD
    )
    install(SCRIPT ${deploy_script})

    if (LINUX)
        install(FILES RhythmGame.sh DESTINATION "${CMAKE_INSTALL_BINDIR}" PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif ()
    
    install(DIRECTORY DESTINATION ${DATA_FOLDER_PREFIX}/profiles/
            COMPONENT RhythmGame_Runtime)

    install(DIRECTORY DESTINATION ${DATA_FOLDER_PREFIX}/tables/
            COMPONENT RhythmGame_Runtime)
else ()
    install(
            TARGETS RhythmGame_exe
            RUNTIME COMPONENT RhythmGame_Runtime
    )
endif ()

set(DATA_FOLDER_PREFIX "data" CACHE STRING "Prefix for data folder installation")

install(FILES data/avatars/mascot.png DESTINATION ${DATA_FOLDER_PREFIX}/avatars
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY data/themes/Default DESTINATION ${DATA_FOLDER_PREFIX}/themes
        COMPONENT RhythmGame_Runtime)


set(CPACK_PACKAGE_VENDOR "Bobini")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RhythmGame")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")

if (WIN32)
    install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc$<$<CONFIG:Debug>:-debug>.dll
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT RhythmGame_Runtime)
    install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc-redirect.dll DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT RhythmGame_Runtime)

    if (PORTABLE_BUILD)
        set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
                "Exec 'icacls \\\"$INSTDIR/data\\\" /grant *S-1-5-32-545:(OI)(CI)F /T'"
        )
    endif ()
    set(CPACK_PACKAGE_EXECUTABLES "RhythmGame" "RhythmGame")
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()