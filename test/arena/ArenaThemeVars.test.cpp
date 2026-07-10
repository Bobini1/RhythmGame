#include "db/SqliteCppDb.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "resource_managers/Profile.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QQmlPropertyMap>
#include <QTemporaryDir>
#include <QThread>

#include <filesystem>
#include <memory>
#include <optional>

namespace {

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "ArenaThemeVarsTests";
    static char* argv[]{ applicationName, nullptr };
    static const auto application =
      std::make_unique<QCoreApplication>(argc, argv);
}

auto
themeFamilies(const std::filesystem::path& root)
  -> QMap<QString, qml_components::ThemeFamily>
{
    const auto primary =
      qml_components::Screen{ QUrl("qrc:/dummy.qml"), {}, {}, {}, false };
    const auto alias =
      qml_components::Screen{ QUrl("qrc:/dummy.qml"), {}, {}, {}, true };
    auto aliasedScreens = QMap<QString, qml_components::Screen>{
        { QStringLiteral("k7"), primary },
        { QStringLiteral("k5"), alias },
        { QStringLiteral("k14"), primary },
        { QStringLiteral("k10"), alias },
        { QStringLiteral("result"), primary },
        { QStringLiteral("select"), primary },
    };
    auto independentScreens = QMap<QString, qml_components::Screen>{
        { QStringLiteral("k7"), primary },
        { QStringLiteral("k5"), primary },
    };
    return {
        { QStringLiteral("Arena Test"),
          qml_components::ThemeFamily{
            support::pathToQString(root), std::move(aliasedScreens), {} } },
        { QStringLiteral("Independent"),
          qml_components::ThemeFamily{
            support::pathToQString(root), std::move(independentScreens), {} } },
    };
}

auto
screenThemeVars(resource_managers::Profile* profile,
                const QString& screen,
                const QString& themeName = QStringLiteral("Arena Test"))
  -> QQmlPropertyMap*
{
    auto* screens = profile->getVars()->getThemeVars();
    auto* screenVars = screens->value(screen).value<QQmlPropertyMap*>();
    if (screenVars == nullptr) {
        return nullptr;
    }
    return screenVars->value(themeName).value<QQmlPropertyMap*>();
}

auto
persistedX(const QString& path) -> std::optional<double>
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::nullopt;
    }
    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return std::nullopt;
    }
    const auto value = document.object()
                         .value(QStringLiteral("k7"))
                         .toObject()
                         .value(QStringLiteral("arenaOverlayK7XNormalized"));
    return value.isDouble() ? std::optional{ value.toDouble() } : std::nullopt;
}

} // namespace

