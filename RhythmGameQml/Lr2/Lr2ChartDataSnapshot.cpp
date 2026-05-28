#include "Lr2ChartDataSnapshot.h"

#include <QList>
#include <QMetaProperty>
#include <QObject>
#include <QStringList>
#include <algorithm>

namespace {

QVariant gadgetProperty(const QVariant& value, const char* name) {
    const QMetaObject* metaObject = value.metaType().metaObject();
    if (!metaObject) {
        return {};
    }
    const int propertyIndex = metaObject->indexOfProperty(name);
    return propertyIndex < 0
        ? QVariant {}
        : metaObject->property(propertyIndex).readOnGadget(value.constData());
}

QVariant propertyValue(const QVariant& value, const char* name) {
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        const auto it = map.constFind(QString::fromLatin1(name));
        return it == map.constEnd() ? QVariant {} : *it;
    }
    if (value.canConvert<QObject*>()) {
        if (QObject* object = value.value<QObject*>()) {
            return object->property(name);
        }
    }
    return gadgetProperty(value, name);
}

QVariant unwrapChart(const QVariant& value) {
    const QVariant wrapped = propertyValue(value, "chartData");
    return wrapped.isValid() && !wrapped.isNull() ? wrapped : value;
}

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

QVariantList listFromValue(const QVariant& value) {
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    if (value.canConvert<QVariantList>()) {
        return value.toList();
    }
    if (value.canConvert<QList<QList<qint64>>>()) {
        return histogramListFromSeries(value.value<QList<QList<qint64>>>());
    }
    if (value.canConvert<QList<qint64>>()) {
        return numberListFromSeries(value.value<QList<qint64>>());
    }
    return {};
}

QVariantList numberSeries(const QVariant& value) {
    const QVariantList source = listFromValue(value);
    QVariantList result;
    result.reserve(source.size());
    for (const QVariant& entry : source) {
        result.append(std::max<qint64>(0, entry.toLongLong()));
    }
    return result;
}

QVariantList histogramFromValue(const QVariant& value) {
    const QVariantList source = listFromValue(value);
    QVariantList result;
    result.reserve(source.size());
    for (const QVariant& series : source) {
        result.append(QVariant::fromValue(numberSeries(series)));
    }
    return result;
}

bool histogramHasData(const QVariantList& histogram) {
    for (const QVariant& seriesValue : histogram) {
        if (!listFromValue(seriesValue).isEmpty()) {
            return true;
        }
    }
    return false;
}

QString histogramRevision(const QVariantList& histogram) {
    QStringList parts;
    for (int i = 0; i < 6; ++i) {
        const QVariant series = i < histogram.size() ? histogram.at(i) : QVariant {};
        parts.append(QString::number(listFromValue(series).size()));
    }
    return parts.join(QLatin1Char(':'));
}

int intProperty(const QVariant& value, const char* name) {
    bool ok = false;
    const int result = propertyValue(value, name).toInt(&ok);
    return ok ? result : 0;
}

qint64 int64Property(const QVariant& value, const char* name) {
    bool ok = false;
    const qint64 result = propertyValue(value, name).toLongLong(&ok);
    return ok ? result : 0;
}

} // namespace

Lr2ChartDataSnapshot::Lr2ChartDataSnapshot(QObject* parent)
    : QObject(parent) {}

QVariant Lr2ChartDataSnapshot::chart() const { return m_chart; }

void Lr2ChartDataSnapshot::setChart(const QVariant& value) {
    if (m_chart == value) {
        return;
    }
    m_chart = value;
    refresh();
    emit chartChanged();
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
    const QVariant chartData = unwrapChart(m_chart);
    const QString nextMd5 = propertyValue(chartData, "md5").toString();
    const qint64 nextLength = int64Property(chartData, "length");
    const int nextNormal = intProperty(chartData, "normalNoteCount");
    const int nextScratch = intProperty(chartData, "scratchCount");
    const int nextLn = intProperty(chartData, "lnCount");
    const int nextBss = intProperty(chartData, "bssCount");
    const int nextMine = intProperty(chartData, "mineCount");
    const QVariantList nextHistogram = histogramFromValue(propertyValue(chartData, "histogramData"));
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
