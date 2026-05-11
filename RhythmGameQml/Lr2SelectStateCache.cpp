#include "Lr2SelectStateCache.h"

#include "Lr2SelectBarCell.h"
#include "gameplay_logic/ChartData.h"

#include <QJSValue>
#include <QMetaProperty>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kJudgementPoor = 0;
constexpr int kJudgementEmptyPoor = 1;
constexpr int kJudgementBad = 2;
constexpr int kJudgementGood = 3;
constexpr int kJudgementGreat = 4;
constexpr int kJudgementPerfect = 5;

int toInt(const QVariant& value, int fallback = 0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int result = value.toInt(&ok);
    return ok ? result : fallback;
}

qreal toReal(const QVariant& value, qreal fallback = 0.0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const qreal result = value.toReal(&ok);
    return ok ? result : fallback;
}

bool toBool(const QVariant& value) {
    return value.isValid() && !value.isNull() && value.toBool();
}

QString toString(const QVariant& value) {
    return value.isValid() && !value.isNull() ? value.toString() : QString();
}

QVariant gadgetProperty(const QVariant& source, const char* name) {
    const QMetaObject* metaObject = source.metaType().metaObject();
    if (!metaObject) {
        return {};
    }
    const int propertyIndex = metaObject->indexOfProperty(name);
    if (propertyIndex < 0) {
        return {};
    }
    return metaObject->property(propertyIndex).readOnGadget(source.constData());
}

QVariant valueProperty(const QVariant& source, const char* name) {
    if (!source.isValid() || source.isNull()) {
        return {};
    }
    if (source.canConvert<QObject*>()) {
        if (QObject* object = source.value<QObject*>()) {
            return object->property(name);
        }
    }
    if (source.canConvert<QVariantMap>()) {
        const QVariantMap map = source.toMap();
        const auto it = map.constFind(QString::fromLatin1(name));
        return it == map.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QVariantHash>()) {
        const QVariantHash hash = source.toHash();
        const auto it = hash.constFind(QString::fromLatin1(name));
        return it == hash.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QJSValue>()) {
        const QJSValue value = source.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(QString::fromLatin1(name));
            return property.isUndefined() || property.isNull() ? QVariant() : property.toVariant();
        }
    }
    return gadgetProperty(source, name);
}

QVariant indexedValue(const QVariant& source, const QString& key) {
    if (!source.isValid() || source.isNull()) {
        return {};
    }
    if (source.canConvert<QVariantMap>()) {
        const QVariantMap map = source.toMap();
        const auto it = map.constFind(key);
        return it == map.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QVariantHash>()) {
        const QVariantHash hash = source.toHash();
        const auto it = hash.constFind(key);
        return it == hash.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QJSValue>()) {
        const QJSValue value = source.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(key);
            return property.isUndefined() || property.isNull() ? QVariant() : property.toVariant();
        }
    }
    if (source.canConvert<QObject*>()) {
        if (QObject* object = source.value<QObject*>()) {
            return object->property(key.toLatin1().constData());
        }
    }
    return {};
}

QVariantList toList(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    if (value.canConvert<QVariantList>()) {
        return value.toList();
    }
    if (value.canConvert<QStringList>()) {
        const QStringList strings = value.toStringList();
        QVariantList result;
        result.reserve(strings.size());
        for (const QString& string : strings) {
            result.append(string);
        }
        return result;
    }
    if (value.canConvert<QJSValue>()) {
        return value.value<QJSValue>().toVariant().toList();
    }
    return {};
}

int judgementCount(const QVariant& countsValue, int judgement) {
    const QVariantList counts = toList(countsValue);
    return judgement >= 0 && judgement < counts.size() ? toInt(counts.at(judgement)) : 0;
}

int clearTypePriority(const QString& clearType) {
    if (clearType == QStringLiteral("FAILED")) {
        return 1;
    }
    if (clearType == QStringLiteral("AEASY")) {
        return 2;
    }
    if (clearType == QStringLiteral("EASY")) {
        return 3;
    }
    if (clearType == QStringLiteral("NORMAL")) {
        return 4;
    }
    if (clearType == QStringLiteral("HARD")) {
        return 5;
    }
    if (clearType == QStringLiteral("EXHARD")) {
        return 6;
    }
    if (clearType == QStringLiteral("FC")) {
        return 7;
    }
    if (clearType == QStringLiteral("PERFECT")) {
        return 8;
    }
    if (clearType == QStringLiteral("MAX")) {
        return 9;
    }
    return 0;
}

int clearTypeLamp(const QString& clearType) {
    if (clearType == QStringLiteral("FAILED")) {
        return 1;
    }
    if (clearType == QStringLiteral("AEASY") || clearType == QStringLiteral("EASY")) {
        return 2;
    }
    if (clearType == QStringLiteral("NORMAL")) {
        return 3;
    }
    if (clearType == QStringLiteral("HARD") || clearType == QStringLiteral("EXHARD")) {
        return 4;
    }
    if (clearType == QStringLiteral("FC")
            || clearType == QStringLiteral("PERFECT")
            || clearType == QStringLiteral("MAX")) {
        return 5;
    }
    return 0;
}

QString clearTypeOf(const QVariant& score);

int beatorajaClearTypeIndex(const QString& clearType) {
    if (clearType == QStringLiteral("FAILED")) {
        return 1;
    }
    if (clearType == QStringLiteral("AEASY")) {
        return 2;
    }
    if (clearType == QStringLiteral("LIGHTASSIST")
            || clearType == QStringLiteral("LIGHT_ASSIST")) {
        return 3;
    }
    if (clearType == QStringLiteral("EASY")) {
        return 4;
    }
    if (clearType == QStringLiteral("NORMAL")) {
        return 5;
    }
    if (clearType == QStringLiteral("HARD")) {
        return 6;
    }
    if (clearType == QStringLiteral("EXHARD")) {
        return 7;
    }
    if (clearType == QStringLiteral("FC")) {
        return 8;
    }
    if (clearType == QStringLiteral("PERFECT")) {
        return 9;
    }
    if (clearType == QStringLiteral("MAX")) {
        return 10;
    }
    return 0;
}

QString bestClearTypeForScores(const QVariantList& scoreList) {
    QString clearType = QStringLiteral("NOPLAY");
    int priority = 0;
    for (const QVariant& score : scoreList) {
        const QString next = clearTypeOf(score);
        const int nextPriority = clearTypePriority(next);
        if (nextPriority > priority) {
            priority = nextPriority;
            clearType = next;
        }
    }
    return clearType;
}

QVariant bestScoreByPoints(const QVariantList& scoreList) {
    QVariant best;
    qreal bestRate = -1.0;
    for (const QVariant& score : scoreList) {
        const QVariant result = valueProperty(score, "result");
        const qreal maxPoints = toReal(valueProperty(result, "maxPoints"));
        if (maxPoints <= 0.0) {
            continue;
        }
        const qreal rate = toReal(valueProperty(result, "points")) / maxPoints;
        if (rate > bestRate) {
            bestRate = rate;
            best = score;
        }
    }
    return best;
}

QVariant normalizedScoreQueryResult(const QVariant& result) {
    const QVariant nested = valueProperty(result, "scores");
    if (nested.isValid() && !nested.isNull()) {
        const QVariant nestedScores = valueProperty(nested, "scores");
        const QVariant nestedUnplayed = valueProperty(nested, "unplayed");
        if (nestedScores.isValid() || nestedUnplayed.isValid()) {
            return nested;
        }
    }
    return result;
}

