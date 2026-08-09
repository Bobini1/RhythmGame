#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"
#include "gameplay_logic/lr2_skin/Lr2SkinModel.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEventLoop>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>
#include <QVariant>

#include <filesystem>
#include <memory>

using gameplay_logic::lr2_skin::Lr2Dst;
using gameplay_logic::lr2_skin::Lr2SkinModel;
using gameplay_logic::lr2_skin::Lr2SkinParser;
using gameplay_logic::lr2_skin::Lr2SrcImage;
using gameplay_logic::lr2_skin::Lr2SrcNumber;

namespace {

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static auto argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    static auto application = std::make_unique<QCoreApplication>(argc, argv);
}

void
waitForSkinLoadCount(Lr2SkinModel& model, const int& loadCount, int expected)
{
    if (loadCount >= expected) {
        return;
    }
    auto loop = QEventLoop{};
    const auto connection = QObject::connect(&model,
                                             &Lr2SkinModel::skinLoaded,
                                             &loop,
                                             [&loop, &loadCount, expected]() {
                                                 if (loadCount >= expected) {
                                                     loop.quit();
                                                 }
                                             });
    QTimer::singleShot(1'000, &loop, &QEventLoop::quit);
    loop.exec();
    QObject::disconnect(connection);
}

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

auto
runtimeGatedSprite(const int option) -> QString
{
    return QStringLiteral("#SRC_IMAGE,0,0,0,0,640,480,1,1,0,0,0,0,0\n"
                          "#DST_IMAGE,0,0,0,0,320,320,0,255,255,255,255,1,0,0,"
                          "0,0,0,%1,0,0\n")
      .arg(option);
}

} // namespace

TEST_CASE("LR2 skin parser infers repeated square canvases", "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n") + fullScreenSprite(0) +
                    fullScreenSprite(100) + fullScreenSprite(200));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    CHECK(skin.skinWidth == 320);
    CHECK(skin.skinHeight == 320);
}

TEST_CASE("LR2 skin parser keeps fallback for isolated small destinations",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n") + fullScreenSprite());

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
                    fullScreenSprite(0) + fullScreenSprite(100) +
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

    writeSkinFile(
      path,
      QStringLiteral("#IMAGE,full.png\n"
                     "#SRC_NUMBER,0,0,0,120,730,120,10,1,0,0,46,2,2\n"
                     "#DST_NUMBER,0,260,248.5,340.9,73.5,120.2,0,255,255,255,"
                     "255,1,0,0,0\n"));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    REQUIRE(skin.elements.size() == 1);
    REQUIRE(skin.elements.first().dsts.size() == 1);

    const auto dst = skin.elements.first().dsts.first().value<Lr2Dst>();
    CHECK(dst.x == 248);
    CHECK(dst.y == 340);
    CHECK(dst.w == 73);
    CHECK(dst.h == 120);
}

TEST_CASE("LR2 skin parser ignores image slots in skipped branches",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,base.png\n"
                                 "#IF,42\n"
                                 "#IMAGE,gauge.png\n"
                                 "#ENDIF\n"
                                 "#IMAGE,notes.png\n"
                                 "#IMAGE,judge.png\n"
                                 "#SRC_NOTE,0,1,0,0,60,30,1,1,0,0,0,0,0\n"));

    const auto skin = Lr2SkinParser::parseData(
      support::pathToQString(path), QVariantMap{}, QVariantList{});

    REQUIRE(skin.noteSources.size() == 1);
    const auto source = skin.noteSources.first().value<Lr2SrcImage>();
    CHECK(source.gr == 1);
    CHECK(source.source.contains(QStringLiteral("notes.png")));
}

TEST_CASE("LR2 skin parser records select detail option gates", "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(
      path,
      QStringLiteral(
        "#IMAGE,full.png\n"
        "#SRC_IMAGE,0,0,0,0,10,10,1,1,0,0,0,0,0\n"
        "#DST_IMAGE,0,0,0,0,10,10,0,255,255,255,255,1,0,0,0,0,0,2,160,0\n"
        "#SRC_IMAGE,1,0,0,0,10,10,1,1,0,0,0,0,0\n"
        "#DST_IMAGE,1,0,0,0,10,10,0,255,255,255,255,1,0,0,0,0,0,2,180,0\n"
        "#SRC_BARGRAPH,0,0,0,0,10,10,1,1,0,0,5,0,0\n"
        "#DST_BARGRAPH,0,0,0,0,10,10,0,255,255,255,255,1,0,0,0,0,0,70,505,"
        "620\n"));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));
    const auto hasUsedOption = [&skin](int option) {
        return skin.usedOptions.contains(QVariant(option));
    };
    const auto hasElementOption = [&skin](int option) {
        return skin.usedElementOptions.contains(QVariant(option));
    };

    CHECK(hasUsedOption(2));
    CHECK(hasUsedOption(70));
    CHECK(hasUsedOption(160));
    CHECK(hasUsedOption(180));
    CHECK(hasUsedOption(505));
    CHECK(hasUsedOption(620));
    CHECK(hasElementOption(160));
    CHECK(hasElementOption(180));
    CHECK(hasElementOption(505));
}

