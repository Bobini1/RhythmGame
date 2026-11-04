include("${CMAKE_CURRENT_LIST_DIR}/x64-linux-sharedqt.cmake")
list(APPEND VCPKG_HASH_ADDITIONAL_FILES "${CMAKE_CURRENT_LIST_DIR}/x64-linux-sharedqt.cmake")

# Linux CI uses Release builds. Avoid compiling and storing unused Debug Qt libraries.
set(VCPKG_BUILD_TYPE release)
