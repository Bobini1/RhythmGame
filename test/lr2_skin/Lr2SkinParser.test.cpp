#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QTemporaryDir>
#include <QString>

#include <filesystem>

using gameplay_logic::lr2_skin::Lr2SkinParser;

namespace {

void
writeSkinFile(const std::filesystem::path& path, const QString& text)
{
    QFile file(support::pathToQString(path));
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(text.toUtf8());
}

auto
tempSkinPath(QTemporaryDir& tempDir) -> std::filesystem::path
{
    REQUIRE(tempDir.isValid());
    return support::qStringToPath(tempDir.path()) / "skin.lr2skin";
}

auto
fullScreenSprite(const int time = 0) -> QString
{
    return QStringLiteral(
             "#SRC_IMAGE,0,0,0,0,640,480,1,1,0,0,0,0,0\n"
             "#DST_IMAGE,%1,0,0,0,320,320,0,255,255,255,255,1,0,0,0\n")
      .arg(time);
}

} // namespace

TEST_CASE("LR2 skin parser infers repeated square canvases", "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n") +
                    fullScreenSprite(0) +
                    fullScreenSprite(100) +
                    fullScreenSprite(200));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    CHECK(skin.skinWidth == 320);
    CHECK(skin.skinHeight == 320);
}

TEST_CASE("LR2 skin parser keeps fallback for isolated small destinations",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path, QStringLiteral("#IMAGE,full.png\n") + fullScreenSprite());

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    CHECK(skin.skinWidth == 640);
    CHECK(skin.skinHeight == 480);
}

TEST_CASE("LR2 skin parser honors explicit resolution over inferred canvas",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#RESOLUTION,1\n#IMAGE,full.png\n") +
                    fullScreenSprite(0) +
                    fullScreenSprite(100) +
                    fullScreenSprite(200));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    CHECK(skin.skinWidth == 1280);
    CHECK(skin.skinHeight == 720);
}
