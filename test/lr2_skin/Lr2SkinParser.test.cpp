#include "gameplay_logic/lr2_skin/BeatorajaLuaSkin.h"
#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QTemporaryDir>

#include <algorithm>
#include <filesystem>

using gameplay_logic::lr2_skin::Lr2Dst;
using gameplay_logic::lr2_skin::Lr2SrcBarImage;
using gameplay_logic::lr2_skin::Lr2SrcBarNumber;
using gameplay_logic::lr2_skin::Lr2SrcBarText;
using gameplay_logic::lr2_skin::Lr2SrcImage;
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

TEST_CASE("LR2 skin parser accepts fractional destination coordinates",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n"
                                 "#SRC_NUMBER,0,0,0,120,730,120,10,1,0,0,46,2,2\n"
                                 "#DST_NUMBER,0,260,248.5,340.9,73.5,120.2,0,255,255,255,255,1,0,0,0\n"));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    REQUIRE(skin.elements.size() == 1);
    REQUIRE(skin.elements.first().dsts.size() == 1);

    const auto dst = skin.elements.first().dsts.first().value<Lr2Dst>();
    CHECK(dst.x == 248);
    CHECK(dst.y == 340);
    CHECK(dst.w == 73);
    CHECK(dst.h == 120);
}

TEST_CASE("Beatoraja Lua skin settings resolve numeric option defaults",
          "[lr2][skin][lua]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());
    const auto skinPath =
      support::qStringToPath(tempDir.path()) / "select.luaskin";

    writeSkinFile(
      skinPath,
      QStringLiteral(R"lua(
return {
    type = 5,
    name = "Numeric Default",
    author = "Tester",
    property = {
        { name = "Label Position", item = {
            { name = "Bottom", op = 955 },
            { name = "Top", op = 956 },
        }, def = 956 }
    }
}
)lua"));

    const auto header =
      gameplay_logic::lr2_skin::loadBeatorajaLuaSkinHeader(skinPath);
    REQUIRE(header.valid);

    const auto settings =
      QJsonDocument::fromJson(header.settingsData.toUtf8()).object();
    const auto items = settings["items"].toArray();
    REQUIRE(items.size() == 1);

    const auto item = items.first().toObject();
    CHECK(item["default"].toString() == QStringLiteral("Top"));
}

TEST_CASE("Beatoraja Lua skin parser executes Lua skin and maps static objects",
          "[lr2][skin][lua]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());
    const auto root = support::qStringToPath(tempDir.path());
    const auto skinPath = root / "select.luaskin";
    const auto imagePath = root / "bg" / "one.png";
    std::filesystem::create_directories(imagePath.parent_path());
    writeSkinFile(imagePath, QString{});

    writeSkinFile(
      skinPath,
      QStringLiteral(R"lua(
local header = {
    type = 5,
    name = "Lua Select",
    author = "Tester",
    w = 1920,
    h = 1080,
    fadeout = 500,
    input = 100,
    scene = 2000,
    property = {
        { name = "Mode", item = {
            { name = "A", op = 900 },
            { name = "B", op = 901 },
        }, def = "B" }
    },
    filepath = {
        { name = "Background", path = "bg/*.png", def = "one" }
    }
}

local function main()
    return {
        type = header.type,
        name = header.name,
        author = header.author,
        w = header.w,
        h = header.h,
        fadeout = header.fadeout,
        input = header.input,
        scene = header.scene,
        property = header.property,
        filepath = header.filepath,
        source = {
            { id = "src", path = "bg/*.png" }
        },
        image = {
            { id = "sprite", src = "src", x = 1, y = 2, w = 64, h = 32, divx = 2, divy = 1 },
            { id = "playButtonDummy", src = "src", x = 0, y = 0, w = 1, h = 1, act = 15 },
            { id = "reverseButtonDummy", src = "src", x = 0, y = 0, w = 1, h = 1, act = 42, click = 1 },
            { id = "bar-song", src = "src", x = 0, y = 0, w = 100, h = 20 },
            { id = "bar-folder", src = "src", x = 0, y = 20, w = 100, h = 20 },
            { id = "bar-table", src = "src", x = 0, y = 40, w = 100, h = 20 },
        },
        imageset = {
            { id = "bar", images = { "bar-song", "bar-folder", "bar-table" } }
        },
        text = {
            { id = "bar-title", font = 0, size = 24, align = 2 }
        },
        value = {
            { id = "bar-level", src = "src", x = 0, y = 60, w = 10, h = 10, divx = 10, divy = 1, ref = 101, digit = 2 }
        },
        songlist = {
            id = "songlist",
            center = 1,
            clickable = { 1 },
            listoff = {
                { id = "bar", dst = { { x = 100, y = 200, w = 300, h = 40 } } },
                { id = "bar", dst = { { x = 120, y = 160, w = 300, h = 40 } } },
            },
            liston = {
                { id = "bar", dst = { { x = 100, y = 200, w = 320, h = 40 } } },
                { id = "bar", dst = { { x = 120, y = 160, w = 320, h = 40 } } },
            },
            text = {
                { id = "bar-title", dst = { { x = 5, y = 6, w = 70, h = 20 } } },
            },
            level = {
                { id = "bar-level", dst = { { x = 7, y = 8, w = 11, h = 12 } } },
            },
            lamp = {
                { id = "bar-song", dst = { { x = 9, y = 10, w = 13, h = 14 } } },
            },
        },
        destination = {
            { id = "sprite", dst = {
                { x = 0, y = 0, w = 1920, h = 1080 }
            } },
            { id = "songlist" },
            { id = "sprite", op = { skin_config.option["Mode"] }, dst = {
                { x = 10, y = 20, w = 30, h = 40 },
                { time = 100, a = 128 },
                { time = 200, h = 80 }
            } },
            { id = -100, dst = {
                { x = 300, y = 400, w = 500, h = 600 }
            } },
            { id = "playButtonDummy", dst = {
                { x = 700, y = 800, w = 90, h = 40 }
            } },
            { id = "reverseButtonDummy", dst = {
                { x = 800, y = 800, w = 90, h = 40 }
            } }
        }
    }
end

if skin_config then
    return main()
end
return header
)lua"));

    QVariantMap settings;
    settings["mode"] = "A";
    settings["background"] = "one";

    const auto skin =
      Lr2SkinParser::parseData(support::pathToQString(skinPath), settings, {});

    CHECK(skin.skinWidth == 1920);
    CHECK(skin.skinHeight == 1080);
    CHECK(skin.fadeOut == 500);
    CHECK(skin.startInput == 100);
    CHECK(skin.sceneTime == 2000);
    CHECK(skin.activeOptions.contains(900));
    CHECK(skin.usedOptions.contains(900));
    REQUIRE(skin.barRows.size() == 2);
    CHECK(skin.barCenter == 1);
    CHECK(skin.barAvailableStart == 1);
    CHECK(skin.barAvailableEnd == 1);

    auto spriteIt = std::find_if(skin.elements.cbegin(),
                                 skin.elements.cend(),
                                 [](const auto& element) {
                                     return element.type == 0;
                                 });
    REQUIRE(spriteIt != skin.elements.cend());
    const auto element = *spriteIt;
    CHECK(element.type == 0);
    const auto src = element.src.value<Lr2SrcImage>();
    CHECK(src.x == 1);
    CHECK(src.y == 2);
    CHECK(src.w == 64);
    CHECK(src.h == 32);
    CHECK(src.div_x == 2);
    CHECK(src.source.endsWith(QStringLiteral("bg/one.png")));

    REQUIRE(element.dsts.size() == 1);
    const auto firstDst = element.dsts.first().value<Lr2Dst>();
    CHECK(firstDst.x == 0);
    CHECK(firstDst.y == 0);
    CHECK(firstDst.w == 1920);
    CHECK(firstDst.h == 1080);

    auto animatedSpriteIt = std::find_if(skin.elements.cbegin(),
                                         skin.elements.cend(),
                                         [](const auto& candidate) {
                                             if (candidate.type != 0 ||
                                                 candidate.dsts.size() != 3) {
                                                 return false;
                                             }
                                             const auto dst =
                                               candidate.dsts.first()
                                                 .value<Lr2Dst>();
                                             return dst.x == 10 && dst.y == 1020;
                                         });
    REQUIRE(animatedSpriteIt != skin.elements.cend());
    const auto secondDst = animatedSpriteIt->dsts[0].value<Lr2Dst>();
    CHECK(secondDst.x == 10);
    CHECK(secondDst.y == 1020);
    CHECK(secondDst.w == 30);
    CHECK(secondDst.h == 40);
    CHECK(secondDst.op1 == 900);

    const auto thirdDst = animatedSpriteIt->dsts[1].value<Lr2Dst>();
    CHECK(thirdDst.time == 100);
    CHECK(thirdDst.x == 10);
    CHECK(thirdDst.y == 1020);
    CHECK(thirdDst.a == 128);

    const auto fourthDst = animatedSpriteIt->dsts[2].value<Lr2Dst>();
    CHECK(fourthDst.time == 200);
    CHECK(fourthDst.x == 10);
    CHECK(fourthDst.y == 980);
    CHECK(fourthDst.h == 80);

    auto stageFileIt = std::find_if(skin.elements.cbegin(),
                                    skin.elements.cend(),
                                    [](const auto& candidate) {
                                        if (candidate.type != 0) {
                                            return false;
                                        }
                                        const auto source =
                                      candidate.src.template value<Lr2SrcImage>();
                                        return source.specialType ==
                                          Lr2SrcImage::StageFile;
                                    });
    REQUIRE(stageFileIt != skin.elements.cend());
    const auto stageFileSource = stageFileIt->src.value<Lr2SrcImage>();
    CHECK(stageFileSource.x == -1);
    CHECK(stageFileSource.y == -1);
    CHECK(stageFileSource.w == -1);
    CHECK(stageFileSource.h == -1);
    const auto stageFileDst = stageFileIt->dsts.first().value<Lr2Dst>();
    CHECK(stageFileDst.x == 300);
    CHECK(stageFileDst.y == 80);
    CHECK(stageFileDst.w == 500);
    CHECK(stageFileDst.h == 600);

    auto playButtonIt = std::find_if(skin.elements.cbegin(),
                                     skin.elements.cend(),
                                     [](const auto& candidate) {
                                         if (candidate.type != 0) {
                                             return false;
                                         }
                                         const auto source =
                                       candidate.src.template value<Lr2SrcImage>();
                                         return source.button &&
                                           source.buttonId == 15;
                                     });
    REQUIRE(playButtonIt != skin.elements.cend());
    const auto playButtonSource =
      playButtonIt->src.value<Lr2SrcImage>();
    CHECK(playButtonSource.buttonClickEnabled);
    CHECK(playButtonSource.buttonClickMode == 0);

    auto reverseButtonIt = std::find_if(skin.elements.cbegin(),
                                        skin.elements.cend(),
                                        [](const auto& candidate) {
                                            if (candidate.type != 0) {
                                                return false;
                                            }
                                            const auto source =
                                          candidate.src.template value<Lr2SrcImage>();
                                            return source.button &&
                                              source.buttonId == 42;
                                        });
    REQUIRE(reverseButtonIt != skin.elements.cend());
    const auto reverseButtonSource =
      reverseButtonIt->src.value<Lr2SrcImage>();
    CHECK(reverseButtonSource.buttonClickEnabled);
    CHECK(reverseButtonSource.buttonClickMode == 1);

    auto barIt = std::find_if(skin.elements.cbegin(),
                              skin.elements.cend(),
                              [](const auto& element) {
                                  return element.type == 3;
                              });
    REQUIRE(barIt != skin.elements.cend());
    const auto barDst = barIt->dsts.first().value<Lr2Dst>();
    CHECK(barDst.sortId > firstDst.sortId);
    CHECK(barDst.sortId < secondDst.sortId);
    CHECK(barDst.x == 100);
    CHECK(barDst.y == 840);
    CHECK(barDst.w == 300);
    CHECK(barDst.h == 40);
    const auto barSource = barIt->src.value<Lr2SrcBarImage>();
    CHECK(barSource.sources.size() == 3);
    CHECK(barSource.sources[0].value<Lr2SrcImage>().y == 0);
    CHECK(barSource.sources[1].value<Lr2SrcImage>().y == 20);
    CHECK(barSource.sources[2].value<Lr2SrcImage>().y == 40);

    auto barTextIt = std::find_if(skin.elements.cbegin(),
                                  skin.elements.cend(),
                                  [](const auto& candidate) {
                                      return candidate.type == 4;
                                  });
    REQUIRE(barTextIt != skin.elements.cend());
    const auto textSource = barTextIt->src.value<Lr2SrcBarText>();
    CHECK(textSource.titleType == 0);
    const auto textDst = barTextIt->dsts.first().value<Lr2Dst>();
    CHECK(textDst.x == 5);
    CHECK(textDst.y == 14);
    CHECK(textDst.w == 70);
    CHECK(textDst.h == 20);

    auto barNumberIt = std::find_if(skin.elements.cbegin(),
                                    skin.elements.cend(),
                                    [](const auto& candidate) {
                                        return candidate.type == 5;
                                    });
    REQUIRE(barNumberIt != skin.elements.cend());
    const auto numberSource = barNumberIt->src.value<Lr2SrcBarNumber>();
    CHECK(numberSource.variant == 0);
    const auto numberDst = barNumberIt->dsts.first().value<Lr2Dst>();
    CHECK(numberDst.x == 7);
    CHECK(numberDst.y == 20);
    CHECK(numberDst.w == 11);
    CHECK(numberDst.h == 12);

    auto lampIt = std::find_if(skin.elements.cbegin(),
                               skin.elements.cend(),
                               [](const auto& candidate) {
                                   if (candidate.type != 3) {
                                       return false;
                                   }
                                   const auto source =
                                     candidate.src
                                       .template value<Lr2SrcBarImage>();
                                   return source.kind == Lr2SrcBarImage::Lamp;
                               });
    REQUIRE(lampIt != skin.elements.cend());
    const auto lampDst = lampIt->dsts.first().value<Lr2Dst>();
    CHECK(lampDst.x == 9);
    CHECK(lampDst.y == 16);
    CHECK(lampDst.w == 13);
    CHECK(lampDst.h == 14);
}
