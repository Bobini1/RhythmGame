set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

if (PORT MATCHES "qt.*")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()

if (PORT MATCHES "icu")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()

if (PORT MATCHES "ffmpeg")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()
