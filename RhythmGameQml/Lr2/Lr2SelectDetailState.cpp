#include "Lr2SelectDetailState.h"

#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QVariantMap>
#include <QSet>
#include <algorithm>

namespace {

constexpr int kEmptyPoorJudgement = 5;

QVariant valueAt(const QVariantList& list, int index) {
	return index >= 0 && index < list.size() ? list.at(index) : QVariant {};
}

QVariantMap objectMap(QObject* object) {
	if (!object) {
		return {};
	}

	QVariantMap result;
	const QMetaObject* metaObject = object->metaObject();
	for (int i = 0; i < metaObject->propertyCount(); ++i) {
		const QMetaProperty property = metaObject->property(i);
		result.insert(QString::fromUtf8(property.name()), property.read(object));
	}
	return result;
}

QVariantMap normalizedMapValue(const QVariant& value) {
	if (value.canConvert<QVariantMap>()) {
		return value.toMap();
	}
	if (value.canConvert<QObject*>()) {
		return objectMap(value.value<QObject*>());
	}
	return {};
}

QVariantMap resultMap(const QVariant& score) {
	return normalizedMapValue(normalizedMapValue(score).value(QStringLiteral("result")));
}

QVariantList normalizedListValue(const QVariant& value) {
	if (value.canConvert<QList<int>>()) {
		const QList<int> values = value.value<QList<int>>();
		QVariantList result;
		result.reserve(values.size());
		for (int entry : values) {
			result.append(entry);
		}
		return result;
	}
	if (value.canConvert<QVariantList>()) {
		return value.toList();
	}
	return {};
}

QString normalizedClearType(const QString& clearType) {
	const QString value = clearType.trimmed().toUpper();
	if (value.isEmpty()) return QStringLiteral("NOPLAY");
	if (value == QStringLiteral("ASSIST")
		|| value == QStringLiteral("ASSISTEASY")
		|| value == QStringLiteral("ASSIST_EASY")) {
		return QStringLiteral("AEASY");
	}
	if (value == QStringLiteral("LIGHT_ASSIST")
		|| value == QStringLiteral("LIGHTASSISTEASY")
		|| value == QStringLiteral("LIGHT_ASSIST_EASY")) {
		return QStringLiteral("LIGHTASSIST");
	}
	if (value == QStringLiteral("EX_HARD")) return QStringLiteral("EXHARD");
	if (value == QStringLiteral("EX_HARD_DAN") || value == QStringLiteral("EXHARD_DAN")) {
		return QStringLiteral("EXHARDDAN");
	}
	return value;
}

QString clearTypeOf(const QVariant& score) {
	return normalizedClearType(resultMap(score).value(QStringLiteral("clearType")).toString());
}

int clearTypePriority(const QString& clearType) {
	const QString value = normalizedClearType(clearType);
	if (value == QStringLiteral("FAILED")) return 1;
	if (value == QStringLiteral("AEASY")) return 2;
	if (value == QStringLiteral("LIGHTASSIST")) return 3;
	if (value == QStringLiteral("EASY")) return 4;
	if (value == QStringLiteral("NORMAL")) return 5;
	if (value == QStringLiteral("HARD")) return 6;
	if (value == QStringLiteral("EXHARD") || value == QStringLiteral("EXHARDDAN")) return 7;
	if (value == QStringLiteral("FC")) return 8;
	if (value == QStringLiteral("PERFECT")) return 9;
	if (value == QStringLiteral("MAX")) return 10;
	return 0;
}

int clearTypeLamp(const QString& clearType) {
	const QString value = normalizedClearType(clearType);
	if (value == QStringLiteral("FAILED")) return 1;
	if (value == QStringLiteral("AEASY") || value == QStringLiteral("LIGHTASSIST") || value == QStringLiteral("EASY")) return 2;
	if (value == QStringLiteral("NORMAL")) return 3;
	if (value == QStringLiteral("HARD") || value == QStringLiteral("EXHARD") || value == QStringLiteral("EXHARDDAN")) return 4;
	if (value == QStringLiteral("FC") || value == QStringLiteral("PERFECT") || value == QStringLiteral("MAX")) return 5;
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

int judgementCount(const QVariantList& counts, int judgement) {
	return judgement >= 0 && judgement < counts.size() ? counts.at(judgement).toInt() : 0;
}

int badPoorForScore(const QVariant& score) {
	const QVariantList counts = normalizedListValue(resultMap(score).value(QStringLiteral("judgementCounts")));
	return judgementCount(counts, 3) + judgementCount(counts, 4) + judgementCount(counts, kEmptyPoorJudgement);
}

QVariantMap statsForScore(const QVariant& score) {
	const QVariantMap result = resultMap(score);
	if (result.isEmpty()) {
		return {};
	}

	const QVariantList counts = normalizedListValue(result.value(QStringLiteral("judgementCounts")));
	const int pg = judgementCount(counts, 0);
	const int gr = judgementCount(counts, 1);
	const int gd = judgementCount(counts, 2);
	const int bd = judgementCount(counts, 3);
	const int poor = judgementCount(counts, 4);
	const int miss = judgementCount(counts, kEmptyPoorJudgement);
	const int pr = poor + miss;
	return {
		{QStringLiteral("pg"), pg},
		{QStringLiteral("gr"), gr},
		{QStringLiteral("gd"), gd},
		{QStringLiteral("bd"), bd},
		{QStringLiteral("poor"), poor},
		{QStringLiteral("miss"), miss},
		{QStringLiteral("pr"), pr},
		{QStringLiteral("totalJudgements"), std::max(1, pg + gr + gd + bd + pr)},
		{QStringLiteral("comboBreak"), bd + poor},
		{QStringLiteral("badPoor"), bd + pr},
		{QStringLiteral("maxCombo"), result.value(QStringLiteral("maxCombo"), 0)},
		{QStringLiteral("score"), result.value(QStringLiteral("points"), 0)},
		{QStringLiteral("exscore"), result.value(QStringLiteral("points"), 0)},
		{QStringLiteral("maxPoints"), result.value(QStringLiteral("maxPoints"), 0)},
		{QStringLiteral("early"), QVariant()},
		{QStringLiteral("late"), QVariant()},
		{QStringLiteral("totalEarly"), 0},
		{QStringLiteral("totalLate"), 0},
	};
}

QVariantMap emptyScoreCounts() {
	return {
		{QStringLiteral("play"), 0},
		{QStringLiteral("clear"), 0},
		{QStringLiteral("fail"), 0},
		{QStringLiteral("noplay"), 0},
		{QStringLiteral("assist"), 0},
		{QStringLiteral("lightAssist"), 0},
		{QStringLiteral("easy"), 0},
		{QStringLiteral("normal"), 0},
		{QStringLiteral("hard"), 0},
		{QStringLiteral("exhard"), 0},
		{QStringLiteral("fc"), 0},
		{QStringLiteral("perfect"), 0},
		{QStringLiteral("max"), 0},
		{QStringLiteral("minBadPoor"), 0},
	};
}

void increment(QVariantMap& map, const QString& key) {
	map.insert(key, map.value(key).toInt() + 1);
}

QVariantMap buildScoreSummary(const QVariantList& scoreList) {
	if (scoreList.isEmpty()) {
		return {
			{QStringLiteral("scoreList"), QVariantList {}},
			{QStringLiteral("bestScore"), QVariant()},
			{QStringLiteral("bestStats"), QVariant()},
			{QStringLiteral("scoreCounts"), emptyScoreCounts()},
			{QStringLiteral("clearType"), QStringLiteral("NOPLAY")},
			{QStringLiteral("lamp"), 0},
			{QStringLiteral("rank"), 0},
			{QStringLiteral("scoreRate"), 0.0},
		};
	}

	QVariantMap counts = emptyScoreCounts();
	QVariant bestScore;
	double bestRate = -1.0;
	QString bestClearType = QStringLiteral("NOPLAY");
	int bestClearPriority = 0;
	int minBadPoor = -1;

	for (const QVariant& score : scoreList) {
		const QVariantMap result = resultMap(score);
		if (result.isEmpty()) {
			continue;
		}

		increment(counts, QStringLiteral("play"));
		const QString clearType = clearTypeOf(score);
		const int priority = clearTypePriority(clearType);
		if (priority > bestClearPriority) {
			bestClearPriority = priority;
			bestClearType = clearType;
		}

		if (clearType != QStringLiteral("FAILED") && clearType != QStringLiteral("NOPLAY")) {
			increment(counts, QStringLiteral("clear"));
		}
		if (clearType == QStringLiteral("FAILED")) increment(counts, QStringLiteral("fail"));
		else if (clearType == QStringLiteral("AEASY")) increment(counts, QStringLiteral("assist"));
		else if (clearType == QStringLiteral("LIGHTASSIST")) increment(counts, QStringLiteral("lightAssist"));
		else if (clearType == QStringLiteral("EASY")) increment(counts, QStringLiteral("easy"));
		else if (clearType == QStringLiteral("NORMAL")) increment(counts, QStringLiteral("normal"));
		else if (clearType == QStringLiteral("HARD")) increment(counts, QStringLiteral("hard"));
		else if (clearType == QStringLiteral("EXHARD") || clearType == QStringLiteral("EXHARDDAN")) increment(counts, QStringLiteral("exhard"));
		else if (clearType == QStringLiteral("FC")) increment(counts, QStringLiteral("fc"));
		else if (clearType == QStringLiteral("PERFECT")) increment(counts, QStringLiteral("perfect"));
		else if (clearType == QStringLiteral("MAX")) increment(counts, QStringLiteral("max"));
		else increment(counts, QStringLiteral("noplay"));

		const int badPoor = badPoorForScore(score);
		minBadPoor = minBadPoor < 0 ? badPoor : std::min(minBadPoor, badPoor);

		const double maxPoints = result.value(QStringLiteral("maxPoints")).toDouble();
		if (maxPoints > 0.0) {
			const double rate = result.value(QStringLiteral("points")).toDouble() / maxPoints;
			if (rate > bestRate) {
				bestRate = rate;
				bestScore = score;
			}
		}
	}

	counts.insert(QStringLiteral("minBadPoor"), std::max(0, minBadPoor));
	const double scoreRate = std::max(0.0, bestRate);
	return {
		{QStringLiteral("scoreList"), scoreList},
		{QStringLiteral("bestScore"), bestScore},
		{QStringLiteral("bestStats"), bestScore.isValid() ? QVariant(statsForScore(bestScore)) : QVariant()},
		{QStringLiteral("scoreCounts"), counts},
		{QStringLiteral("clearType"), bestClearType},
		{QStringLiteral("lamp"), clearTypeLamp(bestClearType)},
		{QStringLiteral("rank"), rankForScoreRate(scoreRate)},
		{QStringLiteral("scoreRate"), scoreRate},
	};
}

void appendScoreClearOptionIds(const QString& clearType, QSet<int>& ids) {
	// Historical score flags are beatoraja trophy options. The exact
	// selected-bar clear options are added from the current summary only.
	const QString value = normalizedClearType(clearType);
	if (value == QStringLiteral("AEASY") || value == QStringLiteral("LIGHTASSIST")) {
		ids.insert(124);
	} else if (value == QStringLiteral("EASY")) {
		ids.insert(121);
	} else if (value == QStringLiteral("NORMAL")) {
		ids.insert(118);
	} else if (value == QStringLiteral("HARD")) {
		ids.insert(119);
	} else if (value == QStringLiteral("EXHARD") || value == QStringLiteral("EXHARDDAN")) {
		ids.insert(125);
	}
}

void appendScoreOptionIds(const QVariant& score, QSet<int>& ids) {
	const QVariantMap result = resultMap(score);
	if (result.isEmpty()) {
		return;
	}
	switch (result.value(QStringLiteral("noteOrderAlgorithm")).toInt()) {
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

	if (result.value(QStringLiteral("dpOptions")).toInt() == 2) {
		const int keymode = result.value(QStringLiteral("keymode")).toInt();
		ids.insert(keymode == 5 || keymode == 7 ? 145 : 144);
	}
}

QVariantList buildScoreOptionIds(const QVariantList& scoreList) {
	QSet<int> ids;
	for (const QVariant& score : scoreList) {
		appendScoreClearOptionIds(clearTypeOf(score), ids);
		appendScoreOptionIds(score, ids);
	}
	QList<int> sorted = ids.values();
	std::sort(sorted.begin(), sorted.end());
	QVariantList result;
	result.reserve(sorted.size());
	for (int id : sorted) {
		result.append(id);
	}
	return result;
}

} // namespace

Lr2SelectDifficultyModel::Lr2SelectDifficultyModel(QObject* parent)
	: QAbstractListModel(parent) {}

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
	case ChartRole: return row.chart;
	case CountRole: return row.count;
	case PlayLevelRole: return row.playLevel;
	case LampRole: return row.lamp;
	default: return {};
	}
}

