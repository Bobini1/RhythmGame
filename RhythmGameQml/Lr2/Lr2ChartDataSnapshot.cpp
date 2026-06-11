#include "Lr2ChartDataSnapshot.h"

#include <QHash>
#include <QList>
#include <algorithm>

namespace {

struct ChartSnapshotData {
    QString md5;
    qint64 length = 0;
    int normalNoteCount = 0;
    int scratchCount = 0;
    int lnCount = 0;
    int bssCount = 0;
    int mineCount = 0;
    QVariantList histogramData;
    QVariantList normalDensityData;
    int normalDensityMax = 20;
};

struct NormalDensityData {
    QVariantList buckets;
    int maxDensity = 20;
};

QVariantList numberListFromSeries(const QList<qint64>& series) {
    QVariantList result;
    result.reserve(series.size());
    for (const qint64 value : series) {
        result.append(std::max<qint64>(0, value));
    }
    return result;
}

QVariantList histogramListFromSeries(const QList<QList<qint64>>& histogram) {
    QVariantList result;
    result.reserve(histogram.size());
    for (const auto& series : histogram) {
        result.append(QVariant::fromValue(numberListFromSeries(series)));
    }
    return result;
}

const QList<qint64>& histogramSeries(const QList<QList<qint64>>& histogram, int index) {
    static const QList<qint64> empty;
    return index >= 0 && index < histogram.size() ? histogram.at(index) : empty;
}

qint64 seriesValueAt(const QList<qint64>& series, qsizetype index) {
    return index >= 0 && index < series.size() ? std::max<qint64>(0, series.at(index)) : 0;
}

int densityMax(const QVariantList& buckets) {
    int maxValue = 20;
    for (const QVariant& bucketValue : buckets) {
        const QVariantList bucket = bucketValue.toList();
        qint64 total = 0;
        for (const QVariant& value : bucket) {
            total += std::max<qint64>(0, value.toLongLong());
        }
        if (maxValue < total) {
            maxValue = std::min<int>(static_cast<int>(total / 10) * 10 + 10, 100);
        }
    }
    return maxValue;
}

NormalDensityData normalDensityDataFromHistogram(const QList<QList<qint64>>& histogram) {
    const QList<qint64>& normal = histogramSeries(histogram, 0);
    const QList<qint64>& scratch = histogramSeries(histogram, 1);
    const QList<qint64>& ln = histogramSeries(histogram, 2);
    const QList<qint64>& bss = histogramSeries(histogram, 3);
    const QList<qint64>& mine = histogramSeries(histogram, 4);
    const qsizetype count = std::max({normal.size(), scratch.size(), ln.size(), bss.size(), mine.size()});
    QVariantList buckets;
    buckets.reserve(count);
    for (qsizetype i = 0; i < count; ++i) {
        QVariantList bucket;
        bucket.reserve(7);
        bucket.append(0);
        bucket.append(seriesValueAt(bss, i));
        bucket.append(seriesValueAt(scratch, i));
        bucket.append(0);
        bucket.append(seriesValueAt(ln, i));
        bucket.append(seriesValueAt(normal, i));
        bucket.append(seriesValueAt(mine, i));
        buckets.append(QVariant::fromValue(bucket));
    }
    return {
        .buckets = buckets,
        .maxDensity = densityMax(buckets),
    };
}

QHash<QObject*, NormalDensityData>& normalDensityCache() {
    static QHash<QObject*, NormalDensityData> cache;
    return cache;
}

const NormalDensityData& cachedNormalDensityData(gameplay_logic::ChartData* chartData) {
    static const NormalDensityData empty;
    if (!chartData) {
        return empty;
    }

    auto& cache = normalDensityCache();
    auto it = cache.find(chartData);
    if (it != cache.end()) {
        return it.value();
    }

    QObject::connect(chartData, &QObject::destroyed, [](QObject* object) {
        normalDensityCache().remove(object);
    });
    it = cache.insert(chartData, normalDensityDataFromHistogram(chartData->getHistogramData()));
    return it.value();
}

bool histogramHasData(const QVariantList& histogram) {
    for (const QVariant& seriesValue : histogram) {
        if (!seriesValue.toList().isEmpty()) {
            return true;
        }
    }
    return false;
}

gameplay_logic::ChartData* chartDataObject(QObject* object) {
    return qobject_cast<gameplay_logic::ChartData*>(object);
}

ChartSnapshotData chartSnapshotData(gameplay_logic::ChartData* chartData) {
    if (!chartData) {
        return {};
    }

    const NormalDensityData& normalDensity = cachedNormalDensityData(chartData);
    return {
        .md5 = chartData->getMd5(),
        .length = chartData->getLength(),
        .normalNoteCount = chartData->getNormalNoteCount(),
        .scratchCount = chartData->getScratchCount(),
        .lnCount = chartData->getLnCount(),
        .bssCount = chartData->getBssCount(),
        .mineCount = chartData->getMineCount(),
        .histogramData = histogramListFromSeries(chartData->getHistogramData()),
        .normalDensityData = normalDensity.buckets,
        .normalDensityMax = normalDensity.maxDensity,
    };
}

} // namespace

