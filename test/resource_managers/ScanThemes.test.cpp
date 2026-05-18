#include "resource_managers/ScanThemes.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QFile>
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

    REQUIRE(themes.contains(QStringLiteral("Seven (play7.lr2skin)")));
    CHECK(themes[QStringLiteral("Seven (play7.lr2skin)")]
            .getScreens()
            .contains(QStringLiteral("k7")));
    CHECK(themes[QStringLiteral("Seven (play7.lr2skin)")]
            .getScreens()
            .contains(QStringLiteral("k5")));

    REQUIRE(
      themes.contains(QStringLiteral("Seven Battle (play7battle.lr2skin)")));
    CHECK(themes[QStringLiteral("Seven Battle (play7battle.lr2skin)")]
            .getScreens()
            .contains(QStringLiteral("k7battle")));
    CHECK(themes[QStringLiteral("Seven Battle (play7battle.lr2skin)")]
            .getScreens()
            .contains(QStringLiteral("k5battle")));

    CHECK_FALSE(themes.contains(QStringLiteral("Nine (play9.lr2skin)")));
    CHECK_FALSE(
      themes.contains(QStringLiteral("Nine Battle (play9battle.lr2skin)")));
    CHECK_FALSE(
      themes.contains(QStringLiteral("Twenty Four (play24.lr2skin)")));
    CHECK_FALSE(
      themes.contains(QStringLiteral("Forty Eight (play48.lr2skin)")));
    CHECK_FALSE(themes.contains(
      QStringLiteral("Twenty Four Battle (play24battle.lr2skin)")));
}
