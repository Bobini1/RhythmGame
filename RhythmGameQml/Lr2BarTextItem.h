#pragma once

#include "Lr2BarPositionMap.h"
#include "Lr2TimelineState.h"

#include <QColor>
#include <QHash>
#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;

class Lr2BarTextItem : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList dsts READ dsts WRITE setDsts NOTIFY dstsChanged)
    Q_PROPERTY(QVariant srcData READ srcData WRITE setSrcData NOTIFY srcDataChanged)
    Q_PROPERTY(QVariantList barCells READ barCells WRITE setBarCells NOTIFY barCellsChanged)
    Q_PROPERTY(Lr2BarPositionMap* barPositionMap READ barPositionMap WRITE setBarPositionMap NOTIFY barPositionMapChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(bool supported READ isSupported NOTIFY supportedChanged)

public:
    explicit Lr2BarTextItem(QQuickItem* parent = nullptr);
    ~Lr2BarTextItem() override;

    QVariantList dsts() const;
    void setDsts(const QVariantList& value);

    QVariant srcData() const;
    void setSrcData(const QVariant& value);

    QVariantList barCells() const;
    void setBarCells(const QVariantList& value);

    Lr2BarPositionMap* barPositionMap() const;
    void setBarPositionMap(Lr2BarPositionMap* value);

    qreal scaleOverride() const;
    void setScaleOverride(qreal value);

    bool isSupported() const;

signals:
    void dstsChanged();
    void srcDataChanged();
    void barCellsChanged();
    void barPositionMapChanged();
    void scaleOverrideChanged();
    void supportedChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;

private:
    struct Source {
        int titleType = -1;
        int align = 0;
        int fontSize = 0;
        int fontThickness = 0;
        int fontType = 0;
        bool bitmapFont = false;
        QString fontPath;
        QString fontFamily;
        bool isLr2Font = false;
    };

    struct TextImage {
        QImage image;
        QSizeF naturalSize;
    };

    void parseSource();
    void updateTimelineDsts();
    void reconnectCells();
    void reconnectPositionMap();
    void updateSupported();
    void requestSceneUpdate();
    TextImage textImageFor(const QString& text, const QColor& color, qreal boxHeight, bool hasEdge);

    QVariantList m_dsts;
    QVariantList m_timelineDsts;
    QVariant m_srcData;
    QVariantList m_barCellValues;
    QVector<QPointer<Lr2SelectBarCell>> m_barCells;
    QVector<QMetaObject::Connection> m_cellConnections;
    QPointer<Lr2BarPositionMap> m_barPositionMap;
    QMetaObject::Connection m_positionCoordinatesConnection;
    QMetaObject::Connection m_positionSlotOffsetConnection;
    QMetaObject::Connection m_positionSlotCountConnection;
    qreal m_scaleOverride = 1.0;
    bool m_supported = false;
    bool m_hasUnsupportedBlend = false;
    Source m_source;
    Lr2TimelineState m_timeline;
    QHash<QString, TextImage> m_textImageCache;
};