Lr2ChartDataSnapshot::Lr2ChartDataSnapshot(QObject* parent)
    : QObject(parent) {}

QVariant Lr2ChartDataSnapshot::chart() const {
    return QVariant::fromValue(static_cast<QObject*>(m_chart.data()));
}

void Lr2ChartDataSnapshot::setChart(QObject* value) {
    if (m_chart == value) {
        return;
    }
    m_chart = value;
    refresh();
    emit chartChanged();
}

void Lr2ChartDataSnapshot::setChart(const QVariant& value) {
    setChart(value.value<QObject*>());
}

bool Lr2ChartDataSnapshot::hasHistogram() const { return m_hasHistogram; }
QString Lr2ChartDataSnapshot::md5() const { return m_md5; }
qint64 Lr2ChartDataSnapshot::length() const { return m_length; }
int Lr2ChartDataSnapshot::normalNoteCount() const { return m_normalNoteCount; }
int Lr2ChartDataSnapshot::scratchCount() const { return m_scratchCount; }
int Lr2ChartDataSnapshot::lnCount() const { return m_lnCount; }
int Lr2ChartDataSnapshot::bssCount() const { return m_bssCount; }
int Lr2ChartDataSnapshot::mineCount() const { return m_mineCount; }
QVariantList Lr2ChartDataSnapshot::histogramData() const { return m_histogramData; }
QVariantList Lr2ChartDataSnapshot::normalDensityData() const { return m_normalDensityData; }
int Lr2ChartDataSnapshot::normalDensityMax() const { return m_normalDensityMax; }

void Lr2ChartDataSnapshot::refresh() {
    const ChartSnapshotData next = chartSnapshotData(chartDataObject(m_chart.data()));
    const QString& nextMd5 = next.md5;
    const qint64 nextLength = next.length;
    const int nextNormal = next.normalNoteCount;
    const int nextScratch = next.scratchCount;
    const int nextLn = next.lnCount;
    const int nextBss = next.bssCount;
    const int nextMine = next.mineCount;
    const QVariantList& nextHistogram = next.histogramData;
    const QVariantList& nextNormalDensity = next.normalDensityData;
    const int nextNormalDensityMax = next.normalDensityMax;
    const bool nextHasHistogram = histogramHasData(nextHistogram);

    if (m_hasHistogram == nextHasHistogram
            && m_md5 == nextMd5
            && m_length == nextLength
            && m_normalNoteCount == nextNormal
            && m_scratchCount == nextScratch
            && m_lnCount == nextLn
            && m_bssCount == nextBss
            && m_mineCount == nextMine
            && m_histogramData == nextHistogram
            && m_normalDensityData == nextNormalDensity
            && m_normalDensityMax == nextNormalDensityMax) {
        return;
    }

    m_hasHistogram = nextHasHistogram;
    m_md5 = nextMd5;
    m_length = nextLength;
    m_normalNoteCount = nextNormal;
    m_scratchCount = nextScratch;
    m_lnCount = nextLn;
    m_bssCount = nextBss;
    m_mineCount = nextMine;
    m_histogramData = nextHistogram;
    m_normalDensityData = nextNormalDensity;
    m_normalDensityMax = nextNormalDensityMax;
    emit dataChanged();
}
