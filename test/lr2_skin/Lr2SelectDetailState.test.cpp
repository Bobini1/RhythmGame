#include "Lr2SelectDetailState.h"

#include <catch2/catch_test_macros.hpp>

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace {

QVariantMap scoreWithClearType(const QString& clearType) {
	return {
		{QStringLiteral("result"), QVariantMap {
			{QStringLiteral("clearType"), clearType},
			{QStringLiteral("points"), 0},
			{QStringLiteral("maxPoints"), 100},
			{QStringLiteral("judgementCounts"), QVariantList {}},
		}},
	};
}

int summaryLampFor(const QString& clearType, bool useBeatorajaSemantics) {
	Lr2SelectDetailState state;
	state.refresh(QStringLiteral("key-%1-%2").arg(clearType).arg(useBeatorajaSemantics),
				  0,
				  0,
				  QVariantMap {},
				  QVariantMap {},
				  QVariantList {scoreWithClearType(clearType)},
				  QVariantList {},
				  QVariantList {},
				  QVariantList {},
				  QVariantList {},
				  useBeatorajaSemantics);
	return state.summary().toMap().value(QStringLiteral("lamp")).toInt();
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
