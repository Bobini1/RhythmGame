install(
        TARGETS RhythmGame_exe
        RUNTIME_DEPENDENCY_SET RuntimeDeps
        RUNTIME COMPONENT RhythmGame_Runtime
)

install(DIRECTORY data/avatars/ DESTINATION data/avatars/
        COMPONENT RhythmGame_Runtime)


install(DIRECTORY data/themes/ DESTINATION data/themes/
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY DESTINATION data/profiles/
        COMPONENT RhythmGame_Runtime)

install(DIRECTORY DESTINATION data/tables/
        COMPONENT RhythmGame_Runtime)

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

if (WIN32)
    install(FILES alsoft.ini DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT RhythmGame_Runtime)
else ()
    install(FILES alsoft.ini DESTINATION "${CMAKE_INSTALL_BINDIR}" RENAME alsoft.conf
            COMPONENT RhythmGame_Runtime)
endif ()

set(CPACK_PACKAGE_VENDOR "Bobini")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RhythmGame")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.md")

if (WIN32)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc-debug.dll DESTINATION "${CMAKE_INSTALL_BINDIR}"
                COMPONENT RhythmGame_Runtime)
    else ()
        install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc.dll DESTINATION "${CMAKE_INSTALL_BINDIR}"
                COMPONENT RhythmGame_Runtime)
    endif ()
    install(FILES ${CMAKE_BINARY_DIR}/bin/mimalloc-redirect.dll DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT RhythmGame_Runtime)

    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS
        "Exec 'icacls \\\"$INSTDIR/data\\\" /grant *S-1-5-32-545:(OI)(CI)F /T'"
    )
    set(CPACK_PACKAGE_EXECUTABLES "RhythmGame" "RhythmGame")
endif ()

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()