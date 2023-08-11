install(
        TARGETS RhythmGame_exe
        RUNTIME COMPONENT RhythmGame_Runtime
)

install(DIRECTORY assets/ DESTINATION assets/
        COMPONENT RhythmGame_Runtime)

qt_generate_deploy_app_script(
        TARGET RhythmGame_exe
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

if (PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif ()