QHash<int, QByteArray> Lr2SelectDifficultyModel::roleNames() const {
	return {
		{DifficultyRole, "difficulty"},
		{ChartRole, "chart"},
		{CountRole, "count"},
		{PlayLevelRole, "playLevel"},
		{LampRole, "lamp"},
	};
}

QVariant Lr2SelectDifficultyModel::chartForDifficulty(int difficulty) const {
	const Row* row = rowForDifficulty(difficulty);
	return row ? row->chart : QVariant {};
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

void Lr2SelectDifficultyModel::setRows(const QVariantList& charts,
									   const QVariantList& counts,
									   const QVariantList& levels,
									   const QVariantList& lamps) {
	QList<Row> nextRows;
	nextRows.reserve(5);
	for (int difficulty = 1; difficulty <= 5; ++difficulty) {
		nextRows.append(Row {
			difficulty,
			valueAt(charts, difficulty),
			valueAt(counts, difficulty).toInt(),
			valueAt(levels, difficulty).toInt(),
			valueAt(lamps, difficulty).toInt(),
		});
	}

	beginResetModel();
	m_rows = std::move(nextRows);
	endResetModel();
}

const Lr2SelectDifficultyModel::Row* Lr2SelectDifficultyModel::rowForDifficulty(int difficulty) const {
	for (const Row& row : m_rows) {
		if (row.difficulty == difficulty) {
			return &row;
		}
	}
	return nullptr;
}

Lr2SelectDetailState::Lr2SelectDetailState(QObject* parent)
	: QObject(parent)
	, m_difficultyModel(this) {}

QString Lr2SelectDetailState::key() const { return m_key; }
void Lr2SelectDetailState::setKey(const QString& value) {
	if (m_key == value) return;
	m_key = value;
	emit keyChanged();
	emit asObjectChanged();
}

int Lr2SelectDetailState::scoreRevision() const { return m_scoreRevision; }
void Lr2SelectDetailState::setScoreRevision(int value) {
	if (m_scoreRevision == value) return;
	m_scoreRevision = value;
	emit scoreRevisionChanged();
	emit asObjectChanged();
}

int Lr2SelectDetailState::listRevision() const { return m_listRevision; }
void Lr2SelectDetailState::setListRevision(int value) {
	if (m_listRevision == value) return;
	m_listRevision = value;
	emit listRevisionChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::item() const { return m_item; }
void Lr2SelectDetailState::setItem(const QVariant& value) {
	if (m_item == value) return;
	m_item = value;
	emit itemChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::chartData() const { return m_chartData; }
void Lr2SelectDetailState::setChartData(const QVariant& value) {
	if (m_chartData == value) return;
	m_chartData = value;
	emit chartDataChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::chartWrapper() const { return m_chartWrapper; }
void Lr2SelectDetailState::setChartWrapper(const QVariant& value) {
	if (m_chartWrapper == value) return;
	m_chartWrapper = value;
	emit chartWrapperChanged();
	emit asObjectChanged();
}

QVariantList Lr2SelectDetailState::scoreList() const { return m_scoreList; }
void Lr2SelectDetailState::setScoreList(const QVariantList& value) {
	if (m_scoreList == value) return;
	m_scoreList = value;
	emit scoreListChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::summary() const { return m_summary; }
void Lr2SelectDetailState::setSummary(const QVariant& value) {
	if (m_summary == value) return;
	m_summary = value;
	emit summaryChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::bestStats() const { return m_bestStats; }
void Lr2SelectDetailState::setBestStats(const QVariant& value) {
	if (m_bestStats == value) return;
	m_bestStats = value;
	emit bestStatsChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::scoreCounts() const { return m_scoreCounts; }
void Lr2SelectDetailState::setScoreCounts(const QVariant& value) {
	if (m_scoreCounts == value) return;
	m_scoreCounts = value;
	emit scoreCountsChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::scoreOptionIds() const { return m_scoreOptionIds; }
void Lr2SelectDetailState::setScoreOptionIds(const QVariant& value) {
	if (m_scoreOptionIds == value) return;
	m_scoreOptionIds = value;
	emit scoreOptionIdsChanged();
	emit asObjectChanged();
}

QVariant Lr2SelectDetailState::difficultyState() const { return m_difficultyState; }
void Lr2SelectDetailState::setDifficultyState(const QVariant& value) {
	if (m_difficultyState == value) return;
	m_difficultyState = value;
	emit difficultyStateChanged();
	emit asObjectChanged();
}

Lr2SelectDifficultyModel* Lr2SelectDetailState::difficultyModel() {
	return &m_difficultyModel;
}

QVariant Lr2SelectDetailState::asObject() const {
	return snapshot();
}

QVariant Lr2SelectDetailState::snapshot() const {
	return QVariantMap {
		{QStringLiteral("key"), m_key},
		{QStringLiteral("scoreRevision"), m_scoreRevision},
		{QStringLiteral("listRevision"), m_listRevision},
		{QStringLiteral("item"), m_item},
		{QStringLiteral("chartData"), m_chartData},
		{QStringLiteral("chartWrapper"), m_chartWrapper},
		{QStringLiteral("scoreList"), m_scoreList},
		{QStringLiteral("summary"), m_summary},
		{QStringLiteral("bestStats"), m_bestStats},
		{QStringLiteral("scoreCounts"), m_scoreCounts},
		{QStringLiteral("scoreOptionIds"), m_scoreOptionIds},
		{QStringLiteral("difficultyState"), m_difficultyState},
	};
}

void Lr2SelectDetailState::apply(const QVariantMap& values) {
	bool changed = false;
	auto setVariant = [&](QVariant& target, const QString& name, auto signal) {
		const QVariant next = values.value(name);
		if (target == next) {
			return;
		}
		target = next;
		changed = true;
		emit (this->*signal)();
	};

	const QString nextKey = values.value(QStringLiteral("key")).toString();
	if (m_key != nextKey) {
		m_key = nextKey;
		changed = true;
		emit keyChanged();
	}

	const int nextScoreRevision = values.value(QStringLiteral("scoreRevision"), -1).toInt();
	if (m_scoreRevision != nextScoreRevision) {
		m_scoreRevision = nextScoreRevision;
		changed = true;
		emit scoreRevisionChanged();
	}

	const int nextListRevision = values.value(QStringLiteral("listRevision"), -1).toInt();
	if (m_listRevision != nextListRevision) {
		m_listRevision = nextListRevision;
		changed = true;
		emit listRevisionChanged();
	}

	setVariant(m_item, QStringLiteral("item"), &Lr2SelectDetailState::itemChanged);
	setVariant(m_chartData, QStringLiteral("chartData"), &Lr2SelectDetailState::chartDataChanged);
	setVariant(m_chartWrapper, QStringLiteral("chartWrapper"), &Lr2SelectDetailState::chartWrapperChanged);

	const QVariantList nextScoreList = values.value(QStringLiteral("scoreList")).toList();
	if (m_scoreList != nextScoreList) {
		m_scoreList = nextScoreList;
		changed = true;
		emit scoreListChanged();
	}

	setVariant(m_summary, QStringLiteral("summary"), &Lr2SelectDetailState::summaryChanged);
	setVariant(m_bestStats, QStringLiteral("bestStats"), &Lr2SelectDetailState::bestStatsChanged);
	setVariant(m_scoreCounts, QStringLiteral("scoreCounts"), &Lr2SelectDetailState::scoreCountsChanged);
	setVariant(m_scoreOptionIds, QStringLiteral("scoreOptionIds"), &Lr2SelectDetailState::scoreOptionIdsChanged);
	setVariant(m_difficultyState, QStringLiteral("difficultyState"), &Lr2SelectDetailState::difficultyStateChanged);
	emitObjectChangedIf(changed);
}

bool Lr2SelectDetailState::refresh(const QString& key,
								   int scoreRevision,
								   int listRevision,
								   const QVariant& item,
								   const QVariant& chartData,
								   const QVariantList& scoreList,
								   const QVariantList& difficultyCharts,
								   const QVariantList& difficultyCounts,
								   const QVariantList& difficultyLevels,
								   const QVariantList& difficultyLamps) {
	if (m_key == key && m_scoreRevision == scoreRevision && m_listRevision == listRevision) {
		return false;
	}

	const QVariantMap summary = buildScoreSummary(scoreList);
	const QVariantMap chartWrapper = buildChartWrapper(chartData);
	const QVariantList optionIds = buildScoreOptionIds(scoreList);
	QVariantMap difficultyState {
		{QStringLiteral("key"), key},
		{QStringLiteral("charts"), difficultyCharts},
		{QStringLiteral("counts"), difficultyCounts},
		{QStringLiteral("levels"), difficultyLevels},
		{QStringLiteral("lamps"), difficultyLamps},
	};

	m_difficultyModel.setRows(difficultyCharts, difficultyCounts, difficultyLevels, difficultyLamps);

	apply({
		{QStringLiteral("key"), key},
		{QStringLiteral("scoreRevision"), scoreRevision},
		{QStringLiteral("listRevision"), listRevision},
		{QStringLiteral("item"), item},
		{QStringLiteral("chartData"), chartData},
		{QStringLiteral("chartWrapper"), chartWrapper},
		{QStringLiteral("scoreList"), scoreList},
		{QStringLiteral("summary"), summary},
		{QStringLiteral("bestStats"), summary.value(QStringLiteral("bestStats"))},
		{QStringLiteral("scoreCounts"), summary.value(QStringLiteral("scoreCounts"))},
		{QStringLiteral("scoreOptionIds"), optionIds},
		{QStringLiteral("difficultyState"), difficultyState},
	});
	return true;
}

void Lr2SelectDetailState::clear() {
	apply({
		{QStringLiteral("key"), QString()},
		{QStringLiteral("scoreRevision"), -1},
		{QStringLiteral("listRevision"), -1},
		{QStringLiteral("scoreList"), QVariantList()},
	});
}

QVariantMap Lr2SelectDetailState::buildChartWrapper(const QVariant& chartData) const {
	return chartData.isValid() && !chartData.isNull()
		? QVariantMap {{QStringLiteral("chartData"), chartData}}
		: QVariantMap {};
}

void Lr2SelectDetailState::emitObjectChangedIf(bool changed) {
	if (changed) {
		emit asObjectChanged();
	}
}