int rankForScoreRate(qreal rate) {
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

QString clearTypeOf(const QVariant& score) {
    const QVariant result = valueProperty(score, "result");
    const QString clearType = toString(valueProperty(result, "clearType"));
    return clearType.isEmpty() ? QStringLiteral("NOPLAY") : clearType;
}

int badPoorForScore(const QVariant& score) {
    const QVariant result = valueProperty(score, "result");
    const QVariant counts = valueProperty(result, "judgementCounts");
    return judgementCount(counts, kJudgementBad)
        + judgementCount(counts, kJudgementPoor)
        + judgementCount(counts, kJudgementEmptyPoor);
}

void appendUniqueOption(QSet<int>& lookup, QVariantList& options, int option) {
    if (lookup.contains(option)) {
        return;
    }
    lookup.insert(option);
    options.append(option);
}

const QVariantMap& emptyScoreCountsValue() {
    static const QVariantMap value {
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
    return value;
}

const QVariantMap& emptyDifficultyStateValue() {
    static const QVariantMap value {
        {QStringLiteral("key"), QStringLiteral("empty")},
        {QStringLiteral("charts"), QVariantList {QVariant(), QVariant(), QVariant(), QVariant(), QVariant(), QVariant()}},
        {QStringLiteral("counts"), QVariantList {0, 0, 0, 0, 0, 0}},
        {QStringLiteral("levels"), QVariantList {0, 0, 0, 0, 0, 0}},
        {QStringLiteral("lamps"), QVariantList {0, 0, 0, 0, 0, 0}},
    };
    return value;
}

QString chartHistogramRevision(const QVariant& chartData) {
    const QVariantList histogram = toList(valueProperty(chartData, "histogramData"));
    if (histogram.isEmpty()) {
        return {};
    }

    QStringList parts;
    parts.reserve(6);
    for (int i = 0; i < 6; ++i) {
        const QVariantList series = i < histogram.size() ? toList(histogram.at(i)) : QVariantList {};
        parts.append(QString::number(series.size()));
    }
    return parts.join(QLatin1Char(':'));
}

QString chartContentRevisionForData(const QString& stateKey, const QVariant& chartData) {
    if (!chartData.isValid() || chartData.isNull()) {
        return stateKey;
    }
    return stateKey
        + QLatin1Char(':') + toString(valueProperty(chartData, "md5"))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "length")))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "normalNoteCount")))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "scratchCount")))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "lnCount")))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "bssCount")))
        + QLatin1Char(':') + QString::number(toInt(valueProperty(chartData, "mineCount")))
        + QLatin1Char(':') + chartHistogramRevision(chartData);
}

} // namespace

Lr2SelectStateCache::Lr2SelectStateCache(QObject* parent) : QObject(parent) {}

QVariant Lr2SelectStateCache::scores() const {
    return m_scores;
}

void Lr2SelectStateCache::setScores(const QVariant& value) {
    if (m_scores == value) {
        return;
    }
    m_scores = value;
    clearScoreCaches();
    emit scoresChanged();
}

QVariant Lr2SelectStateCache::chartDifficultyCache() const {
    return m_chartDifficultyCache;
}

void Lr2SelectStateCache::setChartDifficultyCache(const QVariant& value) {
    if (m_chartDifficultyCache == value) {
        return;
    }
    m_chartDifficultyCache = value;
    clearDifficultyCache();
    emit chartDifficultyCacheChanged();
}

QVariant Lr2SelectStateCache::chartGroupCache() const {
    return m_chartGroupCache;
}

void Lr2SelectStateCache::setChartGroupCache(const QVariant& value) {
    if (m_chartGroupCache == value) {
        return;
    }
    m_chartGroupCache = value;
    clearDifficultyCache();
    emit chartGroupCacheChanged();
}

QVariant Lr2SelectStateCache::folderLampByKey() const {
    return m_folderLampByKey;
}

void Lr2SelectStateCache::setFolderLampByKey(const QVariant& value) {
    if (m_folderLampByKey == value) {
        return;
    }
    m_folderLampByKey = value;
    clearBarCellCache();
    emit folderLampByKeyChanged();
}

QVariant Lr2SelectStateCache::folderDistributionByKey() const {
    return m_folderDistributionByKey;
}

void Lr2SelectStateCache::setFolderDistributionByKey(const QVariant& value) {
    if (m_folderDistributionByKey == value) {
        return;
    }
    m_folderDistributionByKey = value;
    clearBarCellCache();
    emit folderDistributionByKeyChanged();
}

QVariant Lr2SelectStateCache::historyStack() const {
    return m_historyStack;
}

void Lr2SelectStateCache::setHistoryStack(const QVariant& value) {
    if (m_historyStack == value) {
        return;
    }
    m_historyStack = value;
    clearBarCellCache();
    emit historyStackChanged();
}

QVariantMap Lr2SelectStateCache::refreshSelectedState(const QVariant& item,
                                                      int focusedIndex,
                                                      int scoreRevision,
                                                      int listRevision,
                                                      bool rankingMode,
                                                      const QVariant& rankingBaseItem) {
    syncRevisions(scoreRevision, listRevision);

    const ItemKind kind = itemKind(item);
    const bool simpleFolderState = !rankingMode
        && (kind == ItemKind::Folder || kind == ItemKind::Table || kind == ItemKind::Level);
    const QVariant chartData = simpleFolderState ? QVariant() : chartDataForItem(item, rankingMode, rankingBaseItem);
    const QString stateKey = entrySelectionKey(chartData.isValid() && !chartData.isNull() ? chartData : item, focusedIndex);
    if (!stateKey.isEmpty()
            && stateKey == m_selectedStateKey
            && toInt(m_selectedState.value(QStringLiteral("scoreRevision")), -1) == scoreRevision
            && toInt(m_selectedState.value(QStringLiteral("listRevision")), -1) == listRevision) {
        return {
            {QStringLiteral("changed"), false},
            {QStringLiteral("state"), m_selectedState},
        };
    }

    QVariantMap cached = stateKey.isEmpty() ? QVariantMap() : m_selectedStateCache.value(stateKey);
    if (!cached.isEmpty()
            && toInt(cached.value(QStringLiteral("scoreRevision")), -1) == scoreRevision
            && toInt(cached.value(QStringLiteral("listRevision")), -1) == listRevision) {
        m_selectedState = cached;
        m_selectedStateKey = stateKey;
        return {
            {QStringLiteral("changed"), true},
            {QStringLiteral("state"), m_selectedState},
        };
    }

    if (simpleFolderState) {
        QVariantMap nextState {
            {QStringLiteral("key"), stateKey},
            {QStringLiteral("contentRevision"), stateKey},
            {QStringLiteral("scoreRevision"), scoreRevision},
            {QStringLiteral("listRevision"), listRevision},
            {QStringLiteral("item"), item},
            {QStringLiteral("chartData"), QVariant()},
            {QStringLiteral("chartWrapper"), QVariant()},
            {QStringLiteral("scoreList"), QVariantList {}},
            {QStringLiteral("summary"), QVariant()},
            {QStringLiteral("bestStats"), QVariant()},
            {QStringLiteral("scoreCounts"), emptyScoreCounts()},
            {QStringLiteral("scoreOptionIds"), QVariantList {}},
            {QStringLiteral("difficultyState"), emptyDifficultyState()},
        };

        m_selectedState = nextState;
        m_selectedStateKey = stateKey;
        if (!stateKey.isEmpty()) {
            m_selectedStateCache.insert(stateKey, nextState);
        }
        return {
            {QStringLiteral("changed"), true},
            {QStringLiteral("state"), m_selectedState},
        };
    }

    const QVariant targetItem = chartData.isValid() && !chartData.isNull() ? chartData : item;
    const QString summaryKey = scoreSummaryKey(targetItem);
    QVariantMap summary = summaryKey.isEmpty() ? QVariantMap() : m_scoreSummaryCache.value(summaryKey);
    if (summary.isEmpty()) {
        summary = buildScoreSummary(entryScores(targetItem));
        if (!summaryKey.isEmpty()) {
            m_scoreSummaryCache.insert(summaryKey, summary);
            m_scoreLampRankCache.insert(summaryKey, summary);
        }
    }

    QVariantMap nextState {
        {QStringLiteral("key"), stateKey},
        {QStringLiteral("contentRevision"), chartContentRevisionForData(stateKey, chartData)},
        {QStringLiteral("scoreRevision"), scoreRevision},
        {QStringLiteral("listRevision"), listRevision},
        {QStringLiteral("item"), item},
        {QStringLiteral("chartData"), chartData},
        {QStringLiteral("chartWrapper"), chartWrapperForData(chartData)},
        {QStringLiteral("scoreList"), summary.value(QStringLiteral("scoreList"))},
        {QStringLiteral("summary"), summary},
        {QStringLiteral("bestStats"), summary.value(QStringLiteral("bestStats"))},
        {QStringLiteral("scoreCounts"), summary.value(QStringLiteral("scoreCounts"))},
        {QStringLiteral("scoreOptionIds"), buildScoreOptionIds(summary)},
        {QStringLiteral("difficultyState"), buildDifficultyState(chartData, scoreRevision, listRevision)},
    };

    m_selectedState = nextState;
    m_selectedStateKey = stateKey;
    if (!stateKey.isEmpty()) {
        m_selectedStateCache.insert(stateKey, nextState);
        m_scoreSummaryCache.insert(stateKey, summary);
        m_scoreLampRankCache.insert(stateKey, summary);
    }

    return {
        {QStringLiteral("changed"), true},
        {QStringLiteral("state"), m_selectedState},
    };
}

