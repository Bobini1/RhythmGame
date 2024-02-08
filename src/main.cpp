#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/IniImageProvider.h"
#include "sounds/OpenAlSound.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "qml_components/ProgramSettings.h"
#include "qml_components/ChartLoader.h"

#include <QQmlExtensionPlugin>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QObject>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "qml_components/Logger.h"
#include "gameplay_logic/rules/StandardBmsHitRules.h"
#include "gameplay_logic/rules/Lr2Gauge.h"
#include "gameplay_logic/rules/Lr2HitValues.h"
#include "resource_managers/SongDbScanner.h"
#include "resource_managers/DefineDb.h"
#include "input/GamepadManager.h"
#include "input/InputTranslator.h"
#include "qml_components/RootSongFoldersConfig.h"
#include "qml_components/SongFolderFactory.h"
#include "support/PathToQString.h"
#include "qml_components/ProfileList.h"
#include "qml_components/PreviewFilePathFetcher.h"
#include "qml_components/ScoreDb.h"
#include "qml_components/FileValidator.h"
#include "qml_components/Themes.h"
#include "resource_managers/ScanThemes.h"

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

auto
main(int argc, [[maybe_unused]] char* argv[]) -> int
{
    const auto app = QGuiApplication{ argc, argv };

    QGuiApplication::setOrganizationName("Tomasz Kalisiak");
    QGuiApplication::setOrganizationDomain("bemani.pl");
    QGuiApplication::setApplicationName("RhythmGame");

    qInstallMessageHandler(qtLogHandler);

    auto log = qml_components::Logger{ nullptr };
    qmlRegisterSingletonInstance("RhythmGameQml", 1, 0, "Logger", &log);

    auto logger = spdlog::qt_logger_mt("log", &log, "addLog");

    // combine with console logger
    logger->sinks().push_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    // set global log level to debug
    spdlog::set_level(spdlog::level::debug);
    set_default_logger(logger);

    try {
        auto assetsFolder = resource_managers::findAssetsFolder();

        auto engine = QQmlApplicationEngine{};

        auto db = db::SqliteCppDb{ support::pathToQString(
                                     (assetsFolder / "song_db.sqlite"))
                                     .toStdString() };

        resource_managers::defineDb(db);

        auto songDbScanner = resource_managers::SongDbScanner{ &db };

        auto chartPath = QString{};
        if (argc > 1) {
#if defined(WIN32)
            chartPath = QString::fromStdWString(__wargv[1]);
#else
            chartPath = QString::fromStdString(argv[1]);
#endif
        }

        auto programSettings = qml_components::ProgramSettings{
            chartPath, support::pathToQString(assetsFolder / "avatars/")
        };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ProgramSettings", &programSettings);

        qRegisterMetaType<input::Gamepad>("input::Gamepad");

        auto availableThemes =
          resource_managers::scanThemes(assetsFolder / "themes");

        auto themes = qml_components::Themes{ availableThemes };
        qmlRegisterSingletonInstance("RhythmGameQml", 1, 0, "Themes", &themes);
        auto profileList = qml_components::ProfileList{
            &db, availableThemes, assetsFolder / "profiles"
        };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "ProfileList", &profileList);
        auto scoreDb = [&profileList]() -> db::SqliteCppDb& {
            return profileList.getCurrentProfile()->getDb();
        };

        auto gamepadManager = input::GamepadManager{};
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "GamepadManager", &gamepadManager);
        auto inputTranslator = input::InputTranslator{ &gamepadManager };
        qmlRegisterSingletonInstance(
          "RhythmGameQml", 1, 0, "InputTranslator", &inputTranslator);
        QObject::connect(&inputTranslator,
                         &input::InputTranslator::keyConfigModified,
                         [&inputTranslator, &profileList] {
                             profileList.getCurrentProfile()->setKeyConfig(
                               inputTranslator.getKeyConfig());
                         });
        auto saveConfig = [&inputTranslator, &profileList] {
            inputTranslator.setKeyConfig(
              profileList.getCurrentProfile()->getKeyConfig());
        };
        saveConfig();
        QObject::connect(&profileList,
                         &qml_components::ProfileList::currentProfileChanged,
                         saveConfig);

        auto chartFactory =
          resource_managers::ChartFactory{ scoreDb, &inputTranslator };
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
        qmlRegisterUncreatableType<gameplay_logic::rules::BmsGauge>(
          "RhythmGameQml", 1, 0, "BmsGauge", "BmsGauge is abstract");
        qmlRegisterType<gameplay_logic::BmsScore>(
          "RhythmGameQml", 1, 0, "BmsScore");
        qmlRegisterType<gameplay_logic::BmsNotes>(
          "RhythmGameQml", 1, 0, "BmsNotes");
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
        qmlRegisterType<qml_components::Bga>("RhythmGameQml", 1, 0, "Bga");
        qmlRegisterType<qml_components::BgaContainer>(
          "RhythmGameQml", 1, 0, "BgaContainer");
        qmlRegisterType<input::Key>("RhythmGameQml", 1, 0, "Key");
        qmlRegisterUncreatableMetaObject(gameplay_logic::staticMetaObject,
                                         "RhythmGameQml",
                                         1,
                                         0,
                                         "Judgement",
                                         "Access to enums & flags only");
        qmlRegisterUncreatableMetaObject(input::staticMetaObject,
                                         "RhythmGameQml",
                                         1,
                                         0,
                                         "BmsKey",
                                         "Access to enums & flags only");
        qmlRegisterUncreatableType<gameplay_logic::Note>(
          "RhythmGameQml", 1, 0, "Note", "Note is created in C++");

        engine.addImageProvider("ini",
                                new resource_managers::IniImageProvider{});

        engine.load(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));
        if (engine.rootObjects().isEmpty()) {
            throw std::runtime_error{ "Failed to load main qml" };
        }
        engine.rootObjects()[0]->installEventFilter(&inputTranslator);

        return app.exec();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        throw;
    } catch (...) {
        spdlog::critical("Fatal error: unknown exception");
        throw;
    }
}
