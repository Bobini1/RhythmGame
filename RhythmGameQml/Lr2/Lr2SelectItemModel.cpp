#include "Lr2SelectItemModel.h"

#include "Lr2SelectBarCell.h"

#include <QMetaProperty>
#include <QObject>

#include <algorithm>

namespace {

QVariantMap objectToMap(const QVariant& value) {
	if (value.canConvert<QVariantMap>()) {
		return value.toMap();
	}
	return {};
}

QVariant gadgetProperty(const QVariant& value, const char* name) {
	const QMetaObject* metaObject = value.metaType().metaObject();
	if (!metaObject) {
		return {};
	}
	const int propertyIndex = metaObject->indexOfProperty(name);
	return propertyIndex < 0
		? QVariant {}
		: metaObject->property(propertyIndex).readOnGadget(value.constData());
}

bool isObjectOfType(const QVariant& value, const char* className) {
	if (!value.canConvert<QObject*>()) {
		return false;
	}
	const QObject* object = value.value<QObject*>();
	return object && object->inherits(className);
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
		{FolderDistributionRole, "folderDistribution"},
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
}

QVariantMap Lr2SelectItemModel::get(int row) const {
	if (row < 0 || row >= m_items.size()) {
		return {};
	}
	QVariantMap result;
	const auto roles = roleNames();
	const Item& item = m_items.at(row);
	for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
		result.insert(QString::fromUtf8(it.value()), roleData(item, it.key()));
	}
	return result;
}

QVariant Lr2SelectItemModel::rawItemAt(int row) const {
	return row >= 0 && row < m_items.size() ? m_items.at(row).raw : QVariant();
}

int Lr2SelectItemModel::normalizedIndex(int row) const {
	if (m_items.isEmpty()) {
		return 0;
	}
	const int count = m_items.size();
	const int remainder = row % count;
	return remainder < 0 ? remainder + count : remainder;
}

void Lr2SelectItemModel::moveRowTo(int from, int to) {
	if (from < 0 || from >= m_items.size() || to < 0 || to >= m_items.size() || from == to) {
		return;
	}
	const int destination = to > from ? to + 1 : to;
	if (!beginMoveRows({}, from, from, {}, destination)) {
		return;
	}
	m_items.move(from, to);
	endMoveRows();
}

void Lr2SelectItemModel::updateItems(const QVariantList& items) {
	if (items.size() != m_items.size()) {
		setItems(items);
		return;
	}
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
		FolderDistributionRole,
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
		FolderDistributionRole,
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
	item.map = objectToMap(value);
	const QVariant rawItem = item.map.value(QStringLiteral("rawItem"));
	item.raw = rawItem.isValid() && !rawItem.isNull() ? rawItem : value;
	item.kind = kindFor(value, item.map);
	item.folderKey = stringField(value, item.map, "folderKey");
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
	item.rank = doubleField(value, item.map, "rank", 75.0);
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
	} else if (distribution.canConvert<QObject*>()) {
		if (QObject* object = distribution.value<QObject*>()) {
			map.insert(QStringLiteral("lamps"), object->property("lamps"));
			map.insert(QStringLiteral("ranks"), object->property("ranks"));
		}
	} else {
		const QVariant lamps = gadgetProperty(distribution, "lamps");
		const QVariant ranks = gadgetProperty(distribution, "ranks");
		if (lamps.isValid() || ranks.isValid()) {
			map.insert(QStringLiteral("lamps"), lamps);
			map.insert(QStringLiteral("ranks"), ranks);
		}
	}

	result.graphLamps = paddedNumberList(map.value(QStringLiteral("lamps")), 11);
	result.graphRanks = paddedNumberList(map.value(QStringLiteral("ranks")), 28);
	return result;
}

QVariant Lr2SelectItemModel::fieldValue(const QVariant& value, const QVariantMap& map, const char* name) {
	const QString key = QString::fromLatin1(name);
	const auto it = map.constFind(key);
	if (it != map.constEnd()) {
		return *it;
	}
	if (value.canConvert<QObject*>()) {
		if (QObject* object = value.value<QObject*>()) {
			return object->property(name);
		}
	}
	return gadgetProperty(value, name);
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
	if (value.typeId() == QMetaType::QString) {
		return FolderKind;
	}
	if (isObjectOfType(value, "gameplay_logic::ChartData") || isObjectOfType(value, "ChartData")) {
		return ChartKind;
	}
	if (isObjectOfType(value, "entry")) {
		return EntryKind;
	}
	if (isObjectOfType(value, "course")) {
		return CourseKind;
	}
	if (isObjectOfType(value, "table")) {
		return TableKind;
	}
	if (isObjectOfType(value, "level")) {
		return LevelKind;
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
	switch (kind) {
	case EmptyKind:
		return QStringLiteral("empty:%1").arg(fallbackIndex);
	case RankingKind:
		return QStringLiteral("ranking:%1:%2")
			.arg(stringField(value, map, "sourceMd5"))
			.arg(intField(value, map, "rankingIndex"));
	case ChartKind:
		return QStringLiteral("chart:%1").arg(
			!stringField(value, map, "md5").isEmpty() ? stringField(value, map, "md5")
			: !stringField(value, map, "path").isEmpty() ? stringField(value, map, "path")
			: QString::number(fallbackIndex));
	case EntryKind:
		return QStringLiteral("entry:%1").arg(
			!stringField(value, map, "md5").isEmpty() ? stringField(value, map, "md5")
			: !stringField(value, map, "path").isEmpty() ? stringField(value, map, "path")
			: QString::number(fallbackIndex));
	case CourseKind:
		return QStringLiteral("course:%1").arg(
			!stringField(value, map, "identifier").isEmpty() ? stringField(value, map, "identifier")
			: !stringField(value, map, "name").isEmpty() ? stringField(value, map, "name")
			: QString::number(fallbackIndex));
	case TableKind:
		return QStringLiteral("table:%1").arg(
			!stringField(value, map, "url").isEmpty() ? stringField(value, map, "url")
			: !stringField(value, map, "name").isEmpty() ? stringField(value, map, "name")
			: QString::number(fallbackIndex));
	case LevelKind:
		return QStringLiteral("level:%1").arg(
			!stringField(value, map, "name").isEmpty() ? stringField(value, map, "name")
			: QString::number(fallbackIndex));
	case FolderKind:
		return QStringLiteral("folder:%1").arg(value.toString());
	case UnknownKind:
		return QStringLiteral("item:%1").arg(fallbackIndex);
	}
	return QStringLiteral("item:%1").arg(fallbackIndex);
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
	case FolderDistributionRole:
		return QVariantMap {
			{QStringLiteral("lamps"), item.folderGraphLamps},
			{QStringLiteral("ranks"), item.folderGraphRanks},
		};
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