QVariantMap Lr2SelectStateCache::scoreSummaryForItem(const QVariant& item,
                                                     int scoreRevision,
                                                     int listRevision) {
    syncRevisions(scoreRevision, listRevision);
    return cachedScoreSummary(item);
}

QVariantMap Lr2SelectStateCache::scoreLampRankForItem(const QVariant& item,
                                                      int scoreRevision,
                                                      int listRevision) {
    syncRevisions(scoreRevision, listRevision);
    return cachedScoreLampRank(item);
}

QVariantList Lr2SelectStateCache::scoreOptionIdsForItem(const QVariant& item,
                                                        int scoreRevision,
                                                        int listRevision) {
    syncRevisions(scoreRevision, listRevision);
    return cachedScoreOptionIds(item);
}

QVariantMap Lr2SelectStateCache::folderSummaryFromScores(const QVariant& result) const {
    const QVariant scoreResult = normalizedScoreQueryResult(result);
    const QVariant scoresValue = valueProperty(scoreResult, "scores");
    const QVariantMap scores = scoresValue.canConvert<QVariantMap>()
        ? scoresValue.toMap()
        : QVariantMap();
    const int unplayed = std::max(0, toInt(valueProperty(scoreResult, "unplayed")));

    QVariantMap counts {
        {QStringLiteral("total"), unplayed},
        {QStringLiteral("play"), 0},
        {QStringLiteral("clear"), 0},
        {QStringLiteral("fail"), 0},
        {QStringLiteral("noplay"), unplayed},
        {QStringLiteral("assist"), 0},
        {QStringLiteral("lightAssist"), 0},
        {QStringLiteral("easy"), 0},
        {QStringLiteral("normal"), 0},
        {QStringLiteral("hard"), 0},
        {QStringLiteral("exhard"), 0},
        {QStringLiteral("fc"), 0},
        {QStringLiteral("perfect"), 0},
        {QStringLiteral("max"), 0},
    };

    QVariantList lamps {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    QVariantList ranks {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };
    lamps[0] = unplayed;
    ranks[0] = unplayed;

    int folderLamp = 5;
    bool seenScoreList = false;
    const auto increment = [](QVariantMap& map, const QString& key) {
        map.insert(key, map.value(key).toInt() + 1);
    };

    for (auto it = scores.cbegin(); it != scores.cend(); ++it) {
        const QVariantList scoreList = toList(it.value());
        increment(counts, QStringLiteral("total"));

        if (scoreList.isEmpty()) {
            increment(counts, QStringLiteral("noplay"));
            lamps[0] = lamps.at(0).toInt() + 1;
            ranks[0] = ranks.at(0).toInt() + 1;
            continue;
        }

        seenScoreList = true;
        increment(counts, QStringLiteral("play"));
        const QString clearType = bestClearTypeForScores(scoreList);
        if (clearType != QStringLiteral("FAILED") && clearType != QStringLiteral("NOPLAY")) {
            increment(counts, QStringLiteral("clear"));
        }

        if (clearType == QStringLiteral("FAILED")) {
            increment(counts, QStringLiteral("fail"));
        } else if (clearType == QStringLiteral("AEASY")) {
            increment(counts, QStringLiteral("assist"));
        } else if (clearType == QStringLiteral("EASY")) {
            increment(counts, QStringLiteral("easy"));
        } else if (clearType == QStringLiteral("NORMAL")) {
            increment(counts, QStringLiteral("normal"));
        } else if (clearType == QStringLiteral("HARD")) {
            increment(counts, QStringLiteral("hard"));
        } else if (clearType == QStringLiteral("EXHARD")) {
            increment(counts, QStringLiteral("exhard"));
        } else if (clearType == QStringLiteral("FC")) {
            increment(counts, QStringLiteral("fc"));
        } else if (clearType == QStringLiteral("PERFECT")) {
            increment(counts, QStringLiteral("perfect"));
        } else if (clearType == QStringLiteral("MAX")) {
            increment(counts, QStringLiteral("max"));
        } else {
            increment(counts, QStringLiteral("noplay"));
        }

        folderLamp = std::min(folderLamp, clearTypeLamp(clearType));
        const int lampIndex = std::clamp(beatorajaClearTypeIndex(clearType), 0, 10);
        lamps[lampIndex] = lamps.at(lampIndex).toInt() + 1;

        const QVariant best = bestScoreByPoints(scoreList);
        const QVariant bestResult = valueProperty(best, "result");
        const qreal maxPoints = toReal(valueProperty(bestResult, "maxPoints"));
        if (maxPoints <= 0.0) {
            ranks[0] = ranks.at(0).toInt() + 1;
            continue;
        }

        const int rank = std::clamp(
            static_cast<int>(std::floor(toReal(valueProperty(bestResult, "points")) * 27.0 / maxPoints)),
            0,
            27);
        ranks[rank] = ranks.at(rank).toInt() + 1;
    }

    return {
        {QStringLiteral("lamp"), unplayed > 0 ? 0 : (seenScoreList ? folderLamp : 0)},
        {QStringLiteral("counts"), counts},
        {QStringLiteral("distribution"), QVariantMap {
            {QStringLiteral("lamps"), lamps},
            {QStringLiteral("ranks"), ranks},
        }},
    };
}

QVariantMap Lr2SelectStateCache::difficultyStateForChart(const QVariant& chart,
                                                         int scoreRevision,
                                                         int listRevision) {
    syncRevisions(scoreRevision, listRevision);
    return buildDifficultyState(chart, scoreRevision, listRevision);
}

qreal Lr2SelectStateCache::barGraphValue(int type,
                                         int logicalCount,
                                         qreal currentNormalizedVisualIndex) const {
    auto clamp01 = [](qreal value) {
        return std::max<qreal>(0.0, std::min<qreal>(1.0, value));
    };
    auto normalized = [clamp01](qreal value, qreal maximum) {
        return maximum > 0.0 ? clamp01(value / maximum) : 0.0;
    };

    switch (type) {
    case 101:
        return logicalCount > 1
            ? clamp01(currentNormalizedVisualIndex / std::max(1, logicalCount - 1))
            : 0.0;
    case 102:
        return 1.0;
    default:
        break;
    }

    const QVariantMap stats = m_selectedState.value(QStringLiteral("bestStats")).toMap();
    const QVariant chart = m_selectedState.value(QStringLiteral("chartData"));
    const QVariantMap difficultyState = m_selectedState.value(QStringLiteral("difficultyState")).toMap();
    const int keymode = toInt(valueProperty(chart, "keymode"));
    const int levelThreshold = (keymode == 5 || keymode == 10) ? 9 : 12;

    auto stat = [&stats](const QString& key) {
        return toReal(stats.value(key));
    };
    auto chartNotes = [&chart]() {
        return toReal(valueProperty(chart, "normalNoteCount"))
            + toReal(valueProperty(chart, "scratchCount"))
            + toReal(valueProperty(chart, "lnCount"))
            + toReal(valueProperty(chart, "bssCount"));
    };
    auto difficultyGraphValue = [&difficultyState, levelThreshold](int diff) {
        const QVariantList levels = toList(difficultyState.value(QStringLiteral("levels")));
        const int level = diff >= 1 && diff < levels.size() ? toInt(levels.at(diff)) : 0;
        return level > 0 ? static_cast<qreal>(level) / std::max(1, levelThreshold) : 0.0;
    };

    switch (type) {
    case 110:
    case 111:
    case 112:
    case 113:
        return normalized(stat(QStringLiteral("exscore")), stat(QStringLiteral("maxPoints")));
    case 114:
    case 115:
        return 0.0;
    case 103: {
        const qreal level = toReal(valueProperty(chart, "playLevel"));
        return level > 0.0 ? level / std::max(1, levelThreshold) : 0.0;
    }
    case 105:
    case 106:
    case 107:
    case 108:
    case 109:
        return difficultyGraphValue(type - 104);
    case 140:
    case 141:
    case 142:
    case 143:
    case 144:
    case 145:
    case 146:
    case 147:
        return barGraphValue(type - 100, logicalCount, currentNormalizedVisualIndex);
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return difficultyGraphValue(type - 4);
    case 40:
        return normalized(stat(QStringLiteral("pg")), stat(QStringLiteral("totalJudgements")));
    case 41:
        return normalized(stat(QStringLiteral("gr")), stat(QStringLiteral("totalJudgements")));
    case 42:
        return normalized(stat(QStringLiteral("gd")), stat(QStringLiteral("totalJudgements")));
    case 43:
        return normalized(stat(QStringLiteral("bd")), stat(QStringLiteral("totalJudgements")));
    case 44:
        return normalized(stat(QStringLiteral("pr")), stat(QStringLiteral("totalJudgements")));
    case 45:
        return normalized(stat(QStringLiteral("maxCombo")), chartNotes());
    case 46:
    case 47:
        return normalized(stat(QStringLiteral("exscore")), stat(QStringLiteral("maxPoints")));
    default:
        return 0.0;
    }
}

QVariantMap Lr2SelectStateCache::barCellCore(const QVariant& item,
                                             const QVariant& barTitleTypes,
                                             int scoreRevision,
                                             int listRevision,
                                             int folderLampRevision) {
    return barCellCoreMap(cachedBarCellCoreData(
        item,
        barTitleTypes,
        scoreRevision,
        listRevision,
        folderLampRevision));
}

Lr2SelectStateCache::BarCellCoreData Lr2SelectStateCache::cachedBarCellCoreData(
    const QVariant& item,
    const QVariant& barTitleTypes,
    int scoreRevision,
    int listRevision,
    int folderLampRevision) {
    syncBarCellRevisions(barTitleTypes, scoreRevision, listRevision, folderLampRevision);

    const QString key = barCellKey(item);
    if (m_barCellDataCache.contains(key)) {
        return m_barCellDataCache.value(key);
    }

    const ItemKind kind = itemKind(item);
    BarCellCoreData core;
    core.key = key;
    core.valid = kind != ItemKind::Empty;

    switch (kind) {
    case ItemKind::Empty:
        break;
    case ItemKind::Ranking: {
        core.ranking = true;
        core.text = toString(valueProperty(item, "title"));
        core.bodyType = 0;
        core.playLevel = toInt(valueProperty(item, "level"));
        core.keymode = toInt(valueProperty(item, "keymode"));
        core.lamp = clearTypeLamp(toString(valueProperty(item, "bestClearType")));
        core.rank = rankingEntryRank(item);
        break;
    }
    case ItemKind::Chart: {
        core.chartLike = true;
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 2, 0);
        core.bodyType = 0;
        core.playLevel = toInt(valueProperty(item, "playLevel"));
        core.difficulty = entryDifficulty(item);
        core.keymode = toInt(valueProperty(item, "keymode"));
        const QVariantMap summary = cachedScoreLampRank(item);
        core.lamp = summary.value(QStringLiteral("lamp")).toInt();
        core.rank = summary.value(QStringLiteral("rank")).toInt();
        core.labelMask = chartLabelMask(item);
        break;
    }
    case ItemKind::Entry: {
        core.entryLike = true;
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 8, 0);
        core.bodyType = 0;
        core.playLevel = toInt(valueProperty(item, "level"));
        core.keymode = toInt(valueProperty(item, "keymode"));
        const QVariantMap summary = cachedScoreLampRank(item);
        core.lamp = summary.value(QStringLiteral("lamp")).toInt();
        core.rank = summary.value(QStringLiteral("rank")).toInt();
        break;
    }
    case ItemKind::Table:
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 6, 0);
        core.bodyType = 2;
        core.folderLike = true;
        core.lamp = entryLamp(item);
        break;
    case ItemKind::Level:
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 6, 0);
        core.bodyType = 2;
        core.folderLike = true;
        core.lamp = entryLamp(item);
        break;
    case ItemKind::Course: {
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 7, 0);
        core.bodyType = 8;
        const QVariantMap summary = cachedScoreLampRank(item);
        core.lamp = summary.value(QStringLiteral("lamp")).toInt();
        core.rank = summary.value(QStringLiteral("rank")).toInt();
        break;
    }
    case ItemKind::Folder:
        core.text = displayName(item);
        core.titleType = titleTypeWithFallback(barTitleTypes, 4, 0);
        core.folderLike = true;
        core.lamp = entryLamp(item);
        break;
    case ItemKind::Other: {
        core.text = displayName(item);
        core.keymode = toInt(valueProperty(item, "keymode"));
        const QVariantMap summary = cachedScoreLampRank(item);
        core.lamp = summary.value(QStringLiteral("lamp")).toInt();
        core.rank = summary.value(QStringLiteral("rank")).toInt();
        break;
    }
    }

    if (core.folderLike) {
        const QVariantMap distribution = folderDistribution(item);
        core.graphLamps = toList(distribution.value(QStringLiteral("lamps")));
        core.graphRanks = toList(distribution.value(QStringLiteral("ranks")));
    }

    m_barCellDataCache.insert(key, core);
    return core;
}

