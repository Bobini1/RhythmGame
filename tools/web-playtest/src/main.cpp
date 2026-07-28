#include "WebPlaytestChartInstaller.h"
#include "WebPlaytestInputDigest.h"
#include "web_playtest/WebPlaytestRuntime.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QVariantMap>

#include <cstdlib>

int
main(int argc, char* argv[])
{
    QGuiApplication application{ argc, argv };

    QString initializationError;
    const auto installedChartPath =
      web_playtest::WebPlaytestChartInstaller::install(initializationError);
    const auto buildInput = web_playtest::buildInputSha256();
    const auto buildInputSha256 = QString::fromLatin1(
      buildInput.data(), static_cast<qsizetype>(buildInput.size()));
    application.setProperty("rgWebPlaytestBuildInputSha256", buildInputSha256);

    auto* runtime = web_playtest::WebPlaytestRuntime::createProcessLifetime(
      installedChartPath, initializationError);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("webPlaytest"),
                                             runtime);
    engine.setInitialProperties(QVariantMap{
      { QStringLiteral("buildInputSha256"), buildInputSha256 },
    });
    engine.loadFromModule("RhythmGame.WebPlaytest", "Main");
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    return application.exec();
}
