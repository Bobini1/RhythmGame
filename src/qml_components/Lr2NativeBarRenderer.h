#pragma once

#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVariant>
#include <QVariantList>

namespace qml_components {

class Lr2NativeBarText : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant dstState READ dstState WRITE setDstState NOTIFY dstStateChanged)
    Q_PROPERTY(QVariant srcData READ srcData WRITE setSrcData NOTIFY srcDataChanged)
    Q_PROPERTY(QObject* selectContext READ selectContext WRITE setSelectContext NOTIFY selectContextChanged)
    Q_PROPERTY(QVariantList barBaseStates READ barBaseStates WRITE setBarBaseStates NOTIFY barBaseStatesChanged)
    Q_PROPERTY(qreal barScrollOffset READ barScrollOffset WRITE setBarScrollOffset NOTIFY barScrollOffsetChanged)
    Q_PROPERTY(int barCenter READ barCenter WRITE setBarCenter NOTIFY barCenterChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(int revision READ revision WRITE setRevision NOTIFY revisionChanged)
    Q_PROPERTY(int visualBaseIndex READ visualBaseIndex WRITE setVisualBaseIndex NOTIFY visualBaseIndexChanged)

  public:
    explicit Lr2NativeBarText(QQuickItem* parent = nullptr);

    auto dstState() const -> QVariant;
    void setDstState(const QVariant& state);
    auto srcData() const -> QVariant;
    void setSrcData(const QVariant& data);
    auto selectContext() const -> QObject*;
    void setSelectContext(QObject* context);
    auto barBaseStates() const -> QVariantList;
    void setBarBaseStates(const QVariantList& states);
    auto barScrollOffset() const -> qreal;
    void setBarScrollOffset(qreal offset);
    auto barCenter() const -> int;
    void setBarCenter(int center);
    auto scaleOverride() const -> qreal;
    void setScaleOverride(qreal scale);
    auto revision() const -> int;
    void setRevision(int revision);
    auto visualBaseIndex() const -> int;
    void setVisualBaseIndex(int index);

    void paint(QPainter* painter) override;

  signals:
    void dstStateChanged();
    void srcDataChanged();
    void selectContextChanged();
    void barBaseStatesChanged();
    void barScrollOffsetChanged();
    void barCenterChanged();
    void scaleOverrideChanged();
    void revisionChanged();
    void visualBaseIndexChanged();

  private:
    QVariant m_dstState;
    QVariant m_srcData;
    QPointer<QObject> m_selectContext;
    QVariantList m_barBaseStates;
    qreal m_barScrollOffset = 0.0;
    int m_barCenter = 0;
    qreal m_scaleOverride = 1.0;
    int m_revision = 0;
    int m_visualBaseIndex = 0;
};

class Lr2NativeBarNumber : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QVariant dstState READ dstState WRITE setDstState NOTIFY dstStateChanged)
    Q_PROPERTY(QVariant srcData READ srcData WRITE setSrcData NOTIFY srcDataChanged)
    Q_PROPERTY(QObject* selectContext READ selectContext WRITE setSelectContext NOTIFY selectContextChanged)
    Q_PROPERTY(QVariantList barBaseStates READ barBaseStates WRITE setBarBaseStates NOTIFY barBaseStatesChanged)
    Q_PROPERTY(qreal barScrollOffset READ barScrollOffset WRITE setBarScrollOffset NOTIFY barScrollOffsetChanged)
    Q_PROPERTY(int barCenter READ barCenter WRITE setBarCenter NOTIFY barCenterChanged)
    Q_PROPERTY(qreal scaleOverride READ scaleOverride WRITE setScaleOverride NOTIFY scaleOverrideChanged)
    Q_PROPERTY(int revision READ revision WRITE setRevision NOTIFY revisionChanged)
    Q_PROPERTY(int visualBaseIndex READ visualBaseIndex WRITE setVisualBaseIndex NOTIFY visualBaseIndexChanged)

  public:
    explicit Lr2NativeBarNumber(QQuickItem* parent = nullptr);

    auto dstState() const -> QVariant;
    void setDstState(const QVariant& state);
    auto srcData() const -> QVariant;
    void setSrcData(const QVariant& data);
    auto selectContext() const -> QObject*;
    void setSelectContext(QObject* context);
    auto barBaseStates() const -> QVariantList;
    void setBarBaseStates(const QVariantList& states);
    auto barScrollOffset() const -> qreal;
    void setBarScrollOffset(qreal offset);
    auto barCenter() const -> int;
    void setBarCenter(int center);
    auto scaleOverride() const -> qreal;
    void setScaleOverride(qreal scale);
    auto revision() const -> int;
    void setRevision(int revision);
    auto visualBaseIndex() const -> int;
    void setVisualBaseIndex(int index);

    void paint(QPainter* painter) override;

  signals:
    void dstStateChanged();
    void srcDataChanged();
    void selectContextChanged();
    void barBaseStatesChanged();
    void barScrollOffsetChanged();
    void barCenterChanged();
    void scaleOverrideChanged();
    void revisionChanged();
    void visualBaseIndexChanged();

  private:
    QVariant m_dstState;
    QVariant m_srcData;
    QPointer<QObject> m_selectContext;
    QVariantList m_barBaseStates;
    qreal m_barScrollOffset = 0.0;
    int m_barCenter = 0;
    qreal m_scaleOverride = 1.0;
    int m_revision = 0;
    int m_visualBaseIndex = 0;
};

} // namespace qml_components
