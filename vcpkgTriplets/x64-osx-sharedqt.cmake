set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_ARCHITECTURES x86_64)

if (PORT MATCHES "qt.*")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()

if (PORT MATCHES "openal-soft")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()

if (PORT MATCHES "glib")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()
