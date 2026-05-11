#pragma once

#include "Lr2SelectVisualState.h"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;
class Lr2SelectStateCache;

class Lr2SelectBarWindow : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(QObject* nativeState READ nativeState WRITE setNativeState NOTIFY nativeStateChanged)
    Q_PROPERTY(QVariant items READ items WRITE setItems NOTIFY itemsChanged)
    Q_PROPERTY(int logicalCount READ logicalCount WRITE setLogicalCount NOTIFY logicalCountChanged)
    Q_PROPERTY(int listRevision READ listRevision WRITE setListRevision NOTIFY listRevisionChanged)
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int barCenter READ barCenter WRITE setBarCenter NOTIFY barCenterChanged)
    Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
    Q_PROPERTY(int visualBaseIndex READ visualBaseIndex WRITE setVisualBaseIndex NOTIFY visualBaseIndexChanged)
    Q_PROPERTY(QVariant barTitleTypes READ barTitleTypes WRITE setBarTitleTypes NOTIFY barTitleTypesChanged)
    Q_PROPERTY(int scoreRevision READ scoreRevision WRITE setScoreRevision NOTIFY scoreRevisionChanged)
    Q_PROPERTY(int folderLampRevision READ folderLampRevision WRITE setFolderLampRevision NOTIFY folderLampRevisionChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(int slotOffset READ slotOffset NOTIFY slotOffsetChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QVariantList cells READ cells NOTIFY cellsChanged)
    Q_PROPERTY(bool isCurrent READ isCurrent NOTIFY isCurrentChanged)

public:
    explicit Lr2SelectBarWindow(QObject* parent = nullptr);

    QObject* context() const;
    void setContext(QObject* value);

    QObject* nativeState() const;
    void setNativeState(QObject* value);

    QVariant items() const;
    void setItems(const QVariant& value);

    int logicalCount() const;
    void setLogicalCount(int value);

    int listRevision() const;
    void setListRevision(int value);

    int rowCount() const;
    void setRowCount(int value);

    int barCenter() const;
    void setBarCenter(int value);

    Lr2SelectVisualState* visualState() const;
    void setVisualState(Lr2SelectVisualState* value);

    int visualBaseIndex() const;
    void setVisualBaseIndex(int value);

    QVariant barTitleTypes() const;
    void setBarTitleTypes(const QVariant& value);

    int scoreRevision() const;
    void setScoreRevision(int value);

    int folderLampRevision() const;
    void setFolderLampRevision(int value);

    bool isActive() const;
    void setActive(bool value);

    int slotOffset() const;
    QVariantList entries() const;
    QVariantList cells() const;
    bool isCurrent() const;

    Q_INVOKABLE void invalidate();
    Q_INVOKABLE void refresh(bool force = false);
    Q_INVOKABLE int slotForRow(int row) const;
    Q_INVOKABLE QVariant entryAtRow(int row, int fallbackBarCenter) const;
    Q_INVOKABLE QObject* cellAtSlot(int slot) const;

signals:
    void contextChanged();
    void nativeStateChanged();
    void itemsChanged();
    void logicalCountChanged();
    void listRevisionChanged();
    void rowCountChanged();
    void barCenterChanged();
    void visualStateChanged();
    void visualBaseIndexChanged();
    void barTitleTypesChanged();
    void scoreRevisionChanged();
    void folderLampRevisionChanged();
    void activeChanged();
    void slotOffsetChanged();
    void entriesChanged();
    void cellsChanged();
    void isCurrentChanged();

private:
    void refreshIfActive(bool force);
    QVariant itemForRow(int row, int center) const;
    void clearWindow(bool force);
    void shiftWindow(int delta, int effectiveRowCount);
    void rebuild(int effectiveRowCount);
    void ensureCellList(int count);
    Lr2SelectBarCell* ensureCellAtSlot(int slot);
    void updateCell(Lr2SelectBarCell* cell, int row, const QVariant& entry);
    void refreshCellsInPlace();
    void setCachedState(int baseIndex, int listRevision, int rowCount, int center);
    void maybeEmitIsCurrentChanged(bool previous);
    void updateVisualBaseIndexFromState();

    QPointer<QObject> m_context;
    QPointer<Lr2SelectStateCache> m_nativeState;
    QList<QMetaObject::Connection> m_nativeStateConnections;
    QPointer<Lr2SelectVisualState> m_visualState;
    QMetaObject::Connection m_visualStateBaseIndexConnection;
    QVariant m_items;
    QVariantList m_itemList;
    int m_logicalCount = 0;
    int m_listRevision = 0;
    int m_rowCount = 0;
    int m_barCenter = 0;
    int m_visualBaseIndex = 0;
    QVariant m_barTitleTypes;
    int m_scoreRevision = 0;
    int m_folderLampRevision = 0;
    bool m_active = true;
    int m_slotOffset = 0;
    QVariantList m_entries;
    QList<QPointer<Lr2SelectBarCell>> m_cellObjects;
    QVariantList m_cellValues;
    int m_cachedBaseIndex = -1;
    int m_cachedListRevision = -1;
    int m_cachedRowCount = -1;
    int m_cachedCenter = -1;
};
