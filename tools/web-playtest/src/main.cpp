#include "WebPlaytestChartInstaller.h"
#include "WebPlaytestInputDigest.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
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

    QQmlApplicationEngine engine;
    engine.setInitialProperties(QVariantMap{
      { QStringLiteral("installedChartPath"), installedChartPath },
      { QStringLiteral("initializationError"), initializationError },
      { QStringLiteral("buildInputSha256"), buildInputSha256 },
    });
    engine.loadFromModule("RhythmGame.WebPlaytest", "Main");
    if (engine.rootObjects().isEmpty()) {
        return EXIT_FAILURE;
    }

    if (!initializationError.isEmpty()) {
        qCritical().noquote() << initializationError;
        QTimer::singleShot(
          0, &application, [&application] { application.exit(EXIT_FAILURE); });
    }
    return application.exec();
}
