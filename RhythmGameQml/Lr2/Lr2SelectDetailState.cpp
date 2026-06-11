#include "Lr2SelectDetailState.h"

#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/Judgement.h"

#include <QList>
#include <QObject>
#include <QStringView>
#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>

namespace {

QVariant valueAt(const QVariantList& list, int index) {
	return index >= 0 && index < list.size() ? list.at(index) : QVariant {};
}

int jsArrayLength(const QJSValue& value) {
	return std::max(0, value.property(QStringLiteral("length")).toInt());
}

QVariant jsValueVariant(const QJSValue& value) {
	if (QObject* object = value.toQObject()) {
		return QVariant::fromValue(object);
	}
	return value.toVariant();
}

QVariant optionalJsValueVariant(const QJSValue& value) {
	return value.isNull() || value.isUndefined() ? QVariant {} : jsValueVariant(value);
}

int jsIntAt(const QJSValue& list, int length, int index) {
	if (index < 0 || index >= length) {
		return 0;
	}
	return list.property(static_cast<quint32>(index)).toInt();
}

const Lr2SelectScoreSummaryData& emptyScoreSummaryData() {
	static const Lr2SelectScoreSummaryData emptySummary;
	return emptySummary;
}

Lr2SelectScoreSummaryCacheKey scoreSummaryCacheKeyForIdentifier(const QString& identifier,
																int scoreGeneration) {
	return {
		.identifier = identifier,
		.scoreGeneration = scoreGeneration,
	};
}

bool isValidScoreSummaryCacheKey(const Lr2SelectScoreSummaryCacheKey& cacheKey) {
	return !cacheKey.identifier.isEmpty();
}

enum class ClearKind {
	Noplay,
	Failed,
	AssistEasy,
	LightAssist,
	Easy,
	Normal,
	Hard,
	ExHard,
	ExHardDan,
	FullCombo,
	Perfect,
	Max,
};

ClearKind normalizedClearKindValue(QStringView value) {
	if (value.isEmpty() || value == QStringLiteral("NOPLAY")) return ClearKind::Noplay;
	if (value == QStringLiteral("FAILED")) return ClearKind::Failed;
	if (value == QStringLiteral("AEASY")
		|| value == QStringLiteral("ASSIST")
		|| value == QStringLiteral("ASSISTEASY")
		|| value == QStringLiteral("ASSIST_EASY")) {
		return ClearKind::AssistEasy;
	}
	if (value == QStringLiteral("LIGHTASSIST")
		|| value == QStringLiteral("LIGHT_ASSIST")
		|| value == QStringLiteral("LIGHTASSISTEASY")
		|| value == QStringLiteral("LIGHT_ASSIST_EASY")) {
		return ClearKind::LightAssist;
	}
	if (value == QStringLiteral("EASY")) return ClearKind::Easy;
	if (value == QStringLiteral("DAN")) return ClearKind::Normal;
	if (value == QStringLiteral("NORMAL")) return ClearKind::Normal;
	if (value == QStringLiteral("EXDAN")
			|| value == QStringLiteral("HARD_DAN")
			|| value == QStringLiteral("HARD DAN")) return ClearKind::Hard;
	if (value == QStringLiteral("HARD")) return ClearKind::Hard;
	if (value == QStringLiteral("EX_HARD") || value == QStringLiteral("EXHARD")) return ClearKind::ExHard;
	if (value == QStringLiteral("EX_HARD_DAN")
			|| value == QStringLiteral("EXHARD_DAN")
			|| value == QStringLiteral("EXHARDDAN")) return ClearKind::ExHard;
	if (value == QStringLiteral("FC")) return ClearKind::FullCombo;
	if (value == QStringLiteral("PERFECT")) return ClearKind::Perfect;
	if (value == QStringLiteral("MAX")) return ClearKind::Max;
	return ClearKind::Noplay;
}

ClearKind normalizedClearKind(const QString& clearType) {
	const QStringView trimmed = QStringView(clearType).trimmed();
	ClearKind value = normalizedClearKindValue(trimmed);
	if (value != ClearKind::Noplay || trimmed.isEmpty() || trimmed == QStringLiteral("NOPLAY")) {
		return value;
	}
	return normalizedClearKindValue(trimmed.toString().toUpper());
}

ClearKind compatibleClearKind(const QString& clearType, bool useBeatorajaSemantics) {
	const ClearKind value = normalizedClearKind(clearType);
	if (useBeatorajaSemantics) {
		return value;
	}
	if (value == ClearKind::AssistEasy || value == ClearKind::LightAssist) {
		return ClearKind::Failed;
	}
	if (value == ClearKind::ExHard || value == ClearKind::ExHardDan) {
		return ClearKind::Hard;
	}
	return value;
}

QString clearKindString(ClearKind kind) {
	switch (kind) {
	case ClearKind::Failed: return QStringLiteral("FAILED");
	case ClearKind::AssistEasy: return QStringLiteral("AEASY");
	case ClearKind::LightAssist: return QStringLiteral("LIGHTASSIST");
	case ClearKind::Easy: return QStringLiteral("EASY");
	case ClearKind::Normal: return QStringLiteral("NORMAL");
	case ClearKind::Hard: return QStringLiteral("HARD");
	case ClearKind::ExHard: return QStringLiteral("EXHARD");
	case ClearKind::ExHardDan: return QStringLiteral("EXHARDDAN");
	case ClearKind::FullCombo: return QStringLiteral("FC");
	case ClearKind::Perfect: return QStringLiteral("PERFECT");
	case ClearKind::Max: return QStringLiteral("MAX");
	default: return QStringLiteral("NOPLAY");
	}
}

gameplay_logic::BmsScore* scoreFromVariant(const QVariant& value) {
	if (auto* score = value.value<gameplay_logic::BmsScore*>()) {
		return score;
	}
	if (auto* object = value.value<QObject*>()) {
		return qobject_cast<gameplay_logic::BmsScore*>(object);
	}
	return nullptr;
}

gameplay_logic::BmsScoreCourse* courseScoreFromVariant(const QVariant& value) {
	if (auto* score = value.value<gameplay_logic::BmsScoreCourse*>()) {
		return score;
	}
	if (auto* object = value.value<QObject*>()) {
		return qobject_cast<gameplay_logic::BmsScoreCourse*>(object);
	}
	return nullptr;
}

gameplay_logic::ChartData* chartDataFromVariant(const QVariant& value) {
	if (auto* chart = value.value<gameplay_logic::ChartData*>()) {
		return chart;
	}
	if (auto* object = value.value<QObject*>()) {
		return qobject_cast<gameplay_logic::ChartData*>(object);
	}
	return nullptr;
}

QVariant scoreVariant(QObject* scoreObject) {
	return scoreObject ? QVariant::fromValue(scoreObject) : QVariant {};
}

int clearKindPriority(ClearKind kind) {
	switch (kind) {
	case ClearKind::Failed: return 1;
	case ClearKind::AssistEasy: return 2;
	case ClearKind::LightAssist: return 3;
	case ClearKind::Easy: return 4;
	case ClearKind::Normal: return 5;
	case ClearKind::Hard: return 6;
	case ClearKind::ExHard:
	case ClearKind::ExHardDan: return 7;
	case ClearKind::FullCombo: return 8;
	case ClearKind::Perfect: return 9;
	case ClearKind::Max: return 10;
	default: return 0;
	}
}

int clearKindLamp(ClearKind kind, bool useBeatorajaSemantics) {
	if (useBeatorajaSemantics) {
		switch (kind) {
		case ClearKind::Failed: return 1;
		case ClearKind::AssistEasy: return 2;
		case ClearKind::LightAssist: return 3;
		case ClearKind::Easy: return 4;
		case ClearKind::Normal: return 5;
		case ClearKind::Hard: return 6;
		case ClearKind::ExHard:
		case ClearKind::ExHardDan: return 7;
		case ClearKind::FullCombo: return 8;
		case ClearKind::Perfect: return 9;
		case ClearKind::Max: return 10;
		default: break;
		}
		return 0;
	}
	switch (kind) {
	case ClearKind::Failed: return 1;
	case ClearKind::AssistEasy:
	case ClearKind::LightAssist:
	case ClearKind::Easy: return 2;
	case ClearKind::Normal: return 3;
	case ClearKind::Hard:
	case ClearKind::ExHard:
	case ClearKind::ExHardDan: return 4;
	case ClearKind::FullCombo:
	case ClearKind::Perfect:
	case ClearKind::Max: return 5;
	default: break;
	}
	return 0;
}

int rankForScoreRate(double rate) {
	if (rate >= 1.0) return 9;
	if (rate >= 8.0 / 9.0) return 8;
	if (rate >= 7.0 / 9.0) return 7;
	if (rate >= 6.0 / 9.0) return 6;
	if (rate >= 5.0 / 9.0) return 5;
	if (rate >= 4.0 / 9.0) return 4;
	if (rate >= 3.0 / 9.0) return 3;
	if (rate >= 2.0 / 9.0) return 2;
	return rate > 0.0 ? 1 : 0;
}

int percentInteger(double value, double total) {
	return total > 0.0
		? static_cast<int>(std::floor(std::max(0.0, value) * 100.0 / total))
		: 0;
}

int percentAfterDot(double value, double total) {
	return total > 0.0
		? static_cast<int>(std::floor(std::max(0.0, value) * 10000.0 / total)) % 100
		: 0;
}

double normalized(double value, double total) {
	return total > 0.0 ? value / total : 0.0;
}

int chartPlayableNoteCount(gameplay_logic::ChartData* chart) {
	return chart
		? chart->getNormalNoteCount()
			+ chart->getScratchCount()
			+ chart->getLnCount()
			+ chart->getBssCount()
		: 0;
}

int chartLengthSeconds(gameplay_logic::ChartData* chart) {
	if (!chart) {
		return -1;
	}
	return static_cast<int>(std::max<qint64>(0, chart->getLength()) / 1000000000);
}

int chartKeymode(gameplay_logic::ChartData* chart) {
	return chart ? static_cast<int>(chart->getKeymode()) : 0;
}

int levelBarFlashThreshold(gameplay_logic::ChartData* chart) {
	const int keymode = chartKeymode(chart);
	if (keymode == 5 || keymode == 10) {
		return 9;
	}
	return 12;
}

int chartDensityValue(gameplay_logic::ChartData* chart, double density, bool afterDot) {
	if (!chart) {
		return -1;
	}
	density = std::max(0.0, density);
	return afterDot
		? static_cast<int>(std::floor(density * 100.0)) % 100
		: static_cast<int>(std::floor(density));
}

int scorePrintValue(const Lr2SelectScoreStatsData* stats, gameplay_logic::ChartData* chart) {
	if (!stats || !stats->valid) {
		return 0;
	}
	int totalNotes = chartPlayableNoteCount(chart);
	if (totalNotes <= 0) {
		totalNotes = static_cast<int>(std::floor(stats->maxPoints / 2.0));
	}
	if (totalNotes <= 0) {
		return 0;
	}
	const int keymode = chartKeymode(chart);
	if (keymode == 5 || keymode == 10) {
		return static_cast<int>(std::floor((100000.0 * stats->pg
											+ 100000.0 * stats->gr
											+ 50000.0 * stats->gd) / totalNotes));
	}
	if (keymode == 7 || keymode == 14) {
		return static_cast<int>(std::floor((150000.0 * stats->pg
											+ 100000.0 * stats->gr
											+ 20000.0 * stats->gd) / totalNotes))
			+ static_cast<int>(std::floor(50000.0 * stats->maxCombo / totalNotes));
	}
	return static_cast<int>(std::floor((1000000.0 * stats->pg
										+ 700000.0 * stats->gr
										+ 400000.0 * stats->gd) / totalNotes));
}

int nextRankDelta(const Lr2SelectScoreStatsData* stats, gameplay_logic::ChartData* chart) {
	if (!stats || !stats->valid) {
		return 0;
	}
	const double scoreMaxPoints = stats->maxPoints;
	double targetMaxPoints = chartPlayableNoteCount(chart) * 2.0;
	if (targetMaxPoints <= 0.0) {
		targetMaxPoints = scoreMaxPoints;
	}
	if (scoreMaxPoints <= 0.0 || targetMaxPoints <= 0.0) {
		return 0;
	}
	const double rate = std::max(0.0, stats->exscore) / scoreMaxPoints;
	for (int rank = 0; rank < 27; ++rank) {
		const bool qualified = rate >= rank / 27.0;
		if (rank % 3 == 0 && !qualified) {
			return static_cast<int>(std::llround(rank * targetMaxPoints / 27.0
												 - rate * targetMaxPoints));
		}
	}
	return static_cast<int>(std::llround(targetMaxPoints - rate * targetMaxPoints));
}

QVariantMap rankingEntryMap(const QVariant& value) {
	if (value.canConvert<QVariantMap>()) {
		return value.toMap();
	}
	return {};
}

double mapNumber(const QVariantMap& map, const QString& key) {
	return std::max(0.0, map.value(key).toDouble());
}

std::optional<Lr2SelectScoreStatsData> rankingScoreStatsFromMap(const QVariantMap& map) {
	if (!map.value(QStringLiteral("__lr2RankingEntry")).toBool()) {
		return std::nullopt;
	}

	const int pg = static_cast<int>(mapNumber(map, QStringLiteral("pg")));
	const int gr = static_cast<int>(mapNumber(map, QStringLiteral("gr")));
	const int gd = static_cast<int>(mapNumber(map, QStringLiteral("gd")));
	const int bd = static_cast<int>(mapNumber(map, QStringLiteral("bd")));
	const int poor = static_cast<int>(mapNumber(map, QStringLiteral("poor")));
	const int miss = static_cast<int>(mapNumber(map, QStringLiteral("miss")));
	const int pr = poor + miss;
	const double maxPoints = mapNumber(map, QStringLiteral("maxPoints"));
	int totalJudgements = pg + gr + gd + bd + pr;
	if (totalJudgements <= 0 && maxPoints > 0.0) {
		totalJudgements = static_cast<int>(std::floor(maxPoints / 2.0));
	}
	const int bestComboBreaks = static_cast<int>(mapNumber(map, QStringLiteral("bestComboBreaks")));
	const int computedBadPoor = bd + pr;
	const int badPoor = computedBadPoor > 0 ? computedBadPoor : bestComboBreaks;
	int comboBreak = bd + poor;
	if (comboBreak <= 0 && computedBadPoor <= 0) {
		comboBreak = bestComboBreaks;
	}
	const double bestPoints = mapNumber(map, QStringLiteral("bestPoints"));
	return Lr2SelectScoreStatsData {
		.valid = true,
		.pg = pg,
		.gr = gr,
		.gd = gd,
		.bd = bd,
		.poor = poor,
		.miss = miss,
		.pr = pr,
		.totalJudgements = std::max(1, totalJudgements),
		.comboBreak = comboBreak,
		.badPoor = badPoor,
		.maxCombo = static_cast<int>(mapNumber(map, QStringLiteral("bestCombo"))),
		.score = bestPoints,
		.exscore = bestPoints,
		.maxPoints = maxPoints,
		.early = {},
		.late = {},
		.totalEarly = 0,
		.totalLate = 0,
	};
}

std::optional<Lr2SelectScoreStatsData> rankingScoreStatsFromVariant(const QVariant& value) {
	return rankingScoreStatsFromMap(rankingEntryMap(value));
}

int judgementCount(const QList<int>& counts, gameplay_logic::Judgement judgement) {
	const int index = static_cast<int>(judgement);
	return index >= 0 && index < counts.size() ? counts.at(index) : 0;
}

int badPoorForCounts(const QList<int>& counts) {
	return judgementCount(counts, gameplay_logic::Judgement::Bad)
		+ judgementCount(counts, gameplay_logic::Judgement::Poor)
		+ judgementCount(counts, gameplay_logic::Judgement::EmptyPoor);
}

Lr2SelectScoreStatsData statsForValues(const QList<int>& counts,
									   int maxCombo,
									   double points,
									   double maxPoints) {
	const int pg = judgementCount(counts, gameplay_logic::Judgement::Perfect);
	const int gr = judgementCount(counts, gameplay_logic::Judgement::Great);
	const int gd = judgementCount(counts, gameplay_logic::Judgement::Good);
	const int bd = judgementCount(counts, gameplay_logic::Judgement::Bad);
	const int poor = judgementCount(counts, gameplay_logic::Judgement::Poor);
	const int miss = judgementCount(counts, gameplay_logic::Judgement::EmptyPoor);
	const int pr = poor + miss;
	return {
		.valid = true,
		.pg = pg,
		.gr = gr,
		.gd = gd,
		.bd = bd,
		.poor = poor,
		.miss = miss,
		.pr = pr,
		.totalJudgements = std::max(1, pg + gr + gd + bd + pr),
		.comboBreak = bd + poor,
		.badPoor = bd + pr,
		.maxCombo = maxCombo,
		.score = points,
		.exscore = points,
		.maxPoints = maxPoints,
	};
}

class ScoreOptionIds {
public:
	void insert(int id) {
		if (!m_ids.contains(id)) {
			m_ids.append(id);
		}
	}

