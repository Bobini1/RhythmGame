cmake_minimum_required(VERSION 3.20)

include(cmake/prelude.cmake)
project(
        RhythmGame
        VERSION 0.0.0
        DESCRIPTION "A performant and portable rhythm game engine"
        HOMEPAGE_URL "https://github.com/Bobini1"
        LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)


# <------------ lua support --------------->

find_package(sol2 REQUIRED)
#target_link_libraries(RhythmGame_lib PRIVATE sol2::sol2)

# <------------ hiredis (db) --------------->

add_library(hiredis::hiredis UNKNOWN IMPORTED rhythm_game/DB/source/DB.cpp rhythm_game/DB/include/DB.h)
find_path(HIREDIS_HEADER hiredis REQUIRED)
find_library(HIREDIS_LIB hiredis REQUIRED)

set_target_properties(hiredis::hiredis PROPERTIES IMPORTED_LOCATION ${HIREDIS_LIB})
set_target_properties(hiredis::hiredis PROPERTIES IMPORTED_HEADER_LOCATION ${HIREDIS_HEADER})


# <------------ redis-plus-plus (db)-------------->

add_library(redis++::redis++ UNKNOWN IMPORTED)
find_path(REDIS_PP_HEADER sw REQUIRED)
find_library(REDIS_PP_LIB redis++_static REQUIRED)

set_target_properties(redis++::redis++ PROPERTIES IMPORTED_LOCATION "${REDIS_PP_LIB}")
target_include_directories(redis++::redis++ INTERFACE "${REDIS_PP_HEADER}")
target_link_libraries(redis++::redis++ INTERFACE hiredis::hiredis)

# <------------ fmt --------------->


find_package(fmt REQUIRED)

#target_link_libraries(RhythmGame_lib PRIVATE fmt::fmt)

# <------------- folly --------------->


find_package(folly REQUIRED)

#target_link_libraries(RhythmGame_lib PRIVATE Folly::folly)

# ---- Load libraries ----

add_subdirectory(rhythm_game)

# ---- Declare executable ----

add_executable(RhythmGame_exe rhythm_game/main.cpp)
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
)

target_compile_features(RhythmGame_exe PRIVATE cxx_std_20)

target_link_libraries(RhythmGame_exe PRIVATE DB)

# ---- Install rules ----

if (NOT CMAKE_SKIP_INSTALL_RULES)
    include(cmake/install-rules.cmake)
endif ()

# ---- Developer mode ----

if (NOT RhythmGame_DEVELOPER_MODE)
    return()
elseif (NOT PROJECT_IS_TOP_LEVEL)
    message(
            AUTHOR_WARNING
            "Developer mode is intended for developers of RhythmGame"
    )
endif ()

include(cmake/dev-mode.cmake)
