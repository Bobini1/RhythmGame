#include "Lr2SelectItemModel.h"

#include "Lr2SelectBarCell.h"
#include "gameplay_logic/ChartData.h"
#include "resource_managers/Tables.h"

#include <QMetaProperty>
#include <QMetaType>
#include <QObject>

#include <algorithm>

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
	QVariantList source = value.toList();
	QVariantList result;
	result.reserve(size);
	for (int i = 0; i < size; ++i) {
		result.append(i < source.size() ? std::max(0.0, source.at(i).toDouble()) : 0);
	}
	return result;
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

QVariant Lr2SelectItemModel::rawItemAt(int row) const {
	return row >= 0 && row < m_items.size() ? m_items.at(row).raw : QVariant();
}

QString Lr2SelectItemModel::keyAt(int row) const {
	return row >= 0 && row < m_items.size() ? m_items.at(row).key : QString();
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
	item.folderKey = stringField(value, item.map, "folderKey");
	item.scoreIdentifier = scoreIdentifierFor(value, item.map, item.kind);
	item.titleType = intField(value, item.map, "titleType");
	item.bodyType = intField(value, item.map, "bodyType");
	item.title = stringField(value, item.map, "title");
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
	item.difficulty = intField(value, item.map, "difficulty");
	item.keymode = intField(value, item.map, "keymode");
	item.rank = intField(value, item.map, "rank");
	item.duration = intField(value, item.map, "duration");
	item.minBpm = doubleField(value, item.map, "minBpm");
	item.maxBpm = doubleField(value, item.map, "maxBpm");
	item.mainBpm = doubleField(value, item.map, "mainBpm");
	item.lamp = intField(value, item.map, "lamp");
	item.scoreRank = intField(value, item.map, "scoreRank");
	item.scoreRate = doubleField(value, item.map, "scoreRate");
	item.labelMask = intField(value, item.map, "labelMask");
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
	item.displayText = displayTextFor(value, item.map, item.kind);
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
