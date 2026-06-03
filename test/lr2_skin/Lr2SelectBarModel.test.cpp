#include "Lr2SelectBarModel.h"
#include "Lr2SelectBarCell.h"
#include "Lr2ChartDataSnapshot.h"
#include "gameplay_logic/ChartData.h"

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
		rows.append(model.sourceRowAt(row));
	}
	return rows;
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

	auto* centerCell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(1)));
	REQUIRE(centerCell);
	REQUIRE(centerCell->isFolderLike());
	REQUIRE(centerCell->graphValueForType(0, 0) == 1.0);
	REQUIRE(centerCell->graphValueForType(0, 1) == 2.0);
	REQUIRE(centerCell->graphValueForType(0, 2) == 3.0);

	source.updateItems({
		itemWithLampDistribution(QStringLiteral("folder0"), {10, 20, 30}),
		itemWithLampDistribution(QStringLiteral("folder1"), {4, 5, 6}),
		itemWithLampDistribution(QStringLiteral("folder2"), {7, 8, 9}),
	});

	auto* updatedCenterCell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(1)));
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

	auto* centerCell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(1)));
	REQUIRE(centerCell);
	REQUIRE(centerCell->graphValueForType(0, 0) == 0.0);

	source.setFolderSummary(QStringLiteral("folder0"),
							4,
							QVariantMap {},
							QVariantMap {
								{QStringLiteral("lamps"), QVariantList {2, 0, 5}},
								{QStringLiteral("ranks"), QVariantList {}},
							});

	auto* updatedCenterCell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(1)));
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
	REQUIRE(snapshot.revision().contains(QStringLiteral("abc:1000:3:2:1:4:5")));
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

	auto* cell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(0)));
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

	auto* cell = qobject_cast<Lr2SelectBarCell*>(model.cellAtSlot(model.slotForRow(0)));
	REQUIRE(cell);
	REQUIRE(cell->isChartLike());
	REQUIRE_FALSE(cell->isFolderLike());
	REQUIRE(cell->graphValueForType(0, 1) == 1.0);
	REQUIRE(cell->graphValueForType(0, 3) == 2.0);
}
