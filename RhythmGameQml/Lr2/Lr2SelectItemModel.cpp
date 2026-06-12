#include "Lr2SelectItemModel.h"

#include "Lr2SelectBarCell.h"
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartData.h"
#include "resource_managers/Tables.h"

#include <QMetaProperty>
#include <QMetaType>
#include <QObject>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace {

gameplay_logic::ChartData* chartDataObject(const QVariant& value) {
	if (auto* chartData = value.value<gameplay_logic::ChartData*>()) {
		return chartData;
	}
	if (auto* object = value.value<QObject*>()) {
		return qobject_cast<gameplay_logic::ChartData*>(object);
	}
	return nullptr;
}

QVariantMap mapFromVariant(const QVariant& value) {
	if (value.canConvert<QVariantMap>()) {
		return value.toMap();
	}
	return {};
}

QVariantList listFromVariant(const QVariant& value) {
	if (value.canConvert<QVariantList>()) {
		return value.toList();
	}
	return {};
}

QVariant mapFieldValue(const QVariantMap& map, const QString& key) {
	const auto it = map.constFind(key);
	if (it != map.constEnd()) {
		return *it;
	}
	if (key == QStringLiteral("duration")) {
		const auto lengthIt = map.constFind(QStringLiteral("length"));
		if (lengthIt != map.constEnd()) {
			return *lengthIt;
		}
	}
	if (key == QStringLiteral("playLevel")) {
		const auto levelIt = map.constFind(QStringLiteral("level"));
		if (levelIt != map.constEnd()) {
			return *levelIt;
		}
	}
	return {};
}

QObject* objectFromVariant(const QVariant& value) {
	if (auto* object = value.value<QObject*>()) {
		return object;
	}
	return chartDataObject(value);
}

QVariant objectFieldValue(const QVariant& value, const QString& key) {
	QObject* object = objectFromVariant(value);
	if (!object) {
		return {};
	}

	QByteArray propertyNameBytes = key.toLatin1();
	QVariant field = object->property(propertyNameBytes.constData());
	if (!field.isValid() && key == QStringLiteral("duration")) {
		propertyNameBytes = QByteArrayLiteral("length");
		field = object->property(propertyNameBytes.constData());
	}
	return field.isValid() ? field : QVariant {};
}

QVariant gadgetFieldValue(const QVariant& value, const QString& key) {
	if (!value.isValid() || value.isNull() || objectFromVariant(value)) {
		return {};
	}

	const QMetaObject* metaObject = value.metaType().metaObject();
	if (!metaObject) {
		return {};
	}

	QByteArray propertyNameBytes = key.toLatin1();
	int propertyIndex = metaObject->indexOfProperty(propertyNameBytes.constData());
	if (propertyIndex < 0 && key == QStringLiteral("playLevel")) {
		propertyNameBytes = QByteArrayLiteral("level");
		propertyIndex = metaObject->indexOfProperty(propertyNameBytes.constData());
	}
	if (propertyIndex < 0) {
		return {};
	}

	const QMetaProperty property = metaObject->property(propertyIndex);
	const void* gadget = value.constData();
	if (value.metaType().flags().testFlag(QMetaType::PointerToGadget)) {
		gadget = *static_cast<void* const*>(value.constData());
	}
	if (!gadget) {
		return {};
	}

	const QVariant field = property.readOnGadget(gadget);
	return field.isValid() ? field : QVariant {};
}

QVariant variantFieldValue(const QVariant& value, const QString& key) {
	if (!value.isValid() || value.isNull()) {
		return {};
	}

	const QVariantMap map = mapFromVariant(value);
	if (!map.isEmpty()) {
		const QVariant field = mapFieldValue(map, key);
		if (field.isValid()) {
			return field;
		}
	}

	const QVariant objectField = objectFieldValue(value, key);
	if (objectField.isValid()) {
		return objectField;
	}
	return gadgetFieldValue(value, key);
}

template<typename T>
bool variantHasType(const QVariant& value) {
	const QMetaType expectedType = QMetaType::fromType<T>();
	return value.isValid()
		&& !value.isNull()
		&& (value.metaType().id() == expectedType.id()
			|| value.metaType().metaObject() == expectedType.metaObject());
}

Lr2SelectItemModel::ItemKind resourceItemKind(const QVariant& value) {
	if (variantHasType<resource_managers::Entry>(value)) return Lr2SelectItemModel::EntryKind;
	if (variantHasType<resource_managers::Course>(value)) return Lr2SelectItemModel::CourseKind;
	if (variantHasType<resource_managers::Table>(value)) return Lr2SelectItemModel::TableKind;
	if (variantHasType<resource_managers::Level>(value)) return Lr2SelectItemModel::LevelKind;
	return Lr2SelectItemModel::UnknownKind;
}

Lr2SelectItemModel::ItemKind kindFromFolderKey(const QString& key) {
	if (key.startsWith(QStringLiteral("table:"))) return Lr2SelectItemModel::TableKind;
	if (key.startsWith(QStringLiteral("level:"))) return Lr2SelectItemModel::LevelKind;
	if (key.startsWith(QStringLiteral("folder:"))) return Lr2SelectItemModel::FolderKind;
	return Lr2SelectItemModel::UnknownKind;
}

QString normalizedFolderName(QString path) {
	path.replace(QLatin1Char('\\'), QLatin1Char('/'));
	if (path.endsWith(QLatin1Char('/'))) {
		path.chop(1);
	}
	const int slash = path.lastIndexOf(QLatin1Char('/'));
	return slash >= 0 ? path.mid(slash + 1) : path;
}

QVariantList paddedNumberList(const QVariant& value, int size) {
	QVariantList source = listFromVariant(value);
	QVariantList result;
	result.reserve(size);
	for (int i = 0; i < size; ++i) {
		result.append(i < source.size() ? std::max(0.0, source.at(i).toDouble()) : 0);
	}
	return result;
}

bool variantListContainsInt(const QVariantList& values, int needle) {
	for (const QVariant& value : values) {
		if (value.toInt() == needle) {
			return true;
		}
	}
	return false;
}

bool variantListHasExtendedLamp(const QVariantList& values) {
	for (const QVariant& value : values) {
		if (value.toInt() > 5) {
			return true;
		}
	}
	return false;
}

gameplay_logic::BmsScore* scoreObject(const QVariant& value) {
	if (auto* score = value.value<gameplay_logic::BmsScore*>()) {
		return score;
	}
	if (auto* object = value.value<QObject*>()) {
		return qobject_cast<gameplay_logic::BmsScore*>(object);
	}
	return nullptr;
}

int rankForScoreRate(double rate) {
	if (rate >= 1.0) return 9;
	if (rate >= 8.0 / 9.0) return 8;
	if (rate >= 7.0 / 9.0) return 7;
	if (rate >= 6.0 / 9.0) return 6;
	if (rate >= 5.0 / 9.0) return 5;
	if (rate >= 4.0 / 9.0) return 4;
	if (rate >= 3.0 / 9.0) return 3;
	if (rate >= 2.0 / 9.0) return 2;
	return rate > 0.0 ? 1 : 0;
}

