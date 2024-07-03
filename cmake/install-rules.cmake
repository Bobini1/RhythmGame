install(
        TARGETS RhythmGame_exe
        RUNTIME COMPONENT RhythmGame_Runtime
)

install(DIRECTORY assets/avatars/ DESTINATION assets/avatars/
        COMPONENT RhythmGame_Runtime)


install(DIRECTORY assets/themes/ DESTINATION assets/themes/
        COMPONENT RhythmGame_Runtime)

qt_generate_deploy_qml_app_script(
        TARGET RhythmGame_exe
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        DEPLOY_TOOL_OPTIONS "--qmldir \"${CMAKE_SOURCE_DIR}/assets/themes/Default\""
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

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()
