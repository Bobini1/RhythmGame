qt_add_plugin(dds STATIC
        CLASS_NAME DdsPlugin
        DdsPlugin.cpp
        DdsPlugin.h
        DdsHandler.cpp
        DdsHandler.h
)
find_package(OpenImageIO CONFIG REQUIRED)
target_link_libraries(dds PRIVATE Qt6::Gui OpenImageIO::OpenImageIO)