QVariantList scoreListForIdentifier(const QVariantMap& scores, const QString& identifier) {
	if (identifier.isEmpty()) {
		return {};
	}
	QVariant value = scores.value(identifier);
	if (!value.isValid()) {
		value = scores.value(identifier.toUpper());
	}
	if (!value.isValid()) {
		value = scores.value(identifier.toLower());
	}
	return listFromVariant(value);
}

QString normalizedClearType(const QString& clear) {
	const QString value = clear.isEmpty() ? QStringLiteral("NOPLAY") : clear.toUpper();
	if (value == QStringLiteral("ASSIST")
			|| value == QStringLiteral("ASSISTEASY")
			|| value == QStringLiteral("ASSIST_EASY")) {
		return QStringLiteral("AEASY");
	}
	if (value == QStringLiteral("LIGHT_ASSIST")
			|| value == QStringLiteral("LIGHTASSISTEASY")
			|| value == QStringLiteral("LIGHT_ASSIST_EASY")) {
		return QStringLiteral("LIGHTASSIST");
	}
	if (value == QStringLiteral("CLEAR") || value == QStringLiteral("DAN")) {
		return QStringLiteral("NORMAL");
	}
	if (value == QStringLiteral("EX_HARD")) {
		return QStringLiteral("EXHARD");
	}
	if (value == QStringLiteral("EXDAN")
			|| value == QStringLiteral("HARD_DAN")
			|| value == QStringLiteral("HARD DAN")) {
		return QStringLiteral("HARD");
	}
	if (value == QStringLiteral("EXHARDDAN")
			|| value == QStringLiteral("EXHARD_DAN")
			|| value == QStringLiteral("EX_HARD_DAN")) {
		return QStringLiteral("EXHARD");
	}
	if (value == QStringLiteral("FULLCOMBO")
			|| value == QStringLiteral("FULL_COMBO")
			|| value == QStringLiteral("FULL COMBO")) {
		return QStringLiteral("FC");
	}
	if (value == QStringLiteral("NO_PLAY") || value == QStringLiteral("NO PLAY")) {
		return QStringLiteral("NOPLAY");
	}
	return value;
}

QString skinCompatibleClearType(const QString& clear, bool useBeatorajaSelectOptions) {
	const QString value = normalizedClearType(clear);
	if (useBeatorajaSelectOptions) {
		return value;
	}
	if (value == QStringLiteral("AEASY") || value == QStringLiteral("LIGHTASSIST")) {
		return QStringLiteral("FAILED");
	}
	if (value == QStringLiteral("EXHARD") || value == QStringLiteral("EXHARDDAN")) {
		return QStringLiteral("HARD");
	}
	return value;
}

int clearTypePriority(const QString& clear, bool useBeatorajaSelectOptions) {
	const QString value = skinCompatibleClearType(clear, useBeatorajaSelectOptions);
	if (value == QStringLiteral("FAILED")) return 1;
	if (value == QStringLiteral("AEASY")) return 2;
	if (value == QStringLiteral("LIGHTASSIST")) return 3;
	if (value == QStringLiteral("EASY")) return 4;
	if (value == QStringLiteral("NORMAL")) return 5;
	if (value == QStringLiteral("HARD")) return 6;
	if (value == QStringLiteral("EXHARD")) return 7;
	if (value == QStringLiteral("FC")) return 8;
	if (value == QStringLiteral("PERFECT")) return 9;
	if (value == QStringLiteral("MAX")) return 10;
	return 0;
}

int collapsedClearTypeLamp(const QString& clear) {
	const QString value = normalizedClearType(clear);
	if (value == QStringLiteral("FAILED")) return 1;
	if (value == QStringLiteral("AEASY")
			|| value == QStringLiteral("LIGHTASSIST")
			|| value == QStringLiteral("EASY")) {
		return 2;
	}
	if (value == QStringLiteral("NORMAL")) return 3;
	if (value == QStringLiteral("HARD") || value == QStringLiteral("EXHARD")) return 4;
	if (value == QStringLiteral("FC")
			|| value == QStringLiteral("PERFECT")
			|| value == QStringLiteral("MAX")) {
		return 5;
	}
	return 0;
}

int clearTypeBarLamp(const QString& clear, bool useBeatorajaSelectOptions, const QVariantList& variants) {
	const QString value = skinCompatibleClearType(clear, useBeatorajaSelectOptions);
	if (!useBeatorajaSelectOptions || !variantListHasExtendedLamp(variants)) {
		return collapsedClearTypeLamp(value);
	}
	if (value == QStringLiteral("FAILED")) return 1;
	if (value == QStringLiteral("AEASY")) return variantListContainsInt(variants, 9) ? 9 : 2;
	if (value == QStringLiteral("LIGHTASSIST")) return variantListContainsInt(variants, 10) ? 10 : 2;
	if (value == QStringLiteral("EASY")) return 2;
	if (value == QStringLiteral("NORMAL")) return 3;
	if (value == QStringLiteral("HARD")) return 4;
	if (value == QStringLiteral("EXHARD")) return variantListContainsInt(variants, 5) ? 5 : 4;
	if (value == QStringLiteral("FC")) return 6;
	if (value == QStringLiteral("PERFECT")) return variantListContainsInt(variants, 7) ? 7 : 6;
	if (value == QStringLiteral("MAX")) return variantListContainsInt(variants, 8) ? 8 : 6;
	return 0;
}

int rankingEntryRank(double points, double maxPoints) {
	if (maxPoints <= 0.0) {
		return 0;
	}
	int rank = static_cast<int>(std::floor(points * 9.0 / maxPoints));
	if (rank > 7) {
		rank = 8;
	}
	if (rank < 2 && points > 0.0) {
		rank = 1;
	}
	return rank;
}

} // namespace

Lr2SelectItemModel::Lr2SelectItemModel(QObject* parent)
	: QAbstractListModel(parent) {}

int Lr2SelectItemModel::rowCount(const QModelIndex& parent) const {
	return parent.isValid() ? 0 : m_items.size();
}

QVariant Lr2SelectItemModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
		return {};
	}
	return roleData(m_items.at(index.row()), role);
}