QVariantMap Lr2SelectStateCache::barCellCoreMap(const BarCellCoreData& core) const {
    return {
        {QStringLiteral("key"), core.key},
        {QStringLiteral("valid"), core.valid},
        {QStringLiteral("text"), core.text},
        {QStringLiteral("titleType"), core.titleType},
        {QStringLiteral("bodyType"), core.bodyType},
        {QStringLiteral("playLevel"), core.playLevel},
        {QStringLiteral("difficulty"), core.difficulty},
        {QStringLiteral("keymode"), core.keymode},
        {QStringLiteral("ranking"), core.ranking},
        {QStringLiteral("chartLike"), core.chartLike},
        {QStringLiteral("entryLike"), core.entryLike},
        {QStringLiteral("folderLike"), core.folderLike},
        {QStringLiteral("lamp"), core.lamp},
        {QStringLiteral("rank"), core.rank},
        {QStringLiteral("labelMask"), core.labelMask},
        {QStringLiteral("graphLamps"), core.graphLamps},
        {QStringLiteral("graphRanks"), core.graphRanks},
    };
}

void Lr2SelectStateCache::updateBarCell(QObject* cellObject,
                                        int row,
                                        const QVariant& item,
                                        const QVariant& barTitleTypes,
                                        int scoreRevision,
                                        int listRevision,
                                        int folderLampRevision) {
    auto* cell = qobject_cast<Lr2SelectBarCell*>(cellObject);
    if (!cell) {
        return;
    }

    const BarCellCoreData core = cachedBarCellCoreData(item, barTitleTypes, scoreRevision, listRevision, folderLampRevision);
    cell->setCore(
        row,
        core.valid,
        core.text,
        core.titleType,
        core.bodyType,
        core.playLevel,
        core.difficulty,
        core.keymode,
        core.ranking,
        core.chartLike,
        core.entryLike,
        core.folderLike,
        core.lamp,
        core.rank,
        core.labelMask,
        core.graphLamps,
        core.graphRanks);
}

