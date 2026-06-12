qt_add_plugin(cim STATIC
        CLASS_NAME CimPlugin
        CimPlugin.cpp
        CimPlugin.h
        CimHandler.cpp
        CimHandler.h
)
target_link_libraries(cim PUBLIC Qt6::Gui zlib-ng::zlib)