QHash<int, QByteArray> Lr2SelectItemModel::roleNames() const {
	return {
		{KeyRole, "key"},
		{KindRole, "kind"},
		{DisplayTextRole, "displayText"},
		{TitleTypeRole, "titleType"},
		{BodyTypeRole, "bodyType"},
		{TitleRole, "title"},
		{SubtitleRole, "subtitle"},
		{ArtistRole, "artist"},
		{SubartistRole, "subartist"},
		{GenreRole, "genre"},
		{TagRole, "tag"},
		{PathRole, "path"},
		{Md5Role, "md5"},
		{Sha256Role, "sha256"},
		{DirectoryRole, "directory"},
		{PlayLevelRole, "playLevel"},
		{DifficultyRole, "difficulty"},
		{KeymodeRole, "keymode"},
		{RankRole, "rank"},
		{DurationRole, "duration"},
		{MinBpmRole, "minBpm"},
		{MaxBpmRole, "maxBpm"},
		{MainBpmRole, "mainBpm"},
		{LampRole, "lamp"},
		{ScoreRankRole, "scoreRank"},
		{ScoreRateRole, "scoreRate"},
		{LabelMaskRole, "labelMask"},
		{FolderLampRole, "folderLamp"},
		{FolderScoreCountsRole, "folderScoreCounts"},
		{FolderGraphLampsRole, "folderGraphLamps"},
		{FolderGraphRanksRole, "folderGraphRanks"},
		{IsChartRole, "isChart"},
		{IsEntryRole, "isEntry"},
		{IsCourseRole, "isCourse"},
		{IsFolderLikeRole, "isFolderLike"},
		{IsRankingEntryRole, "isRankingEntry"},
		{RawItemRole, "rawItem"},
		{ChartDataRole, "chartData"},
		{ActivationObjectRole, "activationObject"},
	};
}

QVariantList Lr2SelectItemModel::items() const {
	QVariantList result;
	result.reserve(m_items.size());
	for (const Item& item : m_items) {
		result.append(item.raw);
	}
	return result;
}

void Lr2SelectItemModel::setItems(const QVariantList& items) {
	if (items.size() == m_items.size()) {
		updateExistingItems(items);
		return;
	}
	replaceItems(items);
}

int Lr2SelectItemModel::count() const {
	return m_items.size();
}

void Lr2SelectItemModel::replaceItems(const QVariantList& items) {
	beginResetModel();
	m_items.clear();
	m_items.reserve(items.size());
	for (int i = 0; i < items.size(); ++i) {
		m_items.append(itemFromVariant(items.at(i), i));
	}
	const int maxIndex = std::max<int>(0, static_cast<int>(m_items.size()) - 1);
	m_currentIndex = std::clamp<int>(m_currentIndex, 0, maxIndex);
	endResetModel();
	emit itemsChanged();
	emit currentIndexChanged();
	emit currentItemChanged();
}

int Lr2SelectItemModel::currentIndex() const {
	return m_currentIndex;
}

void Lr2SelectItemModel::setCurrentIndex(int index) {
	const int maxIndex = std::max<int>(0, static_cast<int>(m_items.size()) - 1);
	const int next = std::clamp<int>(index, 0, maxIndex);
	if (m_currentIndex == next) {
		return;
	}
	m_currentIndex = next;
	emit currentIndexChanged();
	emit currentItemChanged();
}

QVariant Lr2SelectItemModel::currentItem() const {
	return rawItemAt(m_currentIndex);
}

QString Lr2SelectItemModel::currentKey() const {
	return keyAt(m_currentIndex);
}

bool Lr2SelectItemModel::useBeatorajaBarTextTypes() const {
	return m_useBeatorajaBarTextTypes;
}

void Lr2SelectItemModel::setUseBeatorajaBarTextTypes(bool value) {
	if (m_useBeatorajaBarTextTypes == value) {
		return;
	}
	m_useBeatorajaBarTextTypes = value;
	emit selectSkinOptionsChanged();
	refreshDerivedItems();
}

bool Lr2SelectItemModel::useBeatorajaSelectOptions() const {
	return m_useBeatorajaSelectOptions;
}

void Lr2SelectItemModel::setUseBeatorajaSelectOptions(bool value) {
	if (m_useBeatorajaSelectOptions == value) {
		return;
	}
	m_useBeatorajaSelectOptions = value;
	emit selectSkinOptionsChanged();
	refreshDerivedItems();
}

QVariantList Lr2SelectItemModel::barBodyTypes() const {
	return m_barBodyTypes;
}

void Lr2SelectItemModel::setBarBodyTypes(const QVariantList& types) {
	if (m_barBodyTypes == types) {
		return;
	}
	m_barBodyTypes = types;
	emit selectSkinOptionsChanged();
	refreshDerivedItems();
}

QVariantList Lr2SelectItemModel::barTitleTypes() const {
	return m_barTitleTypes;
}

void Lr2SelectItemModel::setBarTitleTypes(const QVariantList& types) {
	if (m_barTitleTypes == types) {
		return;
	}
	m_barTitleTypes = types;
	emit selectSkinOptionsChanged();
	refreshDerivedItems();
}

QVariantList Lr2SelectItemModel::barLampVariants() const {
	return m_barLampVariants;
}

void Lr2SelectItemModel::setBarLampVariants(const QVariantList& variants) {
	if (m_barLampVariants == variants) {
		return;
	}
	m_barLampVariants = variants;
	emit selectSkinOptionsChanged();
	refreshDerivedItems();
}

resource_managers::ChartFolderModel* Lr2SelectItemModel::chartFolderModel() const {
	return m_chartFolderModel;
}

void Lr2SelectItemModel::setChartFolderModel(resource_managers::ChartFolderModel* model) {
	if (m_chartFolderModel == model) {
		return;
	}
	m_chartFolderModel = model;
	emit chartFolderModelChanged();
	refreshDerivedItems();
}

QString Lr2SelectItemModel::levelFolderParentKey() const {
	return m_levelFolderParentKey;
}

void Lr2SelectItemModel::setLevelFolderParentKey(const QString& key) {
	if (m_levelFolderParentKey == key) {
		return;
	}
	m_levelFolderParentKey = key;
	emit levelFolderParentChanged();
	refreshDerivedItems();
}

QString Lr2SelectItemModel::levelFolderParentSymbol() const {
	return m_levelFolderParentSymbol;
}

void Lr2SelectItemModel::setLevelFolderParentSymbol(const QString& symbol) {
	if (m_levelFolderParentSymbol == symbol) {
		return;
	}
	m_levelFolderParentSymbol = symbol;
	emit levelFolderParentChanged();
	refreshDerivedItems();
}

QVariant Lr2SelectItemModel::rawItemAt(int row) const {
	return row >= 0 && row < m_items.size() ? m_items.at(row).raw : QVariant();
}

QString Lr2SelectItemModel::keyAt(int row) const {
	return row >= 0 && row < m_items.size() ? m_items.at(row).key : QString();
}

int Lr2SelectItemModel::indexOfRawItem(const QVariant& item) const {
	if (!item.isValid() || item.isNull()) {
		return -1;
	}

	const Item needle = itemFromVariant(item, -1);
	if (needle.key.isEmpty()) {
		return -1;
	}
	for (int i = 0; i < m_items.size(); ++i) {
		if (m_items.at(i).key == needle.key) {
			return i;
		}
	}
	return -1;
}

