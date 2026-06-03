#include "Lr2ChartDataSnapshot.h"

#include <QList>
#include <QStringList>
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

bool histogramHasData(const QVariantList& histogram) {
    for (const QVariant& seriesValue : histogram) {
        if (!seriesValue.toList().isEmpty()) {
            return true;
        }
    }
    return false;
}

QString histogramRevision(const QVariantList& histogram) {
    QStringList parts;
    for (int i = 0; i < 6; ++i) {
        const QVariant series = i < histogram.size() ? histogram.at(i) : QVariant {};
        parts.append(QString::number(series.toList().size()));
    }
    return parts.join(QLatin1Char(':'));
}

gameplay_logic::ChartData* chartDataObject(QObject* object) {
    return qobject_cast<gameplay_logic::ChartData*>(object);
}

ChartSnapshotData chartSnapshotData(gameplay_logic::ChartData* chartData) {
    if (!chartData) {
        return {};
    }

    return {
        .md5 = chartData->getMd5(),
        .length = chartData->getLength(),
        .normalNoteCount = chartData->getNormalNoteCount(),
        .scratchCount = chartData->getScratchCount(),
        .lnCount = chartData->getLnCount(),
        .bssCount = chartData->getBssCount(),
        .mineCount = chartData->getMineCount(),
        .histogramData = histogramListFromSeries(chartData->getHistogramData()),
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
QString Lr2ChartDataSnapshot::revision() const { return m_revision; }

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
    const bool nextHasHistogram = histogramHasData(nextHistogram);
    const QString nextRevision = nextHasHistogram
        ? QStringList {
            nextMd5,
            QString::number(nextLength),
            QString::number(nextNormal),
            QString::number(nextScratch),
            QString::number(nextLn),
            QString::number(nextBss),
            QString::number(nextMine),
            histogramRevision(nextHistogram),
        }.join(QLatin1Char(':'))
        : QString {};

    if (m_hasHistogram == nextHasHistogram
            && m_md5 == nextMd5
            && m_length == nextLength
            && m_normalNoteCount == nextNormal
            && m_scratchCount == nextScratch
            && m_lnCount == nextLn
            && m_bssCount == nextBss
            && m_mineCount == nextMine
            && m_histogramData == nextHistogram
            && m_revision == nextRevision) {
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
    m_revision = nextRevision;
    emit dataChanged();
}
