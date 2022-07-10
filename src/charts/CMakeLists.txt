add_library(Charts OBJECT
        chart_readers/BmsChartReader.cpp models/Chart.h models/Chart.cpp models/ChartInfo.cpp models/ChartInfo.h models/MetadataTypes.h models/bms/BmsChart.cpp models/bms/BmsChart.h)
target_link_libraries(
        Charts PRIVATE taocpp::pegtl boost_icl type_safe::type_safe
)
target_compile_features(Charts PUBLIC cxx_std_20)
