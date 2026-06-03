#include "Lr2SelectBarModel.h"

#include "Lr2SelectBarCell.h"

#include <QQmlEngine>

#include <algorithm>

namespace {

int positiveModulo(int value, int modulus) {
	if (modulus <= 0) {
		return 0;
	}
	const int remainder = value % modulus;
	return remainder < 0 ? remainder + modulus : remainder;
}

} // namespace

Lr2SelectBarModel::Lr2SelectBarModel(QObject* parent)
	: QAbstractListModel(parent) {}

int Lr2SelectBarModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : m_rows.size();
}

QVariant Lr2SelectBarModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
		return {};
	}
	return roleData(m_rows.at(index.row()), role);
}

QHash<int, QByteArray> Lr2SelectBarModel::roleNames() const {
	return {
		{RowRole, "row"},
		{SlotRole, "slot"},
		{SourceRowRole, "sourceRow"},
		{EntryKeyRole, "entryKey"},
		{TextRole, "text"},
		{TitleTypeRole, "titleType"},
		{BodyTypeRole, "bodyType"},
		{PlayLevelRole, "playLevel"},
		{DifficultyRole, "difficulty"},
		{KeymodeRole, "keymode"},
		{ChartLikeRole, "chartLike"},
		{EntryLikeRole, "entryLike"},
		{FolderLikeRole, "folderLike"},
		{RankingRole, "ranking"},
		{LampRole, "lamp"},
		{RankRole, "rank"},
		{LabelMaskRole, "labelMask"},
		{GraphLampsRole, "graphLamps"},
		{GraphRanksRole, "graphRanks"},
		{RawItemRole, "rawItem"},
	};
}

Lr2SelectItemModel* Lr2SelectBarModel::sourceModel() const {
	return m_sourceModel;
}

void Lr2SelectBarModel::setSourceModel(Lr2SelectItemModel* model) {
	if (m_sourceModel == model) {
		return;
	}
	disconnectSourceModel();
	m_sourceModel = model;
	connectSourceModel();
	rebuildRows();
	emit sourceModelChanged();
}

int Lr2SelectBarModel::logicalCount() const {
	return m_logicalCount;
}

void Lr2SelectBarModel::setLogicalCount(int count) {
	const int next = std::max(0, count);
	if (m_logicalCount == next) {
		return;
	}
	m_logicalCount = next;
	const int countForRows = sourceCount();
	m_currentIndex = countForRows > 0 ? positiveModulo(m_currentIndex, countForRows) : 0;
	rebuildRows();
	emit logicalCountChanged();
}

int Lr2SelectBarModel::rowCountLimit() const {
	return m_rowCountLimit;
}

void Lr2SelectBarModel::setRowCountLimit(int count) {
	const int next = std::max(0, count);
	if (m_rowCountLimit == next) {
		return;
	}
	m_rowCountLimit = next;
	rebuildRows();
	emit rowCountLimitChanged();
}

int Lr2SelectBarModel::centerRow() const {
	return m_centerRow;
}

void Lr2SelectBarModel::setCenterRow(int row) {
	if (m_centerRow == row) {
		return;
	}
	m_centerRow = row;
	rebuildRows();
	emit centerRowChanged();
}

int Lr2SelectBarModel::currentIndex() const {
	return m_currentIndex;
}

void Lr2SelectBarModel::setCurrentIndex(int index) {
	const int count = sourceCount();
	const int nextIndex = count > 0 ? positiveModulo(index, count) : 0;
	if (m_currentIndex == nextIndex) {
		return;
	}
	if (count <= 0) {
		m_currentIndex = 0;
		rebuildRows();
		emit currentIndexChanged();
		return;
	}

	int delta = nextIndex - m_currentIndex;
	if (count > 1) {
		if (delta > count / 2) {
			delta -= count;
		} else if (delta < -count / 2) {
			delta += count;
		}
	}
	if (std::abs(delta) >= 1 && std::abs(delta) <= m_rows.size() && m_rows.size() > 1) {
		const int step = delta > 0 ? 1 : -1;
		for (int i = 0; i < std::abs(delta); ++i) {
			m_currentIndex = positiveModulo(m_currentIndex + step, count);
			scrollBy(step);
		}
	} else {
		m_currentIndex = nextIndex;
		rebuildRows();
	}
	emit currentIndexChanged();
}

int Lr2SelectBarModel::slotOffset() const {
	return m_slotOffset;
}

QVariantList Lr2SelectBarModel::cells() const {
	return m_cellValues;
}

QVariantList Lr2SelectBarModel::entries() const {
	return buildEntries();
}

void Lr2SelectBarModel::rebuild() {
	rebuildRows();
}

