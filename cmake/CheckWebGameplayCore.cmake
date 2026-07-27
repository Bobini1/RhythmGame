if (NOT DEFINED SOURCE_ROOT)
    message(FATAL_ERROR "SOURCE_ROOT is required")
endif ()

set(core_files
        "${SOURCE_ROOT}/src/gameplay_logic/SinglePlayerGameplayCore.h"
        "${SOURCE_ROOT}/src/gameplay_logic/SinglePlayerGameplayCore.cpp"
        "${SOURCE_ROOT}/src/gameplay_logic/GameplayTrace.h"
        "${SOURCE_ROOT}/src/gameplay_logic/GameplayTrace.cpp"
        "${SOURCE_ROOT}/tools/web-playtest-native/TraceRunner.cpp"
)
set(forbidden
        "sqlitecpp"
        "vars.h"
        "miniaudiobackend"
        "audioengine"
        "profile.h"
        "chartrunner"
        "bga"
        "qtmultimedia"
        "llfio"
        "wil/"
)

foreach (file IN LISTS core_files)
    file(READ "${file}" contents)
    string(TOLOWER "${contents}" contents)
    foreach (token IN LISTS forbidden)
        string(FIND "${contents}" "${token}" found)
        if (NOT found EQUAL -1)
            message(FATAL_ERROR
                    "Portable gameplay source ${file} contains forbidden dependency token: ${token}")
        endif ()
    endforeach ()
endforeach ()

message(STATUS "Portable gameplay core source contract passed")
