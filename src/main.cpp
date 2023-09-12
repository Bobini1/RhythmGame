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
main(int argc, char* argv[]) -> int
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

#ifdef DEBUG
        // combine with console logger
        logger->sinks().push_back(
          std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

        // set global log level to debug
        spdlog::set_level(spdlog::level::debug);
#endif

        spdlog::set_default_logger(logger);

        auto themeConfigLoader = [assetsFolder] {
            try {
                const auto configMap = resource_managers::loadConfig(
                  assetsFolder / "themes" / "Default" / "scripts" /
                  "scripts.ini")["ScriptNames"];
                const auto scriptsFolder = QUrl::fromLocalFile(
                  QString::fromStdString((assetsFolder / "themes" / "Default" /
                                         "scripts").string()) +
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

        auto sceneUrls =
          qml_components::SceneUrls{ std::move(themeConfigLoader) };
        qml_components::SceneUrls::setInstance(&sceneUrls);

        auto chartPath = QUrl{};
        if (argc > 1) {
            chartPath = QUrl::fromLocalFile(argv[1]);
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
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::error("Fatal error: unknown");
        return 1;
    }
}