void Lr2SelectItemModel::addToMinimumCount(QVariantList& items, int minimumCount) {
	const int length = items.size();
	if (length <= 0 || length >= minimumCount) {
		return;
	}

	const int limit = ((minimumCount + length - 1) / length) * length;
	items.reserve(limit);
	for (int i = length; i < limit; ++i) {
		items.append(items.at(i % length));
	}
}

QVariantMap Lr2SelectItemModel::setFolderItems(resource_managers::ChartFolderModel* folderModel,
											   const QVariantList& folderContents,
											   const QVariant& preferredItem,
											   int minimumCount) {
	Q_ASSERT(folderModel);

	QVariantList sortedFiltered = folderModel->filterAndSort(folderContents);
	const int realCount = sortedFiltered.size();
	addToMinimumCount(sortedFiltered, minimumCount);
	setItems(sortedFiltered);

	const int preferredIndex = indexOfRawItem(preferredItem);
	return QVariantMap {
		{QStringLiteral("realItemCount"), realCount},
		{QStringLiteral("preferredIndex"), preferredIndex},
	};
}

void Lr2SelectItemModel::moveRowTo(int from, int to) {
	if (from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size() || from == to) {
		return;
	}
	const bool currentItemMoved = m_currentIndex >= std::min(from, to)
		&& m_currentIndex <= std::max(from, to);
	const int destination = to > from ? to + 1 : to;
	if (!beginMoveRows({}, from, from, {}, destination)) {
		return;
	}
	m_items.move(from, to);
	endMoveRows();
	if (currentItemMoved) {
		emit currentItemChanged();
	}
}

void Lr2SelectItemModel::updateExistingItems(const QVariantList& items) {
	if (items.isEmpty()) {
		return;
	}
	for (int i = 0; i < items.size(); ++i) {
		m_items[i] = itemFromVariant(items.at(i), i);
	}
	QList<int> roles;
	const auto names = roleNames();
	roles.reserve(names.size());
	for (auto it = names.cbegin(); it != names.cend(); ++it) {
		roles.append(it.key());
	}
	emit dataChanged(index(0, 0), index(m_items.size() - 1, 0), roles);
	emit itemsChanged();
	emit currentItemChanged();
}

void Lr2SelectItemModel::refreshDerivedItems() {
	if (m_items.isEmpty()) {
		return;
	}
	updateExistingItems(items());
}

