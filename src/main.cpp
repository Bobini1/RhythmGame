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
#include "qml_components/SongFolderFactory.h"
#include "support/PathToQString.h"
#include "qml_components/ProfileList.h"
#include "qml_components/InputItem.h"
#include "qml_components/PreviewFilePathFetcher.h"
#include "qml_components/ScoreDb.h"
#include "qml_components/FileValidator.h"
#include "qml_components/CycleModel.h"
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
    auto loc = msg.toUtf8();

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
        case AV_LOG_PANIC:
            spdlog::critical("{}", message);
            break;
        default:
            spdlog::info("{}", message);
            break;
    }
}

auto
main(int argc, [[maybe_unused]] char* argv[]) -> int
{
    try {
        auto assetsFolder = resource_managers::findAssetsFolder();

#if defined(Q_OS_WIN)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

        const auto app = QGuiApplication{ __argc, __argv };
#else
        const auto app = QGuiApplication{ argc, argv };
#endif

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

        auto db = db::SqliteCppDb{ support::pathToQString(
                                     (assetsFolder / "song_db.sqlite"))
                                     .toStdString() };

        defineDb(db);

        auto songDbScanner = resource_managers::SongDbScanner{ &db };

        auto themeConfigLoader = [assetsFolder] {
            try {
                const auto configMap = resource_managers::loadConfig(
                  assetsFolder / "themes" / "Default" /
                  "theme.ini")["ScriptNames"];
                const auto scriptsFolder = assetsFolder / "themes" / "Default";
                return resource_managers::models::ThemeConfig{
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Main")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Gameplay")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("SongWheel")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Settings")).wstring()),
                    QString::fromStdWString(
                      (scriptsFolder / configMap.at("Result")).wstring())
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
            chartPath = QString::fromStdWString(__wargv[1]);
#else
            chartPath = QString::fromStdString(argv[1]);
#endif
        }

        auto programSettings = qml_components::ProgramSettings{ chartPath };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ProgramSettings", &programSettings);

        auto profileList =
          qml_components::ProfileList{ &db, assetsFolder / "profiles" };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ProfileList", &profileList);

        auto scoreDb = [&profileList]() -> db::SqliteCppDb& {
            return profileList.getCurrentProfile()->getDb();
        };

        auto chartFactory = resource_managers::ChartFactory{ scoreDb };
        auto hitRulesFactory =
          [](gameplay_logic::rules::TimingWindows timingWindows,
             std::function<double(std::chrono::nanoseconds)> hitValuesFactory) {
              return std::make_unique<
                gameplay_logic::rules::StandardBmsHitRules>(
                std::move(timingWindows), std::move(hitValuesFactory));
          };
        auto chartDataFactory = resource_managers::ChartDataFactory{};
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

        auto songFolderFactory = qml_components::SongFolderFactory{ &db };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "SongFolderFactory", &songFolderFactory);

        auto previewFilePathFetcher =
          qml_components::PreviewFilePathFetcher{ &db };
        qmlRegisterSingletonInstance("RhythmGameQml",
                                     1,
                                     0,
                                     "PreviewFilePathFetcher",
                                     &previewFilePathFetcher);

        auto scoreDbSingleton = qml_components::ScoreDb{ scoreDb };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ScoreDb", &scoreDbSingleton);

        auto fileValidator = qml_components::FileValidator{};
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "FileValidator", &fileValidator);

        // add all other common types

        qmlRegisterType<gameplay_logic::Chart>("RhythmGameQml", 1, 0, "Chart");
        qmlRegisterType<gameplay_logic::ChartData>(
          "RhythmGameQml", 1, 0, "ChartData");
        qmlRegisterType<gameplay_logic::rules::BmsGauge>(
          "RhythmGameQml", 1, 0, "BmsGauge");
        qmlRegisterType<gameplay_logic::BmsScore>(
          "RhythmGameQml", 1, 0, "BmsScore");
        qmlRegisterType<gameplay_logic::BmsNotes>(
          "RhythmGameQml", 1, 0, "BmsNotes");
        qmlRegisterType<qml_components::Folder>(
          "RhythmGameQml", 1, 0, "Folder");
        qmlRegisterType<resource_managers::Profile>(
          "RhythmGameQml", 1, 0, "BmsProfile");
        qmlRegisterType<gameplay_logic::BmsScoreAftermath>(
          "RhythmGameQml", 1, 0, "BmsScoreAftermath");
        qmlRegisterType<gameplay_logic::BmsResult>(
          "RhythmGameQml", 1, 0, "BmsResult");
        qmlRegisterType<gameplay_logic::BmsReplayData>(
          "RhythmGameQml", 1, 0, "BmsReplayData");
        qmlRegisterType<gameplay_logic::BmsGaugeHistory>(
          "RhythmGameQml", 1, 0, "BmsGaugeHistory");
        qmlRegisterType<qml_components::InputItem>(
          "RhythmGameQml", 1, 0, "InputItem");
        qmlRegisterType<qml_components::CycleModel>(
          "RhythmGameQml", 1, 0, "CycleModel");
        qmlRegisterType<qml_components::Bga>("RhythmGameQml", 1, 0, "Bga");
        qmlRegisterType<qml_components::BgaContainer>(
          "RhythmGameQml", 1, 0, "BgaContainer");
        qmlRegisterUncreatableMetaObject(gameplay_logic::staticMetaObject,
                                         "RhythmGameQml",
                                         1,
                                         0,
                                         "Judgement",
                                         "Access to enums & flags only");

        engine.addImageProvider("ini",
                                new resource_managers::IniImageProvider{});

        engine.load(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));
        if (engine.rootObjects().isEmpty()) {
            throw std::runtime_error{ "Failed to load main qml" };
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