TEST_CASE("LR2 skin parser records conditional option dependencies",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n"
                                 "#IF,38,!987\n") +
                    fullScreenSprite() + QStringLiteral("#ENDIF\n"));

    const auto skin = Lr2SkinParser::parseData(
      support::pathToQString(path), QVariantMap{}, QVariantList{ 38 });

    CHECK(skin.conditionOptions.contains(QVariant(38)));
    CHECK(skin.conditionOptions.contains(QVariant(987)));
}

TEST_CASE("LR2 skin model keeps DST-only runtime option changes hot",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n") + runtimeGatedSprite(39));

    Lr2SkinModel model;
    int loadCount = 0;
    QObject::connect(
      &model, &Lr2SkinModel::skinLoaded, [&loadCount]() { ++loadCount; });

    model.setActiveOptions(QVariantList{ 34, 39 });
    model.setCsvPath(support::pathToQString(path));
    REQUIRE(loadCount == 1);

    model.setActiveOptions(QVariantList{ 35, 39 });

    CHECK(loadCount == 1);
    CHECK(model.activeOptions() == QVariantList{ 35, 39 });
}

TEST_CASE("LR2 skin model reloads when conditional options change",
          "[lr2][skin]")
{
    ensureCoreApplication();
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(path,
                  QStringLiteral("#IMAGE,full.png\n"
                                 "#IF,34\n") +
                    fullScreenSprite() + QStringLiteral("#ELSE\n") +
                    fullScreenSprite(100) + QStringLiteral("#ENDIF\n"));

    Lr2SkinModel model;
    int loadCount = 0;
    QObject::connect(
      &model, &Lr2SkinModel::skinLoaded, [&loadCount]() { ++loadCount; });

    model.setActiveOptions(QVariantList{ 34, 39 });
    model.setCsvPath(support::pathToQString(path));
    REQUIRE(loadCount == 1);
    REQUIRE(model.conditionOptions().contains(QVariant(34)));

    model.setActiveOptions(QVariantList{ 35, 39 });
    waitForSkinLoadCount(model, loadCount, 2);

    CHECK(loadCount == 2);
}

