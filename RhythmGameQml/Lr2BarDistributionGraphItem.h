#pragma once

#include "Lr2BarPositionCache.h"

#include <QColor>
#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;

class Lr2BarDistributionGraphItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariant srcData READ srcData WRITE setSrcData NOTIFY srcDataChanged)
    Q_PROPERTY(QVariant stateData READ stateData WRITE setStateData NOTIFY stateDataChanged)
    Q_PROPERTY(QVariantList barCells READ barCells WRITE setBarCells NOTIFY barCellsChanged)
    Q_PROPERTY(Lr2BarPositionCache* barPositionCache READ barPositionCache WRITE setBarPositionCache NOTIFY barPositionCacheChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(int frameOverrideBase READ frameOverrideBase WRITE setFrameOverrideBase NOTIFY frameOverrideBaseChanged)
    Q_PROPERTY(QColor transColor READ transColor WRITE setTransColor NOTIFY transColorChanged)
    Q_PROPERTY(bool colorKeyEnabled READ colorKeyEnabled WRITE setColorKeyEnabled NOTIFY colorKeyEnabledChanged)

public:
    explicit Lr2BarDistributionGraphItem(QQuickItem* parent = nullptr);
    ~Lr2BarDistributionGraphItem() override;

    QVariant srcData() const;
    void setSrcData(const QVariant& value);

    QVariant stateData() const;
    void setStateData(const QVariant& value);

    QVariantList barCells() const;
    void setBarCells(const QVariantList& value);

    Lr2BarPositionCache* barPositionCache() const;
    void setBarPositionCache(Lr2BarPositionCache* value);

    qreal scaleOverride() const;
    void setScaleOverride(qreal value);

    int frameOverrideBase() const;
    void setFrameOverrideBase(int value);

    QColor transColor() const;
    void setTransColor(const QColor& value);

    bool colorKeyEnabled() const;
    void setColorKeyEnabled(bool value);

signals:
    void srcDataChanged();
    void stateDataChanged();
    void barCellsChanged();
    void barPositionCacheChanged();
    void scaleOverrideChanged();
    void frameOverrideBaseChanged();
    void transColorChanged();
    void colorKeyEnabledChanged();

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
        QString source;
    };

    void parseSource();
    void loadSourceImage();
    void reconnectCells();
    void requestSceneUpdate();

    QVariant m_srcData;
    QVariant m_stateData;
    QVariantList m_barCellsValue;
    QVector<QPointer<Lr2SelectBarCell>> m_barCells;
    QVector<QMetaObject::Connection> m_cellConnections;
    QPointer<Lr2BarPositionCache> m_barPositionCache;
    QMetaObject::Connection m_positionRevisionConnection;
    QMetaObject::Connection m_positionSlotOffsetConnection;
    QMetaObject::Connection m_positionSlotCountConnection;
    qreal m_scaleOverride = 1.0;
    int m_frameOverrideBase = 0;
    QColor m_transColor = Qt::black;
    bool m_colorKeyEnabled = false;
    Source m_source;
    QImage m_sourceImage;
    bool m_textureDirty = true;
};