void Lr2SelectStateCache::clearScoreCaches() {
    m_selectedStateCache.clear();
    m_scoreSummaryCache.clear();
    m_scoreLampRankCache.clear();
    m_scoreOptionIdsCache.clear();
    clearBarCellCache();
    m_selectedState = {};
    m_selectedStateKey.clear();
}

void Lr2SelectStateCache::clearDifficultyCache() {
    m_difficultyStateCache.clear();
    clearScoreCaches();
}

void Lr2SelectStateCache::clearBarCellCache() {
    m_barCellDataCache.clear();
}

void Lr2SelectStateCache::syncRevisions(int scoreRevision, int listRevision) {
    if (m_scoreRevision == scoreRevision && m_listRevision == listRevision) {
        return;
    }
    m_scoreRevision = scoreRevision;
    m_listRevision = listRevision;
    clearDifficultyCache();
}

void Lr2SelectStateCache::syncBarCellRevisions(const QVariant& barTitleTypes,
                                               int scoreRevision,
                                               int listRevision,
                                               int folderLampRevision) {
    syncRevisions(scoreRevision, listRevision);
    const QString titleKey = barTitleTypesKey(barTitleTypes);
    if (m_barCellScoreRevision == scoreRevision
            && m_barCellListRevision == listRevision
            && m_barCellFolderLampRevision == folderLampRevision
            && m_barCellTitleTypesKey == titleKey) {
        return;
    }

    m_barCellScoreRevision = scoreRevision;
    m_barCellListRevision = listRevision;
    m_barCellFolderLampRevision = folderLampRevision;
    m_barCellTitleTypesKey = titleKey;
    clearBarCellCache();
}

QVariant Lr2SelectStateCache::chartDataForItem(const QVariant& item,
                                               bool rankingMode,
                                               const QVariant& rankingBaseItem) const {
    if (rankingMode && rankingBaseItem.isValid() && !rankingBaseItem.isNull()) {
        return rankingBaseItem;
    }
    const ItemKind kind = itemKind(item);
    return kind == ItemKind::Chart || kind == ItemKind::Entry ? item : QVariant();
}

QVariantList Lr2SelectStateCache::entryScores(const QVariant& item) const {
    const QString id = entryIdentifier(item);
    if (id.isEmpty()) {
        return {};
    }
    return toList(indexedValue(m_scores, id));
}

