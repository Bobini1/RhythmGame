install(
        TARGETS RhythmGame_exe
        RUNTIME COMPONENT RhythmGame_Runtime
)

install(DIRECTORY assets/avatars/ DESTINATION assets/avatars/
        COMPONENT RhythmGame_Runtime)


install(DIRECTORY assets/themes/ DESTINATION assets/themes/
        COMPONENT RhythmGame_Runtime
        PATTERN "assets/themes/Default/scripts/third_party/*" EXCLUDE)

file(GLOB_RECURSE third_party_dirs FOLLOW_SYMLINKS RELATIVE ${CMAKE_SOURCE_DIR}/assets/themes/Default/scripts/third_party
        ${CMAKE_SOURCE_DIR}/assets/themes/Default/scripts/third_party/*)
message(STATUS "third_party_dirs: ${third_party_dirs}")
foreach (file IN LISTS third_party_dirs)
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES assets/themes/Default/scripts/third_party/${file} DESTINATION assets/themes/Default/scripts/third_party/${dir}
            COMPONENT RhythmGame_Runtime)
endforeach ()

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
