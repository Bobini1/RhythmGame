#pragma once

#include "gameplay_logic/ChartData.h"

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2ChartDataSnapshot : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant chart READ chart WRITE setChart NOTIFY chartChanged)
    Q_PROPERTY(bool hasHistogram READ hasHistogram NOTIFY dataChanged)
    Q_PROPERTY(QString md5 READ md5 NOTIFY dataChanged)
    Q_PROPERTY(qint64 length READ length NOTIFY dataChanged)
    Q_PROPERTY(int normalNoteCount READ normalNoteCount NOTIFY dataChanged)
    Q_PROPERTY(int scratchCount READ scratchCount NOTIFY dataChanged)
    Q_PROPERTY(int lnCount READ lnCount NOTIFY dataChanged)
    Q_PROPERTY(int bssCount READ bssCount NOTIFY dataChanged)
    Q_PROPERTY(int mineCount READ mineCount NOTIFY dataChanged)
    Q_PROPERTY(QVariantList histogramData READ histogramData NOTIFY dataChanged)
    Q_PROPERTY(QVariantList normalDensityData READ normalDensityData NOTIFY dataChanged)
    Q_PROPERTY(int normalDensityMax READ normalDensityMax NOTIFY dataChanged)

public:
    explicit Lr2ChartDataSnapshot(QObject* parent = nullptr);

    QVariant chart() const;
    void setChart(QObject* value);
    void setChart(const QVariant& value);

    bool hasHistogram() const;
    QString md5() const;
    qint64 length() const;
    int normalNoteCount() const;
    int scratchCount() const;
    int lnCount() const;
    int bssCount() const;
    int mineCount() const;
    QVariantList histogramData() const;
    QVariantList normalDensityData() const;
    int normalDensityMax() const;

signals:
    void chartChanged();
    void dataChanged();

private:
    void refresh();

    QPointer<QObject> m_chart;
    bool m_hasHistogram = false;
    QString m_md5;
    qint64 m_length = 0;
    int m_normalNoteCount = 0;
    int m_scratchCount = 0;
    int m_lnCount = 0;
    int m_bssCount = 0;
    int m_mineCount = 0;
    QVariantList m_histogramData;
    QVariantList m_normalDensityData;
    int m_normalDensityMax = 20;
};