void Lr2SelectItemModel::clearFolderSummaries() {
	if (m_folderSummaries.isEmpty() && m_items.isEmpty()) {
		return;
	}

	m_folderSummaries.clear();
	QList<int> changedRoles {
		LampRole,
		FolderLampRole,
		FolderScoreCountsRole,
		FolderGraphLampsRole,
		FolderGraphRanksRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.folderKey.isEmpty()
				&& item.folderLamp == 0
				&& item.folderGraphLamps.isEmpty()
				&& item.folderGraphRanks.isEmpty()
				&& (!item.folderScoreCounts.isValid() || item.folderScoreCounts.isNull())) {
			continue;
		}
		item.folderLamp = 0;
		item.folderScoreCounts = {};
		item.folderGraphLamps = {};
		item.folderGraphRanks = {};
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::setFolderSummary(const QString& key,
										  int lamp,
										  const QVariant& scoreCounts,
										  const QVariant& distribution) {
	if (key.isEmpty()) {
		return;
	}

	FolderSummary summary = folderSummaryFromValues(lamp, scoreCounts, distribution);
	m_folderSummaries.insert(key, summary);

	QList<int> changedRoles {
		LampRole,
		FolderLampRole,
		FolderScoreCountsRole,
		FolderGraphLampsRole,
		FolderGraphRanksRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.folderKey != key) {
			continue;
		}
		const bool changed = item.folderLamp != summary.lamp
			|| item.folderScoreCounts != summary.scoreCounts
			|| item.folderGraphLamps != summary.graphLamps
			|| item.folderGraphRanks != summary.graphRanks;
		if (!changed) {
			continue;
		}
		item.folderLamp = summary.lamp;
		item.folderScoreCounts = summary.scoreCounts;
		item.folderGraphLamps = summary.graphLamps;
		item.folderGraphRanks = summary.graphRanks;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::setFolderSummaries(const QVariantMap& lamps,
											const QVariantMap& scoreCounts,
											const QVariantMap& distributions) {
	if (lamps.isEmpty()) {
		return;
	}

	for (auto it = lamps.constBegin(); it != lamps.constEnd(); ++it) {
		const QString& key = it.key();
		if (key.isEmpty()) {
			continue;
		}

		m_folderSummaries.insert(
			key,
			folderSummaryFromValues(it.value().toInt(),
									scoreCounts.value(key),
									distributions.value(key)));
	}

	QList<int> changedRoles {
		LampRole,
		FolderLampRole,
		FolderScoreCountsRole,
		FolderGraphLampsRole,
		FolderGraphRanksRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.folderKey.isEmpty() || !lamps.contains(item.folderKey)) {
			continue;
		}
		const auto summaryIt = m_folderSummaries.constFind(item.folderKey);
		if (summaryIt == m_folderSummaries.constEnd()) {
			continue;
		}
		const FolderSummary& summary = summaryIt.value();
		const bool changed = item.folderLamp != summary.lamp
			|| item.folderScoreCounts != summary.scoreCounts
			|| item.folderGraphLamps != summary.graphLamps
			|| item.folderGraphRanks != summary.graphRanks;
		if (!changed) {
			continue;
		}
		item.folderLamp = summary.lamp;
		item.folderScoreCounts = summary.scoreCounts;
		item.folderGraphLamps = summary.graphLamps;
		item.folderGraphRanks = summary.graphRanks;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::clearScoreSummaries() {
	QList<int> changedRoles {
		LampRole,
		ScoreRankRole,
		ScoreRateRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.kind != ChartKind && item.kind != EntryKind && item.kind != CourseKind) {
			continue;
		}
		if (item.lamp == 0 && item.scoreRank == 0 && item.scoreRate == 0.0) {
			continue;
		}
		item.lamp = 0;
		item.scoreRank = 0;
		item.scoreRate = 0.0;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::setScoreSummary(const QString& identifier,
										 int lamp,
										 int scoreRank,
										 double scoreRate) {
	if (identifier.isEmpty()) {
		return;
	}

	const int normalizedLamp = std::max(0, lamp);
	const int normalizedRank = std::max(0, scoreRank);
	const double normalizedRate = std::max(0.0, scoreRate);
	QList<int> changedRoles {
		LampRole,
		ScoreRankRole,
		ScoreRateRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.scoreIdentifier != identifier) {
			continue;
		}
		const bool changed = item.lamp != normalizedLamp
			|| item.scoreRank != normalizedRank
			|| item.scoreRate != normalizedRate;
		if (!changed) {
			continue;
		}
		item.lamp = normalizedLamp;
		item.scoreRank = normalizedRank;
		item.scoreRate = normalizedRate;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::setScoreSummaries(const QVariantMap& lamps,
										   const QVariantMap& scoreRanks,
										   const QVariantMap& scoreRates) {
	QList<int> changedRoles {
		LampRole,
		ScoreRankRole,
		ScoreRateRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.kind != ChartKind && item.kind != EntryKind && item.kind != CourseKind) {
			continue;
		}

		const bool hasSummary = !item.scoreIdentifier.isEmpty()
			&& lamps.contains(item.scoreIdentifier);
		const int normalizedLamp = hasSummary
			? std::max(0, lamps.value(item.scoreIdentifier).toInt())
			: 0;
		const int normalizedRank = hasSummary
			? std::max(0, scoreRanks.value(item.scoreIdentifier).toInt())
			: 0;
		const double normalizedRate = hasSummary
			? std::max(0.0, scoreRates.value(item.scoreIdentifier).toDouble())
			: 0.0;

		const bool changed = item.lamp != normalizedLamp
			|| item.scoreRank != normalizedRank
			|| item.scoreRate != normalizedRate;
		if (!changed) {
			continue;
		}

		item.lamp = normalizedLamp;
		item.scoreRank = normalizedRank;
		item.scoreRate = normalizedRate;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

void Lr2SelectItemModel::setScoreSummariesFromScores(const QVariantMap& scores) {
	struct ScoreSummary {
		int lamp = 0;
		int rank = 0;
		double rate = 0.0;
	};

	auto summaryFor = [this, &scores](const QString& identifier) {
		ScoreSummary summary;
		const QVariantList scoreList = scoreListForIdentifier(scores, identifier);
		if (scoreList.isEmpty()) {
			return summary;
		}

		QString bestClearType = QStringLiteral("NOPLAY");
		int bestClearPriority = 0;
		double bestRate = -1.0;
		bool bestHasMaxPoints = false;
		bool hasBestScore = false;

		for (const QVariant& scoreValue : scoreList) {
			const auto* score = scoreObject(scoreValue);
			const auto* result = score ? score->getResult() : nullptr;
			if (!result) {
				continue;
			}

			const QString clearType = result->getClearType();
			const int clearPriority = clearTypePriority(clearType, m_useBeatorajaSelectOptions);
			if (clearPriority > bestClearPriority) {
				bestClearPriority = clearPriority;
				bestClearType = skinCompatibleClearType(clearType, m_useBeatorajaSelectOptions);
			}

			const double maxPoints = result->getMaxPoints();
			const bool hasMaxPoints = maxPoints > 0.0;
			const double rate = hasMaxPoints ? result->getPoints() / maxPoints : 0.0;
			if (rate > bestRate || (rate == bestRate && hasMaxPoints && !bestHasMaxPoints)) {
				bestRate = rate;
				bestHasMaxPoints = hasMaxPoints;
				hasBestScore = true;
			}
		}

		if (bestClearType != QStringLiteral("NOPLAY")) {
			summary.lamp = clearTypeBarLamp(
				bestClearType,
				m_useBeatorajaSelectOptions,
				m_barLampVariants);
		}
		summary.rate = std::max(0.0, bestRate);
		summary.rank = hasBestScore
			? std::max(1, rankForScoreRate(summary.rate))
			: 0;
		return summary;
	};

	QHash<QString, ScoreSummary> summaries;
	QList<int> changedRoles {
		LampRole,
		ScoreRankRole,
		ScoreRateRole,
	};
	int firstChanged = -1;
	int lastChanged = -1;
	for (int row = 0; row < m_items.size(); ++row) {
		Item& item = m_items[row];
		if (item.kind != ChartKind && item.kind != EntryKind && item.kind != CourseKind) {
			continue;
		}

		ScoreSummary summary;
		if (!item.scoreIdentifier.isEmpty()) {
			if (!summaries.contains(item.scoreIdentifier)) {
				summaries.insert(item.scoreIdentifier, summaryFor(item.scoreIdentifier));
			}
			summary = summaries.value(item.scoreIdentifier);
		}

		const bool changed = item.lamp != summary.lamp
			|| item.scoreRank != summary.rank
			|| item.scoreRate != summary.rate;
		if (!changed) {
			continue;
		}

		item.lamp = summary.lamp;
		item.scoreRank = summary.rank;
		item.scoreRate = summary.rate;
		if (firstChanged < 0) {
			firstChanged = row;
		}
		lastChanged = row;
	}
	if (firstChanged >= 0) {
		emit dataChanged(index(firstChanged, 0), index(lastChanged, 0), changedRoles);
	}
}

bool Lr2SelectItemModel::populateBarCell(int sourceRow, int visualRow, Lr2SelectBarCell* cell) const {
	if (!cell) {
		return false;
	}
	if (sourceRow < 0 || sourceRow >= m_items.size()) {
		cell->setEntry({});
		cell->setCore(visualRow,
					  false,
					  {},
					  0,
					  0,
					  0,
					  0,
					  0,
					  false,
					  false,
					  false,
					  false,
					  0,
					  0,
					  0,
					  {},
					  {});
		return false;
	}

	const Item& item = m_items.at(sourceRow);
	const bool folderLike = item.kind == FolderKind || item.kind == TableKind || item.kind == LevelKind;
	cell->setEntry(item.raw);
	cell->setCore(visualRow,
				  true,
				  item.displayText,
				  item.titleType,
				  item.bodyType,
				  item.playLevel,
				  item.difficulty,
				  item.keymode,
				  item.kind == RankingKind,
				  item.kind == ChartKind,
				  item.kind == EntryKind,
				  folderLike,
				  folderLike ? item.folderLamp : item.lamp,
				  item.scoreRank,
				  item.labelMask,
				  item.folderGraphLamps,
				  item.folderGraphRanks);
	return true;
}

Lr2SelectItemModel::Item Lr2SelectItemModel::itemFromVariant(const QVariant& value, int fallbackIndex) const {
	Item item;
	item.map = mapFromVariant(value);
	const QVariant rawItem = item.map.value(QStringLiteral("rawItem"));
	item.raw = rawItem.isValid() && !rawItem.isNull() ? rawItem : value;
	item.kind = kindFor(value, item.map);

	if (item.kind == ChartKind && item.map.isEmpty()) {
		if (auto* chart = chartDataObject(item.raw)) {
			item.scoreIdentifier = chart->getMd5();
			item.titleType = titleTypeForItem(value, item.map, item.kind);
			item.bodyType = bodyTypeForItem(value, item.map, item.kind);
			item.title = chart->getTitle();
			item.subtitle = chart->getSubtitle();
			item.artist = chart->getArtist();
			item.subartist = chart->getSubartist();
			item.genre = chart->getGenre();
			item.path = chart->getPath();
			item.md5 = chart->getMd5();
			item.sha256 = chart->getSha256();
			item.directory = chart->getChartDirectory();
			if (item.directory.isEmpty()) {
				item.directory = chart->getDirectory();
			}
			item.playLevel = chart->getPlayLevel();
			item.difficulty = m_chartFolderModel
				? m_chartFolderModel->difficultyForChart(value)
				: std::max(0, chart->getDifficulty());
			item.keymode = static_cast<int>(chart->getKeymode());
			item.rank = static_cast<int>(chart->getRank());
			item.duration = static_cast<int>(chart->getLength());
			item.minBpm = chart->getMinBpm();
			item.maxBpm = chart->getMaxBpm();
			item.mainBpm = chart->getMainBpm();
			item.labelMask = 0;
			if (chart->getLnCount() + chart->getBssCount() > 0) {
				item.labelMask |= 1 << 0;
			}
			if (chart->getIsRandom()) {
				item.labelMask |= 1 << 1;
			}
			if (chart->getMineCount() > 0) {
				item.labelMask |= 1 << 2;
			}
			item.displayText = item.subtitle.isEmpty()
				? item.title
				: item.title + QLatin1Char(' ') + item.subtitle;
			const QString fallbackKey = QStringLiteral("index:%1").arg(fallbackIndex);
			item.key = QStringLiteral("chart:%1").arg(
				!item.md5.isEmpty() ? item.md5
				: !item.sha256.isEmpty() ? item.sha256
				: !item.path.isEmpty() ? item.path
				: fallbackKey);
			applyFolderSummary(item);
			return item;
		}
	}

	item.folderKey = stringField(value, item.map, "folderKey");
	if (item.folderKey.isEmpty()) {
		item.folderKey = folderLampKeyFor(value, item.map, item.kind);
	}
	item.scoreIdentifier = scoreIdentifierFor(value, item.map, item.kind);
	item.titleType = item.map.contains(QStringLiteral("titleType"))
		? intField(value, item.map, "titleType")
		: titleTypeForItem(value, item.map, item.kind);
	item.bodyType = item.map.contains(QStringLiteral("bodyType"))
		? intField(value, item.map, "bodyType")
		: bodyTypeForItem(value, item.map, item.kind);
	item.title = item.map.contains(QStringLiteral("title")) && item.map.contains(QStringLiteral("rawItem"))
		? stringField(value, item.map, "title")
		: titleForItem(value, item.map, item.kind);
	item.subtitle = stringField(value, item.map, "subtitle");
	item.artist = stringField(value, item.map, "artist");
	item.subartist = stringField(value, item.map, "subartist");
	item.genre = stringField(value, item.map, "genre");
	item.tag = stringField(value, item.map, "tag");
	item.path = stringField(value, item.map, "path");
	item.md5 = stringField(value, item.map, "md5");
	item.sha256 = stringField(value, item.map, "sha256");
	item.directory = stringField(value, item.map, "chartDirectory");
	if (item.directory.isEmpty()) {
		item.directory = stringField(value, item.map, "directory");
	}
	item.playLevel = intField(value, item.map, "playLevel");
	item.difficulty = item.kind != ChartKind && item.map.contains(QStringLiteral("difficulty"))
		? intField(value, item.map, "difficulty")
		: difficultyForItem(value, item.map, item.kind);
	item.keymode = intField(value, item.map, "keymode");
	item.rank = intField(value, item.map, "rank");
	item.duration = intField(value, item.map, "duration");
	item.minBpm = doubleField(value, item.map, "minBpm");
	item.maxBpm = doubleField(value, item.map, "maxBpm");
	item.mainBpm = doubleField(value, item.map, "mainBpm");
	item.lamp = item.map.contains(QStringLiteral("lamp"))
		? intField(value, item.map, "lamp")
		: lampForItem(value, item.map, item.kind);
	item.scoreRank = item.map.contains(QStringLiteral("scoreRank"))
		? intField(value, item.map, "scoreRank")
		: scoreRankForItem(value, item.map, item.kind);
	item.scoreRate = doubleField(value, item.map, "scoreRate");
	item.labelMask = item.map.contains(QStringLiteral("labelMask"))
		? intField(value, item.map, "labelMask")
		: labelMaskForItem(value, item.map, item.kind);
	item.folderLamp = intField(value, item.map, "folderLamp");
	item.folderScoreCounts = fieldValue(value, item.map, "folderScoreCounts");
	item.folderGraphLamps = listField(value, item.map, "folderGraphLamps");
	item.folderGraphRanks = listField(value, item.map, "folderGraphRanks");
	if (item.folderGraphLamps.isEmpty() || item.folderGraphRanks.isEmpty()) {
		const QVariant distribution = fieldValue(value, item.map, "folderDistribution");
		if (distribution.canConvert<QVariantMap>()) {
			const QVariantMap map = distribution.toMap();
			if (item.folderGraphLamps.isEmpty()) {
				item.folderGraphLamps = map.value(QStringLiteral("lamps")).toList();
			}
			if (item.folderGraphRanks.isEmpty()) {
				item.folderGraphRanks = map.value(QStringLiteral("ranks")).toList();
			}
		}
	}
	item.displayText = item.map.contains(QStringLiteral("displayText"))
		? displayTextFor(value, item.map, item.kind)
		: displayTextForItem(value, item.map, item.kind);
	item.key = keyFor(value, item.map, item.kind, fallbackIndex);
	applyFolderSummary(item);
	return item;
}

QString Lr2SelectItemModel::scoreIdentifierFor(const QVariant& value, const QVariantMap& map, ItemKind kind) {
	switch (kind) {
	case ChartKind:
	case EntryKind:
		return stringField(value, map, "md5");
	case CourseKind:
		return stringField(value, map, "identifier");
	default:
		return {};
	}
}

void Lr2SelectItemModel::applyFolderSummary(Item& item) const {
	if (item.folderKey.isEmpty()) {
		return;
	}
	const auto it = m_folderSummaries.constFind(item.folderKey);
	if (it == m_folderSummaries.constEnd()) {
		return;
	}
	item.folderLamp = it->lamp;
	item.folderScoreCounts = it->scoreCounts;
	item.folderGraphLamps = it->graphLamps;
	item.folderGraphRanks = it->graphRanks;
}

Lr2SelectItemModel::FolderSummary Lr2SelectItemModel::folderSummaryFromValues(
	int lamp,
	const QVariant& scoreCounts,
	const QVariant& distribution) {
	FolderSummary result;
	result.lamp = std::max(0, lamp);
	result.scoreCounts = scoreCounts;

	QVariantMap map;
	if (distribution.canConvert<QVariantMap>()) {
		map = distribution.toMap();
	}

	result.graphLamps = paddedNumberList(map.value(QStringLiteral("lamps")), 11);
	result.graphRanks = paddedNumberList(map.value(QStringLiteral("ranks")), 28);
	return result;
}

QVariant Lr2SelectItemModel::fieldValue(const QVariant& value, const QVariantMap& map, const char* name) {
	const QString key = QString::fromLatin1(name);
	const QVariant mapField = mapFieldValue(map, key);
	if (mapField.isValid()) {
		return mapField;
	}
	const QVariant rawItem = map.value(QStringLiteral("rawItem"));
	if (rawItem.isValid() && !rawItem.isNull()) {
		const QVariant rawField = variantFieldValue(rawItem, key);
		if (rawField.isValid()) {
			return rawField;
		}
	}
	return variantFieldValue(value, key);
}

QVariantList Lr2SelectItemModel::listField(const QVariant& value, const QVariantMap& map, const char* name) {
	return fieldValue(value, map, name).toList();
}

QString Lr2SelectItemModel::stringField(const QVariant& value, const QVariantMap& map, const char* name) {
	const QVariant field = fieldValue(value, map, name);
	return field.isValid() && !field.isNull() ? field.toString() : QString();
}

int Lr2SelectItemModel::intField(const QVariant& value, const QVariantMap& map, const char* name, int fallback) {
	const QVariant field = fieldValue(value, map, name);
	bool ok = false;
	const int result = field.toInt(&ok);
	return ok ? result : fallback;
}

double Lr2SelectItemModel::doubleField(const QVariant& value, const QVariantMap& map, const char* name, double fallback) {
	const QVariant field = fieldValue(value, map, name);
	bool ok = false;
	const double result = field.toDouble(&ok);
	return ok ? result : fallback;
}

bool Lr2SelectItemModel::boolField(const QVariant& value, const QVariantMap& map, const char* name) {
	const QVariant field = fieldValue(value, map, name);
	return field.isValid() && !field.isNull() && field.toBool();
}

Lr2SelectItemModel::ItemKind Lr2SelectItemModel::kindFor(const QVariant& value, const QVariantMap& map) {
	if (!value.isValid() || value.isNull()) {
		return EmptyKind;
	}
	if (boolField(value, map, "__lr2RankingEntry")) {
		return RankingKind;
	}
	const QVariant rawItem = map.value(QStringLiteral("rawItem"));
	if (map.contains(QStringLiteral("rawItem")) && (!rawItem.isValid() || rawItem.isNull())) {
		return EmptyKind;
	}
	if (value.typeId() == QMetaType::QString || rawItem.typeId() == QMetaType::QString) {
		return FolderKind;
	}
	if (chartDataObject(value) || chartDataObject(rawItem)) {
		return ChartKind;
	}
	const ItemKind rawResourceKind = resourceItemKind(rawItem);
	if (rawResourceKind != UnknownKind) {
		return rawResourceKind;
	}
	const ItemKind resourceKind = resourceItemKind(value);
	if (resourceKind != UnknownKind) {
		return resourceKind;
	}
	const ItemKind folderKeyKind = kindFromFolderKey(stringField(value, map, "folderKey"));
	if (folderKeyKind != UnknownKind) {
		return folderKeyKind;
	}
	const QString type = stringField(value, map, "type").toLower();
	if (type == QStringLiteral("chart")) return ChartKind;
	if (type == QStringLiteral("entry")) return EntryKind;
	if (type == QStringLiteral("course")) return CourseKind;
	if (type == QStringLiteral("table")) return TableKind;
	if (type == QStringLiteral("level")) return LevelKind;
	if (type == QStringLiteral("folder")) return FolderKind;
	return UnknownKind;
}

QString Lr2SelectItemModel::displayTextFor(const QVariant& value, const QVariantMap& map, ItemKind kind) {
	if (kind == FolderKind && value.typeId() == QMetaType::QString) {
		return normalizedFolderName(value.toString());
	}
	const QString displayText = stringField(value, map, "displayText");
	if (!displayText.isEmpty()) {
		return displayText;
	}
	const QString text = stringField(value, map, "text");
	if (!text.isEmpty()) {
		return text;
	}
	const QString title = stringField(value, map, "title");
	const QString subtitle = stringField(value, map, "subtitle");
	if (!title.isEmpty()) {
		return subtitle.isEmpty() ? title : title + QLatin1Char(' ') + subtitle;
	}
	const QString name = stringField(value, map, "name");
	if (!name.isEmpty()) {
		return name;
	}
	if (value.typeId() == QMetaType::QString) {
		return normalizedFolderName(value.toString());
	}
	return {};
}

QString Lr2SelectItemModel::folderLampKeyFor(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	switch (kind) {
	case FolderKind:
		return QStringLiteral("folder:%1").arg(
			value.typeId() == QMetaType::QString ? value.toString() : stringField(value, map, "rawItem"));
	case TableKind: {
		const QString url = stringField(value, map, "url");
		const QString name = stringField(value, map, "name");
		return QStringLiteral("table:%1").arg(!url.isEmpty() ? url : name);
	}
	case LevelKind: {
		const QString name = stringField(value, map, "name");
		return QStringLiteral("level:%1:%2").arg(m_levelFolderParentKey, name);
	}
	default:
		return {};
	}
}

QString Lr2SelectItemModel::displayTextForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind == RankingKind) {
		return stringField(value, map, "title");
	}
	if (kind == ChartKind || kind == EntryKind) {
		const QString title = stringField(value, map, "title");
		const QString subtitle = stringField(value, map, "subtitle");
		const QString prefix = kind == EntryKind && !m_useBeatorajaBarTextTypes
			? QStringLiteral("(missing) ")
			: QString();
		return prefix + (subtitle.isEmpty() ? title : title + QLatin1Char(' ') + subtitle);
	}
	if (kind == LevelKind) {
		return m_levelFolderParentSymbol + stringField(value, map, "name");
	}
	return displayTextFor(value, map, kind);
}

QString Lr2SelectItemModel::titleForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind == RankingKind) {
		return stringField(value, map, "title");
	}
	if (kind == ChartKind || kind == EntryKind) {
		const QString prefix = kind == EntryKind && !m_useBeatorajaBarTextTypes
			? QStringLiteral("(missing) ")
			: QString();
		return prefix + stringField(value, map, "title");
	}
	return displayTextForItem(value, map, kind);
}

int Lr2SelectItemModel::titleTypeForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (!m_useBeatorajaBarTextTypes) {
		return 0;
	}
	auto withFallback = [this](int type, int fallback) {
		const int normalized = std::max(0, type);
		if (normalized <= 1 || variantListContainsInt(m_barTitleTypes, normalized)) {
			return normalized;
		}
		return std::max(0, fallback);
	};
	switch (kind) {
	case RankingKind:
	case ChartKind:
		return withFallback(2, 0);
	case EntryKind:
		return withFallback(8, 0);
	case FolderKind:
		return value.typeId() == QMetaType::QString ? withFallback(4, 0) : 0;
	case TableKind:
	case LevelKind:
		return withFallback(6, 0);
	case CourseKind:
		return withFallback(listField(value, map, "md5s").isEmpty() ? 8 : 7, 0);
	default:
		return 0;
	}
}

int Lr2SelectItemModel::bodyTypeForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (!m_useBeatorajaBarTextTypes) {
		switch (kind) {
		case RankingKind:
		case ChartKind:
			return 0;
		case EntryKind:
			return 5;
		case TableKind:
		case LevelKind:
			return 2;
		case CourseKind:
			return listField(value, map, "md5s").isEmpty() ? 9 : 8;
		default:
			return 1;
		}
	}

	auto withFallback = [this](int type, int fallback) {
		const int normalized = std::max(0, type);
		if (normalized <= 1 || variantListContainsInt(m_barBodyTypes, normalized)) {
			return normalized;
		}
		return std::max(0, fallback);
	};
	switch (kind) {
	case RankingKind:
	case ChartKind:
		return withFallback(0, 0);
	case EntryKind:
		return withFallback(4, 0);
	case TableKind:
	case LevelKind:
		return withFallback(2, 1);
	case CourseKind:
		return withFallback(listField(value, map, "md5s").isEmpty() ? 4 : 3, 0);
	default:
		return withFallback(1, 0);
	}
}

