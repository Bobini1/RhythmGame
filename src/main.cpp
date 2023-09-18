#include "resource_managers/FindAssetsFolderBoost.h"
#include "resource_managers/LoadConfig.h"
#include "resource_managers/models/ThemeConfig.h"
#include "resource_managers/IniImageProvider.h"
#include "sounds/OpenAlSound.h"
#include "gameplay_logic/rules/Lr2TimingWindows.h"
#include "../RhythmGameQml/SceneUrls.h"
#include "../RhythmGameQml/ProgramSettings.h"
#include "../RhythmGameQml/ChartLoader.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QtQml/QQmlExtensionPlugin>
#include <spdlog/sinks/qt_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "sounds/OpenAlSoundBuffer.h"
#include "../RhythmGameQml/Logger.h"
#include "gameplay_logic/rules/StandardBmsHitRules.h"
#include "gameplay_logic/rules/Lr2Gauge.h"
#include "gameplay_logic/rules/Lr2HitValues.h"
#include "resource_managers/SongDbScanner.h"

#include <iostream>

extern "C" {
#include <libavutil/log.h>
}

Q_IMPORT_QML_PLUGIN(RhythmGameQmlPlugin)

void
qtLogHandler(QtMsgType type,
             const QMessageLogContext& context,
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
libavLogHandler(void* ptr, int level, const char* fmt, va_list vl)
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

        auto engine = QQmlApplicationEngine{};

        av_log_set_callback(libavLogHandler);

        qInstallMessageHandler(qtLogHandler);

        auto log = qml_components::Logger{ nullptr };
        qml_components::Logger::setInstance(&log);

        auto logger = spdlog::qt_logger_mt("log", &log, "addLog");

        // combine with console logger
        logger->sinks().push_back(
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        // set global log level to debug
        // spdlog::set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);

        std::filesystem::remove(assetsFolder / "song_db.sqlite");
        auto db = db::SqliteCppDb{ (assetsFolder / "song_db.sqlite").string() };
        // create charts table if it doesn't exist
        db.execute("CREATE TABLE IF NOT EXISTS charts ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "title TEXT NOT NULL,"
                   "artist TEXT NOT NULL,"
                   "subtitle TEXT NOT NULL,"
                   "subartist TEXT NOT NULL,"
                   "genre TEXT NOT NULL,"
                   "rank INTEGER NOT NULL,"
                   "total REAL NOT NULL,"
                   "play_level INTEGER NOT NULL,"
                   "difficulty INTEGER NOT NULL,"
                   "is_random INTEGER NOT NULL,"
                   "note_count INTEGER NOT NULL,"
                   "length INTEGER NOT NULL,"
                   "path TEXT NOT NULL UNIQUE,"
                   "directory_in_db TEXT NOT NULL,"
                   "sha256 TEXT NOT NULL,"
                   "note_data BLOB NOT NULL"
                   ");");
        auto songDbScanner = resource_managers::SongDbScanner{ &db };
        auto start = std::chrono::high_resolution_clock::now();
        songDbScanner.scanDirectories(
          { { "/run/media/bobini/Elements/BMS/_spackage" } });
        auto end = std::chrono::high_resolution_clock::now();
        spdlog::info(
          "Database created in {} us",
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count());
        // query for charts table size
        auto query = db.createStatement("SELECT COUNT(*) FROM charts");
        auto count = query.executeAndGet<int>();
        spdlog::info("Found {} charts in database", *count);
        // time per chart
        spdlog::info(
          "Time per chart: {} us",
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
              .count() /
            *count);

        // get all unique directory_in_db
        query =
          db.createStatement("SELECT DISTINCT directory_in_db FROM charts");
        auto directories = query.executeAndGetAll<std::string>();
        // create a parent_dir table if it doesn't exist
        db.execute("CREATE TABLE IF NOT EXISTS parent_dir ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "parent_dir TEXT NOT NULL,"
                   "path TEXT NOT NULL UNIQUE"
                   ");");
        auto insertQuery = db.createStatement(
          "INSERT OR IGNORE INTO parent_dir (parent_dir, path) "
          "VALUES (:parent_dir, :path)");
        start = std::chrono::high_resolution_clock::now();
        for (auto directory : directories) {
            while (directory.size() != 1) {
                auto parentDirectory = directory;
                parentDirectory.resize(parentDirectory.size() - 1);
                auto lastSlashIndex = parentDirectory.find_last_of("/");
                parentDirectory.erase(lastSlashIndex + 1,
                                      parentDirectory.size() - lastSlashIndex -
                                        1);
                insertQuery.bind(":parent_dir", parentDirectory);
                insertQuery.bind(":path", directory);
                insertQuery.execute();
                insertQuery.reset();
                directory = std::move(parentDirectory);
            }
        }
        end = std::chrono::high_resolution_clock::now();
        spdlog::info(
          "Parent directories inserted in {} us",
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
            .count());

        // get all inserted charts
        start = std::chrono::high_resolution_clock::now();
        auto selectQuery = db.createStatement("SELECT * FROM charts");
        auto result =
          selectQuery.executeAndGetAll<gameplay_logic::ChartData::DTO>();
        for (const auto& chartDataDto : result) {
            auto* noteData = new gameplay_logic::BmsNotes{};
            auto buffer = QByteArray::fromStdString(chartDataDto.noteData);
            auto stream = QDataStream{ &buffer, QIODevice::ReadOnly };
            stream >> *noteData;
        }
        end = std::chrono::high_resolution_clock::now();
        spdlog::info(
          "Charts loaded in {} us",
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
              .count() /
            result.size());

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
        qml_components::SceneUrls::setInstance(&sceneUrls);

        auto chartPath = QString{};
        if (argc > 1) {
#if defined(WIN32)
            chartPath = QString::fromStdWString(argv[1]);
#else
            chartPath = QString::fromStdString(argv[1]);
#endif
        }

        auto programSettings = qml_components::ProgramSettings{ chartPath };
        qml_components::ProgramSettings::setInstance(&programSettings);

        using namespace std::chrono_literals;

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
        qml_components::ChartLoader::setInstance(&chartLoader);

        engine.addImageProvider("ini",
                                new resource_managers::IniImageProvider{});

        auto view = QQuickView(&engine, nullptr);
        view.setSource(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));

        view.setWidth(1920);
        view.setHeight(1080);

        view.show();

        return app.exec();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "Fatal error: unknown" << std::endl;
        throw;
    }
}