	QVariantList toSortedVariantList() const {
		QList<int> sorted = m_ids;
		std::sort(sorted.begin(), sorted.end());
		QVariantList result;
		result.reserve(sorted.size());
		for (int id : sorted) {
			result.append(id);
		}
		return result;
	}

private:
	QList<int> m_ids;
};

void appendScoreClearOptionIds(ClearKind kind, ScoreOptionIds& ids) {
	// Historical score flags are beatoraja trophy options. The exact
	// selected-bar clear options are added from the current summary only.
	if (kind == ClearKind::AssistEasy || kind == ClearKind::LightAssist) {
		ids.insert(124);
	} else if (kind == ClearKind::Easy) {
		ids.insert(121);
	} else if (kind == ClearKind::Normal) {
		ids.insert(118);
	} else if (kind == ClearKind::Hard) {
		ids.insert(119);
	} else if (kind == ClearKind::ExHard || kind == ClearKind::ExHardDan) {
		ids.insert(125);
	}
}

void appendScoreOptionIdsForValues(int noteOrderAlgorithm, int dpOptions, int keymode, ScoreOptionIds& ids) {
	switch (noteOrderAlgorithm) {
	case 1:
		ids.insert(127);
		break;
	case 2:
	case 3:
		ids.insert(128);
		break;
	case 4:
	case 5:
		ids.insert(129);
		break;
	case 6:
		ids.insert(1128);
		ids.insert(130);
		break;
	default:
		ids.insert(126);
		break;
	}

	if (dpOptions == 2) {
		ids.insert(keymode == 5 || keymode == 7 ? 145 : 144);
	}
}

template<typename ForEachScore>
Lr2SelectScoreSummaryData buildScoreSummaryDataForEach(bool useBeatorajaSemantics,
													   bool buildScoreOptionIds,
													   ForEachScore forEachScore) {
	ScoreOptionIds optionIds;
	QVariant bestScore;
	Lr2SelectScoreStatsData bestStatsValue;
	double bestRate = -1.0;
	bool bestHasMaxPoints = false;
	QString bestClearType = QStringLiteral("NOPLAY");
	ClearKind bestClearKind = ClearKind::Noplay;
	int bestClearPriority = 0;
	int minBadPoor = -1;

	int play = 0;
	int clear = 0;
	int fail = 0;
	int noplay = 0;
	int assist = 0;
	int lightAssist = 0;
	int easy = 0;
	int normal = 0;
	int hard = 0;
	int exhard = 0;
	int fc = 0;
	int perfect = 0;
	int max = 0;

	auto processScore = [&](const QString& clearType,
							const QList<int>& judgementCounts,
							int maxCombo,
							double maxPoints,
							double points,
							int noteOrderAlgorithm,
							int dpOptions,
							int keymode,
							auto scoreValue) {
		++play;
		const ClearKind clearKind = compatibleClearKind(clearType, useBeatorajaSemantics);
		if (buildScoreOptionIds) {
			appendScoreClearOptionIds(clearKind, optionIds);
			appendScoreOptionIdsForValues(noteOrderAlgorithm, dpOptions, keymode, optionIds);
		}

		const int priority = clearKindPriority(clearKind);
		if (priority > bestClearPriority) {
			bestClearPriority = priority;
			bestClearKind = clearKind;
			bestClearType = clearKindString(clearKind);
		}

		if (clearKind != ClearKind::Failed && clearKind != ClearKind::Noplay) {
			++clear;
		}
		switch (clearKind) {
		case ClearKind::Failed: ++fail; break;
		case ClearKind::AssistEasy: ++assist; break;
		case ClearKind::LightAssist: ++lightAssist; break;
		case ClearKind::Easy: ++easy; break;
		case ClearKind::Normal: ++normal; break;
		case ClearKind::Hard: ++hard; break;
		case ClearKind::ExHard:
		case ClearKind::ExHardDan: ++exhard; break;
		case ClearKind::FullCombo: ++fc; break;
		case ClearKind::Perfect: ++perfect; break;
		case ClearKind::Max: ++max; break;
		default: ++noplay; break;
		}

		const int badPoor = badPoorForCounts(judgementCounts);
		minBadPoor = minBadPoor < 0 ? badPoor : std::min(minBadPoor, badPoor);

		const bool hasMaxPoints = maxPoints > 0.0;
		const double rate = hasMaxPoints ? points / maxPoints : 0.0;
		if (rate < bestRate) {
			return;
		}
		if (qFuzzyCompare(rate + 1.0, bestRate + 1.0)
				&& (bestHasMaxPoints || !hasMaxPoints)) {
			return;
		}
		bestRate = rate;
		bestHasMaxPoints = hasMaxPoints;
		bestScore = scoreValue();
		bestStatsValue = statsForValues(judgementCounts, maxCombo, points, maxPoints);
	};

	forEachScore(processScore);

	const double scoreRate = std::max(0.0, bestRate);
	const Lr2SelectScoreCountsData counts {
		.play = play,
		.clear = clear,
		.fail = fail,
		.noplay = noplay,
		.assist = assist,
		.lightAssist = lightAssist,
		.easy = easy,
		.normal = normal,
		.hard = hard,
		.exhard = exhard,
		.fc = fc,
		.perfect = perfect,
		.max = max,
		.minBadPoor = std::max(0, minBadPoor),
	};
	return {
		bestScore,
		bestStatsValue,
		counts,
		bestClearType,
		clearKindLamp(bestClearKind, useBeatorajaSemantics),
		bestScore.isValid() && bestClearKind != ClearKind::Noplay
			? std::max(1, rankForScoreRate(scoreRate))
			: 0,
		scoreRate,
		buildScoreOptionIds ? optionIds.toSortedVariantList() : QVariantList {},
	};
}

Lr2SelectScoreSummaryData buildScoreSummaryData(const QVariantList& scoreList,
												bool useBeatorajaSemantics,
												bool buildScoreOptionIds) {
	return buildScoreSummaryDataForEach(
		useBeatorajaSemantics,
		buildScoreOptionIds,
		[&](const auto& processScore) {
	for (const QVariant& scoreValue : scoreList) {
		if (auto* scoreObject = scoreFromVariant(scoreValue)) {
			const gameplay_logic::BmsResult* result = scoreObject->getResult();
			if (!result) {
				continue;
			}
			processScore(result->getClearType(),
                         result->getJudgementCounts(),
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [&]() { return scoreValue; });
			continue;
		}

		if (auto* scoreCourse = courseScoreFromVariant(scoreValue)) {
			const gameplay_logic::BmsResultCourse* result = scoreCourse->getResult();
			if (!result) {
				continue;
			}
			const QList<int> judgementCounts = result->getJudgementCounts();
			processScore(result->getClearType(),
						 judgementCounts,
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [&]() { return scoreValue; });
			continue;
		}
	}
		});
}

Lr2SelectScoreSummaryData buildScoreSummaryDataFromJsValue(const QJSValue& scoreList,
														   bool useBeatorajaSemantics,
														   bool buildScoreOptionIds) {
	return buildScoreSummaryDataForEach(
		useBeatorajaSemantics,
		buildScoreOptionIds,
		[&](const auto& processScore) {
	for (int i = 0, length = jsArrayLength(scoreList); i < length; ++i) {
		const QJSValue scoreValue = scoreList.property(static_cast<quint32>(i));
		QObject* scoreObjectValue = scoreValue.toQObject();
		if (auto* scoreObject = qobject_cast<gameplay_logic::BmsScore*>(scoreObjectValue)) {
			const gameplay_logic::BmsResult* result = scoreObject->getResult();
			if (!result) {
				continue;
			}
			processScore(result->getClearType(),
                         result->getJudgementCounts(),
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [scoreObject]() { return scoreVariant(scoreObject); });
			continue;
		}

		if (auto* scoreCourse = qobject_cast<gameplay_logic::BmsScoreCourse*>(scoreObjectValue)) {
			const gameplay_logic::BmsResultCourse* result = scoreCourse->getResult();
			if (!result) {
				continue;
			}
			const QList<int> judgementCounts = result->getJudgementCounts();
			processScore(result->getClearType(),
						 judgementCounts,
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [scoreCourse]() { return scoreVariant(scoreCourse); });
			continue;
		}

		const QVariant variantScoreValue = scoreValue.toVariant();
		if (auto* scoreObject = scoreFromVariant(variantScoreValue)) {
			const gameplay_logic::BmsResult* result = scoreObject->getResult();
			if (!result) {
				continue;
			}
			processScore(result->getClearType(),
                         result->getJudgementCounts(),
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [&]() { return variantScoreValue; });
			continue;
		}

		if (auto* scoreCourse = courseScoreFromVariant(variantScoreValue)) {
			const gameplay_logic::BmsResultCourse* result = scoreCourse->getResult();
			if (!result) {
				continue;
			}
			const QList<int> judgementCounts = result->getJudgementCounts();
			processScore(result->getClearType(),
						 judgementCounts,
						 result->getMaxCombo(),
						 result->getMaxPoints(),
						 result->getPoints(),
						 static_cast<int>(result->getNoteOrderAlgorithm()),
						 static_cast<int>(result->getDpOptions()),
						 static_cast<int>(result->getKeymode()),
						 [&]() { return variantScoreValue; });
		}
	}
		});
}

} // namespace

