#include "Lr2SelectStateCache.h"
#include "Lr2SelectBarCell.h"
#include "Lr2SelectBarWindow.h"

#include <catch2/catch_test_macros.hpp>

#include <QVariantMap>

namespace {

QVariantMap chart(QString md5, QString path, int difficulty, int playLevel) {
    return {
        {QStringLiteral("md5"), std::move(md5)},
        {QStringLiteral("path"), std::move(path)},
        {QStringLiteral("keymode"), 7},
        {QStringLiteral("chartDirectory"), QStringLiteral("song")},
        {QStringLiteral("difficulty"), difficulty},
        {QStringLiteral("playLevel"), playLevel},
        {QStringLiteral("normalNoteCount"), 100},
        {QStringLiteral("scratchCount"), 10},
        {QStringLiteral("lnCount"), 5},
        {QStringLiteral("bssCount"), 0},
        {QStringLiteral("mineCount"), 0},
    };
}

QVariantMap score(QString clearType, int points, int maxPoints) {
    return {
        {QStringLiteral("result"), QVariantMap {
            {QStringLiteral("clearType"), std::move(clearType)},
            {QStringLiteral("points"), points},
            {QStringLiteral("maxPoints"), maxPoints},
            {QStringLiteral("maxCombo"), 123},
            {QStringLiteral("judgementCounts"), QVariantList {1, 2, 3, 4, 5, 6}},
            {QStringLiteral("noteOrderAlgorithm"), 2},
            {QStringLiteral("dpOptions"), 2},
            {QStringLiteral("keymode"), 7},
        }},
    };
}

} // namespace

TEST_CASE("LR2 select state cache builds focused score snapshots natively", "[lr2][runtime][select]") {
    Lr2SelectStateCache cache;
    const QVariantMap selectedChart = chart(QStringLiteral("ABC"), QStringLiteral("song/a.bms"), 4, 12);
    const QVariantMap easyChart = chart(QStringLiteral("DEF"), QStringLiteral("song/easy.bms"), 2, 5);

    cache.setScores(QVariantMap {
        {QStringLiteral("ABC"), QVariantList {score(QStringLiteral("HARD"), 160, 200)}},
        {QStringLiteral("DEF"), QVariantList {score(QStringLiteral("EASY"), 80, 200)}},
    });
    cache.setChartGroupCache(QVariantMap {
        {QStringLiteral("song\n7"), QVariantList {selectedChart, easyChart}},
    });

    const QVariantMap result = cache.refreshSelectedState(selectedChart, 0, 1, 1, false, {});
    REQUIRE(result.value(QStringLiteral("changed")).toBool());

    const QVariantMap state = result.value(QStringLiteral("state")).toMap();
    const QVariantMap summary = state.value(QStringLiteral("summary")).toMap();
    REQUIRE(summary.value(QStringLiteral("clearType")).toString() == QStringLiteral("HARD"));
    REQUIRE(summary.value(QStringLiteral("lamp")).toInt() == 4);
    REQUIRE(summary.value(QStringLiteral("rank")).toInt() == 7);

    const QVariantList scoreOptions = state.value(QStringLiteral("scoreOptionIds")).toList();
    REQUIRE(scoreOptions.contains(119));
    REQUIRE(scoreOptions.contains(128));
    REQUIRE(scoreOptions.contains(145));

    const QVariantMap difficultyState = state.value(QStringLiteral("difficultyState")).toMap();
    const QVariantList counts = difficultyState.value(QStringLiteral("counts")).toList();
    const QVariantList levels = difficultyState.value(QStringLiteral("levels")).toList();
    const QVariantList lamps = difficultyState.value(QStringLiteral("lamps")).toList();
    REQUIRE(counts.at(4).toInt() == 1);
    REQUIRE(counts.at(2).toInt() == 1);
    REQUIRE(levels.at(4).toInt() == 12);
    REQUIRE(lamps.at(4).toInt() == 4);
}

TEST_CASE("LR2 select state cache builds folder bar cells natively", "[lr2][runtime][select]") {
    Lr2SelectStateCache cache;
    const QString folder = QStringLiteral("C:/BMS/Packs/Folder One");
    cache.setFolderLampByKey(QVariantMap {
        {QStringLiteral("folder:") + folder, 3},
    });
    cache.setFolderDistributionByKey(QVariantMap {
        {QStringLiteral("folder:") + folder, QVariantMap {
            {QStringLiteral("lamps"), QVariantList {1, 2, 3}},
            {QStringLiteral("ranks"), QVariantList {4, 5, 6}},
        }},
    });

    const QVariantList titleTypes {4, 0};
    const QVariantMap core = cache.barCellCore(folder, titleTypes, 1, 1, 1);
    REQUIRE(core.value(QStringLiteral("key")).toString() == QStringLiteral("folder:") + folder);
    REQUIRE(core.value(QStringLiteral("text")).toString() == QStringLiteral("Folder One"));
    REQUIRE(core.value(QStringLiteral("titleType")).toInt() == 4);
    REQUIRE(core.value(QStringLiteral("bodyType")).toInt() == 1);
    REQUIRE(core.value(QStringLiteral("folderLike")).toBool());
    REQUIRE(core.value(QStringLiteral("lamp")).toInt() == 3);
    REQUIRE(core.value(QStringLiteral("graphLamps")).toList().size() == 3);

    Lr2SelectBarCell cell;
    cache.updateBarCell(&cell, 2, folder, titleTypes, 1, 1, 1);
    REQUIRE(cell.row() == 2);
    REQUIRE(cell.text() == QStringLiteral("Folder One"));
    REQUIRE(cell.titleType() == 4);
    REQUIRE(cell.isFolderLike());
    REQUIRE(cell.lamp() == 3);
}

