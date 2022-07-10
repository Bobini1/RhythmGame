add_library(ChartReading OBJECT
        ChartReaders/ChartReader.cpp)
target_include_directories(
        ChartReading ${warning_guard}
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(
        ChartReading PRIVATE taocpp::pegtl Folly::folly
)
target_compile_features(ChartReading PUBLIC cxx_std_20)
