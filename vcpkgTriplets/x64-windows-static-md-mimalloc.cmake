set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

if (PORT MATCHES "spdlog")
    set(SPDLOG_WCHAR_FILENAMES ON)
endif ()

if (PORT MATCHES "gstreamer")
    set(VCPKG_LIBRARY_LINKAGE dynamic)
endif ()