bool Lr2SelectScoreStatsData::operator==(const Lr2SelectScoreStatsData& other) const {
	return valid == other.valid
		&& pg == other.pg
		&& gr == other.gr
		&& gd == other.gd
		&& bd == other.bd
		&& poor == other.poor
		&& miss == other.miss
		&& pr == other.pr
		&& totalJudgements == other.totalJudgements
		&& comboBreak == other.comboBreak
		&& badPoor == other.badPoor
		&& maxCombo == other.maxCombo
		&& qFuzzyCompare(score + 1.0, other.score + 1.0)
		&& qFuzzyCompare(exscore + 1.0, other.exscore + 1.0)
		&& qFuzzyCompare(maxPoints + 1.0, other.maxPoints + 1.0)
		&& early == other.early
		&& late == other.late
		&& totalEarly == other.totalEarly
		&& totalLate == other.totalLate;
}

Lr2SelectScoreCounts::Lr2SelectScoreCounts(QObject* parent)
	: QObject(parent) {}

int Lr2SelectScoreCounts::play() const { return m_play; }
int Lr2SelectScoreCounts::clear() const { return m_clear; }
int Lr2SelectScoreCounts::fail() const { return m_fail; }
int Lr2SelectScoreCounts::noplay() const { return m_noplay; }
int Lr2SelectScoreCounts::assist() const { return m_assist; }
int Lr2SelectScoreCounts::lightAssist() const { return m_lightAssist; }
int Lr2SelectScoreCounts::easy() const { return m_easy; }
int Lr2SelectScoreCounts::normal() const { return m_normal; }
int Lr2SelectScoreCounts::hard() const { return m_hard; }
int Lr2SelectScoreCounts::exhard() const { return m_exhard; }
int Lr2SelectScoreCounts::fc() const { return m_fc; }
int Lr2SelectScoreCounts::perfect() const { return m_perfect; }
int Lr2SelectScoreCounts::max() const { return m_max; }
int Lr2SelectScoreCounts::minBadPoor() const { return m_minBadPoor; }