TEST_CASE("LR2 select bar cell builds distribution graph segments natively", "[lr2][runtime][select]") {
    Lr2SelectBarCell cell;
    cell.setCore(
        0,
        true,
        QStringLiteral("Folder"),
        4,
        1,
        0,
        0,
        0,
        false,
        false,
        false,
        true,
        0,
        0,
        0,
        QVariantList {1, 2, 3},
        QVariantList {});

    const QVariantMap state {
        {QStringLiteral("x"), 5},
        {QStringLiteral("y"), 7},
        {QStringLiteral("w"), 60},
        {QStringLiteral("h"), 10},
        {QStringLiteral("a"), 255},
        {QStringLiteral("r"), 255},
        {QStringLiteral("g"), 255},
        {QStringLiteral("b"), 255},
        {QStringLiteral("blend"), 0},
        {QStringLiteral("filter"), 0},
        {QStringLiteral("angle"), 0},
        {QStringLiteral("center"), 0},
        {QStringLiteral("sortId"), 42},
        {QStringLiteral("op1"), 0},
        {QStringLiteral("op2"), 0},
        {QStringLiteral("op3"), 0},
        {QStringLiteral("op4"), 0},
    };

    const QVariantList segments = cell.graphSegmentModel(0, 3, 20, state);
    REQUIRE(segments.size() == 3);

    const QVariantMap first = segments.at(2).toMap();
    REQUIRE(first.value(QStringLiteral("segment")).toInt() == 2);
    REQUIRE(first.value(QStringLiteral("frameOffset")).toInt() == 2);
    REQUIRE(first.value(QStringLiteral("visible")).toBool());
    const QVariantMap firstState = first.value(QStringLiteral("state")).toMap();
    REQUIRE(firstState.value(QStringLiteral("x")).toReal() == 5.0);
    REQUIRE(firstState.value(QStringLiteral("w")).toReal() == 30.0);
    REQUIRE(firstState.value(QStringLiteral("sortId")).toInt() == 42);

    const QVariantMap last = segments.at(0).toMap();
    REQUIRE(last.value(QStringLiteral("segment")).toInt() == 0);
    REQUIRE(last.value(QStringLiteral("frameOffset")).toInt() == 0);
    REQUIRE(last.value(QStringLiteral("visible")).toBool());
    const QVariantMap lastState = last.value(QStringLiteral("state")).toMap();
    REQUIRE(lastState.value(QStringLiteral("x")).toReal() == 55.0);
    REQUIRE(lastState.value(QStringLiteral("w")).toReal() == 10.0);
}

TEST_CASE("LR2 select bar window shifts visible rows natively", "[lr2][runtime][select]") {
    Lr2SelectStateCache cache;
    Lr2SelectBarWindow window;
    window.setNativeState(&cache);
    window.setItems(QVariantList {
        QStringLiteral("A"),
        QStringLiteral("B"),
        QStringLiteral("C"),
        QStringLiteral("D"),
    });
    window.setLogicalCount(4);
    window.setRowCount(3);
    window.setBarCenter(1);
    window.setVisualBaseIndex(1);
    window.setBarTitleTypes(QVariantList {4});
    window.setListRevision(1);

    window.refresh(true);
    REQUIRE(window.cells().size() == 3);
    REQUIRE(window.slotOffset() == 0);
    REQUIRE(window.entryAtRow(0, 1).toString() == QStringLiteral("A"));
    REQUIRE(window.entryAtRow(1, 1).toString() == QStringLiteral("B"));
    REQUIRE(window.entryAtRow(2, 1).toString() == QStringLiteral("C"));

    auto* firstVisible = qobject_cast<Lr2SelectBarCell*>(window.cellAtSlot(window.slotForRow(0)));
    REQUIRE(firstVisible);
    REQUIRE(firstVisible->text() == QStringLiteral("A"));
    REQUIRE(firstVisible->titleType() == 4);

    window.setVisualBaseIndex(2);
    window.refresh(false);
    REQUIRE(window.slotOffset() == 1);
    REQUIRE(window.entryAtRow(0, 1).toString() == QStringLiteral("B"));
    REQUIRE(window.entryAtRow(1, 1).toString() == QStringLiteral("C"));
    REQUIRE(window.entryAtRow(2, 1).toString() == QStringLiteral("D"));

    auto* entering = qobject_cast<Lr2SelectBarCell*>(window.cellAtSlot(window.slotForRow(2)));
    REQUIRE(entering);
    REQUIRE(entering->row() == 2);
    REQUIRE(entering->text() == QStringLiteral("D"));
}
