add_library(DB OBJECT
        DB.cpp)
target_link_libraries(
        DB PRIVATE SQLiteCpp
)
target_compile_features(DB PUBLIC cxx_std_20)
