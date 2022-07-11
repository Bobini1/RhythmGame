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

# <------------ SQLiteCpp (db) --------------->

find_package(SQLiteCpp REQUIRED)

# <------------ pegtl (parsing) --------------->

find_package(pegtl REQUIRED)

# <------------ fmt --------------->

find_package(fmt REQUIRED)

#target_link_libraries(RhythmGame_lib PRIVATE fmt::fmt)

# <------------- folly --------------->

find_package(folly REQUIRED)

# <------------ boost --------------->

find_package(Boost REQUIRED)

# <------------ type_safe --------------->


find_package(type_safe REQUIRED)


# ---- Load libraries ----

add_subdirectory(src)

# ---- Declare executable ----

add_executable(RhythmGame_exe src/main.cpp)
add_executable(RhythmGame::exe ALIAS RhythmGame_exe)

set_target_properties(
        RhythmGame_exe PROPERTIES
        OUTPUT_NAME RhythmGame
        EXPORT_NAME exe
)

target_compile_features(RhythmGame_exe PRIVATE cxx_std_20)

target_link_libraries(RhythmGame_exe PRIVATE DB Charts)

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