bool Lr2SelectScoreCounts::hasValues() const {
	return m_play != 0
		|| m_clear != 0
		|| m_fail != 0
		|| m_noplay != 0
		|| m_assist != 0
		|| m_lightAssist != 0
		|| m_easy != 0
		|| m_normal != 0
		|| m_hard != 0
		|| m_exhard != 0
		|| m_fc != 0
		|| m_perfect != 0
		|| m_max != 0
		|| m_minBadPoor != 0;
}

bool Lr2SelectScoreCounts::setValues(const Lr2SelectScoreCountsData& values) {
	if (m_play == values.play
			&& m_clear == values.clear
			&& m_fail == values.fail
			&& m_noplay == values.noplay
			&& m_assist == values.assist
			&& m_lightAssist == values.lightAssist
			&& m_easy == values.easy
			&& m_normal == values.normal
			&& m_hard == values.hard
			&& m_exhard == values.exhard
			&& m_fc == values.fc
			&& m_perfect == values.perfect
			&& m_max == values.max
			&& m_minBadPoor == values.minBadPoor) {
		return false;
	}

	m_play = values.play;
	m_clear = values.clear;
	m_fail = values.fail;
	m_noplay = values.noplay;
	m_assist = values.assist;
	m_lightAssist = values.lightAssist;
	m_easy = values.easy;
	m_normal = values.normal;
	m_hard = values.hard;
	m_exhard = values.exhard;
	m_fc = values.fc;
	m_perfect = values.perfect;
	m_max = values.max;
	m_minBadPoor = values.minBadPoor;
	emit changed();
	return true;
}

void Lr2SelectScoreCounts::clearValues() {
	setValues(Lr2SelectScoreCountsData {});
}

Lr2SelectScoreStats::Lr2SelectScoreStats(QObject* parent)
	: QObject(parent) {}

bool Lr2SelectScoreStats::hasValues() const { return m_values.valid; }
int Lr2SelectScoreStats::pg() const { return m_values.pg; }
int Lr2SelectScoreStats::gr() const { return m_values.gr; }
int Lr2SelectScoreStats::gd() const { return m_values.gd; }
int Lr2SelectScoreStats::bd() const { return m_values.bd; }
int Lr2SelectScoreStats::poor() const { return m_values.poor; }
int Lr2SelectScoreStats::miss() const { return m_values.miss; }
int Lr2SelectScoreStats::pr() const { return m_values.pr; }
int Lr2SelectScoreStats::totalJudgements() const { return m_values.totalJudgements; }
int Lr2SelectScoreStats::comboBreak() const { return m_values.comboBreak; }
int Lr2SelectScoreStats::badPoor() const { return m_values.badPoor; }
int Lr2SelectScoreStats::maxCombo() const { return m_values.maxCombo; }
double Lr2SelectScoreStats::score() const { return m_values.score; }
double Lr2SelectScoreStats::exscore() const { return m_values.exscore; }
double Lr2SelectScoreStats::maxPoints() const { return m_values.maxPoints; }
QVariant Lr2SelectScoreStats::early() const { return m_values.early; }
QVariant Lr2SelectScoreStats::late() const { return m_values.late; }
int Lr2SelectScoreStats::totalEarly() const { return m_values.totalEarly; }
int Lr2SelectScoreStats::totalLate() const { return m_values.totalLate; }

const Lr2SelectScoreStatsData& Lr2SelectScoreStats::values() const {
	return m_values;
}

bool Lr2SelectScoreStats::setValues(const Lr2SelectScoreStatsData& value) {
	if (m_values == value) {
		return false;
	}
	m_values = value;
	emit changed();
	return true;
}

void Lr2SelectScoreStats::clearValues() {
	setValues(Lr2SelectScoreStatsData {});
}

