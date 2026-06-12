#pragma once

#include "Lr2SelectItemModel.h"
#include "Lr2SelectVisualState.h"

#include <QAbstractListModel>
#include <QMetaObject>
#include <QPointer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SelectBarCell;

class Lr2SelectBarModel : public QAbstractListModel {
	Q_OBJECT
	QML_ELEMENT
	Q_PROPERTY(Lr2SelectItemModel* sourceModel READ sourceModel WRITE setSourceModel NOTIFY sourceModelChanged)
	Q_PROPERTY(int logicalCount READ logicalCount WRITE setLogicalCount NOTIFY logicalCountChanged)
	Q_PROPERTY(int rowCountLimit READ rowCountLimit WRITE setRowCountLimit NOTIFY rowCountLimitChanged)
	Q_PROPERTY(int centerRow READ centerRow WRITE setCenterRow NOTIFY centerRowChanged)
	Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
	Q_PROPERTY(Lr2SelectVisualState* visualState READ visualState WRITE setVisualState NOTIFY visualStateChanged)
	Q_PROPERTY(int slotOffset READ slotOffset NOTIFY slotOffsetChanged)
	Q_PROPERTY(QVariantList cells READ cells NOTIFY cellsChanged)

public:
	enum Roles {
		RowRole = Qt::UserRole + 1,
		SlotRole,
		SourceRowRole,
		EntryKeyRole,
		TextRole,
		TitleTypeRole,
		BodyTypeRole,
		PlayLevelRole,
		DifficultyRole,
		KeymodeRole,
		ChartLikeRole,
		EntryLikeRole,
		FolderLikeRole,
		RankingRole,
		LampRole,
		RankRole,
		LabelMaskRole,
		GraphLampsRole,
		GraphRanksRole,
		RawItemRole
	};
	Q_ENUM(Roles)

	explicit Lr2SelectBarModel(QObject* parent = nullptr);

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	QHash<int, QByteArray> roleNames() const override;

	Lr2SelectItemModel* sourceModel() const;
	void setSourceModel(Lr2SelectItemModel* model);

	int logicalCount() const;
	void setLogicalCount(int count);

	int rowCountLimit() const;
	void setRowCountLimit(int count);

	int centerRow() const;
	void setCenterRow(int row);

	int currentIndex() const;
	void setCurrentIndex(int index);

	Lr2SelectVisualState* visualState() const;
	void setVisualState(Lr2SelectVisualState* state);

	int slotOffset() const;
	QVariantList cells() const;

	Q_INVOKABLE void rebuild();
	Q_INVOKABLE void scrollBy(int delta);

signals:
	void sourceModelChanged();
	void logicalCountChanged();
	void rowCountLimitChanged();
	void centerRowChanged();
	void currentIndexChanged();
	void visualStateChanged();
	void slotOffsetChanged();
	void cellsChanged();

private:
	struct BarRow {
		int visualRow = 0;
		int sourceRow = 0;
		int slot = 0;
	};

	QVariant sourceData(const BarRow& row, int sourceRole) const;
	QVariant roleData(const BarRow& row, int role) const;
	int sourceCount() const;
	int normalizedSourceRow(int row) const;
	int sourceRowForVisualRow(int visualRow) const;
	int slotForRow(int row) const;
	void disconnectSourceModel();
	void connectSourceModel();
	void rebuildRows();
	void emitAllRowsChanged();
	void ensureCellList(int count);
	void setSlotOffsetValue(int offset);
	void syncRowSlots();
	void updateCellAtSlot(int slot, const BarRow& row);
	void updateCellForVisualRow(int row);
	void updateCellsForSourceRows(int first, int last);
	void updateAllCells();
	void disconnectVisualState();
	void connectVisualState();
	void syncCurrentIndexFromVisualState();

	QPointer<Lr2SelectItemModel> m_sourceModel;
	QPointer<Lr2SelectVisualState> m_visualState;
	QList<QMetaObject::Connection> m_sourceConnections;
	QMetaObject::Connection m_visualStateConnection;
	QList<BarRow> m_rows;
	QList<QPointer<Lr2SelectBarCell>> m_cellObjects;
	QVariantList m_cellValues;
	int m_logicalCount = 0;
	int m_rowCountLimit = 0;
	int m_centerRow = 0;
	int m_currentIndex = 0;
	int m_slotOffset = 0;
};
