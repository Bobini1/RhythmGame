add_library(Charts OBJECT
        chart_readers/bms/BmsChartReader.cpp models/Chart.h models/Chart.cpp models/ChartInfo.cpp models/ChartInfo.h models/MetadataTypes.h models/bms/BmsChart.cpp models/bms/BmsChart.h ../../test/BmsChartReader.test.cpp)
target_link_libraries(
        Charts PRIVATE taocpp::pegtl Boost::boost type_safe::type_safe
)
target_compile_features(Charts PUBLIC cxx_std_20)