Lr2SelectScoreSummary::Lr2SelectScoreSummary(QObject* parent)
	: QObject(parent)
	, m_bestStats(this)
	, m_scoreCounts(this) {}

QVariant Lr2SelectScoreSummary::bestScore() const { return m_bestScore; }
QObject* Lr2SelectScoreSummary::bestStatsObject() const {
	return m_bestStats.hasValues()
		? const_cast<Lr2SelectScoreStats*>(&m_bestStats)
		: nullptr;
}
Lr2SelectScoreStats* Lr2SelectScoreSummary::bestStatsStats() const {
	return const_cast<Lr2SelectScoreStats*>(&m_bestStats);
}
QObject* Lr2SelectScoreSummary::scoreCountsObject() const { return scoreCounts(); }
Lr2SelectScoreCounts* Lr2SelectScoreSummary::scoreCounts() const {
	return const_cast<Lr2SelectScoreCounts*>(&m_scoreCounts);
}
QString Lr2SelectScoreSummary::clearType() const { return m_clearType; }
int Lr2SelectScoreSummary::lamp() const { return m_lamp; }
int Lr2SelectScoreSummary::rank() const { return m_rank; }
double Lr2SelectScoreSummary::scoreRate() const { return m_scoreRate; }
QVariant Lr2SelectScoreSummary::optionIds() const { return m_optionIds; }

bool Lr2SelectScoreSummary::setValues(const Lr2SelectScoreSummaryData& values) {
	bool didChange = m_scoreCounts.setValues(values.counts);
	if (m_bestScore != values.bestScore) {
		m_bestScore = values.bestScore;
		didChange = true;
	}
	if (m_bestStats.setValues(values.bestStats)) {
		didChange = true;
	}
	if (m_clearType != values.clearType) {
		m_clearType = values.clearType;
		didChange = true;
	}
	if (m_lamp != values.lamp) {
		m_lamp = values.lamp;
		didChange = true;
	}
	if (m_rank != values.rank) {
		m_rank = values.rank;
		didChange = true;
	}
	if (!qFuzzyCompare(m_scoreRate + 1.0, values.scoreRate + 1.0)) {
		m_scoreRate = values.scoreRate;
		didChange = true;
	}
	if (m_optionIds != values.optionIds) {
		m_optionIds = values.optionIds;
		didChange = true;
	}
	if (didChange) {
		emit changed();
	}
	return didChange;
}

void Lr2SelectScoreSummary::clearValues() {
	setValues(Lr2SelectScoreSummaryData {});
}

Lr2SelectDifficultyModel::Lr2SelectDifficultyModel(QObject* parent)
	: QAbstractListModel(parent) {}

bool Lr2SelectDifficultyModel::Row::operator==(const Row& other) const {
	return difficulty == other.difficulty
		&& count == other.count
		&& playLevel == other.playLevel
		&& lamp == other.lamp;
}

int Lr2SelectDifficultyModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : m_rows.size();
}

QVariant Lr2SelectDifficultyModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
		return {};
	}
	const Row& row = m_rows.at(index.row());
	switch (role) {
	case DifficultyRole: return row.difficulty;
	case CountRole: return row.count;
	case PlayLevelRole: return row.playLevel;
	case LampRole: return row.lamp;
	default: return {};
	}
}

QHash<int, QByteArray> Lr2SelectDifficultyModel::roleNames() const {
	return {
		{DifficultyRole, "difficulty"},
		{CountRole, "count"},
		{PlayLevelRole, "playLevel"},
		{LampRole, "lamp"},
	};
}

int Lr2SelectDifficultyModel::countForDifficulty(int difficulty) const {
	const Row* row = rowForDifficulty(difficulty);
	return row ? row->count : 0;
}

int Lr2SelectDifficultyModel::playLevelForDifficulty(int difficulty) const {
	const Row* row = rowForDifficulty(difficulty);
	return row ? row->playLevel : 0;
}

int Lr2SelectDifficultyModel::lampForDifficulty(int difficulty) const {
	const Row* row = rowForDifficulty(difficulty);
	return row ? row->lamp : 0;
}

QVariantList Lr2SelectDifficultyModel::optionIdsForKeymode(int keymode, bool includeLamps) const {
	const int flashThreshold = keymode == 5 || keymode == 10 ? 9 : 12;
	QVariantList result;
	result.reserve(includeLamps ? 15 : 10);
	for (int difficulty = 1; difficulty <= 5; ++difficulty) {
		const Row* row = rowForDifficulty(difficulty);
		const int count = row ? row->count : 0;
		const int playLevel = row ? row->playLevel : 0;
		if (count > 0) {
			result.append(504 + difficulty);
			result.append(playLevel > flashThreshold ? 74 + difficulty : 69 + difficulty);
		} else {
			result.append(499 + difficulty);
		}

		if (count == 1) {
			result.append(509 + difficulty);
		} else if (count > 1) {
			result.append(514 + difficulty);
		}

		if (includeLamps) {
			const int lamp = row ? row->lamp : 0;
			result.append(510 + difficulty * 10 + lamp);
		}
	}
	return result;
}

bool Lr2SelectDifficultyModel::setRows(const QVariantList& counts,
									   const QVariantList& levels,
									   const QVariantList& lamps,
									   int selectedDifficulty,
									   int selectedLamp) {
	return setRows(rowsFromValues(counts,
								  levels,
								  lamps,
								  selectedDifficulty,
								  selectedLamp));
}

bool Lr2SelectDifficultyModel::setRowsFromJsValues(const QJSValue& counts,
												   const QJSValue& levels,
												   const QJSValue& lamps,
												   int selectedDifficulty,
												   int selectedLamp) {
	return setRows(rowsFromJsValues(counts,
									levels,
									lamps,
									selectedDifficulty,
									selectedLamp));
}

bool Lr2SelectDifficultyModel::setRows(QList<Row> nextRows) {
	if (m_rows == nextRows) {
		return false;
	}

	if (m_rows.size() != nextRows.size()) {
		beginResetModel();
		m_rows = std::move(nextRows);
		endResetModel();
		return true;
	}

	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_rows.size(); ++row) {
		if (m_rows.at(row) == nextRows.at(row)) {
			continue;
		}
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	m_rows = std::move(nextRows);
	if (firstChanged >= 0) {
		static const QList<int> roles {
			DifficultyRole,
			CountRole,
			PlayLevelRole,
			LampRole,
		};
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), roles);
	}
	return true;
}

const Lr2SelectDifficultyModel::Row* Lr2SelectDifficultyModel::rowForDifficulty(int difficulty) const {
	for (const Row& row : m_rows) {
		if (row.difficulty == difficulty) {
			return &row;
		}
	}
	return nullptr;
}

QList<Lr2SelectDifficultyModel::Row> Lr2SelectDifficultyModel::rowsFromValues(const QVariantList& counts,
																			  const QVariantList& levels,
																			  const QVariantList& lamps,
																			  int selectedDifficulty,
																			  int selectedLamp) {
	if (counts.isEmpty() && levels.isEmpty() && lamps.isEmpty()) {
		return {};
	}
	QList<Row> rows;
	rows.reserve(5);
	for (int difficulty = 1; difficulty <= 5; ++difficulty) {
		rows.append(Row {
			difficulty,
			valueAt(counts, difficulty).toInt(),
			valueAt(levels, difficulty).toInt(),
			!lamps.isEmpty() && difficulty == selectedDifficulty
				? selectedLamp
				: valueAt(lamps, difficulty).toInt(),
		});
	}
	return rows;
}

QList<Lr2SelectDifficultyModel::Row> Lr2SelectDifficultyModel::rowsFromJsValues(const QJSValue& counts,
																				const QJSValue& levels,
																				const QJSValue& lamps,
																				int selectedDifficulty,
																				int selectedLamp) {
	const int countCount = jsArrayLength(counts);
	const int levelCount = jsArrayLength(levels);
	const int lampCount = jsArrayLength(lamps);
	if (countCount == 0
			&& levelCount == 0
			&& lampCount == 0) {
		return {};
	}
	QList<Row> rows;
	rows.reserve(5);
	for (int difficulty = 1; difficulty <= 5; ++difficulty) {
		rows.append(Row {
			difficulty,
			jsIntAt(counts, countCount, difficulty),
			jsIntAt(levels, levelCount, difficulty),
			lampCount > 0 && difficulty == selectedDifficulty
				? selectedLamp
				: jsIntAt(lamps, lampCount, difficulty),
		});
	}
	return rows;
}

