#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/QmlScriptFinderImpl.h"
#include "sounds/OpenAlSound.h"
#include "../RhythmGameQml/SceneSwitcher.h"

#include <QOpenGLWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QQuickView>
#include <QQmlEngine>
#include <QtQml/QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

auto
main(int argc, char* argv[]) -> int
{
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();
        auto scriptConfig = resource_managers::loadConfig(
          assetsFolder / "themes" / "Default" / "scripts" / "scripts.ini");
        auto qmlScriptFinder =
          resource_managers::QmlScriptFinderImpl{ assetsFolder / "themes" /
                                                    "Default" / "scripts",
                                                  scriptConfig["ScriptNames"] };

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        auto format = QSurfaceFormat::defaultFormat();
        format.setSwapInterval(0);
        QSurfaceFormat::setDefaultFormat(format);

        const auto app = QGuiApplication(argc, argv);

        auto engine = QQmlApplicationEngine{};
        qml_components::SceneSwitcher::setQmlScriptFinder(qmlScriptFinder);
        auto pathToStartQml = qmlScriptFinder("Main");

        auto view = QQuickView(&engine, nullptr);

        view.setSource(QUrl::fromLocalFile(pathToStartQml.string().c_str()));

        view.show();

        view.setWidth(800);
        view.setHeight(600);

        return app.exec();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }
}
