#include "Lr2SelectBarModel.h"
#include "Lr2SelectBarCell.h"
#include "Lr2ChartDataSnapshot.h"
#include "gameplay_logic/ChartData.h"
#include "resource_managers/Tables.h"

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <utility>

namespace {

QVariantMap item(QString key) {
	return {
		{QStringLiteral("key"), key},
		{QStringLiteral("folderKey"), key},
		{QStringLiteral("displayText"), key},
		{QStringLiteral("type"), QStringLiteral("folder")},
	};
}

QVariantMap itemWithLampDistribution(QString key, QVariantList lamps) {
	QVariantMap value = item(std::move(key));
	value.insert(QStringLiteral("folderDistribution"), QVariantMap {
		{QStringLiteral("lamps"), std::move(lamps)},
		{QStringLiteral("ranks"), QVariantList {}},
	});
	return value;
}

QVariantMap itemWithFlatLampDistribution(QString key, QVariantList lamps) {
	QVariantMap value = item(std::move(key));
	value.insert(QStringLiteral("folderGraphLamps"), std::move(lamps));
	value.insert(QStringLiteral("folderGraphRanks"), QVariantList {});
	return value;
}

QVariantMap chartItemWithFlatLampDistribution(QString key, QVariantList lamps) {
	QVariantMap value = itemWithFlatLampDistribution(std::move(key), std::move(lamps));
	value.insert(QStringLiteral("type"), QStringLiteral("chart"));
	return value;
}

QVariantMap chartItem(QString key, QString md5) {
	QVariantMap value = item(std::move(key));
	value.insert(QStringLiteral("type"), QStringLiteral("chart"));
	value.insert(QStringLiteral("md5"), std::move(md5));
	return value;
}

QVariantList paddedItems(int logicalCount, int minimumCount) {
	QVariantList values;
	for (int i = 0; i < logicalCount; ++i) {
		values.append(item(QStringLiteral("item%1").arg(i)));
	}
	const int originalCount = values.size();
	for (int i = originalCount; i < minimumCount; ++i) {
		values.append(values.at(i % originalCount));
	}
	return values;
}

QList<int> sourceRows(const Lr2SelectBarModel& model) {
	QList<int> rows;
	for (int row = 0; row < model.rowCount(); ++row) {
		rows.append(model.data(model.index(row, 0), Lr2SelectBarModel::SourceRowRole).toInt());
	}
	return rows;
}

Lr2SelectBarCell* cellForRow(const Lr2SelectBarModel& model, int row) {
	const int slot = model.data(model.index(row, 0), Lr2SelectBarModel::SlotRole).toInt();
	const QVariantList cells = model.cells();
	if (slot < 0 || slot >= cells.size()) {
		return nullptr;
	}
	return qobject_cast<Lr2SelectBarCell*>(cells.at(slot).value<QObject*>());
}

std::unique_ptr<gameplay_logic::ChartData> chartWithHistogram(QObject* parent = nullptr) {
	return std::make_unique<gameplay_logic::ChartData>(
		QStringLiteral("title"),
		QStringLiteral("artist"),
		QString {},
		QString {},
		QString {},
		QString {},
		QString {},
		QString {},
		75.0,
		160.0,
		10,
		3,
		false,
		QList<qint64> {},
		3,
		2,
		1,
		4,
		5,
		1000,
		120.0,
		120.0,
		120.0,
		120.0,
		120.0,
		0.0,
		0.0,
		0.0,
		QStringLiteral("path"),
		0,
		QStringLiteral("sha"),
		QStringLiteral("abc"),
		gameplay_logic::ChartData::Keymode::K7,
		QList<QList<qint64>> {
			QList<qint64> {1, 2, 3},
			QList<qint64> {4},
			QList<qint64> {},
			QList<qint64> {5, 6},
			QList<qint64> {7},
		},
		QList<gameplay_logic::BpmChange> {},
		0,
		parent);
}

} // namespace