void Lr2SelectBarModel::scrollBy(int delta) {
	if (delta == 0 || m_rows.size() <= 1) {
		emitAllRowsChanged();
		return;
	}

	if (std::abs(delta) != 1) {
		rebuildRows();
		return;
	}

	if (delta > 0) {
		const int last = m_rows.size() - 1;
		const int recycledSlot = m_slotOffset;
		BarRow row = m_rows.takeFirst();
		m_rows.append(row);
		setSlotOffsetValue(m_slotOffset + 1);
		syncRowSlots();
		BarRow& recycled = m_rows[last];
		recycled.visualRow = last;
		recycled.sourceRow = sourceRowForVisualRow(last);
		recycled.slot = recycledSlot;
		updateCellAtSlot(recycledSlot, recycled);
	} else {
		const int recycledSlot = positiveModulo(m_slotOffset - 1, m_rows.size());
		BarRow row = m_rows.takeLast();
		m_rows.prepend(row);
		setSlotOffsetValue(m_slotOffset - 1);
		syncRowSlots();
		BarRow& recycled = m_rows[0];
		recycled.visualRow = 0;
		recycled.sourceRow = sourceRowForVisualRow(0);
		recycled.slot = recycledSlot;
		updateCellAtSlot(recycledSlot, recycled);
	}
}

int Lr2SelectBarModel::sourceRowAt(int row) const {
	return row >= 0 && row < m_rows.size() ? m_rows.at(row).sourceRow : -1;
}

int Lr2SelectBarModel::slotForRow(int row) const {
	return row >= 0 && row < m_rows.size() ? positiveModulo(row + m_slotOffset, m_rows.size()) : -1;
}

QObject* Lr2SelectBarModel::cellAtSlot(int slot) const {
	return slot >= 0 && slot < m_cellObjects.size() ? m_cellObjects.at(slot).data() : nullptr;
}

QVariant Lr2SelectBarModel::entryAtRow(int row) const {
	if (row < 0 || row >= m_rows.size()) {
		return {};
	}
	return sourceData(m_rows.at(row), Lr2SelectItemModel::RawItemRole);
}

QVariant Lr2SelectBarModel::sourceData(const BarRow& row, int sourceRole) const {
	if (!m_sourceModel || row.sourceRow < 0 || row.sourceRow >= m_sourceModel->rowCount()) {
		return {};
	}
	return m_sourceModel->data(m_sourceModel->index(row.sourceRow, 0), sourceRole);
}

QVariant Lr2SelectBarModel::roleData(const BarRow& row, int role) const {
	switch (role) {
	case RowRole: return row.visualRow;
	case SlotRole: return row.slot;
	case SourceRowRole: return row.sourceRow;
	case EntryKeyRole: return sourceData(row, Lr2SelectItemModel::KeyRole);
	case Qt::DisplayRole:
	case TextRole: return sourceData(row, Lr2SelectItemModel::DisplayTextRole);
	case TitleTypeRole: return sourceData(row, Lr2SelectItemModel::TitleTypeRole);
	case BodyTypeRole: return sourceData(row, Lr2SelectItemModel::BodyTypeRole);
	case PlayLevelRole: return sourceData(row, Lr2SelectItemModel::PlayLevelRole);
	case DifficultyRole: return sourceData(row, Lr2SelectItemModel::DifficultyRole);
	case KeymodeRole: return sourceData(row, Lr2SelectItemModel::KeymodeRole);
	case ChartLikeRole: return sourceData(row, Lr2SelectItemModel::IsChartRole);
	case EntryLikeRole: return sourceData(row, Lr2SelectItemModel::IsEntryRole);
	case FolderLikeRole: return sourceData(row, Lr2SelectItemModel::IsFolderLikeRole);
	case RankingRole: return sourceData(row, Lr2SelectItemModel::IsRankingEntryRole);
	case LampRole: return sourceData(row, Lr2SelectItemModel::LampRole);
	case RankRole: return sourceData(row, Lr2SelectItemModel::ScoreRankRole);
	case LabelMaskRole: return sourceData(row, Lr2SelectItemModel::LabelMaskRole);
	case GraphLampsRole: return sourceData(row, Lr2SelectItemModel::FolderGraphLampsRole);
	case GraphRanksRole: return sourceData(row, Lr2SelectItemModel::FolderGraphRanksRole);
	case RawItemRole: return sourceData(row, Lr2SelectItemModel::RawItemRole);
	default: return {};
	}
}

int Lr2SelectBarModel::sourceCount() const {
	const int modelCount = m_sourceModel ? m_sourceModel->rowCount() : 0;
	if (modelCount <= 0) {
		return 0;
	}
	return m_logicalCount > 0 ? std::min(m_logicalCount, modelCount) : modelCount;
}

int Lr2SelectBarModel::normalizedSourceRow(int row) const {
	const int count = sourceCount();
	return count > 0 ? positiveModulo(row, count) : -1;
}