int Lr2SelectItemModel::difficultyForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind != ChartKind) {
		return 0;
	}
	if (m_chartFolderModel) {
		return m_chartFolderModel->difficultyForChart(value);
	}
	return std::max(0, intField(value, map, "difficulty"));
}

int Lr2SelectItemModel::lampForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind != RankingKind) {
		return 0;
	}
	return clearTypeBarLamp(stringField(value, map, "bestClearType"),
							m_useBeatorajaSelectOptions,
							m_barLampVariants);
}

int Lr2SelectItemModel::scoreRankForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind != RankingKind) {
		return 0;
	}
	return rankingEntryRank(doubleField(value, map, "bestPoints"),
							doubleField(value, map, "maxPoints"));
}

int Lr2SelectItemModel::labelMaskForItem(const QVariant& value, const QVariantMap& map, ItemKind kind) const {
	if (kind != ChartKind) {
		return 0;
	}
	int mask = 0;
	if (intField(value, map, "lnCount") + intField(value, map, "bssCount") > 0) {
		mask |= 1 << 0;
	}
	if (boolField(value, map, "isRandom")) {
		mask |= 1 << 1;
	}
	if (intField(value, map, "mineCount") > 0) {
		mask |= 1 << 2;
	}
	return mask;
}