QVariantMap Lr2SelectStateCache::buildScoreSummary(const QVariantList& scoreList) const {
    if (scoreList.isEmpty()) {
        return emptyScoreSummary();
    }

    QVariantMap counts {
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
    QVariant bestScore;
    qreal bestRate = -1.0;
    QString bestClearType = QStringLiteral("NOPLAY");
    int bestClearPriority = 0;
    int minBadPoor = -1;

    for (const QVariant& score : scoreList) {
        const QVariant result = valueProperty(score, "result");
        if (!result.isValid() || result.isNull()) {
            continue;
        }

        counts.insert(QStringLiteral("play"), counts.value(QStringLiteral("play")).toInt() + 1);
        const QString clearType = clearTypeOf(score);
        const int clearPriority = clearTypePriority(clearType);
        if (clearPriority > bestClearPriority) {
            bestClearPriority = clearPriority;
            bestClearType = clearType;
        }

        if (clearType != QStringLiteral("FAILED") && clearType != QStringLiteral("NOPLAY")) {
            counts.insert(QStringLiteral("clear"), counts.value(QStringLiteral("clear")).toInt() + 1);
        }
        if (clearType == QStringLiteral("FAILED")) {
            counts.insert(QStringLiteral("fail"), counts.value(QStringLiteral("fail")).toInt() + 1);
        } else if (clearType == QStringLiteral("AEASY")) {
            counts.insert(QStringLiteral("assist"), counts.value(QStringLiteral("assist")).toInt() + 1);
        } else if (clearType == QStringLiteral("EASY")) {
            counts.insert(QStringLiteral("easy"), counts.value(QStringLiteral("easy")).toInt() + 1);
        } else if (clearType == QStringLiteral("NORMAL")) {
            counts.insert(QStringLiteral("normal"), counts.value(QStringLiteral("normal")).toInt() + 1);
        } else if (clearType == QStringLiteral("HARD")) {
            counts.insert(QStringLiteral("hard"), counts.value(QStringLiteral("hard")).toInt() + 1);
        } else if (clearType == QStringLiteral("EXHARD")) {
            counts.insert(QStringLiteral("exhard"), counts.value(QStringLiteral("exhard")).toInt() + 1);
        } else if (clearType == QStringLiteral("FC")) {
            counts.insert(QStringLiteral("fc"), counts.value(QStringLiteral("fc")).toInt() + 1);
        } else if (clearType == QStringLiteral("PERFECT")) {
            counts.insert(QStringLiteral("perfect"), counts.value(QStringLiteral("perfect")).toInt() + 1);
        } else if (clearType == QStringLiteral("MAX")) {
            counts.insert(QStringLiteral("max"), counts.value(QStringLiteral("max")).toInt() + 1);
        } else {
            counts.insert(QStringLiteral("noplay"), counts.value(QStringLiteral("noplay")).toInt() + 1);
        }

        const int badPoor = badPoorForScore(score);
        minBadPoor = minBadPoor < 0 ? badPoor : std::min(minBadPoor, badPoor);

        const qreal maxPoints = toReal(valueProperty(result, "maxPoints"));
        if (maxPoints > 0.0) {
            const qreal rate = toReal(valueProperty(result, "points")) / maxPoints;
            if (rate > bestRate) {
                bestRate = rate;
                bestScore = score;
            }
        }
    }

    counts.insert(QStringLiteral("minBadPoor"), std::max(0, minBadPoor));
    const qreal scoreRate = std::max<qreal>(0.0, bestRate);
    return {
        {QStringLiteral("scoreList"), scoreList},
        {QStringLiteral("bestScore"), bestScore},
        {QStringLiteral("bestStats"), bestScore.isValid() && !bestScore.isNull() ? statsForScore(bestScore) : QVariant()},
        {QStringLiteral("scoreCounts"), counts},
        {QStringLiteral("clearType"), bestClearType},
        {QStringLiteral("lamp"), clearTypeLamp(bestClearType)},
        {QStringLiteral("rank"), rankForScoreRate(scoreRate)},
        {QStringLiteral("scoreRate"), scoreRate},
    };
}

QVariantMap Lr2SelectStateCache::buildScoreLampRank(const QVariantList& scoreList) const {
    if (scoreList.isEmpty()) {
        return {
            {QStringLiteral("clearType"), QStringLiteral("NOPLAY")},
            {QStringLiteral("lamp"), 0},
            {QStringLiteral("rank"), 0},
            {QStringLiteral("scoreRate"), 0},
        };
    }

    qreal bestRate = -1.0;
    QString bestClearType = QStringLiteral("NOPLAY");
    int bestClearPriority = 0;
    for (const QVariant& score : scoreList) {
        const QVariant result = valueProperty(score, "result");
        if (!result.isValid() || result.isNull()) {
            continue;
        }
        const QString clearType = clearTypeOf(score);
        const int clearPriority = clearTypePriority(clearType);
        if (clearPriority > bestClearPriority) {
            bestClearPriority = clearPriority;
            bestClearType = clearType;
        }

        const qreal maxPoints = toReal(valueProperty(result, "maxPoints"));
        if (maxPoints > 0.0) {
            bestRate = std::max(bestRate, toReal(valueProperty(result, "points")) / maxPoints);
        }
    }

    const qreal scoreRate = std::max<qreal>(0.0, bestRate);
    return {
        {QStringLiteral("clearType"), bestClearType},
        {QStringLiteral("lamp"), clearTypeLamp(bestClearType)},
        {QStringLiteral("rank"), rankForScoreRate(scoreRate)},
        {QStringLiteral("scoreRate"), scoreRate},
    };
}

QVariantMap Lr2SelectStateCache::buildDifficultyState(const QVariant& chart,
                                                      int scoreRevision,
                                                      int listRevision) {
    Q_UNUSED(scoreRevision)
    Q_UNUSED(listRevision)

    const QString key = chart.isValid() && !chart.isNull()
        ? chartDifficultyGroupKey(chart) + QStringLiteral("\n") + entrySelectionKey(chart, 0)
        : QStringLiteral("empty");
    if (m_difficultyStateCache.contains(key)) {
        return m_difficultyStateCache.value(key);
    }

    QVariantList charts = toList(indexedValue(m_chartGroupCache, chartDifficultyGroupKey(chart)));
    if (charts.isEmpty() && chart.isValid() && !chart.isNull()) {
        charts.append(chart);
    }

    QVariantList byDiff {QVariant(), QVariant(), QVariant(), QVariant(), QVariant(), QVariant()};
    QVariantList counts {0, 0, 0, 0, 0, 0};
    QVariantList levels {0, 0, 0, 0, 0, 0};
    QVariantList lamps {0, 0, 0, 0, 0, 0};

    for (const QVariant& candidate : charts) {
        const int diff = entryDifficulty(candidate);
        if (diff < 1 || diff > 5) {
            continue;
        }
        counts[diff] = counts.at(diff).toInt() + 1;
        if (byDiff.at(diff).isNull()
                || (chart.isValid() && !chart.isNull()
                    && entrySelectionKey(candidate, 0) == entrySelectionKey(chart, 0))) {
            byDiff[diff] = candidate;
        }
    }
    for (int diff = 1; diff <= 5; ++diff) {
        const QVariant candidate = byDiff.at(diff);
        if (!candidate.isValid() || candidate.isNull()) {
            continue;
        }
        levels[diff] = entryPlayLevel(candidate);
        lamps[diff] = entryLamp(candidate);
    }

    QVariantMap state {
        {QStringLiteral("key"), key},
        {QStringLiteral("charts"), byDiff},
        {QStringLiteral("counts"), counts},
        {QStringLiteral("levels"), levels},
        {QStringLiteral("lamps"), lamps},
    };
    m_difficultyStateCache.insert(key, state);
    return state;
}

QVariantList Lr2SelectStateCache::buildScoreOptionIds(const QVariantMap& summary) const {
    const QVariantList scoreList = toList(summary.value(QStringLiteral("scoreList")));
    if (scoreList.isEmpty()) {
        return {};
    }

    QSet<int> lookup;
    QVariantList ids;
    const QString clearType = toString(summary.value(QStringLiteral("clearType")));
    if (clearType == QStringLiteral("AEASY")) {
        appendUniqueOption(lookup, ids, 124);
        appendUniqueOption(lookup, ids, 1100);
        appendUniqueOption(lookup, ids, 121);
    } else if (clearType == QStringLiteral("EASY")) {
        appendUniqueOption(lookup, ids, 121);
    } else if (clearType == QStringLiteral("NORMAL")) {
        appendUniqueOption(lookup, ids, 118);
    } else if (clearType == QStringLiteral("HARD")) {
        appendUniqueOption(lookup, ids, 119);
    } else if (clearType == QStringLiteral("EXHARD")) {
        appendUniqueOption(lookup, ids, 119);
        appendUniqueOption(lookup, ids, 125);
        appendUniqueOption(lookup, ids, 1102);
    } else if (clearType == QStringLiteral("FC")) {
        appendUniqueOption(lookup, ids, 105);
    } else if (clearType == QStringLiteral("PERFECT")) {
        appendUniqueOption(lookup, ids, 122);
        appendUniqueOption(lookup, ids, 1103);
    } else if (clearType == QStringLiteral("MAX")) {
        appendUniqueOption(lookup, ids, 122);
        appendUniqueOption(lookup, ids, 1104);
    }

    for (const QVariant& score : scoreList) {
        const QVariant result = valueProperty(score, "result");
        if (!result.isValid() || result.isNull()) {
            continue;
        }
        switch (toInt(valueProperty(result, "noteOrderAlgorithm"))) {
        case 1:
            appendUniqueOption(lookup, ids, 127);
            break;
        case 2:
        case 5:
            appendUniqueOption(lookup, ids, 128);
            break;
        case 3:
        case 6:
            appendUniqueOption(lookup, ids, 129);
            break;
        case 4:
            appendUniqueOption(lookup, ids, 1128);
            appendUniqueOption(lookup, ids, 130);
            break;
        default:
            appendUniqueOption(lookup, ids, 126);
            break;
        }

        if (toInt(valueProperty(result, "dpOptions")) == 2) {
            const int keymode = toInt(valueProperty(result, "keymode"));
            appendUniqueOption(lookup, ids, keymode == 5 || keymode == 7 ? 145 : 144);
        }
    }

    std::sort(ids.begin(), ids.end(), [](const QVariant& lhs, const QVariant& rhs) {
        return lhs.toInt() < rhs.toInt();
    });
    return ids;
}

QVariantMap Lr2SelectStateCache::statsForScore(const QVariant& score) const {
    const QVariant result = valueProperty(score, "result");
    if (!result.isValid() || result.isNull()) {
        return {};
    }

    const QVariant counts = valueProperty(result, "judgementCounts");
    const int pg = judgementCount(counts, kJudgementPerfect);
    const int gr = judgementCount(counts, kJudgementGreat);
    const int gd = judgementCount(counts, kJudgementGood);
    const int bd = judgementCount(counts, kJudgementBad);
    const int poor = judgementCount(counts, kJudgementPoor);
    const int miss = judgementCount(counts, kJudgementEmptyPoor);
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
        {QStringLiteral("maxCombo"), toInt(valueProperty(result, "maxCombo"))},
        {QStringLiteral("score"), toReal(valueProperty(result, "points"))},
        {QStringLiteral("exscore"), toReal(valueProperty(result, "points"))},
        {QStringLiteral("maxPoints"), toReal(valueProperty(result, "maxPoints"))},
        {QStringLiteral("early"), QVariant()},
        {QStringLiteral("late"), QVariant()},
        {QStringLiteral("totalEarly"), 0},
        {QStringLiteral("totalLate"), 0},
    };
}

QVariantMap Lr2SelectStateCache::cachedScoreSummary(const QVariant& item) {
    const QString key = scoreSummaryKey(item);
    if (key.isEmpty()) {
        return emptyScoreSummary();
    }
    if (!m_scoreSummaryCache.contains(key)) {
        const QVariantMap summary = buildScoreSummary(entryScores(item));
        m_scoreSummaryCache.insert(key, summary);
        m_scoreLampRankCache.insert(key, summary);
    }
    return m_scoreSummaryCache.value(key);
}

