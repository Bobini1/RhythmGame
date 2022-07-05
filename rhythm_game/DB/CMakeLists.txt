add_library(DB OBJECT
        source/DB.cpp)
target_include_directories(
        DB ${warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(
        DB PRIVATE redis++::redis++
)
target_compile_features(DB PUBLIC cxx_std_20)