Lr2SelectDetailState::Lr2SelectDetailState(QObject* parent)
	: QObject(parent)
	, m_summary(this)
	, m_difficultyModel(this) {}

int Lr2SelectDetailState::scoreGeneration() const { return m_scoreGeneration; }
void Lr2SelectDetailState::setScoreGeneration(int value) {
	if (m_scoreGeneration == value) return;
	m_scoreGeneration = value;
	emit scoreGenerationChanged();
}

int Lr2SelectDetailState::listGeneration() const { return m_listGeneration; }
void Lr2SelectDetailState::setListGeneration(int value) {
	if (m_listGeneration == value) return;
	m_listGeneration = value;
	emit listGenerationChanged();
}

QVariant Lr2SelectDetailState::item() const { return m_item; }
void Lr2SelectDetailState::setItem(const QVariant& value) {
	if (m_item == value) return;
	m_item = value;
	emit itemChanged();
}

QVariant Lr2SelectDetailState::chartData() const { return m_chartData; }
void Lr2SelectDetailState::setChartData(const QVariant& value) {
	if (m_chartData == value) return;
	m_chartData = value;
	emit chartDataChanged();
}

QObject* Lr2SelectDetailState::summaryObject() const {
	return summary();
}

Lr2SelectScoreSummary* Lr2SelectDetailState::summary() const {
	return const_cast<Lr2SelectScoreSummary*>(&m_summary);
}

QObject* Lr2SelectDetailState::bestStatsObject() const { return m_summary.bestStatsObject(); }

QObject* Lr2SelectDetailState::scoreCountsObject() const {
	return scoreCounts();
}

Lr2SelectScoreCounts* Lr2SelectDetailState::scoreCounts() const {
	return m_summary.scoreCounts();
}

QVariant Lr2SelectDetailState::scoreOptionIds() const { return m_scoreOptionIds; }
void Lr2SelectDetailState::setScoreOptionIds(const QVariant& value) {
	if (m_scoreOptionIds == value) return;
	m_scoreOptionIds = value;
	emit scoreOptionIdsChanged();
}

Lr2SelectDifficultyModel* Lr2SelectDetailState::difficultyModel() {
	return &m_difficultyModel;
}

bool Lr2SelectDetailState::selectedRefreshMatches(const QString& itemKey,
												  const QString& targetItemKey,
												  bool rankingMode,
												  int scoreGeneration,
												  int listGeneration,
												  bool useBeatorajaSemantics,
												  bool buildScoreOptionIds,
												  bool difficultyStateUsed,
												  bool difficultyLampStateUsed) const {
	return m_selectedIdentityValid
		&& m_selectedItemKey == itemKey
		&& m_selectedTargetItemKey == targetItemKey
		&& m_selectedRankingMode == rankingMode
		&& m_scoreGeneration == scoreGeneration
		&& m_listGeneration == listGeneration
		&& m_useBeatorajaSemantics == useBeatorajaSemantics
		&& m_buildScoreOptionIds == buildScoreOptionIds
		&& m_selectedDifficultyStateUsed == difficultyStateUsed
		&& m_selectedDifficultyLampStateUsed == difficultyLampStateUsed;
}

void Lr2SelectDetailState::rememberSelectedIdentity(const QString& itemKey,
													const QString& targetItemKey,
													bool rankingMode,
													bool difficultyStateUsed,
													bool difficultyLampStateUsed) {
	m_selectedIdentityValid = true;
	m_selectedItemKey = itemKey;
	m_selectedTargetItemKey = targetItemKey;
	m_selectedRankingMode = rankingMode;
	m_selectedDifficultyStateUsed = difficultyStateUsed;
	m_selectedDifficultyLampStateUsed = difficultyLampStateUsed;
}

void Lr2SelectDetailState::clearSelectedIdentity() {
	m_selectedIdentityValid = false;
	m_selectedItemKey.clear();
	m_selectedTargetItemKey.clear();
	m_selectedRankingMode = false;
	m_selectedDifficultyStateUsed = true;
	m_selectedDifficultyLampStateUsed = true;
}

bool Lr2SelectDetailState::applyRefreshData(int scoreGeneration,
											int listGeneration,
											const QVariant& item,
											const QVariant& chartData,
											bool useBeatorajaSemantics,
											bool buildScoreOptionIds,
											const Lr2SelectScoreSummaryData& scoreSummary) {
	m_useBeatorajaSemantics = useBeatorajaSemantics;
	m_buildScoreOptionIds = buildScoreOptionIds;

	if (m_scoreGeneration != scoreGeneration) {
		m_scoreGeneration = scoreGeneration;
		emit scoreGenerationChanged();
	}
	if (m_listGeneration != listGeneration) {
		m_listGeneration = listGeneration;
		emit listGenerationChanged();
	}
	if (m_item != item) {
		m_item = item;
		emit itemChanged();
	}
	if (m_chartData != chartData) {
		m_chartData = chartData;
		emit chartDataChanged();
	}

	const bool hadBestStats = m_summary.bestStatsObject() != nullptr;
	m_summary.setValues(scoreSummary);
	if (hadBestStats != (m_summary.bestStatsObject() != nullptr)) {
		emit bestStatsChanged();
	}

	const QVariant nextScoreOptionIds = scoreSummary.optionIds;
	if (m_scoreOptionIds != nextScoreOptionIds) {
		m_scoreOptionIds = nextScoreOptionIds;
		emit scoreOptionIdsChanged();
	}
	return true;
}

const Lr2SelectScoreSummaryData& Lr2SelectDetailState::cachedScoreSummaryData(
	const Lr2SelectScoreSummaryCacheKey& cacheKey,
	const QJSValue& scoreList,
	bool useBeatorajaSemantics,
	bool buildScoreOptionIds) {
	ensureScoreSummaryCacheSemantics(useBeatorajaSemantics, buildScoreOptionIds);
	if (!isValidScoreSummaryCacheKey(cacheKey)) {
		return emptyScoreSummaryData();
	}

	auto it = m_scoreSummaryCache.find(cacheKey);
	if (it != m_scoreSummaryCache.end()) {
		return it->data;
	}
	if (jsArrayLength(scoreList) == 0) {
		return emptyScoreSummaryData();
	}

	CachedScoreSummary entry;
	entry.data = buildScoreSummaryDataFromJsValue(scoreList,
												  useBeatorajaSemantics,
												  buildScoreOptionIds);
	it = m_scoreSummaryCache.insert(cacheKey, entry);
	return it->data;
}

QObject* Lr2SelectDetailState::cachedScoreSummaryForIdentifier(const QString& identifier,
															   int scoreGeneration,
															   QJSValue scoreList,
															   bool useBeatorajaSemantics,
															   bool buildScoreOptionIds) {
	const Lr2SelectScoreSummaryCacheKey cacheKey = scoreSummaryCacheKeyForIdentifier(identifier,
																					scoreGeneration);
	ensureScoreSummaryCacheSemantics(useBeatorajaSemantics, buildScoreOptionIds);
	if (!isValidScoreSummaryCacheKey(cacheKey)) {
		return nullptr;
	}

	auto it = m_scoreSummaryCache.find(cacheKey);
	if (it == m_scoreSummaryCache.end()) {
		const Lr2SelectScoreSummaryData& data = cachedScoreSummaryData(cacheKey,
																	   scoreList,
																	   useBeatorajaSemantics,
																	   buildScoreOptionIds);
		it = m_scoreSummaryCache.find(cacheKey);
		if (it == m_scoreSummaryCache.end()) {
			CachedScoreSummary entry;
			entry.data = data;
			it = m_scoreSummaryCache.insert(cacheKey, entry);
		}
	}
	if (it == m_scoreSummaryCache.end()) {
		return nullptr;
	}
	if (!it->object) {
		it->object = new Lr2SelectScoreSummary(this);
		it->object->setValues(it->data);
	}
	return it->object;
}