QVariantMap Lr2SelectStateCache::cachedScoreLampRank(const QVariant& item) {
    if (itemKind(item) == ItemKind::Ranking) {
        const qreal points = toReal(valueProperty(item, "bestPoints"));
        const qreal maxPoints = toReal(valueProperty(item, "maxPoints"));
        int rank = 0;
        if (maxPoints > 0.0) {
            rank = static_cast<int>(std::floor(points * 9.0 / maxPoints));
            if (rank > 7) {
                rank = 8;
            }
            if (rank < 2 && points > 0.0) {
                rank = 1;
            }
        }
        const QString clearType = toString(valueProperty(item, "bestClearType"));
        return {
            {QStringLiteral("clearType"), clearType.isEmpty() ? QStringLiteral("NOPLAY") : clearType},
            {QStringLiteral("lamp"), clearTypeLamp(clearType)},
            {QStringLiteral("rank"), rank},
            {QStringLiteral("scoreRate"), maxPoints > 0.0 ? points / maxPoints : 0.0},
        };
    }

    const QString key = scoreSummaryKey(item);
    if (key.isEmpty()) {
        return buildScoreLampRank({});
    }
    if (!m_scoreLampRankCache.contains(key)) {
        m_scoreLampRankCache.insert(key, buildScoreLampRank(entryScores(item)));
    }
    return m_scoreLampRankCache.value(key);
}

QVariantList Lr2SelectStateCache::cachedScoreOptionIds(const QVariant& item) {
    const QString key = scoreSummaryKey(item);
    if (key.isEmpty()) {
        return {};
    }
    if (!m_scoreOptionIdsCache.contains(key)) {
        m_scoreOptionIdsCache.insert(key, buildScoreOptionIds(cachedScoreSummary(item)));
    }
    return m_scoreOptionIdsCache.value(key);
}

Lr2SelectStateCache::ItemKind Lr2SelectStateCache::itemKind(const QVariant& item) const {
    if (!item.isValid() || item.isNull()) {
        return ItemKind::Empty;
    }
    if (item.typeId() == QMetaType::QString) {
        return ItemKind::Folder;
    }
    if (toBool(valueProperty(item, "__lr2RankingEntry"))) {
        return ItemKind::Ranking;
    }
    if (item.canConvert<QObject*>()) {
        if (qobject_cast<gameplay_logic::ChartData*>(item.value<QObject*>())) {
            return ItemKind::Chart;
        }
    }

    const QString type = itemTypeName(item);
    if (type.endsWith(QStringLiteral("ChartData"))) {
        return ItemKind::Chart;
    }
    if (type.endsWith(QStringLiteral("Entry"))) {
        return ItemKind::Entry;
    }
    if (type.endsWith(QStringLiteral("Course"))) {
        return ItemKind::Course;
    }
    if (type.endsWith(QStringLiteral("Level"))) {
        return ItemKind::Level;
    }
    if (type.endsWith(QStringLiteral("Table"))) {
        return ItemKind::Table;
    }
    if (valueProperty(item, "path").isValid() && valueProperty(item, "keymode").isValid()) {
        return ItemKind::Chart;
    }
    if (valueProperty(item, "md5").isValid() && valueProperty(item, "level").isValid()) {
        return ItemKind::Entry;
    }
    return ItemKind::Other;
}

QString Lr2SelectStateCache::itemTypeName(const QVariant& item) const {
    if (item.canConvert<QObject*>()) {
        if (QObject* object = item.value<QObject*>()) {
            return QString::fromLatin1(object->metaObject()->className());
        }
    }
    if (const QMetaObject* metaObject = item.metaType().metaObject()) {
        return QString::fromLatin1(metaObject->className());
    }
    return QString::fromLatin1(item.metaType().name());
}

QString Lr2SelectStateCache::entryIdentifier(const QVariant& item) const {
    switch (itemKind(item)) {
    case ItemKind::Ranking:
        return {};
    case ItemKind::Chart:
    case ItemKind::Entry:
        return toString(valueProperty(item, "md5"));
    case ItemKind::Course:
        return toString(valueProperty(item, "identifier"));
    default:
        return {};
    }
}

QString Lr2SelectStateCache::entrySelectionKey(const QVariant& item, int fallbackIndex) const {
    const ItemKind kind = itemKind(item);
    if (kind == ItemKind::Empty) {
        return QStringLiteral("empty");
    }
    if (kind == ItemKind::Ranking) {
        return QStringLiteral("ranking:%1:%2")
            .arg(toString(valueProperty(item, "sourceMd5")))
            .arg(toInt(valueProperty(item, "rankingIndex")));
    }
    if (kind == ItemKind::Chart) {
        QString identity = toString(valueProperty(item, "md5"));
        if (identity.isEmpty()) identity = toString(valueProperty(item, "sha256"));
        if (identity.isEmpty()) identity = toString(valueProperty(item, "path"));
        if (identity.isEmpty()) {
            identity = QStringLiteral("%1\n%2\n%3\n%4\n%5\n%6")
                .arg(toString(valueProperty(item, "title")))
                .arg(toString(valueProperty(item, "subtitle")))
                .arg(toString(valueProperty(item, "artist")))
                .arg(toInt(valueProperty(item, "playLevel")))
                .arg(toInt(valueProperty(item, "keymode")))
                .arg(toInt(valueProperty(item, "difficulty")));
        }
        if (identity.isEmpty()) {
            identity = QStringLiteral("index:%1").arg(fallbackIndex);
        }
        return QStringLiteral("chart:%1").arg(identity);
    }
    if (kind == ItemKind::Entry) {
        QString identity = toString(valueProperty(item, "md5"));
        if (identity.isEmpty()) identity = toString(valueProperty(item, "path"));
        if (identity.isEmpty()) {
            identity = QStringLiteral("%1\n%2\n%3\n%4\n%5")
                .arg(toString(valueProperty(item, "title")))
                .arg(toString(valueProperty(item, "subtitle")))
                .arg(toString(valueProperty(item, "artist")))
                .arg(toString(valueProperty(item, "level")))
                .arg(toInt(valueProperty(item, "difficulty")));
        }
        if (identity.isEmpty()) {
            identity = QStringLiteral("index:%1").arg(fallbackIndex);
        }
        return QStringLiteral("entry:%1").arg(identity);
    }
    if (kind == ItemKind::Course) {
        QString identity = toString(valueProperty(item, "identifier"));
        if (identity.isEmpty()) identity = toString(valueProperty(item, "name"));
        if (identity.isEmpty()) identity = QStringLiteral("index:%1").arg(fallbackIndex);
        return QStringLiteral("course:%1").arg(identity);
    }
    if (kind == ItemKind::Table) {
        QString identity = toString(valueProperty(item, "url"));
        if (identity.isEmpty()) identity = toString(valueProperty(item, "name"));
        if (identity.isEmpty()) identity = QStringLiteral("index:%1").arg(fallbackIndex);
        return QStringLiteral("table:%1").arg(identity);
    }
    if (kind == ItemKind::Level) {
        QString identity = toString(valueProperty(item, "name"));
        if (identity.isEmpty()) identity = QStringLiteral("index:%1").arg(fallbackIndex);
        return QStringLiteral("level:%1").arg(identity);
    }
    if (kind == ItemKind::Folder) {
        return QStringLiteral("folder:%1").arg(item.toString());
    }
    const QString name = displayName(item);
    return QStringLiteral("item:%1").arg(name.isEmpty() ? QStringLiteral("index:%1").arg(fallbackIndex) : name);
}

QString Lr2SelectStateCache::scoreSummaryKey(const QVariant& item) const {
    if (!item.isValid() || item.isNull()) {
        return QStringLiteral("empty");
    }
    const QString id = entryIdentifier(item);
    if (!id.isEmpty()) {
        return QStringLiteral("score:%1").arg(id);
    }
    return entrySelectionKey(item, 0);
}

QString Lr2SelectStateCache::chartDifficultyGroupKey(const QVariant& chart) const {
    if (!chart.isValid() || chart.isNull()) {
        return {};
    }
    return chartGroupKey(chart) + QStringLiteral("\n") + QString::number(toInt(valueProperty(chart, "keymode")));
}