QString Lr2SelectItemModel::keyFor(const QVariant& value, const QVariantMap& map, ItemKind kind, int fallbackIndex) {
	const QString explicitKey = stringField(value, map, "key");
	if (!explicitKey.isEmpty()) {
		return explicitKey;
	}
	const QString fallbackKey = QStringLiteral("index:%1").arg(fallbackIndex);
	switch (kind) {
	case EmptyKind:
		return QStringLiteral("empty");
	case RankingKind:
		return QStringLiteral("ranking:%1:%2")
			.arg(stringField(value, map, "sourceMd5"))
			.arg(intField(value, map, "rankingIndex"));
	case ChartKind: {
		const QString md5 = stringField(value, map, "md5");
		const QString sha256 = stringField(value, map, "sha256");
		const QString path = stringField(value, map, "path");
		return QStringLiteral("chart:%1").arg(
			!md5.isEmpty() ? md5
			: !sha256.isEmpty() ? sha256
			: !path.isEmpty() ? path
			: fallbackKey);
	}
	case EntryKind: {
		const QString md5 = stringField(value, map, "md5");
		const QString path = stringField(value, map, "path");
		return QStringLiteral("entry:%1").arg(
			!md5.isEmpty() ? md5
			: !path.isEmpty() ? path
			: fallbackKey);
	}
	case CourseKind: {
		const QString identifier = stringField(value, map, "identifier");
		const QString name = stringField(value, map, "name");
		return QStringLiteral("course:%1").arg(
			!identifier.isEmpty() ? identifier
			: !name.isEmpty() ? name
			: fallbackKey);
	}
	case TableKind: {
		const QString url = stringField(value, map, "url");
		const QString name = stringField(value, map, "name");
		return QStringLiteral("table:%1").arg(
			!url.isEmpty() ? url
			: !name.isEmpty() ? name
			: fallbackKey);
	}
	case LevelKind: {
		const QString folderKey = stringField(value, map, "folderKey");
		const QString name = stringField(value, map, "name");
		return QStringLiteral("level:%1").arg(
			!folderKey.isEmpty() ? folderKey
			: !name.isEmpty() ? name
			: fallbackKey);
	}
	case FolderKind:
		return QStringLiteral("folder:%1").arg(
			value.typeId() == QMetaType::QString ? value.toString() : stringField(value, map, "rawItem"));
	case UnknownKind:
		return QStringLiteral("item:%1").arg(fallbackKey);
	}
	return QStringLiteral("item:%1").arg(fallbackKey);
}