bool Lr2SelectDetailState::hasCachedScoreSummaryForIdentifier(const QString& identifier,
															  int scoreGeneration,
															  bool useBeatorajaSemantics,
															  bool buildScoreOptionIds) {
	ensureScoreSummaryCacheSemantics(useBeatorajaSemantics, buildScoreOptionIds);
	const Lr2SelectScoreSummaryCacheKey cacheKey = scoreSummaryCacheKeyForIdentifier(identifier,
																					scoreGeneration);
	return isValidScoreSummaryCacheKey(cacheKey)
		&& m_scoreSummaryCache.constFind(cacheKey) != m_scoreSummaryCache.constEnd();
}

bool Lr2SelectDetailState::resolvesSelectedNumberValue(int num) const {
	switch (num) {
	case 42:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 70:
	case 71:
	case 72:
	case 73:
	case 74:
	case 75:
	case 76:
	case 77:
	case 78:
	case 79:
	case 80:
	case 81:
	case 82:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 96:
	case 100:
	case 101:
	case 102:
	case 103:
	case 104:
	case 105:
	case 106:
	case 108:
	case 110:
	case 111:
	case 112:
	case 113:
	case 114:
	case 115:
	case 116:
	case 128:
	case 150:
	case 152:
	case 154:
	case 290:
	case 291:
	case 350:
	case 351:
	case 352:
	case 353:
	case 354:
	case 360:
	case 361:
	case 362:
	case 363:
	case 364:
	case 365:
	case 368:
	case 420:
	case 425:
	case 426:
	case 427:
	case 1163:
	case 1164:
		return true;
	default:
		return false;
	}
}

int Lr2SelectDetailState::selectedNumberValue(int num) const {
	gameplay_logic::ChartData* chart = chartDataFromVariant(m_chartData);
	switch (num) {
	case 42:
	case 96:
		return chart ? chart->getPlayLevel() : 0;
	case 45:
	case 46:
	case 47:
	case 48:
	case 49: {
		const int difficulty = num - 44;
		return m_difficultyModel.countForDifficulty(difficulty) > 0
			? m_difficultyModel.playLevelForDifficulty(difficulty)
			: -1;
	}
	case 90:
	case 290: {
		if (!chart) {
			return -1;
		}
		const double bpm = chart->getMaxBpm() != 0.0 ? chart->getMaxBpm() : chart->getMainBpm();
		return bpm != 0.0 ? static_cast<int>(std::llround(bpm)) : -1;
	}
	case 91:
	case 291: {
		if (!chart) {
			return -1;
		}
		const double bpm = chart->getMinBpm() != 0.0 ? chart->getMinBpm() : chart->getMainBpm();
		return bpm != 0.0 ? static_cast<int>(std::llround(bpm)) : -1;
	}
	case 106:
		return chartPlayableNoteCount(chart);
	case 350:
		return chart ? chart->getNormalNoteCount() : -1;
	case 351:
		return chart ? chart->getLnCount() : -1;
	case 352:
		return chart ? chart->getScratchCount() : -1;
	case 353:
		return chart ? chart->getBssCount() : -1;
	case 354:
		return chart ? chart->getMineCount() : -1;
	case 360:
		return chartDensityValue(chart, chart ? chart->getPeakDensity() : 0.0, false);
	case 361:
		return chartDensityValue(chart, chart ? chart->getPeakDensity() : 0.0, true);
	case 362:
		return chartDensityValue(chart, chart ? chart->getEndDensity() : 0.0, false);
	case 363:
		return chartDensityValue(chart, chart ? chart->getEndDensity() : 0.0, true);
	case 364:
		return chartDensityValue(chart, chart ? chart->getAvgDensity() : 0.0, false);
	case 365:
		return chartDensityValue(chart, chart ? chart->getAvgDensity() : 0.0, true);
	case 368:
		return chart ? static_cast<int>(std::floor(chart->getTotal())) : -1;
	case 1163: {
		const int seconds = chartLengthSeconds(chart);
		return seconds >= 0 ? (seconds / 60) % 60 : -1;
	}
	case 1164: {
		const int seconds = chartLengthSeconds(chart);
		return seconds >= 0 ? seconds % 60 : -1;
	}
	default:
		break;
	}

	QVariantMap rankingMap;
	bool rankingEntry = false;
	std::optional<Lr2SelectScoreStatsData> rankingStats;
	if (m_selectedRankingMode) {
		rankingMap = rankingEntryMap(m_item);
		rankingEntry = rankingMap.value(QStringLiteral("__lr2RankingEntry")).toBool();
		if (rankingEntry) {
			rankingStats = rankingScoreStatsFromMap(rankingMap);
		}
	}
	const Lr2SelectScoreStats* selectedStatsObject = m_summary.bestStatsStats();
	const Lr2SelectScoreStatsData* stats = rankingStats
		? &*rankingStats
		: (selectedStatsObject->hasValues() ? &selectedStatsObject->values() : nullptr);
	const Lr2SelectScoreCounts* counts = m_summary.scoreCounts();
	const auto rankingNumber = [&](const QString& key) {
		return mapNumber(rankingMap, key);
	};

	switch (num) {
	case 70:
		return rankingEntry
			? static_cast<int>(std::llround(rankingNumber(QStringLiteral("bestPoints"))))
			: (stats ? static_cast<int>(std::llround(stats->score)) : 0);
	case 71:
		return rankingEntry
			? static_cast<int>(std::llround(rankingNumber(QStringLiteral("bestPoints"))))
			: (stats ? static_cast<int>(std::llround(stats->exscore)) : 0);
	case 72:
		return rankingEntry
			? static_cast<int>(std::llround(rankingNumber(QStringLiteral("maxPoints"))))
			: (stats ? static_cast<int>(std::llround(stats->maxPoints)) : 0);
	case 73: {
		if (rankingEntry) {
			const double maxPoints = rankingNumber(QStringLiteral("maxPoints"));
			return maxPoints > 0.0
				? static_cast<int>(std::llround(rankingNumber(QStringLiteral("bestPoints")) * 100.0 / maxPoints))
				: 0;
		}
		return stats && stats->maxPoints > 0.0
			? static_cast<int>(std::llround(stats->score * 100.0 / stats->maxPoints))
			: 0;
	}
	case 74:
		return rankingEntry
			? static_cast<int>(std::llround(rankingNumber(QStringLiteral("maxPoints")) / 2.0))
			: chartPlayableNoteCount(chart);
	case 75:
		return rankingEntry
			? static_cast<int>(rankingNumber(QStringLiteral("bestCombo")))
			: (stats ? stats->maxCombo : 0);
	case 76:
		return rankingEntry ? (stats ? stats->badPoor : 0) : counts->minBadPoor();
	case 77:
		return rankingEntry ? static_cast<int>(rankingNumber(QStringLiteral("scoreCount"))) : counts->play();
	case 78:
		return counts->clear();
	case 79:
		return counts->fail();
	case 80:
		return stats ? stats->pg : 0;
	case 81:
		return stats ? stats->gr : 0;
	case 82:
		return stats ? stats->gd : 0;
	case 83:
		return stats ? stats->bd : 0;
	case 84:
		return stats ? stats->poor : 0;
	case 85:
		return stats ? percentInteger(stats->pg, stats->totalJudgements) : 0;
	case 86:
		return stats ? percentInteger(stats->gr, stats->totalJudgements) : 0;
	case 87:
		return stats ? percentInteger(stats->gd, stats->totalJudgements) : 0;
	case 88:
		return stats ? percentInteger(stats->bd, stats->totalJudgements) : 0;
	case 89:
		return stats ? percentInteger(stats->poor, stats->totalJudgements) : 0;
	case 100:
		return scorePrintValue(stats, chart);
	case 101:
	case 108:
	case 128:
	case 150:
	case 152:
		return stats ? static_cast<int>(std::llround(stats->exscore)) : 0;
	case 102:
		return stats ? percentInteger(stats->exscore, stats->maxPoints) : 0;
	case 103:
		return stats ? percentAfterDot(stats->exscore, stats->maxPoints) : 0;
	case 104:
	case 105:
		return stats ? stats->maxCombo : 0;
	case 110:
		return stats ? stats->pg : 0;
	case 111:
		return stats ? stats->gr : 0;
	case 112:
		return stats ? stats->gd : 0;
	case 113:
		return stats ? stats->bd : 0;
	case 114:
		return stats ? stats->poor : 0;
	case 115:
		return stats ? percentInteger(stats->exscore, stats->maxPoints) : 0;
	case 116:
		return stats ? percentAfterDot(stats->exscore, stats->maxPoints) : 0;
	case 154:
		return nextRankDelta(stats, chart);
	case 420:
		return stats ? stats->miss : 0;
	case 425:
		return stats ? stats->comboBreak : 0;
	case 426:
		return stats ? stats->pr : 0;
	case 427:
		return stats ? stats->badPoor : 0;
	default:
		return 0;
	}
}

