install(
        TARGETS RhythmGame_exe
        RUNTIME COMPONENT RhythmGame_Runtime
)

install(DIRECTORY assets/avatars/ DESTINATION assets/avatars/
        COMPONENT RhythmGame_Runtime)


install(DIRECTORY assets/themes/ DESTINATION assets/themes/
        COMPONENT RhythmGame_Runtime)

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