QVariant Lr2SelectItemModel::roleData(const Item& item, int role) const {
	switch (role) {
	case Qt::DisplayRole:
	case DisplayTextRole: return item.displayText;
	case KeyRole: return item.key;
	case KindRole: return item.kind;
	case TitleTypeRole: return item.titleType;
	case BodyTypeRole: return item.bodyType;
	case TitleRole: return item.title;
	case SubtitleRole: return item.subtitle;
	case ArtistRole: return item.artist;
	case SubartistRole: return item.subartist;
	case GenreRole: return item.genre;
	case TagRole: return item.tag;
	case PathRole: return item.path;
	case Md5Role: return item.md5;
	case Sha256Role: return item.sha256;
	case DirectoryRole: return item.directory;
	case PlayLevelRole: return item.playLevel;
	case DifficultyRole: return item.difficulty;
	case KeymodeRole: return item.keymode;
	case RankRole: return item.rank;
	case DurationRole: return item.duration;
	case MinBpmRole: return item.minBpm;
	case MaxBpmRole: return item.maxBpm;
	case MainBpmRole: return item.mainBpm;
	case LampRole:
		return (item.kind == FolderKind || item.kind == TableKind || item.kind == LevelKind)
			? item.folderLamp
			: item.lamp;
	case ScoreRankRole: return item.scoreRank;
	case ScoreRateRole: return item.scoreRate;
	case LabelMaskRole: return item.labelMask;
	case FolderLampRole: return item.folderLamp;
	case FolderScoreCountsRole: return item.folderScoreCounts;
	case FolderGraphLampsRole: return item.folderGraphLamps;
	case FolderGraphRanksRole: return item.folderGraphRanks;
	case IsChartRole: return item.kind == ChartKind;
	case IsEntryRole: return item.kind == EntryKind;
	case IsCourseRole: return item.kind == CourseKind;
	case IsFolderLikeRole: return item.kind == FolderKind || item.kind == TableKind || item.kind == LevelKind;
	case IsRankingEntryRole: return item.kind == RankingKind;
	case RawItemRole:
	case ActivationObjectRole: return item.raw;
	case ChartDataRole: return item.kind == ChartKind ? item.raw : QVariant();
	default: return {};
	}
}