TEST_CASE("LR2 select bar model wraps by logical count, not padded model count",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems(paddedItems(7, 32));

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(7);
	model.setRowCountLimit(11);
	model.setCenterRow(5);
	model.setCurrentIndex(0);

	REQUIRE(sourceRows(model) == QList<int>({2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5}));

	model.setCurrentIndex(6);
	REQUIRE(sourceRows(model) == QList<int>({1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4}));

	model.setCurrentIndex(0);
	REQUIRE(sourceRows(model) == QList<int>({2, 3, 4, 5, 6, 0, 1, 2, 3, 4, 5}));
}

TEST_CASE("LR2 select bar cells update folder graph data without resetting",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		itemWithLampDistribution(QStringLiteral("folder0"), {1, 2, 3}),
		itemWithLampDistribution(QStringLiteral("folder1"), {4, 5, 6}),
		itemWithLampDistribution(QStringLiteral("folder2"), {7, 8, 9}),
	});

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(3);
	model.setRowCountLimit(3);
	model.setCenterRow(1);
	model.setCurrentIndex(0);

	auto* centerCell = cellForRow(model, 1);
	REQUIRE(centerCell);
	REQUIRE(centerCell->isFolderLike());
	REQUIRE(centerCell->graphValueForType(0, 0) == 1.0);
	REQUIRE(centerCell->graphValueForType(0, 1) == 2.0);
	REQUIRE(centerCell->graphValueForType(0, 2) == 3.0);

	source.setItems({
		itemWithLampDistribution(QStringLiteral("folder0"), {10, 20, 30}),
		itemWithLampDistribution(QStringLiteral("folder1"), {4, 5, 6}),
		itemWithLampDistribution(QStringLiteral("folder2"), {7, 8, 9}),
	});

	auto* updatedCenterCell = cellForRow(model, 1);
	REQUIRE(updatedCenterCell == centerCell);
	REQUIRE(updatedCenterCell->graphValueForType(0, 0) == 10.0);
	REQUIRE(updatedCenterCell->graphValueForType(0, 1) == 20.0);
	REQUIRE(updatedCenterCell->graphValueForType(0, 2) == 30.0);
}

TEST_CASE("LR2 select item model applies async folder summaries by key",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		item(QStringLiteral("folder0")),
		item(QStringLiteral("folder1")),
		item(QStringLiteral("folder2")),
	});

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(3);
	model.setRowCountLimit(3);
	model.setCenterRow(1);
	model.setCurrentIndex(0);

	auto* centerCell = cellForRow(model, 1);
	REQUIRE(centerCell);
	REQUIRE(centerCell->graphValueForType(0, 0) == 0.0);

	source.setFolderSummary(QStringLiteral("folder0"),
							4,
							QVariantMap {},
							QVariantMap {
								{QStringLiteral("lamps"), QVariantList {2, 0, 5}},
								{QStringLiteral("ranks"), QVariantList {}},
							});

	auto* updatedCenterCell = cellForRow(model, 1);
	REQUIRE(updatedCenterCell == centerCell);
	REQUIRE(updatedCenterCell->lamp() == 4);
	REQUIRE(updatedCenterCell->graphValueForType(0, 0) == 2.0);
	REQUIRE(updatedCenterCell->graphValueForType(0, 2) == 5.0);
}

TEST_CASE("LR2 chart data snapshot reads typed chart data for note graph data",
		  "[lr2][runtime][select]")
{
	Lr2ChartDataSnapshot snapshot;
	auto chart = chartWithHistogram(&snapshot);
	snapshot.setChart(chart.get());

	REQUIRE(snapshot.hasHistogram());
	REQUIRE(snapshot.md5() == QStringLiteral("abc"));
	REQUIRE(snapshot.normalNoteCount() == 3);
	REQUIRE(snapshot.histogramData().size() == 5);
    REQUIRE(snapshot.histogramData().at(0).toList() == QVariantList({1LL, 2LL, 3LL}));
	REQUIRE(snapshot.normalDensityData().size() == 3);
	REQUIRE(snapshot.normalDensityData().at(0).toList()
			== QVariantList({0, 5LL, 4LL, 0, 0, 1LL, 7LL}));
	REQUIRE(snapshot.normalDensityData().at(1).toList()
			== QVariantList({0, 6LL, 0, 0, 0, 2LL, 0}));
	REQUIRE(snapshot.normalDensityData().at(2).toList()
			== QVariantList({0, 0, 0, 0, 0, 3LL, 0}));
	REQUIRE(snapshot.normalDensityMax() == 20);
}