TEST_CASE("LR2 NOWJUDGE and NOWCOMBO select their judgement directly",
          "[lr2][skin]")
{
    QTemporaryDir tempDir;
    const auto path = tempSkinPath(tempDir);

    writeSkinFile(
      path,
      QStringLiteral(
        "#IMAGE,judge.png\n"
        "#SRC_NOWJUDGE_1P,4,0,0,0,100,20,1,1,0,46,0,0,0\n"
        "#DST_NOWJUDGE_1P,4,0,0,0,100,20,0,255,255,255,255,1,0,0,0,-1,46\n"
        "#SRC_NOWCOMBO_1P,4,0,0,0,100,20,10,1,0,46,0,0,4\n"
        "#DST_NOWCOMBO_1P,4,0,0,0,10,20,0,255,255,255,255,1,0,0,0,-1,46\n"
        "#SRC_NOWJUDGE_2P,3,0,0,0,100,20,1,1,0,46,0,0,0\n"
        "#DST_NOWJUDGE_2P,3,0,0,0,100,20,0,255,255,255,255,1,0,0,0,-1,46\n"
        "#SRC_NOWCOMBO_2P,3,0,0,0,100,20,10,1,0,46,0,0,4\n"
        "#DST_NOWCOMBO_2P,3,0,0,0,10,20,0,255,255,255,255,1,0,0,0,-1,46\n") +
        QStringLiteral(
          "#SRC_NOWJUDGE_1P,6,0,0,0,100,20,1,1,0,46,1,0,0\n"
          "#DST_NOWJUDGE_1P,6,0,0,0,100,20,0,255,255,255,255,1,0,0,0,-1,46\n"
          "#SRC_NOWCOMBO_1P,6,0,0,0,100,20,10,1,0,446,0,0,4\n"
          "#DST_NOWCOMBO_1P,6,0,0,0,10,20,0,255,255,255,255,1,0,0,0,-1,446\n"
          "#SRC_NOWJUDGE_2P,6,0,0,0,100,20,1,1,0,47,1,0,0\n"
          "#DST_NOWJUDGE_2P,6,0,0,0,100,20,0,255,255,255,255,1,0,0,0,-1,47\n"
          "#SRC_NOWCOMBO_2P,6,0,0,0,100,20,10,1,0,447,0,0,4\n"
          "#DST_NOWCOMBO_2P,6,0,0,0,10,20,0,255,255,255,255,1,0,0,0,-1,447\n") +
        runtimeGatedSprite(261));

    const auto skin = Lr2SkinParser::parseData(support::pathToQString(path));

    REQUIRE(skin.elements.size() == 9);
    const auto judgeSource = skin.elements.at(0).src.value<Lr2SrcImage>();
    CHECK(judgeSource.nowJudge);
    CHECK(judgeSource.side == 1);
    CHECK(judgeSource.judgementIndex == 4);
    CHECK(judgeSource.timer == 46);
    const auto comboSource = skin.elements.at(1).src.value<Lr2SrcNumber>();
    CHECK(comboSource.nowCombo);
    CHECK(comboSource.side == 1);
    CHECK(comboSource.judgementIndex == 4);
    CHECK(comboSource.timer == 46);
    const auto judgeSource2 = skin.elements.at(2).src.value<Lr2SrcImage>();
    CHECK(judgeSource2.nowJudge);
    CHECK(judgeSource2.side == 2);
    CHECK(judgeSource2.judgementIndex == 3);
    CHECK(judgeSource2.timer == 47);
    const auto comboSource2 = skin.elements.at(3).src.value<Lr2SrcNumber>();
    CHECK(comboSource2.nowCombo);
    CHECK(comboSource2.side == 2);
    CHECK(comboSource2.judgementIndex == 3);
    CHECK(comboSource2.timer == 47);
    const auto maxJudgeSource = skin.elements.at(4).src.value<Lr2SrcImage>();
    CHECK(maxJudgeSource.nowJudge);
    CHECK(maxJudgeSource.side == 1);
    CHECK(maxJudgeSource.judgementIndex == 6);
    CHECK(maxJudgeSource.timer == 46);
    CHECK(maxJudgeSource.op1 == 1);
    const auto maxComboSource = skin.elements.at(5).src.value<Lr2SrcNumber>();
    CHECK(maxComboSource.nowCombo);
    CHECK(maxComboSource.side == 1);
    CHECK(maxComboSource.judgementIndex == 6);
    CHECK(maxComboSource.timer == 446);
    const auto maxJudgeSource2 = skin.elements.at(6).src.value<Lr2SrcImage>();
    CHECK(maxJudgeSource2.nowJudge);
    CHECK(maxJudgeSource2.side == 2);
    CHECK(maxJudgeSource2.judgementIndex == 6);
    CHECK(maxJudgeSource2.timer == 47);
    CHECK(maxJudgeSource2.op1 == 1);
    const auto maxComboSource2 = skin.elements.at(7).src.value<Lr2SrcNumber>();
    CHECK(maxComboSource2.nowCombo);
    CHECK(maxComboSource2.side == 2);
    CHECK(maxComboSource2.judgementIndex == 6);
    CHECK(maxComboSource2.timer == 447);
    for (qsizetype index = 0; index < 8; ++index) {
        const auto& element = skin.elements.at(index);
        REQUIRE(element.dsts.size() == 1);
        const auto dst = element.dsts.first().value<Lr2Dst>();
        CHECK(dst.op1 == 0);
        CHECK(dst.op2 == 0);
        CHECK(dst.op3 == 0);
        if (index == 5) {
            CHECK(dst.timer == 446);
        } else if (index == 7) {
            CHECK(dst.timer == 447);
        }
    }
    CHECK_FALSE(skin.usedOptions.contains(QVariant(242)));
    CHECK_FALSE(skin.usedOptions.contains(QVariant(263)));
    const auto ordinarySource = skin.elements.at(8).src.value<Lr2SrcImage>();
    CHECK_FALSE(ordinarySource.nowJudge);
    const auto ordinaryDst = skin.elements.at(8).dsts.first().value<Lr2Dst>();
    CHECK(ordinaryDst.op1 == 261);
    CHECK(skin.usedOptions.contains(QVariant(261)));

    Lr2SkinModel model;
    int loadCount = 0;
    QObject::connect(
      &model, &Lr2SkinModel::skinLoaded, [&loadCount]() { ++loadCount; });
    model.setCsvPath(support::pathToQString(path));
    REQUIRE(loadCount == 1);
    CHECK(model.hasNowJudgeMaxGaugeVariant1());
    CHECK(model.hasNowJudgeMaxGaugeVariant2());
}