TEST_CASE("Arena overlay geometry uses profile theme vars",
          "[arena][ArenaThemeVars]")
{
    ensureCoreApplication();
    QTemporaryDir directory;
    REQUIRE(directory.isValid());
    const auto root = support::qStringToPath(directory.path());
    const auto songDbPath = root / "songs.sqlite";
    auto songDb = db::SqliteCppDb(songDbPath);
    songDb.execute("CREATE TABLE properties (key TEXT PRIMARY KEY, value)");
    auto network = QNetworkAccessManager{};
    const auto families = themeFamilies(root);
    REQUIRE(families.size() == 2);
    REQUIRE(families.value(QStringLiteral("Arena Test"))
              .getScreens()
              .contains(QStringLiteral("k7")));
    auto profiles =
      std::make_unique<qml_components::ProfileList>(songDbPath,
                                                    &songDb,
                                                    families,
                                                    root / "profiles",
                                                    QList<QString>{},
                                                    &network);

    auto* profileA = profiles->getMainProfile();
    REQUIRE(profileA != nullptr);
    CHECK(profileA->getVars()->getGeneralVars()->getArenaOverlayHintVersion() ==
          0);
    profileA->getVars()->getGeneralVars()->setArenaOverlayHintVersion(1);
    auto* gameplay = screenThemeVars(profileA, QStringLiteral("k7"));
    auto* gameplayAlias = screenThemeVars(profileA, QStringLiteral("k5"));
    auto* doublePlay = screenThemeVars(profileA, QStringLiteral("k14"));
    auto* doublePlayAlias = screenThemeVars(profileA, QStringLiteral("k10"));
    auto* result = screenThemeVars(profileA, QStringLiteral("result"));
    REQUIRE(gameplay != nullptr);
    REQUIRE(gameplayAlias != nullptr);
    REQUIRE(doublePlay != nullptr);
    REQUIRE(doublePlayAlias != nullptr);
    REQUIRE(result != nullptr);
    CHECK(gameplayAlias == gameplay);
    CHECK(doublePlayAlias == doublePlay);
    CHECK(screenThemeVars(profileA, QStringLiteral("select")) == nullptr);

    for (const auto& variant : { QStringLiteral("K5"), QStringLiteral("K7") }) {
        CHECK(gameplay->value(QStringLiteral("arenaOverlay") + variant +
                              QStringLiteral("XNormalized")) == -1.0);
        CHECK(gameplay->value(QStringLiteral("arenaOverlay") + variant +
                              QStringLiteral("YNormalized")) == -1.0);
        CHECK(gameplay->value(QStringLiteral("arenaOverlay") + variant +
                              QStringLiteral("WidthNormalized")) == -1.0);
        CHECK(gameplay->value(QStringLiteral("arenaOverlay") + variant +
                              QStringLiteral("HeightNormalized")) == -1.0);
    }
    for (const auto& variant :
         { QStringLiteral("K10"), QStringLiteral("K14") }) {
        CHECK(doublePlay->value(QStringLiteral("arenaOverlay") + variant +
                                QStringLiteral("XNormalized")) == -1.0);
        CHECK(doublePlay->value(QStringLiteral("arenaOverlay") + variant +
                                QStringLiteral("YNormalized")) == -1.0);
        CHECK(doublePlay->value(QStringLiteral("arenaOverlay") + variant +
                                QStringLiteral("WidthNormalized")) == -1.0);
        CHECK(doublePlay->value(QStringLiteral("arenaOverlay") + variant +
                                QStringLiteral("HeightNormalized")) == -1.0);
    }

    auto* independentK7 = screenThemeVars(
      profileA, QStringLiteral("k7"), QStringLiteral("Independent"));
    auto* independentK5 = screenThemeVars(
      profileA, QStringLiteral("k5"), QStringLiteral("Independent"));
    REQUIRE(independentK7 != nullptr);
    REQUIRE(independentK5 != nullptr);
    CHECK(independentK7 != independentK5);
    CHECK(independentK7->contains(QStringLiteral("arenaOverlayK7XNormalized")));
    CHECK_FALSE(
      independentK7->contains(QStringLiteral("arenaOverlayK5XNormalized")));
    CHECK(independentK5->contains(QStringLiteral("arenaOverlayK5XNormalized")));
    CHECK_FALSE(
      independentK5->contains(QStringLiteral("arenaOverlayK7XNormalized")));
    CHECK(result->value(QStringLiteral("arenaOverlayResultXNormalized")) ==
          -1.0);
    CHECK(result->value(QStringLiteral("arenaOverlayResultYNormalized")) ==
          -1.0);
    CHECK(result->value(QStringLiteral("arenaOverlayResultWidthNormalized")) ==
          -1.0);
    CHECK(result->value(QStringLiteral("arenaOverlayResultHeightNormalized")) ==
          -1.0);

    REQUIRE(gameplay->setProperty("arenaOverlayK7XNormalized", 0.25));
    REQUIRE(gameplay->setProperty("arenaOverlayK7YNormalized", 0.05));
    REQUIRE(gameplay->setProperty("arenaOverlayK7WidthNormalized", 0.40));
    REQUIRE(gameplay->setProperty("arenaOverlayK7HeightNormalized", 0.50));

    const auto varsPath = support::pathToQString(
      profileA->getPath().parent_path() / "Arena Test-vars.json");
    QElapsedTimer timeout;
    timeout.start();
    while (persistedX(varsPath) != std::optional{ 0.25 } &&
           timeout.elapsed() < 2'000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(1);
    }
    REQUIRE(persistedX(varsPath) == std::optional{ 0.25 });

    QFile file(varsPath);
    REQUIRE(file.open(QIODevice::ReadOnly));
    const auto bytes = file.readAll();
    CHECK_FALSE(bytes.contains("room-sentinel"));
    CHECK_FALSE(bytes.contains("member-sentinel"));
    CHECK_FALSE(bytes.contains("chat-sentinel"));
    CHECK_FALSE(bytes.contains("token-sentinel"));
    file.close();

    auto* profileB = profiles->createProfile();
    REQUIRE(profileB != nullptr);
    profiles->setMainProfile(profileB);
    CHECK(profileB->getVars()->getGeneralVars()->getArenaOverlayHintVersion() ==
          0);
    auto* profileBGameplay = screenThemeVars(profileB, QStringLiteral("k7"));
    REQUIRE(profileBGameplay != nullptr);
    CHECK(profileBGameplay->value(
            QStringLiteral("arenaOverlayK7XNormalized")) == -1.0);

    profiles->setMainProfile(profileA);
    CHECK(profileA->getVars()->getGeneralVars()->getArenaOverlayHintVersion() ==
          1);
    CHECK(screenThemeVars(profileA, QStringLiteral("k7"))
            ->value(QStringLiteral("arenaOverlayK7XNormalized")) == 0.25);

    const auto profileAPath = profileA->getPath();
    profiles.reset();
    auto reloaded = resource_managers::Profile{
        songDbPath, profileAPath, families, {}, &network
    };
    CHECK(screenThemeVars(&reloaded, QStringLiteral("k7"))
            ->value(QStringLiteral("arenaOverlayK7XNormalized")) == 0.25);
    CHECK(reloaded.getVars()->getGeneralVars()->getArenaOverlayHintVersion() ==
          1);
}
