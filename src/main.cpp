#include "resource_managers/TextureLoaderImpl.h"

#include "resource_managers/FontLoaderImpl.h"
#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/QmlScriptFinderImpl.h"
#include "sounds/OpenAlSound.h"
#include "resource_managers/SoundLoaderImpl.h"

#include <QWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QScreen>
#include <QTimer>

auto
main(int argc, char* argv[]) -> int
{
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();
        auto textureConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "textures" / "textures.ini");
        auto fontConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "fonts" / "fonts.ini");
        auto soundConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "sounds" / "sounds.ini");
        auto scriptConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "scripts" / "scripts.ini");
        auto fontManager =
          resource_managers::FontLoaderImpl{ assetsFolder / "themes" /
                                               "Default" / "fonts",
                                             fontConfig["FontNames"] };
        auto textureManager =
          resource_managers::TextureLoaderImpl{ assetsFolder / "themes" /
                                                  "Default" / "textures",
                                                textureConfig["TextureNames"] };
        auto soundManager =
          resource_managers::SoundLoaderImpl{ assetsFolder / "themes" /
                                                "Default" / "sounds",
                                              soundConfig["SoundNames"] };
        auto luaScriptFinder =
          resource_managers::QmlScriptFinderImpl{ assetsFolder / "themes" /
                                                    "Default" / "scripts",
                                                  scriptConfig["ScriptNames"] };

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSwapInterval(0);
        QSurfaceFormat::setDefaultFormat(format);

        QGuiApplication app(argc, argv);

        QQmlApplicationEngine engine;

        auto pathToStartQml = luaScriptFinder("HelloWorld.qml");

        // load the main qml file
        engine.load(QUrl::fromLocalFile(pathToStartQml.string().c_str()));
        if (engine.rootObjects().isEmpty())
            return -1;

        // set width and height of the window
        auto window = qobject_cast<QWindow*>(engine.rootObjects().first());
        window->setWidth(800);
        window->setHeight(600);

        return app.exec();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }

    return 0;
}
