#include "resource_managers/ScanThemes.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QString>

#include <filesystem>

namespace {

void
writeLr2Skin(const std::filesystem::path& path,
             const int type,
             const QString& title)
{
    std::filesystem::create_directories(path.parent_path());

    QFile file(support::pathToQString(path));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QString("#INFORMATION,%1,%2,Tester\n#ENDOFHEADER\n")
                 .arg(type)
                 .arg(title)
                 .toUtf8());
}

void
writeBeatorajaCsvSkin(const std::filesystem::path& path,
                      const int type,
                      const QString& title)
{
    std::filesystem::create_directories(path.parent_path());

    QFile file(support::pathToQString(path));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QString("#RESOLUTION,2\n#INFORMATION,%1,%2,Tester\n#ENDOFHEADER\n")
                 .arg(type)
                 .arg(title)
                 .toUtf8());
}

void
writeLuaSkin(const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path() / "bg");

    QFile image(support::pathToQString(path.parent_path() / "bg" / "one.png"));
    REQUIRE(image.open(QIODevice::WriteOnly));

    QFile file(support::pathToQString(path));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(QStringLiteral(R"lua(
local header = {
    type = 5,
    name = "Lua Select",
    author = "Tester",
    w = 1920,
    h = 1080,
    property = {
        { name = "Mode", item = {
            { name = "A", op = 900 },
            { name = "B", op = 901 },
        }, def = "B" }
    },
    filepath = {
        { name = "Background", path = "bg/*.png", def = "one.png" }
    }
}
if skin_config then
    return header
end
return header
)lua")
                 .toUtf8());
}

auto
makeThemesRoot(QTemporaryDir& tempDir) -> std::filesystem::path
{
    REQUIRE(tempDir.isValid());
    return support::qStringToPath(tempDir.path());
}

} // namespace

TEST_CASE("LR2 skin scanner skips unsupported beatoraja keymodes",
          "[themes][lr2]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "MixedSkin";

    writeLr2Skin(skinRoot / "play7.lr2skin", 0, QStringLiteral("Seven"));
    writeLr2Skin(skinRoot / "play7battle.lr2skin",
                 12,
                 QStringLiteral("Seven Battle"));
    writeLr2Skin(skinRoot / "play9.lr2skin", 4, QStringLiteral("Nine"));
    writeLr2Skin(skinRoot / "play9battle.lr2skin",
                 14,
                 QStringLiteral("Nine Battle"));
    writeLr2Skin(skinRoot / "play24.lr2skin",
                 16,
                 QStringLiteral("Twenty Four"));
    writeLr2Skin(skinRoot / "play48.lr2skin",
                 17,
                 QStringLiteral("Forty Eight"));
    writeLr2Skin(skinRoot / "play24battle.lr2skin",
                 18,
                 QStringLiteral("Twenty Four Battle"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    REQUIRE(themes.contains(QStringLiteral("Seven")));
    CHECK(themes[QStringLiteral("Seven")]
            .getScreens()
            .contains(QStringLiteral("k7")));
    CHECK(themes[QStringLiteral("Seven")]
            .getScreens()
            .contains(QStringLiteral("k5")));

    REQUIRE(themes.contains(QStringLiteral("Seven Battle")));
    CHECK(themes[QStringLiteral("Seven Battle")]
            .getScreens()
            .contains(QStringLiteral("k7battle")));
    CHECK(themes[QStringLiteral("Seven Battle")]
            .getScreens()
            .contains(QStringLiteral("k5battle")));

    CHECK_FALSE(themes.contains(QStringLiteral("Nine")));
    CHECK_FALSE(themes.contains(QStringLiteral("Nine Battle")));
    CHECK_FALSE(themes.contains(QStringLiteral("Twenty Four")));
    CHECK_FALSE(themes.contains(QStringLiteral("Forty Eight")));
    CHECK_FALSE(themes.contains(QStringLiteral("Twenty Four Battle")));
}

TEST_CASE("Theme scanner discovers beatoraja Lua skins",
          "[themes][lr2][lua]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "LuaSkin";
    const auto skinPath = skinRoot / "select.luaskin";
    writeLuaSkin(skinPath);

    const auto themes = resource_managers::scanThemes(themesRoot);

    REQUIRE(themes.contains(QStringLiteral("Lua Select")));
    const auto family = themes[QStringLiteral("Lua Select")];
    REQUIRE(family.getScreens().contains(QStringLiteral("select")));

    const auto screen = family.getScreens().value(QStringLiteral("select"));
    CHECK(screen.getCsvPath().endsWith(QStringLiteral("select.luaskin")));

    const auto settings =
      QJsonDocument::fromJson(screen.getSettingsData().toUtf8()).object();
    CHECK(settings["format"].toString() == QStringLiteral("beatoraja"));
    CHECK(settings["sourceFormat"].toString() == QStringLiteral("lua"));
    CHECK(settings["type"].toInt() == 5);
    CHECK(settings["title"].toString() == QStringLiteral("Lua Select"));
}

TEST_CASE("Theme scanner marks beatoraja CSV skins by source format",
          "[themes][lr2]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "CsvSkin";
    writeBeatorajaCsvSkin(skinRoot / "select.lr2skin",
                          5,
                          QStringLiteral("CSV Select"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    REQUIRE(themes.contains(QStringLiteral("CSV Select")));
    const auto screen =
      themes[QStringLiteral("CSV Select")].getScreens().value(QStringLiteral("select"));
    const auto settings =
      QJsonDocument::fromJson(screen.getSettingsData().toUtf8()).object();
    CHECK(settings["format"].toString() == QStringLiteral("beatoraja"));
    CHECK(settings["sourceFormat"].toString() == QStringLiteral("csv"));
}