double Lr2SelectDetailState::selectedBarGraphValue(int type) const {
	if ((type >= 110 && type <= 115) || (type >= 140 && type <= 147)) {
		return selectedBarGraphValue(type - 100);
	}

	gameplay_logic::ChartData* chart = chartDataFromVariant(m_chartData);
	std::optional<Lr2SelectScoreStatsData> rankingStats;
	if (m_selectedRankingMode) {
		rankingStats = rankingScoreStatsFromVariant(m_item);
	}
	const Lr2SelectScoreStats* selectedStatsObject = m_summary.bestStatsStats();
	const Lr2SelectScoreStatsData* stats = rankingStats
		? &*rankingStats
		: (selectedStatsObject->hasValues() ? &selectedStatsObject->values() : nullptr);

	switch (type) {
	case 100:
	case 102:
	case 103:
		return 0.0;
	case 5:
	case 6:
	case 7:
	case 8:
	case 9: {
		const int level = m_difficultyModel.playLevelForDifficulty(type - 4);
		if (level <= 0) {
			return 0.0;
		}
		return static_cast<double>(level) / std::max(1, levelBarFlashThreshold(chart));
	}
	case 40:
		return stats ? normalized(stats->pg, stats->totalJudgements) : 0.0;
	case 41:
		return stats ? normalized(stats->gr, stats->totalJudgements) : 0.0;
	case 42:
		return stats ? normalized(stats->gd, stats->totalJudgements) : 0.0;
	case 43:
		return stats ? normalized(stats->bd, stats->totalJudgements) : 0.0;
	case 44:
		return stats ? normalized(stats->pr, stats->totalJudgements) : 0.0;
	case 45:
		return stats ? normalized(stats->maxCombo, chartPlayableNoteCount(chart)) : 0.0;
	case 46:
	case 47:
		return stats && stats->maxPoints > 0.0 ? stats->exscore / stats->maxPoints : 0.0;
	default:
		return 0.0;
	}
}

void Lr2SelectDetailState::clearScoreSummaryCache() {
	for (const CachedScoreSummary& entry : std::as_const(m_scoreSummaryCache)) {
		delete entry.object;
	}
	m_scoreSummaryCache.clear();
}

void Lr2SelectDetailState::ensureScoreSummaryCacheSemantics(bool useBeatorajaSemantics,
															bool buildScoreOptionIds) {
	if (m_scoreSummaryCacheSemanticsInitialized
			&& m_scoreSummaryCacheUseBeatorajaSemantics == useBeatorajaSemantics
			&& m_scoreSummaryCacheBuildScoreOptionIds == buildScoreOptionIds) {
		return;
	}
	if (m_scoreSummaryCacheSemanticsInitialized) {
		clearScoreSummaryCache();
	}
	m_scoreSummaryCacheSemanticsInitialized = true;
	m_scoreSummaryCacheUseBeatorajaSemantics = useBeatorajaSemantics;
	m_scoreSummaryCacheBuildScoreOptionIds = buildScoreOptionIds;
}

bool Lr2SelectDetailState::selectedIdentityMatches(const QString& itemKey,
												   const QString& targetItemKey,
												   bool rankingMode,
												   int scoreGeneration,
												   int listGeneration,
												   bool useBeatorajaSemantics,
												   bool buildScoreOptionIds,
												   bool difficultyStateUsed,
												   bool difficultyLampStateUsed) const {
	return selectedRefreshMatches(itemKey,
								  targetItemKey,
								  rankingMode,
								  scoreGeneration,
								  listGeneration,
								  useBeatorajaSemantics,
								  buildScoreOptionIds,
								  difficultyStateUsed,
								  difficultyLampStateUsed);
}

bool Lr2SelectDetailState::refreshSelectedFromQmlIdentityForIdentifier(const QString& itemKey,
																	   const QString& targetItemKey,
																	   bool rankingMode,
																	   int scoreGeneration,
																	   int listGeneration,
																	   QJSValue item,
																	   QJSValue chartData,
																	   QJSValue scoreList,
																	   const QString& scoreIdentifier,
																	   int selectedDifficulty,
																	   QJSValue difficultyCounts,
																	   QJSValue difficultyLevels,
																	   QJSValue difficultyLamps,
																	   bool useBeatorajaSemantics,
																	   bool buildScoreOptionIds,
																	   bool difficultyStateUsed,
																	   bool difficultyLampStateUsed) {
	if (selectedRefreshMatches(itemKey,
							   targetItemKey,
							   rankingMode,
							   scoreGeneration,
							   listGeneration,
							   useBeatorajaSemantics,
							   buildScoreOptionIds,
							   difficultyStateUsed,
							   difficultyLampStateUsed)) {
		return false;
	}

	Lr2SelectScoreSummaryData builtScoreSummary;
	const Lr2SelectScoreSummaryData* scoreSummary = nullptr;
	const Lr2SelectScoreSummaryCacheKey scoreCacheKey = scoreSummaryCacheKeyForIdentifier(scoreIdentifier,
																						  scoreGeneration);
	if (!isValidScoreSummaryCacheKey(scoreCacheKey)) {
		scoreSummary = jsArrayLength(scoreList) == 0
			? &emptyScoreSummaryData()
			: &(builtScoreSummary = buildScoreSummaryDataFromJsValue(scoreList,
																	 useBeatorajaSemantics,
																	 buildScoreOptionIds));
	} else {
		scoreSummary = &cachedScoreSummaryData(scoreCacheKey,
											   scoreList,
											   useBeatorajaSemantics,
											   buildScoreOptionIds);
	}
	m_difficultyModel.setRowsFromJsValues(difficultyCounts,
										  difficultyLevels,
										  difficultyLamps,
										  selectedDifficulty,
										  scoreSummary->lamp);

	rememberSelectedIdentity(itemKey,
							 targetItemKey,
							 rankingMode,
							 difficultyStateUsed,
							 difficultyLampStateUsed);

	return applyRefreshData(scoreGeneration,
							listGeneration,
							optionalJsValueVariant(item),
							optionalJsValueVariant(chartData),
							useBeatorajaSemantics,
							buildScoreOptionIds,
							*scoreSummary);
}

void Lr2SelectDetailState::clear() {
	auto setValue = [&](auto& target, const auto& next, auto signal) {
		if (target == next) {
			return;
		}
		target = next;
		emit (this->*signal)();
	};

	setValue(m_scoreGeneration, -1, &Lr2SelectDetailState::scoreGenerationChanged);
	setValue(m_listGeneration, -1, &Lr2SelectDetailState::listGenerationChanged);
	setValue(m_item, QVariant(), &Lr2SelectDetailState::itemChanged);
	setValue(m_chartData, QVariant(), &Lr2SelectDetailState::chartDataChanged);
	clearSelectedIdentity();
	const bool hadBestStats = m_summary.bestStatsObject() != nullptr;
	m_summary.clearValues();
	if (hadBestStats != (m_summary.bestStatsObject() != nullptr)) {
		emit bestStatsChanged();
	}
	setValue(m_scoreOptionIds, QVariant(), &Lr2SelectDetailState::scoreOptionIdsChanged);
	m_difficultyModel.setRows({}, {}, {});
}
