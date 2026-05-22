#pragma once

#include "Lr2BarPositionMap.h"

#include <QColor>
#include <QImage>
#include <QMetaObject>
#include <QPointer>
#include <QQuickItem>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;
class Lr2SkinClock;

class Lr2BarDistributionGraphItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant srcData READ srcData WRITE setSrcData NOTIFY srcDataChanged)
    Q_PROPERTY(QObject* skinClock READ skinClock WRITE setSkinClock NOTIFY skinClockChanged)
    Q_PROPERTY(int sourceSkinClockMode READ sourceSkinClockMode WRITE setSourceSkinClockMode NOTIFY sourceSkinClockModeChanged)
    Q_PROPERTY(int sourceSkinTime READ sourceSkinTime WRITE setSourceSkinTime NOTIFY sourceSkinTimeChanged)
    Q_PROPERTY(int sourceTimerFire READ sourceTimerFire WRITE setSourceTimerFire NOTIFY sourceTimerFireChanged)
    Q_PROPERTY(QVariant stateData READ stateData WRITE setStateData NOTIFY stateDataChanged)
    Q_PROPERTY(int sourceGraphType READ sourceGraphType WRITE setSourceGraphType NOTIFY sourceFieldsChanged)
    Q_PROPERTY(int sourceSpecialType READ sourceSpecialType WRITE setSourceSpecialType NOTIFY sourceFieldsChanged)
    Q_PROPERTY(qreal sourceX READ sourceX WRITE setSourceX NOTIFY sourceFieldsChanged)
    Q_PROPERTY(qreal sourceY READ sourceY WRITE setSourceY NOTIFY sourceFieldsChanged)
    Q_PROPERTY(qreal sourceW READ sourceW WRITE setSourceW NOTIFY sourceFieldsChanged)
    Q_PROPERTY(qreal sourceH READ sourceH WRITE setSourceH NOTIFY sourceFieldsChanged)
    Q_PROPERTY(int sourceDivX READ sourceDivX WRITE setSourceDivX NOTIFY sourceFieldsChanged)
    Q_PROPERTY(int sourceDivY READ sourceDivY WRITE setSourceDivY NOTIFY sourceFieldsChanged)
    Q_PROPERTY(QString sourcePath READ sourcePath WRITE setSourcePath NOTIFY sourceFieldsChanged)
    Q_PROPERTY(QVariantList barCells READ barCells WRITE setBarCells NOTIFY barCellsChanged)
    Q_PROPERTY(Lr2BarPositionMap* barPositionMap READ barPositionMap WRITE setBarPositionMap NOTIFY barPositionMapChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(int frameOverrideBase READ frameOverrideBase WRITE setFrameOverrideBase NOTIFY frameOverrideBaseChanged)
    Q_PROPERTY(QColor transColor READ transColor WRITE setTransColor NOTIFY transColorChanged)
    Q_PROPERTY(bool colorKeyEnabled READ colorKeyEnabled WRITE setColorKeyEnabled NOTIFY colorKeyEnabledChanged)
    Q_PROPERTY(QString chartAssetSource READ chartAssetSource WRITE setChartAssetSource NOTIFY chartAssetSourceChanged)

public:
    explicit Lr2BarDistributionGraphItem(QQuickItem* parent = nullptr);
    ~Lr2BarDistributionGraphItem() override;

    QVariant srcData() const;
    void setSrcData(const QVariant& value);

    QObject* skinClock() const;
    void setSkinClock(QObject* value);

    int sourceSkinClockMode() const;
    void setSourceSkinClockMode(int value);

    int sourceSkinTime() const;
    void setSourceSkinTime(int value);

    int sourceTimerFire() const;
    void setSourceTimerFire(int value);

    QVariant stateData() const;
    void setStateData(const QVariant& value);

    int sourceGraphType() const;
    void setSourceGraphType(int value);

    int sourceSpecialType() const;
    void setSourceSpecialType(int value);

    qreal sourceX() const;
    void setSourceX(qreal value);

    qreal sourceY() const;
    void setSourceY(qreal value);

    qreal sourceW() const;
    void setSourceW(qreal value);

    qreal sourceH() const;
    void setSourceH(qreal value);

    int sourceDivX() const;
    void setSourceDivX(int value);

    int sourceDivY() const;
    void setSourceDivY(int value);

    QString sourcePath() const;
    void setSourcePath(const QString& value);

    QVariantList barCells() const;
    void setBarCells(const QVariantList& value);

    Lr2BarPositionMap* barPositionMap() const;
    void setBarPositionMap(Lr2BarPositionMap* value);

    qreal scaleOverride() const;
    void setScaleOverride(qreal value);

    int frameOverrideBase() const;
    void setFrameOverrideBase(int value);

    QColor transColor() const;
    void setTransColor(const QColor& value);

    bool colorKeyEnabled() const;
    void setColorKeyEnabled(bool value);

    QString chartAssetSource() const;
    void setChartAssetSource(const QString& value);

signals:
    void srcDataChanged();
    void skinClockChanged();
    void sourceSkinClockModeChanged();
    void sourceSkinTimeChanged();
    void sourceTimerFireChanged();
    void stateDataChanged();
    void sourceFieldsChanged();
    void barCellsChanged();
    void barPositionMapChanged();
    void scaleOverrideChanged();
    void frameOverrideBaseChanged();
    void transColorChanged();
    void colorKeyEnabledChanged();
    void chartAssetSourceChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    struct Source {
        int graphType = 0;
        int specialType = 0;
        qreal x = 0.0;
        qreal y = 0.0;
        qreal w = 0.0;
        qreal h = 0.0;
        int divX = 1;
        int divY = 1;
        int cycle = 0;
        int timer = 0;
    };

    void parseSource();
    void loadSourceImage();
    QString resolvedSource() const;
    void reconnectCells();
    void reconnectClock();
    void requestSceneUpdate();
    void refreshAnimationFrameBase();
    int sourceClockSkinTime() const;
    int computedFrameOverrideBase() const;

    QVariant m_srcData;
    QPointer<Lr2SkinClock> m_skinClock;
    QMetaObject::Connection m_clockConnection;
    int m_sourceSkinClockMode = 0;
    int m_sourceSkinTime = 0;
    int m_sourceTimerFire = -2147483648;
    QVariant m_stateData;
    QVariantList m_barCellsValue;
    QVector<QPointer<Lr2SelectBarCell>> m_barCells;
    QVector<QMetaObject::Connection> m_cellConnections;
    QPointer<Lr2BarPositionMap> m_barPositionMap;
    QMetaObject::Connection m_positionCoordinatesConnection;
    QMetaObject::Connection m_positionSlotOffsetConnection;
    QMetaObject::Connection m_positionSlotCountConnection;
    qreal m_scaleOverride = 1.0;
    int m_frameOverrideBase = 0;
    QColor m_transColor = Qt::black;
    bool m_colorKeyEnabled = false;
    QString m_chartAssetSource;
    Source m_source;
    QString m_sourcePath;
    QImage m_sourceImage;
    bool m_textureDirty = true;
};
