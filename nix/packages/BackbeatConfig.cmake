if(TARGET Backbeat::Backbeat)
    set(Backbeat_FOUND TRUE)
    return()
endif()

include(CMakeFindDependencyMacro)
find_dependency(zstd CONFIG)

get_filename_component(_backbeat_prefix "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(_backbeat_extra_libraries zstd::libzstd_shared)
include("${CMAKE_CURRENT_LIST_DIR}/BackbeatTargets.cmake")
