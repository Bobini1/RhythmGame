#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/models/ThemeConfig.h"
#include "resource_managers/IniImageProvider.h"
#include "sounds/OpenAlSound.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "qml_components/SceneUrls.h"
#include "qml_components/ProgramSettings.h"
#include "qml_components/ChartLoader.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QtQml/QQmlExtensionPlugin>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "sounds/OpenAlSoundBuffer.h"
#include "qml_components/Logger.h"
#include "gameplay_logic/rules/StandardBmsHitRules.h"
#include "gameplay_logic/rules/Lr2Gauge.h"
#include "gameplay_logic/rules/Lr2HitValues.h"
#include "resource_managers/SongDbScanner.h"
#include "DefineDb.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/RootSongFolder.h"

#include <iostream>

extern "C" {
#include <libavutil/log.h>
}

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

void
qtLogHandler(QtMsgType type,
             const QMessageLogContext& /*context*/,
             const QString& msg)
{
    QByteArray loc = msg.toUtf8();

    switch (type) {
        case QtDebugMsg:
            spdlog::debug("{}", loc.constData());
            break;
        case QtInfoMsg:
            spdlog::info("{}", loc.constData());
            break;
        case QtWarningMsg:
            spdlog::warn("{}", loc.constData());
            break;
        case QtCriticalMsg:
            spdlog::critical("{}", loc.constData());
            break;
        case QtFatalMsg:
            spdlog::critical("{}", loc.constData());
            break;
    }
}

void
libavLogHandler(void* /*ptr*/, int level, const char* fmt, va_list vl)
{
    if (level > av_log_get_level()) {
        return;
    }

    static char message[8192];
    auto ret = vsnprintf(message, sizeof(message), fmt, vl);
    if (ret < 0) {
        return;
    }
    switch (level) {
        case AV_LOG_DEBUG:
            spdlog::debug("{}", message);
            break;
        case AV_LOG_VERBOSE:
            spdlog::info("{}", message);
            break;
        case AV_LOG_INFO:
            spdlog::info("{}", message);
            break;
        case AV_LOG_WARNING:
            spdlog::warn("{}", message);
            break;
        case AV_LOG_ERROR:
            spdlog::error("{}", message);
            break;
        case AV_LOG_FATAL:
            spdlog::critical("{}", message);
            break;
        case AV_LOG_PANIC:
            spdlog::critical("{}", message);
            break;
        default:
            spdlog::info("{}", message);
            break;
    }
}

auto
#if defined(WIN32)
wmain(int argc, wchar_t* argv[]) -> int
#else
main(int argc, char* argv[]) -> int
#endif
{
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

        const auto app = QGuiApplication(argc, argv);

        QGuiApplication::setOrganizationName("Tomasz Kalisiak");
        QGuiApplication::setOrganizationDomain("bemani.pl");
        QGuiApplication::setApplicationName("RhythmGame");

        auto engine = QQmlApplicationEngine{};

        av_log_set_callback(libavLogHandler);

        qInstallMessageHandler(qtLogHandler);

        auto log = qml_components::Logger{ nullptr };
        qmlRegisterSingletonInstance("RhythmGameQml", 1, 0, "Logger", &log);

        auto logger = spdlog::qt_logger_mt("log", &log, "addLog");

        // combine with console logger
        logger->sinks().push_back(
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        // set global log level to debug
        // spdlog::set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);

        auto db = db::SqliteCppDb{ (assetsFolder / "song_db.sqlite").string() };

        defineDb(db);

        auto songDbScanner = resource_managers::SongDbScanner{ &db };

        auto themeConfigLoader = [assetsFolder] {
            try {
                const auto configMap = resource_managers::loadConfig(
                  assetsFolder / "themes" / "Default" / "scripts" /
                  "scripts.ini")["ScriptNames"];
                const auto scriptsFolder =
                  assetsFolder / "themes" / "Default" / "scripts";
                return resource_managers::models::ThemeConfig{
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Main")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Gameplay")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("SongWheel")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Settings")).wstring())
                };
            } catch (const std::exception& e) {
                spdlog::error("Failed to load theme config: {}", e.what());
                throw;
            }
        };

        auto sceneUrls =
          qml_components::SceneUrls{ std::move(themeConfigLoader) };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "SceneUrls", &sceneUrls);

        auto chartPath = QString{};
        if (argc > 1) {
#if defined(WIN32)
            chartPath = QString::fromStdWString(argv[1]);
#else
            chartPath = QString::fromStdString(argv[1]);
#endif
        }

        auto programSettings =
          qml_components::ProgramSettings{ QUrl::fromLocalFile(chartPath) };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ProgramSettings", &programSettings);

        auto chartDataFactory = resource_managers::ChartDataFactory{};
        auto chartFactory = resource_managers::ChartFactory{};
        auto hitRulesFactory =
          [](gameplay_logic::rules::TimingWindows timingWindows,
             std::function<double(std::chrono::nanoseconds)> hitValuesFactory) {
              return std::make_unique<
                gameplay_logic::rules::StandardBmsHitRules>(
                std::move(timingWindows), std::move(hitValuesFactory));
          };
        auto chartLoader = qml_components::ChartLoader{
            &chartDataFactory,
            &gameplay_logic::rules::lr2_timing_windows::getTimingWindows,
            std::move(hitRulesFactory),
            &gameplay_logic::rules::lr2_hit_values::getLr2HitValue,
            &gameplay_logic::rules::Lr2Gauge::getGauges,
            &chartFactory,
            2.0
        };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ChartLoader", &chartLoader);

        auto rootSongFoldersConfig =
          qml_components::RootSongFoldersConfig{ &db, songDbScanner };
        qmlRegisterSingletonInstance("RhythmGameQml",
                                     1,
                                     0,
                                     "RootSongFoldersConfig",
                                     &rootSongFoldersConfig);

        auto rootSongFolder = qml_components::RootSongFolder{ &db };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "RootSongFolder", &rootSongFolder);

        engine.addImageProvider("ini",
                                new resource_managers::IniImageProvider{});

        engine.load(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));
        if (engine.rootObjects().isEmpty()) {
            return -1;
        }

        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Fatal error: unknown" << std::endl;
        throw;
    }
}