TEST_CASE("LR2 select bar cells consume flat folder graph arrays",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		itemWithFlatLampDistribution(QStringLiteral("folder0"), {3, 1, 4}),
	});

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(1);
	model.setRowCountLimit(1);
	model.setCenterRow(0);
	model.setCurrentIndex(0);

	auto* cell = cellForRow(model, 0);
	REQUIRE(cell);
	REQUIRE(cell->graphValueForType(0, 0) == 3.0);
	REQUIRE(cell->graphValueForType(0, 1) == 1.0);
	REQUIRE(cell->graphValueForType(0, 2) == 4.0);
}

TEST_CASE("LR2 select bar cells keep graph data on chart rows",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		chartItemWithFlatLampDistribution(QStringLiteral("chart0"), {0, 1, 0, 2}),
	});

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(1);
	model.setRowCountLimit(1);
	model.setCenterRow(0);
	model.setCurrentIndex(0);

	auto* cell = cellForRow(model, 0);
	REQUIRE(cell);
	REQUIRE(cell->isChartLike());
	REQUIRE_FALSE(cell->isFolderLike());
	REQUIRE(cell->graphValueForType(0, 1) == 1.0);
	REQUIRE(cell->graphValueForType(0, 3) == 2.0);
}

TEST_CASE("LR2 select item model reads raw table entry fields from lean row maps",
		  "[lr2][runtime][select]")
{
	resource_managers::Entry entry;
	entry.title = QStringLiteral("Song");
	entry.subtitle = QStringLiteral("Sub");
	entry.artist = QStringLiteral("Artist");
	entry.subartist = QStringLiteral("Subartist");
	entry.md5 = QStringLiteral("md5");
	entry.sha256 = QStringLiteral("sha");
	entry.level = QStringLiteral("12");

	Lr2SelectItemModel source;
	source.setItems({
		QVariantMap {
			{QStringLiteral("rawItem"), QVariant::fromValue(entry)},
			{QStringLiteral("displayText"), QStringLiteral("(missing) Song Sub")},
			{QStringLiteral("title"), QStringLiteral("(missing) Song")},
			{QStringLiteral("titleType"), 0},
			{QStringLiteral("bodyType"), 4},
			{QStringLiteral("lamp"), 2},
			{QStringLiteral("scoreRank"), 6},
			{QStringLiteral("labelMask"), 0},
		},
	});

	const QModelIndex row = source.index(0, 0);
	REQUIRE(source.data(row, Lr2SelectItemModel::IsEntryRole).toBool());
	REQUIRE(source.data(row, Lr2SelectItemModel::KeyRole).toString() == QStringLiteral("entry:md5"));
	REQUIRE(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() == QStringLiteral("(missing) Song Sub"));
	REQUIRE(source.data(row, Lr2SelectItemModel::TitleRole).toString() == QStringLiteral("(missing) Song"));
	REQUIRE(source.data(row, Lr2SelectItemModel::SubtitleRole).toString() == QStringLiteral("Sub"));
	REQUIRE(source.data(row, Lr2SelectItemModel::ArtistRole).toString() == QStringLiteral("Artist"));
	REQUIRE(source.data(row, Lr2SelectItemModel::SubartistRole).toString() == QStringLiteral("Subartist"));
	REQUIRE(source.data(row, Lr2SelectItemModel::Md5Role).toString() == QStringLiteral("md5"));
	REQUIRE(source.data(row, Lr2SelectItemModel::Sha256Role).toString() == QStringLiteral("sha"));
	REQUIRE(source.data(row, Lr2SelectItemModel::PlayLevelRole).toInt() == 12);
}

