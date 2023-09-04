#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/models/ThemeConfig.h"
#include "resource_managers/IniImageProvider.h"
#include "sounds/OpenAlSound.h"
#include "../RhythmGameQml/SceneUrls.h"
#include "../RhythmGameQml/ProgramSettings.h"
#include "../RhythmGameQml/ChartLoader.h"

#include <QOpenGLWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QtQml/QQmlExtensionPlugin>
#include "sounds/OpenAlSoundBuffer.h"

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

auto
main(int argc, char* argv[]) -> int
{
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        auto format = QSurfaceFormat::defaultFormat();
        format.setSwapInterval(0);
        QSurfaceFormat::setDefaultFormat(format);

        const auto app = QGuiApplication(argc, argv);

        auto engine = QQmlApplicationEngine{};

        auto themeConfigLoader = [assetsFolder] {
            try {
                const auto configMap = resource_managers::loadConfig(
                  assetsFolder / "themes" / "Default" / "scripts" /
                  "scripts.ini")["ScriptNames"];
                const auto scriptsFolder = QUrl::fromLocalFile(
                  QString::fromStdString(assetsFolder / "themes" / "Default" /
                                         "scripts") +
                  '/');
                return resource_managers::models::ThemeConfig{
                    scriptsFolder.resolved(
                      QString::fromStdString(configMap.at("Main"))),
                    scriptsFolder.resolved(
                      QString::fromStdString(configMap.at("Gameplay"))),
                    scriptsFolder.resolved(
                      QString::fromStdString(configMap.at("SongWheel"))),
                };
            } catch (const std::exception& e) {
                spdlog::error("Failed to load theme config: {}", e.what());
                throw;
            }
        };

        auto sceneUrls = qml_components::SceneUrls{ themeConfigLoader };
        qml_components::SceneUrls::setInstance(&sceneUrls);

        auto chartPath = QUrl{};
        if (argc > 1) {
            chartPath = QUrl::fromLocalFile(argv[1]);
        }

        auto programSettings = qml_components::ProgramSettings{ chartPath };
        qml_components::ProgramSettings::setInstance(&programSettings);

        auto chartDataFactory = resource_managers::ChartDataFactory{};
        auto chartFactory =
          resource_managers::ChartFactory{ &chartDataFactory };
        auto chartLoader = qml_components::ChartLoader{ &chartFactory };
        qml_components::ChartLoader::setInstance(&chartLoader);

        auto iniImageProvider = resource_managers::IniImageProvider{};
        engine.addImageProvider("ini", &iniImageProvider);

        auto view = QQuickView(&engine, nullptr);
        view.setSource(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));

        view.setWidth(1920);
        view.setHeight(1080);

        view.show();

        return app.exec();
    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }
}
