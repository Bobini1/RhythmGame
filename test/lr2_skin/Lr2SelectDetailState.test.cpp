#include "Lr2SelectDetailState.h"

#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsScore.h"

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QVariantList>

#include <memory>

namespace {

gameplay_logic::BmsScore* scoreWithClearType(const QString& clearType, QObject* parent) {
	const QString guid = QStringLiteral("test-%1").arg(clearType);
	auto result = std::make_unique<gameplay_logic::BmsResult>(
		100.0,
		0,
		0,
		0,
		0,
		0,
		0,
		clearType,
		QList<int> {0, 0, 0, 0, 0, 0},
		0,
		0.0,
		0,
		0,
		0,
		QList<qint64> {},
		0,
		resource_managers::NoteOrderAlgorithm::Normal,
		resource_managers::NoteOrderAlgorithm::Normal,
		resource_managers::DpOptions::Off,
		gameplay_logic::ChartData::Keymode::K7,
		guid,
		QStringLiteral("sha256-%1").arg(clearType),
		QStringLiteral("md5-%1").arg(clearType));
	return new gameplay_logic::BmsScore(
		std::move(result),
		std::make_unique<gameplay_logic::BmsReplayData>(QList<gameplay_logic::HitEvent> {}, guid),
		std::make_unique<gameplay_logic::BmsGaugeHistory>(QList<gameplay_logic::BmsGaugeInfo> {}, guid),
		parent);
}

int summaryLampFor(const QString& clearType, bool useBeatorajaSemantics) {
	Lr2SelectDetailState state;
	auto* score = scoreWithClearType(clearType, &state);
	state.refresh(QStringLiteral("key-%1-%2").arg(clearType).arg(useBeatorajaSemantics),
				  0,
				  0,
				  QVariantMap {},
				  QVariantMap {},
				  QVariantList {QVariant::fromValue(score)},
				  0,
				  QVariantList {},
				  QVariantList {},
				  QVariantList {},
				  useBeatorajaSemantics,
				  true);
	return state.summary()->lamp();
}

} // namespace

TEST_CASE("LR2 select detail state uses exact Beatoraja clear lamp ids",
		  "[lr2][runtime][select]")
{
	CHECK(summaryLampFor(QStringLiteral("NOPLAY"), true) == 0);
	CHECK(summaryLampFor(QStringLiteral("FAILED"), true) == 1);
	CHECK(summaryLampFor(QStringLiteral("AEASY"), true) == 2);
	CHECK(summaryLampFor(QStringLiteral("LIGHTASSIST"), true) == 3);
	CHECK(summaryLampFor(QStringLiteral("EASY"), true) == 4);
	CHECK(summaryLampFor(QStringLiteral("NORMAL"), true) == 5);
	CHECK(summaryLampFor(QStringLiteral("HARD"), true) == 6);
	CHECK(summaryLampFor(QStringLiteral("EXHARD"), true) == 7);
	CHECK(summaryLampFor(QStringLiteral("EXHARDDAN"), true) == 7);
	CHECK(summaryLampFor(QStringLiteral("FC"), true) == 8);
	CHECK(summaryLampFor(QStringLiteral("PERFECT"), true) == 9);
	CHECK(summaryLampFor(QStringLiteral("MAX"), true) == 10);
}

TEST_CASE("LR2 select detail state keeps LR2 clear lamp buckets",
		  "[lr2][runtime][select]")
{
	CHECK(summaryLampFor(QStringLiteral("AEASY"), false) == 1);
	CHECK(summaryLampFor(QStringLiteral("LIGHTASSIST"), false) == 1);
	CHECK(summaryLampFor(QStringLiteral("EASY"), false) == 2);
	CHECK(summaryLampFor(QStringLiteral("NORMAL"), false) == 3);
	CHECK(summaryLampFor(QStringLiteral("HARD"), false) == 4);
	CHECK(summaryLampFor(QStringLiteral("EXHARD"), false) == 4);
	CHECK(summaryLampFor(QStringLiteral("FC"), false) == 5);
	CHECK(summaryLampFor(QStringLiteral("PERFECT"), false) == 5);
	CHECK(summaryLampFor(QStringLiteral("MAX"), false) == 5);
}

TEST_CASE("LR2 select detail state fills selected difficulty lamp from summary",
		  "[lr2][runtime][select]")
{
	Lr2SelectDetailState state;
	auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

	state.refresh(QStringLiteral("selected-difficulty-lamp"),
				  0,
				  0,
				  QVariantMap {},
				  QVariantMap {},
				  QVariantList {QVariant::fromValue(score)},
				  3,
				  QVariantList {0, 0, 0, 1, 0, 0},
				  QVariantList {0, 0, 0, 10, 0, 0},
				  QVariantList {0, 0, 0, 0, 0, 0},
				  true,
				  true);

	CHECK(state.difficultyModel()->lampForDifficulty(3) == 4);
	const QVariantList optionIds = state.difficultyModel()->optionIdsForKeymode(7, true);
	CHECK(optionIds.contains(507));
	CHECK(optionIds.contains(72));
	CHECK(optionIds.contains(512));
	CHECK(optionIds.contains(544));
}

TEST_CASE("LR2 select detail state can skip selected difficulty lamp",
		  "[lr2][runtime][select]")
{
	Lr2SelectDetailState state;
	auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

	state.refresh(QStringLiteral("selected-difficulty-lamp-skipped"),
				  0,
				  0,
				  QVariantMap {},
				  QVariantMap {},
				  QVariantList {QVariant::fromValue(score)},
				  3,
				  QVariantList {0, 0, 0, 1, 0, 0},
				  QVariantList {0, 0, 0, 10, 0, 0},
				  QVariantList {},
				  true,
				  true);

	CHECK(state.difficultyModel()->lampForDifficulty(3) == 0);
}

TEST_CASE("LR2 select detail state can skip score option ids",
		  "[lr2][runtime][select]")
{
	Lr2SelectDetailState state;
	auto* score = scoreWithClearType(QStringLiteral("EASY"), &state);

	state.refresh(QStringLiteral("score-option-skip"),
				  0,
				  0,
				  QVariantMap {},
				  QVariantMap {},
				  QVariantList {QVariant::fromValue(score)},
				  0,
				  QVariantList {},
				  QVariantList {},
				  QVariantList {},
				  true,
				  false);

	CHECK(state.summary()->lamp() == 4);
	CHECK(state.scoreOptionIds().toList().isEmpty());
}