int Lr2SelectBarModel::sourceRowForVisualRow(int visualRow) const {
	return normalizedSourceRow(m_currentIndex + visualRow - m_centerRow);
}

void Lr2SelectBarModel::disconnectSourceModel() {
	for (const QMetaObject::Connection& connection : m_sourceConnections) {
		disconnect(connection);
	}
	m_sourceConnections.clear();
}

void Lr2SelectBarModel::connectSourceModel() {
	if (!m_sourceModel) {
		return;
	}
	m_sourceConnections.append(connect(m_sourceModel, &QAbstractItemModel::modelReset, this, &Lr2SelectBarModel::rebuildRows));
	m_sourceConnections.append(connect(m_sourceModel, &QAbstractItemModel::rowsInserted, this, &Lr2SelectBarModel::rebuildRows));
	m_sourceConnections.append(connect(m_sourceModel, &QAbstractItemModel::rowsRemoved, this, &Lr2SelectBarModel::rebuildRows));
	m_sourceConnections.append(connect(m_sourceModel, &QAbstractItemModel::rowsMoved, this, &Lr2SelectBarModel::rebuildRows));
	m_sourceConnections.append(connect(
		m_sourceModel,
		&QAbstractItemModel::dataChanged,
		this,
		[this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>&) {
			updateCellsForSourceRows(topLeft.row(), bottomRight.row());
		}));
}

void Lr2SelectBarModel::rebuildRows() {
	beginResetModel();
	m_rows.clear();
	const int count = sourceCount();
	if (count > 0 && m_rowCountLimit > 0) {
		m_rows.reserve(m_rowCountLimit);
		for (int row = 0; row < m_rowCountLimit; ++row) {
			m_rows.append(BarRow{
				.visualRow = row,
				.sourceRow = sourceRowForVisualRow(row),
				.slot = row,
			});
		}
	}
	ensureCellList(m_rows.size());
	m_slotOffset = 0;
	updateAllCells();
	endResetModel();
	emit slotOffsetChanged();
	emit cellsChanged();
	emit entriesChanged();
}

void Lr2SelectBarModel::emitAllRowsChanged() {
	if (m_rows.isEmpty()) {
		return;
	}
	updateAllCells();
	emit entriesChanged();
}

void Lr2SelectBarModel::ensureCellList(int count) {
	while (m_cellObjects.size() > count) {
		delete m_cellObjects.takeLast();
		m_cellValues.removeLast();
	}
	while (m_cellObjects.size() < count) {
		auto* cell = new Lr2SelectBarCell(this);
		QQmlEngine::setObjectOwnership(cell, QQmlEngine::CppOwnership);
		m_cellObjects.append(cell);
		m_cellValues.append(QVariant::fromValue(static_cast<QObject*>(cell)));
	}
}

void Lr2SelectBarModel::setSlotOffsetValue(int offset) {
	const int next = positiveModulo(offset, m_rows.size());
	if (m_slotOffset == next) {
		return;
	}
	m_slotOffset = next;
	emit slotOffsetChanged();
}

void Lr2SelectBarModel::syncRowSlots() {
	for (int row = 0; row < m_rows.size(); ++row) {
		m_rows[row].visualRow = row;
		m_rows[row].slot = slotForRow(row);
	}
}

void Lr2SelectBarModel::updateCellAtSlot(int slot, const BarRow& row) {
	if (slot < 0 || slot >= m_cellObjects.size() || !m_cellObjects.at(slot)) {
		return;
	}
	auto* cell = m_cellObjects.at(slot).data();
	if (!m_sourceModel || !m_sourceModel->populateBarCell(row.sourceRow, row.visualRow, cell)) {
		return;
	}
}

void Lr2SelectBarModel::updateCellForVisualRow(int row) {
	if (row < 0 || row >= m_rows.size()) {
		return;
	}
	updateCellAtSlot(m_rows.at(row).slot, m_rows.at(row));
}

void Lr2SelectBarModel::updateCellsForSourceRows(int first, int last) {
	if (m_rows.isEmpty()) {
		return;
	}
	if (first > last) {
		std::swap(first, last);
	}
	for (int row = 0; row < m_rows.size(); ++row) {
		const int sourceRow = m_rows.at(row).sourceRow;
		if (sourceRow < first || sourceRow > last) {
			continue;
		}
		updateCellForVisualRow(row);
	}
}

void Lr2SelectBarModel::updateAllCells() {
	ensureCellList(m_rows.size());
	for (int row = 0; row < m_rows.size(); ++row) {
		updateCellForVisualRow(row);
	}
}

QVariantList Lr2SelectBarModel::buildEntries() const {
	QVariantList result;
	result.reserve(m_rows.size());
	for (const BarRow& row : m_rows) {
		result.append(sourceData(row, Lr2SelectItemModel::RawItemRole));
	}
	return result;
}