QString Lr2SelectStateCache::chartGroupKey(const QVariant& chart) const {
    if (!chart.isValid() || chart.isNull()) {
        return {};
    }
    QString key = toString(valueProperty(chart, "chartDirectory"));
    if (key.isEmpty()) {
        key = toString(valueProperty(chart, "directory"));
    }
    return key;
}

QString Lr2SelectStateCache::displayName(const QVariant& item) const {
    if (item.typeId() == QMetaType::QString) {
        QString normalized = item.toString();
        normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
        if (normalized.endsWith(QLatin1Char('/'))) {
            normalized.chop(1);
        }
        const int slash = normalized.lastIndexOf(QLatin1Char('/'));
        return slash >= 0 ? normalized.mid(slash + 1) : normalized;
    }

    const QString title = toString(valueProperty(item, "title"));
    const QString subtitle = toString(valueProperty(item, "subtitle"));
    if (!title.isEmpty()) {
        return subtitle.isEmpty() ? title : title + QLatin1Char(' ') + subtitle;
    }
    if (itemKind(item) == ItemKind::Level) {
        const QVariantList history = toList(m_historyStack);
        const QVariant parent = history.isEmpty() ? QVariant() : history.constLast();
        const QString symbol = itemKind(parent) == ItemKind::Table
            ? toString(valueProperty(parent, "symbol"))
            : QString();
        return symbol + toString(valueProperty(item, "name"));
    }
    const QString name = toString(valueProperty(item, "name"));
    return name;
}

QString Lr2SelectStateCache::folderLampKey(const QVariant& item) const {
    const ItemKind kind = itemKind(item);
    if (kind == ItemKind::Folder) {
        return QStringLiteral("folder:%1").arg(item.toString());
    }
    if (kind == ItemKind::Table) {
        QString identity = toString(valueProperty(item, "url"));
        if (identity.isEmpty()) {
            identity = toString(valueProperty(item, "name"));
        }
        return QStringLiteral("table:%1").arg(identity);
    }
    if (kind == ItemKind::Level) {
        const QVariantList history = toList(m_historyStack);
        const QVariant parent = history.isEmpty() ? QVariant() : history.constLast();
        QString parentKey;
        if (itemKind(parent) == ItemKind::Table) {
            parentKey = toString(valueProperty(parent, "url"));
            if (parentKey.isEmpty()) {
                parentKey = toString(valueProperty(parent, "name"));
            }
        }
        return QStringLiteral("level:%1:%2")
            .arg(parentKey, toString(valueProperty(item, "name")));
    }
    return {};
}

QString Lr2SelectStateCache::barCellKey(const QVariant& item) const {
    const ItemKind kind = itemKind(item);
    if (kind == ItemKind::Level) {
        const QString key = folderLampKey(item);
        return QStringLiteral("level:%1").arg(key.isEmpty() ? toString(valueProperty(item, "name")) : key);
    }
    return entrySelectionKey(item, 0);
}

QString Lr2SelectStateCache::barTitleTypesKey(const QVariant& barTitleTypes) const {
    const QVariantList list = toList(barTitleTypes);
    QStringList parts;
    parts.reserve(list.size());
    for (const QVariant& value : list) {
        parts.append(QString::number(toInt(value)));
    }
    return parts.join(QLatin1Char(','));
}

int Lr2SelectStateCache::entryDifficulty(const QVariant& item) const {
    if (itemKind(item) != ItemKind::Chart) {
        return 0;
    }
    const QString path = toString(valueProperty(item, "path"));
    const QVariant cached = indexedValue(m_chartDifficultyCache, path);
    if (cached.isValid()) {
        return toInt(cached);
    }
    return std::max(0, toInt(valueProperty(item, "difficulty")));
}

int Lr2SelectStateCache::entryPlayLevel(const QVariant& item) const {
    if (itemKind(item) == ItemKind::Ranking) {
        return toInt(valueProperty(item, "level"));
    }
    if (itemKind(item) == ItemKind::Chart) {
        return toInt(valueProperty(item, "playLevel"));
    }
    if (itemKind(item) == ItemKind::Entry) {
        return toInt(valueProperty(item, "level"));
    }
    return 0;
}

int Lr2SelectStateCache::entryLamp(const QVariant& item) {
    const ItemKind kind = itemKind(item);
    if (kind == ItemKind::Ranking) {
        return clearTypeLamp(toString(valueProperty(item, "bestClearType")));
    }
    if (kind == ItemKind::Folder || kind == ItemKind::Table || kind == ItemKind::Level) {
        const QString key = folderLampKey(item);
        return key.isEmpty() ? 0 : toInt(indexedValue(m_folderLampByKey, key));
    }
    return cachedScoreLampRank(item).value(QStringLiteral("lamp")).toInt();
}

int Lr2SelectStateCache::titleTypeWithFallback(const QVariant& barTitleTypes,
                                               int preferred,
                                               int fallback) const {
    const QVariantList list = toList(barTitleTypes);
    bool hasPreferred = false;
    bool hasFallback = false;
    for (const QVariant& value : list) {
        const int titleType = toInt(value);
        hasPreferred = hasPreferred || titleType == preferred;
        hasFallback = hasFallback || titleType == fallback;
    }
    if (hasPreferred) {
        return preferred;
    }
    return hasFallback ? fallback : preferred;
}

int Lr2SelectStateCache::rankingEntryRank(const QVariant& item) const {
    const qreal points = toReal(valueProperty(item, "bestPoints"));
    const qreal maxPoints = toReal(valueProperty(item, "maxPoints"));
    if (maxPoints <= 0.0) {
        return 0;
    }

    int rank = static_cast<int>(std::floor(points * 9.0 / maxPoints));
    if (rank > 7) {
        rank = 8;
    }
    if (rank < 2 && points > 0.0) {
        rank = 1;
    }
    return rank;
}

int Lr2SelectStateCache::chartLabelMask(const QVariant& item) const {
    if (itemKind(item) != ItemKind::Chart) {
        return 0;
    }

    int mask = 0;
    if (toInt(valueProperty(item, "lnCount")) + toInt(valueProperty(item, "bssCount")) > 0) {
        mask |= 1 << 0;
    }
    if (toBool(valueProperty(item, "isRandom"))) {
        mask |= 1 << 1;
    }
    if (toInt(valueProperty(item, "mineCount")) > 0) {
        mask |= 1 << 2;
    }
    return mask;
}

QVariantMap Lr2SelectStateCache::folderDistribution(const QVariant& item) const {
    const QString key = folderLampKey(item);
    if (key.isEmpty()) {
        return {};
    }
    const QVariant value = indexedValue(m_folderDistributionByKey, key);
    return value.canConvert<QVariantMap>() ? value.toMap() : QVariantMap();
}

QVariantMap Lr2SelectStateCache::emptyScoreSummary() const {
    return {
        {QStringLiteral("scoreList"), QVariantList {}},
        {QStringLiteral("bestScore"), QVariant()},
        {QStringLiteral("bestStats"), QVariant()},
        {QStringLiteral("scoreCounts"), emptyScoreCounts()},
        {QStringLiteral("clearType"), QStringLiteral("NOPLAY")},
        {QStringLiteral("lamp"), 0},
        {QStringLiteral("rank"), 0},
        {QStringLiteral("scoreRate"), 0},
    };
}

QVariantMap Lr2SelectStateCache::emptyScoreCounts() const {
    return emptyScoreCountsValue();
}

QVariantMap Lr2SelectStateCache::emptyDifficultyState() const {
    return emptyDifficultyStateValue();
}

QVariant Lr2SelectStateCache::chartWrapperForData(const QVariant& chartData) const {
    if (!chartData.isValid() || chartData.isNull()) {
        return {};
    }
    return QVariantMap {{QStringLiteral("chartData"), chartData}};
}
