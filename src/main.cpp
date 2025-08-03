#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/IniImageProvider.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "qml_components/ProgramSettings.h"
#include "qml_components/ChartLoader.h"
#ifdef _WIN32
#include <mimalloc-new-delete.h>
#endif
#include <QGuiApplication>
#include <QObject>
#include <QtQuick>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "qml_components/Logger.h"
#include "gameplay_logic/rules/StandardBmsHitRules.h"
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
#include "qml_components/FileQuery.h"
#include "qml_components/InputAttached.h"
#include "../RhythmGameQml/Rg.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "input/CustomNotifyApp.h"
#include "qml_components/QmlUtils.h"
#include "qml_components/Themes.h"
#include "resource_managers/GaugeFactory.h"
#include "resource_managers/Languages.h"
#include "resource_managers/ScanThemes.h"
#include "resource_managers/Tables.h"
#include "support/PathToUtfString.h"
#include "support/UtfStringToPath.h"
#include "gameplay_logic/CourseRunner.h"

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
    qInstallMessageHandler(qtLogHandler);

    struct UnregisterHandler
    {
        ~UnregisterHandler() { qInstallMessageHandler(nullptr); }
    } unregisterHandler;

    auto log = qml_components::Logger{ nullptr };
    qmlRegisterSingletonInstance("RhythmGameQml", 1, 0, "Logger", &log);

    auto logger = spdlog::qt_logger_mt("log", &log, "addLog");

    // combine with console logger
    logger->sinks().push_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    // set global log level to debug
    spdlog::set_level(spdlog::level::debug);
    set_default_logger(logger);

    auto app = input::CustomNotifyApp{ argc, argv };

    QGuiApplication::setOrganizationName("Tomasz Kalisiak");
    QGuiApplication::setOrganizationDomain("rhythmgame.eu");
    QGuiApplication::setApplicationName("RhythmGame");

    try {
        auto assetsFolder = resource_managers::findAssetsFolder();

        qputenv("QML_XHR_ALLOW_FILE_READ", QByteArray("1"));

        auto db = db::SqliteCppDb{ assetsFolder / "song_db.sqlite" };

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
        auto avatarPath = support::pathToQString(assetsFolder / "avatars/");
        if (!avatarPath.startsWith("/")) {
            avatarPath = "/" + avatarPath;
        }
        avatarPath = "file://" + avatarPath;

        auto programSettings =
          qml_components::ProgramSettings{ chartPath, avatarPath };

        qRegisterMetaType<input::Gamepad>("input::Gamepad");

        auto availableThemes =
          resource_managers::scanThemes(assetsFolder / "themes");
        if (availableThemes.empty()) {
            throw std::runtime_error("No themes available");
        }
        auto gamepadManager = input::GamepadManager{};

        auto themes = qml_components::Themes{ availableThemes };
        auto profileList =
          qml_components::ProfileList{ assetsFolder / "song_db.sqlite",
                                       &db,
                                       availableThemes,
                                       assetsFolder / "profiles",
                                       avatarPath };

        auto engine = QQmlApplicationEngine{};
        auto languages =
          resource_managers::Languages{ availableThemes, &engine };
        auto setLang = [&profileList,
                        &languages,
                        connection = QMetaObject::Connection{}]() mutable {
            languages.setSelectedLanguage(profileList.getMainProfile()
                                            ->getVars()
                                            ->getGeneralVars()
                                            ->getLanguage());
            connection = QObject::connect(
              profileList.getMainProfile()->getVars()->getGeneralVars(),
              &resource_managers::GeneralVars::languageChanged,
              &languages,
              [mainProfileVars =
                 profileList.getMainProfile()->getVars()->getGeneralVars(),
               &languages]() {
                  languages.setSelectedLanguage(mainProfileVars->getLanguage());
              });
        };
        QObject::connect(&profileList,
                         &qml_components::ProfileList::mainProfileChanged,
                         &languages,
                         setLang);
        setLang();

        auto inputTranslator = input::InputTranslator{ &db };
        QObject::connect(&gamepadManager,
                         &input::GamepadManager::axisMoved,
                         &inputTranslator,
                         &input::InputTranslator::handleAxis);
        QObject::connect(&gamepadManager,
                         &input::GamepadManager::buttonPressed,
                         &inputTranslator,
                         &input::InputTranslator::handlePress);
        QObject::connect(&gamepadManager,
                         &input::GamepadManager::buttonReleased,
                         &inputTranslator,
                         &input::InputTranslator::handleRelease);

        auto chartFactory = resource_managers::ChartFactory{ &inputTranslator };
        auto chartDataFactory = resource_managers::ChartDataFactory{};
        auto gaugeFactoryGeneral = resource_managers::GaugeFactory{};
        auto gaugeFactory =
          [gaugeFactoryGeneral](
            resource_managers::Profile* profile, double total, int noteCount) {
              return gaugeFactoryGeneral.getStandardGauges(
                profile, total, noteCount);
          };
        auto gaugeFactoryCourse =
          [gaugeFactoryGeneral](resource_managers::Profile* profile,
                                const QHash<QString, double>& initialValues) {
              return gaugeFactoryGeneral.getCourseGauges(profile,
                                                         initialValues);
          };
        auto getChartPathFromSha256 = [&db](const QString& md5,
                                            const std::filesystem::path& hint) {
            // Check if the hint path exists and matches the hash
            if (!hint.empty()) {
                auto hintStatement =
                  db.createStatement("SELECT md5 FROM charts WHERE path = ?;");
                hintStatement.bind(1, support::pathToUtfString(hint));
                if (const auto hintResult =
                      hintStatement.executeAndGet<std::string>()) {
                    if (*hintResult == md5.toStdString()) {
                        return std::optional{ hint };
                    }
                }
            }

            auto statement =
              db.createStatement("SELECT path FROM charts WHERE md5 = ?;");
            statement.bind(1, md5.toStdString());
            return statement.executeAndGet<std::string>().transform(
              support::utfStringToPath);
        };
        auto chartLoader = qml_components::ChartLoader{
            &profileList,
            &inputTranslator,
            &chartDataFactory,
            &gameplay_logic::rules::lr2_timing_windows::getTimingWindows,
            &gameplay_logic::rules::lr2_hit_values::getLr2HitValue,
            gaugeFactory,
            gaugeFactoryCourse,
            getChartPathFromSha256,
            &chartFactory,
            &db
        };

        auto scanningQueue =
          qml_components::ScanningQueue{ &db, songDbScanner };

        auto folders = qml_components::RootSongFolders{ &db, &scanningQueue };

        auto rootSongFoldersConfig =
          qml_components::RootSongFoldersConfig{ &folders, &scanningQueue };

        auto songFolderFactory = qml_components::SongFolderFactory{ &db };
        auto previewFilePathFetcher =
          qml_components::PreviewFilePathFetcher{ &db };

        auto fileQuery = qml_components::FileQuery{};

        auto networkManager = QNetworkAccessManager{};
        auto tables = resource_managers::Tables{ &networkManager,
                                                 assetsFolder / "tables",
                                                 &db };

        auto rg = Rg{ &programSettings,
                      &inputTranslator,
                      &chartLoader,
                      &rootSongFoldersConfig,
                      &songFolderFactory,
                      &previewFilePathFetcher,
                      &fileQuery,
                      &themes,
                      &gamepadManager,
                      &profileList,
                      &tables,
                      &languages };

        Rg::instance = &rg;

        // add all other common types
        qmlRegisterType<resource_managers::Level>(
          "RhythmGameQml", 1, 0, "level");
        qmlRegisterType<resource_managers::Course>(
          "RhythmGameQml", 1, 0, "course");
        qmlRegisterType<resource_managers::Trophy>(
          "RhythmGameQml", 1, 0, "trophy");
        qmlRegisterType<resource_managers::Table>(
          "RhythmGameQml", 1, 0, "table");
        qmlRegisterType<resource_managers::Entry>(
          "RhythmGameQml", 1, 0, "entry");
        qmlRegisterType<qml_components::ScoreQueryResult>(
          "RhythmGameQml", 1, 0, "scoreQueryResult");
        qmlRegisterType<qml_components::TableQueryResult>(
        "RhythmGameQml", 1, 0, "tableQueryResult");
        qmlRegisterType<resource_managers::TableInfo>(
          "RhythmGameQml", 1, 0, "tableInfo");
        qmlRegisterType<gameplay_logic::ChartRunner>(
          "RhythmGameQml", 1, 0, "ChartRunner");
        qmlRegisterType<gameplay_logic::CourseRunner>(
          "RhythmGameQml", 1, 0, "CourseRunner");
        qmlRegisterType<gameplay_logic::ChartData>(
          "RhythmGameQml", 1, 0, "ChartData");
        qmlRegisterType<resource_managers::Profile>(
          "RhythmGameQml", 1, 0, "Profile");
        qmlRegisterType<gameplay_logic::Player>(
          "RhythmGameQml", 1, 0, "Player");
        qmlRegisterUncreatableType<gameplay_logic::rules::BmsGauge>(
          "RhythmGameQml", 1, 0, "BmsGauge", "BmsGauge is abstract");
        qmlRegisterType<gameplay_logic::BmsLiveScore>(
          "RhythmGameQml", 1, 0, "BmsLiveScore");
        qmlRegisterType<gameplay_logic::BmsNotes>(
          "RhythmGameQml", 1, 0, "BmsNotes");
        qmlRegisterType<resource_managers::Profile>(
          "RhythmGameQml", 1, 0, "BmsProfile");
        qmlRegisterType<gameplay_logic::BmsScore>(
          "RhythmGameQml", 1, 0, "BmsScore");
        qmlRegisterType<gameplay_logic::BmsScoreCourse>(
          "RhythmGameQml", 1, 0, "BmsScoreCourse");
        qmlRegisterType<gameplay_logic::BmsResultCourse>(
          "RhythmGameQml", 1, 0, "BmsResultCourse");
        qmlRegisterType<gameplay_logic::BmsPoints>(
          "RhythmGameQml", 1, 0, "bmsPoints");
        qmlRegisterType<gameplay_logic::BmsResult>(
          "RhythmGameQml", 1, 0, "BmsResult");
        qmlRegisterType<gameplay_logic::BmsReplayData>(
          "RhythmGameQml", 1, 0, "BmsReplayData");
        qmlRegisterType<gameplay_logic::BmsGaugeHistory>(
          "RhythmGameQml", 1, 0, "BmsGaugeHistory");
        qmlRegisterType<gameplay_logic::HitEvent>(
          "RhythmGameQml", 1, 0, "hitEvent");
        qmlRegisterType<qml_components::Bga>("RhythmGameQml", 1, 0, "Bga");
        qmlRegisterType<qml_components::BgaContainer>(
          "RhythmGameQml", 1, 0, "BgaContainer");
        qmlRegisterType<input::Key>("RhythmGameQml", 1, 0, "key");
        qmlRegisterType<input::Gamepad>("RhythmGameQml", 1, 0, "gamepad");
        qmlRegisterType<input::AnalogAxisConfig>(
          "RhythmGameQml", 1, 0, "analogAxisConfig");
        qmlRegisterUncreatableMetaObject(
          gameplay_logic::judgement::staticMetaObject,
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
        qmlRegisterUncreatableMetaObject(
          resource_managers::dp_options::staticMetaObject,
          "RhythmGameQml",
          1,
          0,
          "DpOptions",
          "Access to enums & flags only");
        qmlRegisterUncreatableMetaObject(
          resource_managers::hi_speed_fix::staticMetaObject,
          "RhythmGameQml",
          1,
          0,
          "HiSpeedFix",
          "Access to enums & flags only");
        qmlRegisterUncreatableMetaObject(
          resource_managers::gauge_mode::staticMetaObject,
          "RhythmGameQml",
          1,
          0,
          "GaugeMode",
          "Access to enums & flags only");
        qmlRegisterUncreatableMetaObject(
          resource_managers::note_order_algorithm::staticMetaObject,
          "RhythmGameQml",
          1,
          0,
          "NoteOrderAlgorithm",
          "Access to enums & flags only");
        qmlRegisterUncreatableType<gameplay_logic::Note>(
          "RhythmGameQml", 1, 0, "note", "Note is created in C++");
        qmlRegisterUncreatableType<qml_components::InputAttached>(
          "RhythmGameQml",
          1,
          0,
          "Input",
          "Input is only accessible as an attached property");

        auto inputSignalProvider =
          qml_components::InputSignalProvider{ &inputTranslator };
        qml_components::InputAttached::inputSignalProvider =
          &inputSignalProvider;
        qml_components::QmlUtilsAttached::getThemeNameForRootFile =
          [&availableThemes](const QUrl& rootFile) {
              for (const auto& [themeName, family] :
                   availableThemes.asKeyValueRange()) {
                  for (const auto& screen : family.getScreens()) {
                      if (screen.getScript() == rootFile) {
                          return themeName;
                      }
                  }
              }
              return QString{};
          };
        qmlRegisterUncreatableType<qml_components::QmlUtilsAttached>(
          "RhythmGameQml",
          1,
          0,
          "QmlUtils",
          "QmlUtils is only accessible as an attached property");

        engine.addImageProvider("ini",
                                new resource_managers::IniImageProvider{});

        engine.load(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));
        if (engine.rootObjects().isEmpty()) {
            throw std::runtime_error{ "Failed to load main qml" };
        }
        app.setInputTranslator(&inputTranslator);

        return app.exec();
    } catch (const std::exception& e) {
        spdlog::critical("Fatal: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("Fatal: Unknown exception");
        return 1;
    }
}