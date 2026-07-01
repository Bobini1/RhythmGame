qt_add_plugin(tga STATIC
        CLASS_NAME TgaPlugin
        TgaPlugin.cpp
        TgaPlugin.h
        TgaHandler.cpp
        TgaHandler.h
)
find_package(SDL2 REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
target_link_libraries(tga PUBLIC Qt6::Qml Qt6::Network Qt6::Gui SDL2::SDL2 $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