TEST_CASE("LR2 select item model detects lean folder rows from raw strings",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		QVariantMap {
			{QStringLiteral("rawItem"), QStringLiteral("C:/Songs/Folder/")},
			{QStringLiteral("folderKey"), QStringLiteral("C:/Songs/Folder/")},
			{QStringLiteral("displayText"), QStringLiteral("Folder")},
			{QStringLiteral("bodyType"), 1},
		},
	});

	const QModelIndex row = source.index(0, 0);
	REQUIRE(source.data(row, Lr2SelectItemModel::IsFolderLikeRole).toBool());
	REQUIRE(source.data(row, Lr2SelectItemModel::KeyRole).toString() == QStringLiteral("folder:C:/Songs/Folder/"));
	REQUIRE(source.data(row, Lr2SelectItemModel::DisplayTextRole).toString() == QStringLiteral("Folder"));
	REQUIRE(source.data(row, Lr2SelectItemModel::LampRole).toInt() == 0);
}

TEST_CASE("LR2 select item model detects lean table and level rows from folder keys",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		QVariantMap {
			{QStringLiteral("rawItem"), QVariantMap {
				{QStringLiteral("name"), QStringLiteral("Table")},
				{QStringLiteral("url"), QStringLiteral("https://example.test/table")},
			}},
			{QStringLiteral("folderKey"), QStringLiteral("table:https://example.test/table")},
			{QStringLiteral("displayText"), QStringLiteral("Table")},
			{QStringLiteral("bodyType"), 2},
		},
		QVariantMap {
			{QStringLiteral("rawItem"), QVariantMap {
				{QStringLiteral("name"), QStringLiteral("Level 12")},
			}},
			{QStringLiteral("folderKey"), QStringLiteral("level:https://example.test/table:Level 12")},
			{QStringLiteral("displayText"), QStringLiteral("Level 12")},
			{QStringLiteral("bodyType"), 3},
		},
	});

	for (int row = 0; row < source.rowCount(); ++row) {
		const QModelIndex index = source.index(row, 0);
		REQUIRE(source.data(index, Lr2SelectItemModel::IsFolderLikeRole).toBool());
	}

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(2);
	model.setRowCountLimit(2);
	model.setCenterRow(0);
	model.setCurrentIndex(0);

	source.setFolderSummary(QStringLiteral("table:https://example.test/table"),
							5,
							QVariantMap {},
							QVariantMap {
								{QStringLiteral("lamps"), QVariantList {1, 2, 3}},
								{QStringLiteral("ranks"), QVariantList {}},
							});

	auto* tableCell = cellForRow(model, 0);
	REQUIRE(tableCell);
	REQUIRE(tableCell->isFolderLike());
	REQUIRE(tableCell->lamp() == 5);
	REQUIRE(tableCell->graphValueForType(0, 0) == 1.0);
	REQUIRE(tableCell->graphValueForType(0, 1) == 2.0);
	REQUIRE(tableCell->graphValueForType(0, 2) == 3.0);
}

TEST_CASE("LR2 select item model applies score summaries by identifier",
		  "[lr2][runtime][select]")
{
	Lr2SelectItemModel source;
	source.setItems({
		chartItem(QStringLiteral("chart0"), QStringLiteral("abc")),
	});

	Lr2SelectBarModel model;
	model.setSourceModel(&source);
	model.setLogicalCount(1);
	model.setRowCountLimit(1);
	model.setCenterRow(0);
	model.setCurrentIndex(0);

	auto* cell = cellForRow(model, 0);
	REQUIRE(cell);
	REQUIRE(cell->lamp() == 0);
	REQUIRE(cell->rank() == 0);

	source.setScoreSummary(QStringLiteral("abc"), 4, 7, 0.875);
	REQUIRE(source.data(source.index(0, 0), Lr2SelectItemModel::LampRole).toInt() == 4);
	REQUIRE(source.data(source.index(0, 0), Lr2SelectItemModel::ScoreRankRole).toInt() == 7);
	REQUIRE(cell->lamp() == 4);
	REQUIRE(cell->rank() == 7);

	source.clearScoreSummaries();
	REQUIRE(source.data(source.index(0, 0), Lr2SelectItemModel::LampRole).toInt() == 0);
	REQUIRE(source.data(source.index(0, 0), Lr2SelectItemModel::ScoreRankRole).toInt() == 0);
	REQUIRE(cell->lamp() == 0);
	REQUIRE(cell->rank() == 0);
}
