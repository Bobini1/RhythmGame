#include "resource_managers/ScanThemes.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonArray>
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
writeLr2SkinBytes(const std::filesystem::path& path, const QByteArray& data)
{
    std::filesystem::create_directories(path.parent_path());

    QFile file(support::pathToQString(path));
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(data) == data.size());
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
    writeLr2Skin(
      skinRoot / "play7battle.lr2skin", 12, QStringLiteral("Seven Battle"));
    writeLr2Skin(skinRoot / "play9.lr2skin", 4, QStringLiteral("Nine"));
    writeLr2Skin(
      skinRoot / "play9battle.lr2skin", 14, QStringLiteral("Nine Battle"));
    writeLr2Skin(
      skinRoot / "play24.lr2skin", 16, QStringLiteral("Twenty Four"));
    writeLr2Skin(
      skinRoot / "play48.lr2skin", 17, QStringLiteral("Forty Eight"));
    writeLr2Skin(skinRoot / "play24battle.lr2skin",
                 18,
                 QStringLiteral("Twenty Four Battle"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    REQUIRE(themes.contains(QStringLiteral("Seven (play7.lr2skin)")));
    CHECK(themes[QStringLiteral("Seven (play7.lr2skin)")].getScreens().contains(
      QStringLiteral("k7")));
    CHECK(themes[QStringLiteral("Seven (play7.lr2skin)")].getScreens().contains(
      QStringLiteral("k5")));

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

TEST_CASE("LR2 skin scanner reads no-BOM headers as CP932",
          "[themes][lr2][encoding]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "Cp932Skin";

    writeLr2SkinBytes(
      skinRoot / "play7.lr2skin",
      QByteArray("#INFORMATION,0,\x87\x40,Tester\n"
                 "#CUSTOMOPTION,\x87\x40(Custom),900,\x82\xa0(Default),"
                 "\x82\xa2(Alt)\n"
                 "#ENDOFHEADER\n"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    const auto familyName =
      QString(QChar(0x2460)) + QStringLiteral(" (play7.lr2skin)");
    REQUIRE(themes.contains(familyName));

    const auto screens = themes[familyName].getScreens();
    REQUIRE(screens.contains(QStringLiteral("k7")));

    const auto settingsData = screens[QStringLiteral("k7")].getSettingsData();
    const auto settings =
      QJsonDocument::fromJson(settingsData.toUtf8()).object();
    const auto items = settings[QStringLiteral("items")].toArray();
    REQUIRE(items.size() == 1);

    const auto item = items[0].toObject();
    CHECK(item[QStringLiteral("name")]
            .toObject()[QStringLiteral("en")]
            .toString() == QString(QChar(0x2460)) + QStringLiteral("(Custom)"));

    const auto choices = item[QStringLiteral("choices")].toArray();
    REQUIRE(choices.size() == 2);
    CHECK(choices[0]
            .toObject()[QStringLiteral("name")]
            .toObject()[QStringLiteral("en")]
            .toString() ==
          QString(QChar(0x3042)) + QStringLiteral("(Default)"));
    CHECK(choices[1]
            .toObject()[QStringLiteral("name")]
            .toObject()[QStringLiteral("en")]
            .toString() == QString(QChar(0x3044)) + QStringLiteral("(Alt)"));
}

TEST_CASE("LR2 skin scanner falls back to first custom file when default stem "
          "is missing",
          "[themes][lr2][settings]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "MissingDefaultSkin";

    writeLr2SkinBytes(skinRoot / "cover" / "Default.png",
                      QByteArray("image", 5));
    writeLr2SkinBytes(skinRoot / "play7.lr2skin",
                      QByteArray("#INFORMATION,0,Missing Default,Tester\n"
                                 "#CUSTOMFILE,Cover,cover/*.png,Random\n"
                                 "#ENDOFHEADER\n"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    const auto familyName = QStringLiteral("Missing Default (play7.lr2skin)");
    REQUIRE(themes.contains(familyName));

    const auto settingsData =
      themes[familyName].getScreens()[QStringLiteral("k7")].getSettingsData();
    const auto settings =
      QJsonDocument::fromJson(settingsData.toUtf8()).object();
    const auto items = settings[QStringLiteral("items")].toArray();
    REQUIRE(items.size() == 1);

    const auto item = items[0].toObject();
    CHECK(item[QStringLiteral("type")].toString() == QStringLiteral("file"));
    CHECK(item[QStringLiteral("default")].toString() ==
          QStringLiteral("Default.png"));
}

TEST_CASE("LR2 skin scanner keeps custom option and file ids distinct",
          "[themes][lr2][settings]")
{
    QTemporaryDir tempDir;
    const auto themesRoot = makeThemesRoot(tempDir);
    const auto skinRoot = themesRoot / "CollidingIdsSkin";

    writeLr2SkinBytes(skinRoot / "cover" / "LR2.png", QByteArray("image", 5));
    writeLr2SkinBytes(
      skinRoot / "play7.lr2skin",
      QByteArray("#INFORMATION,0,Colliding IDs,Tester\n"
                 "#CUSTOMOPTION,SUDDEN+ Lane(SUDDEN+ Lane),970,Default,"
                 "Customize\n"
                 "#CUSTOMFILE,SUDDEN Lane(SUDDEN Lane),cover/*.png,LR2\n"
                 "#ENDOFHEADER\n"));

    const auto themes = resource_managers::scanThemes(themesRoot);

    const auto familyName = QStringLiteral("Colliding IDs (play7.lr2skin)");
    REQUIRE(themes.contains(familyName));

    const auto settingsData =
      themes[familyName].getScreens()[QStringLiteral("k7")].getSettingsData();
    const auto settings =
      QJsonDocument::fromJson(settingsData.toUtf8()).object();
    const auto items = settings[QStringLiteral("items")].toArray();
    REQUIRE(items.size() == 2);

    const auto optionItem = items[0].toObject();
    CHECK(optionItem[QStringLiteral("type")].toString() ==
          QStringLiteral("choice"));
    CHECK(optionItem[QStringLiteral("id")].toString() ==
          QStringLiteral("opt_970"));
    CHECK(optionItem[QStringLiteral("default")].toString() ==
          QStringLiteral("Default"));

    const auto fileItem = items[1].toObject();
    CHECK(fileItem[QStringLiteral("type")].toString() ==
          QStringLiteral("file"));
    CHECK(fileItem[QStringLiteral("id")].toString() ==
          QStringLiteral("sudden_lane_sudden_lane"));
    CHECK(fileItem[QStringLiteral("default")].toString() ==
          QStringLiteral("LR2.png"));
}
