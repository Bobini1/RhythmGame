#include "resource_managers/ChartFolderModel.h"

#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/Judgement.h"
#include "resource_managers/Tables.h"

#include <QHash>
#include <QMetaProperty>
#include <QMetaType>
#include <QObject>
#include <QVector>

#include <algorithm>

namespace {

enum ItemKind
{
    EmptyKind = 0,
    ChartKind,
    EntryKind,
    CourseKind,
    TableKind,
    LevelKind,
    FolderKind,
    UnknownKind
};

struct SortEntry
{
    QVariant value;
    QVariantMap map;
    ItemKind kind = EmptyKind;
    int sourceIndex = 0;
    QString displayName;
    QString title;
    QString subtitle;
    QString artist;
    QString subartist;
    QString path;
    QString directory;
    QString rawString;
    QString name;
    QString md5;
    QString identifier;
    int playLevel = 0;
    int difficulty = 0;
    int keymode = 0;
    double minBpm = 0.0;
    double maxBpm = 0.0;
    double mainBpm = 0.0;
    double length = 0.0;
    double totalNotes = 0.0;
    int noteCount = 0;
    bool hasScore = false;
    int clearPriority = 0;
    double scoreRate = 0.0;
    int minBadPoor = 0;
};

struct FolderIndexEntry
{
    QVariant value;
    QString folderKey;
    QString keymodeKey;
    QString path;
    QString title;
    QString subtitle;
    int keymode = 0;
    int difficulty = 0;
    int playLevel = 0;
    int noteCount = 0;
    bool valid = false;
};

gameplay_logic::ChartData*
chartDataObject(const QVariant& value)
{
    if (auto* chartData = value.value<gameplay_logic::ChartData*>()) {
        return chartData;
    }
    if (auto* object = value.value<QObject*>()) {
        return qobject_cast<gameplay_logic::ChartData*>(object);
    }
    return nullptr;
}

QVariantMap
mapFromVariant(const QVariant& value)
{
    if (value.canConvert<QVariantMap>()) {
        return value.toMap();
    }
    return {};
}

QVariantList
listFromVariant(const QVariant& value)
{
    if (value.canConvert<QVariantList>()) {
        return value.toList();
    }
    return {};
}

QVariant
mapFieldValue(const QVariantMap& map, const QString& key)
{
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

QObject*
objectFromVariant(const QVariant& value)
{
    if (auto* object = value.value<QObject*>()) {
        return object;
    }
    return chartDataObject(value);
}

QVariant
objectFieldValue(const QVariant& value, const QString& key)
{
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
    return field.isValid() ? field : QVariant{};
}

QVariant
gadgetFieldValue(const QVariant& value, const QString& key)
{
    if (!value.isValid() || value.isNull() || objectFromVariant(value)) {
        return {};
    }

    const QMetaObject* metaObject = value.metaType().metaObject();
    if (!metaObject) {
        return {};
    }

    QByteArray propertyNameBytes = key.toLatin1();
    int propertyIndex =
      metaObject->indexOfProperty(propertyNameBytes.constData());
    if (propertyIndex < 0 && key == QStringLiteral("playLevel")) {
        propertyNameBytes = QByteArrayLiteral("level");
        propertyIndex =
          metaObject->indexOfProperty(propertyNameBytes.constData());
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
    return field.isValid() ? field : QVariant{};
}

QVariant
variantFieldValue(const QVariant& value, const QString& key)
{
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

QVariant
fieldValue(const QVariant& value, const QVariantMap& map, const QString& key)
{
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

QString
stringField(const QVariant& value, const QVariantMap& map, const QString& key)
{
    const QVariant field = fieldValue(value, map, key);
    return field.isValid() && !field.isNull() ? field.toString() : QString();
}

int
intField(const QVariant& value,
         const QVariantMap& map,
         const QString& key,
         int fallback = 0)
{
    const QVariant field = fieldValue(value, map, key);
    bool ok = false;
    const int result = field.toInt(&ok);
    return ok ? result : fallback;
}

double
doubleField(const QVariant& value,
            const QVariantMap& map,
            const QString& key,
            double fallback = 0.0)
{
    const QVariant field = fieldValue(value, map, key);
    bool ok = false;
    const double result = field.toDouble(&ok);
    return ok ? result : fallback;
}

template<typename T>
bool
variantHasType(const QVariant& value)
{
    const QMetaType expectedType = QMetaType::fromType<T>();
    return value.isValid() && !value.isNull() &&
           (value.metaType().id() == expectedType.id() ||
            value.metaType().metaObject() == expectedType.metaObject());
}

ItemKind
resourceItemKind(const QVariant& value)
{
    if (variantHasType<resource_managers::Entry>(value))
        return EntryKind;
    if (variantHasType<resource_managers::Course>(value))
        return CourseKind;
    if (variantHasType<resource_managers::Table>(value))
        return TableKind;
    if (variantHasType<resource_managers::Level>(value))
        return LevelKind;
    return UnknownKind;
}

ItemKind
kindFromFolderKey(const QString& key)
{
    if (key.startsWith(QStringLiteral("table:")))
        return TableKind;
    if (key.startsWith(QStringLiteral("level:")))
        return LevelKind;
    if (key.startsWith(QStringLiteral("folder:")))
        return FolderKind;
    return UnknownKind;
}

ItemKind
kindFor(const QVariant& value, const QVariantMap& map)
{
    if (!value.isValid() || value.isNull()) {
        return EmptyKind;
    }

    const QVariant rawItem = map.value(QStringLiteral("rawItem"));
    if (map.contains(QStringLiteral("rawItem")) &&
        (!rawItem.isValid() || rawItem.isNull())) {
        return EmptyKind;
    }
    if (value.typeId() == QMetaType::QString ||
        rawItem.typeId() == QMetaType::QString) {
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
    const ItemKind folderKeyKind =
      kindFromFolderKey(stringField(value, map, QStringLiteral("folderKey")));
    if (folderKeyKind != UnknownKind) {
        return folderKeyKind;
    }

    const QString type =
      stringField(value, map, QStringLiteral("type")).toLower();
    if (type == QStringLiteral("chart"))
        return ChartKind;
    if (type == QStringLiteral("entry"))
        return EntryKind;
    if (type == QStringLiteral("course"))
        return CourseKind;
    if (type == QStringLiteral("table"))
        return TableKind;
    if (type == QStringLiteral("level"))
        return LevelKind;
    if (type == QStringLiteral("folder"))
        return FolderKind;
    return UnknownKind;
}

QString
normalizedFolderName(QString path)
{
    path.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    return slash >= 0 ? path.mid(slash + 1) : path;
}

QString
normalizedClearType(const QString& clear)
{
    const QString value =
      clear.isEmpty() ? QStringLiteral("NOPLAY") : clear.toUpper();
    if (value == QStringLiteral("ASSIST") ||
        value == QStringLiteral("ASSISTEASY") ||
        value == QStringLiteral("ASSIST_EASY")) {
        return QStringLiteral("AEASY");
    }
    if (value == QStringLiteral("LIGHT_ASSIST") ||
        value == QStringLiteral("LIGHTASSISTEASY") ||
        value == QStringLiteral("LIGHT_ASSIST_EASY")) {
        return QStringLiteral("LIGHTASSIST");
    }
    if (value == QStringLiteral("CLEAR") || value == QStringLiteral("DAN")) {
        return QStringLiteral("NORMAL");
    }
    if (value == QStringLiteral("EX_HARD")) {
        return QStringLiteral("EXHARD");
    }
    if (value == QStringLiteral("EXDAN") ||
        value == QStringLiteral("HARD_DAN") ||
        value == QStringLiteral("HARD DAN")) {
        return QStringLiteral("HARD");
    }
    if (value == QStringLiteral("EXHARDDAN") ||
        value == QStringLiteral("EXHARD_DAN") ||
        value == QStringLiteral("EX_HARD_DAN")) {
        return QStringLiteral("EXHARD");
    }
    if (value == QStringLiteral("FULLCOMBO") ||
        value == QStringLiteral("FULL_COMBO") ||
        value == QStringLiteral("FULL COMBO")) {
        return QStringLiteral("FC");
    }
    if (value == QStringLiteral("NO_PLAY") ||
        value == QStringLiteral("NO PLAY")) {
        return QStringLiteral("NOPLAY");
    }
    return value;
}

int
clearTypePriority(const QString& clear)
{
    const QString value = normalizedClearType(clear);
    if (value == QStringLiteral("FAILED"))
        return 1;
    if (value == QStringLiteral("AEASY"))
        return 2;
    if (value == QStringLiteral("LIGHTASSIST"))
        return 3;
    if (value == QStringLiteral("EASY"))
        return 4;
    if (value == QStringLiteral("NORMAL"))
        return 5;
    if (value == QStringLiteral("HARD"))
        return 6;
    if (value == QStringLiteral("EXHARD"))
        return 7;
    if (value == QStringLiteral("FC"))
        return 8;
    if (value == QStringLiteral("PERFECT"))
        return 9;
    if (value == QStringLiteral("MAX"))
        return 10;
    return 0;
}

gameplay_logic::BmsScore*
scoreObject(const QVariant& value)
{
    if (auto* score = value.value<gameplay_logic::BmsScore*>()) {
        return score;
    }
    if (auto* object = value.value<QObject*>()) {
        return qobject_cast<gameplay_logic::BmsScore*>(object);
    }
    return nullptr;
}

int
judgementCount(const QList<int>& counts, gameplay_logic::Judgement judgement)
{
    const int index = static_cast<int>(judgement);
    return index >= 0 && index < counts.size() ? counts.at(index) : 0;
}

QVariantList
scoreListForIdentifier(const QVariantMap& scores, const QString& identifier)
{
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

QString
scoreIdentifierFor(const SortEntry& entry)
{
    if (entry.kind == ChartKind || entry.kind == EntryKind) {
        return entry.md5;
    }
    if (entry.kind == CourseKind) {
        return entry.identifier;
    }
    return {};
}

void
fillScoreFields(SortEntry& entry, const QVariantMap& scores)
{
    const QVariantList scoreList =
      scoreListForIdentifier(scores, scoreIdentifierFor(entry));
    if (scoreList.isEmpty()) {
        return;
    }

    double bestRate = -1.0;
    bool bestHasMaxPoints = false;
    int bestClearPriority = 0;
    int minBadPoor = -1;
    for (const QVariant& scoreValue : scoreList) {
        const auto* score = scoreObject(scoreValue);
        const auto* result = score ? score->getResult() : nullptr;
        if (!result) {
            continue;
        }

        entry.hasScore = true;
        bestClearPriority = std::max(bestClearPriority,
                                     clearTypePriority(result->getClearType()));

        const QList<int> counts = result->getJudgementCounts();
        const int badPoor =
          judgementCount(counts, gameplay_logic::Judgement::Bad) +
          judgementCount(counts, gameplay_logic::Judgement::Poor) +
          judgementCount(counts, gameplay_logic::Judgement::EmptyPoor);
        minBadPoor = minBadPoor < 0 ? badPoor : std::min(minBadPoor, badPoor);

        const double maxPoints = result->getMaxPoints();
        const bool hasMaxPoints = maxPoints > 0.0;
        const double rate =
          hasMaxPoints ? result->getPoints() / maxPoints : 0.0;
        if (rate > bestRate ||
            (rate == bestRate && hasMaxPoints && !bestHasMaxPoints)) {
            bestRate = rate;
            bestHasMaxPoints = hasMaxPoints;
        }
    }

    if (!entry.hasScore) {
        return;
    }
    entry.clearPriority = bestClearPriority;
    entry.scoreRate = std::max(0.0, bestRate);
    entry.minBadPoor = std::max(0, minBadPoor);
}

QString
displayNameFor(const SortEntry& entry)
{
    if (entry.kind == ChartKind || entry.kind == EntryKind) {
        return entry.subtitle.isEmpty()
                 ? entry.title
                 : entry.title + QLatin1Char(' ') + entry.subtitle;
    }
    if (entry.kind == CourseKind || entry.kind == TableKind ||
        entry.kind == LevelKind) {
        return entry.name;
    }
    if (entry.kind == FolderKind) {
        return normalizedFolderName(entry.rawString);
    }
    return {};
}

SortEntry
makeEntry(const QVariant& value,
          int sourceIndex,
          const QHash<QString, int>& chartDifficultyByPath,
          const QVariantMap& scores)
{
    SortEntry entry;
    entry.value = value;
    entry.map = mapFromVariant(value);
    const QVariant rawItem = entry.map.value(QStringLiteral("rawItem"));
    const QVariant raw =
      rawItem.isValid() && !rawItem.isNull() ? rawItem : value;
    entry.kind = kindFor(value, entry.map);
    entry.sourceIndex = sourceIndex;
    entry.rawString = raw.toString();

    if (entry.kind == ChartKind && entry.map.isEmpty()) {
        if (auto* chart = chartDataObject(raw)) {
            entry.title = chart->getTitle();
            entry.subtitle = chart->getSubtitle();
            entry.artist = chart->getArtist();
            entry.subartist = chart->getSubartist();
            entry.path = chart->getPath();
            entry.directory = chart->getChartDirectory();
            if (entry.directory.isEmpty()) {
                entry.directory = chart->getDirectory();
            }
            entry.md5 = chart->getMd5();
            entry.playLevel = chart->getPlayLevel();
            entry.difficulty = chart->getDifficulty();
            entry.keymode = static_cast<int>(chart->getKeymode());
            entry.minBpm = chart->getMinBpm();
            entry.maxBpm = chart->getMaxBpm();
            entry.mainBpm = chart->getMainBpm();
            entry.length = static_cast<double>(chart->getLength());
            entry.totalNotes = chart->getTotal();
            entry.noteCount = chart->getNormalNoteCount() +
                              chart->getScratchCount() + chart->getLnCount() +
                              chart->getBssCount() + chart->getMineCount();
            if (!entry.path.isEmpty()) {
                const auto overrideDifficulty =
                  chartDifficultyByPath.constFind(entry.path);
                if (overrideDifficulty != chartDifficultyByPath.constEnd()) {
                    entry.difficulty = overrideDifficulty.value();
                }
            }
            entry.displayName = displayNameFor(entry);
            fillScoreFields(entry, scores);
            return entry;
        }
    }

    entry.title = stringField(value, entry.map, QStringLiteral("title"));
    entry.subtitle = stringField(value, entry.map, QStringLiteral("subtitle"));
    entry.artist = stringField(value, entry.map, QStringLiteral("artist"));
    entry.subartist =
      stringField(value, entry.map, QStringLiteral("subartist"));
    entry.path = stringField(value, entry.map, QStringLiteral("path"));
    entry.directory =
      stringField(value, entry.map, QStringLiteral("chartDirectory"));
    if (entry.directory.isEmpty()) {
        entry.directory =
          stringField(value, entry.map, QStringLiteral("directory"));
    }
    entry.name = stringField(value, entry.map, QStringLiteral("name"));
    entry.md5 = stringField(value, entry.map, QStringLiteral("md5"));
    entry.identifier =
      stringField(value, entry.map, QStringLiteral("identifier"));
    entry.playLevel = intField(value, entry.map, QStringLiteral("playLevel"));
    entry.difficulty = intField(value, entry.map, QStringLiteral("difficulty"));
    entry.keymode = intField(value, entry.map, QStringLiteral("keymode"));
    entry.minBpm = doubleField(value, entry.map, QStringLiteral("minBpm"));
    entry.maxBpm = doubleField(value, entry.map, QStringLiteral("maxBpm"));
    entry.mainBpm = doubleField(value, entry.map, QStringLiteral("mainBpm"));
    entry.length = doubleField(value, entry.map, QStringLiteral("length"));
    if (entry.length <= 0.0) {
        entry.length =
          doubleField(value, entry.map, QStringLiteral("duration"));
    }
    entry.totalNotes = doubleField(value, entry.map, QStringLiteral("total"));
    entry.noteCount =
      intField(value, entry.map, QStringLiteral("normalNoteCount")) +
      intField(value, entry.map, QStringLiteral("scratchCount")) +
      intField(value, entry.map, QStringLiteral("lnCount")) +
      intField(value, entry.map, QStringLiteral("bssCount")) +
      intField(value, entry.map, QStringLiteral("mineCount"));
    if (entry.kind == ChartKind && !entry.path.isEmpty()) {
        const auto overrideDifficulty =
          chartDifficultyByPath.constFind(entry.path);
        if (overrideDifficulty != chartDifficultyByPath.constEnd()) {
            entry.difficulty = overrideDifficulty.value();
        }
    }
    entry.displayName = displayNameFor(entry);
    fillScoreFields(entry, scores);
    return entry;
}

bool
isSortableChartLike(const SortEntry& entry)
{
    return entry.kind == ChartKind || entry.kind == EntryKind;
}

int
compareStrings(const QString& a, const QString& b)
{
    return QString::localeAwareCompare(a, b);
}

int
compareByNameOnly(const SortEntry& a, const SortEntry& b)
{
    return compareStrings(a.displayName, b.displayName);
}

int
compareNumberWithMissing(double aValue,
                         bool aMissing,
                         double bValue,
                         bool bMissing,
                         bool missingLast = true)
{
    if (aMissing && bMissing)
        return 0;
    if (aMissing)
        return missingLast ? 1 : -1;
    if (bMissing)
        return missingLast ? -1 : 1;
    if (aValue < bValue)
        return -1;
    if (aValue > bValue)
        return 1;
    return 0;
}

int
compareByLevel(const SortEntry& a, const SortEntry& b)
{
    const int diff = compareNumberWithMissing(
      a.playLevel, a.playLevel <= 0, b.playLevel, b.playLevel <= 0);
    return diff != 0 ? diff : compareByNameOnly(a, b);
}

int
compareByTitle(const SortEntry& a, const SortEntry& b)
{
    const int titleDiff = compareByNameOnly(a, b);
    return titleDiff != 0 ? titleDiff : compareByLevel(a, b);
}

int
compareByArtist(const SortEntry& a, const SortEntry& b)
{
    int artistDiff = compareStrings(a.artist, b.artist);
    if (artistDiff != 0) {
        return artistDiff;
    }
    artistDiff = compareStrings(a.subartist, b.subartist);
    return artistDiff != 0 ? artistDiff : compareByTitle(a, b);
}

double
bpmForSort(const SortEntry& entry)
{
    if (entry.mainBpm > 0.0)
        return entry.mainBpm;
    if (entry.maxBpm > 0.0)
        return entry.maxBpm;
    if (entry.minBpm > 0.0)
        return entry.minBpm;
    return 0.0;
}

int
compareByBpm(const SortEntry& a, const SortEntry& b)
{
    const double aBpm = bpmForSort(a);
    const double bBpm = bpmForSort(b);
    const int diff =
      compareNumberWithMissing(aBpm, aBpm <= 0.0, bBpm, bBpm <= 0.0);
    return diff != 0 ? diff : compareByTitle(a, b);
}

int
compareByLength(const SortEntry& a, const SortEntry& b)
{
    const int diff = compareNumberWithMissing(
      a.length, a.length <= 0.0, b.length, b.length <= 0.0);
    return diff != 0 ? diff : compareByTitle(a, b);
}

int
compareByTotalNotes(const SortEntry& a, const SortEntry& b)
{
    const int diff = compareNumberWithMissing(
      a.totalNotes, a.totalNotes <= 0.0, b.totalNotes, b.totalNotes <= 0.0);
    return diff != 0 ? diff : compareByTitle(a, b);
}

int
compareByClear(const SortEntry& a, const SortEntry& b, bool unscoredItemsLast)
{
    if (a.hasScore != b.hasScore) {
        return a.hasScore ? (unscoredItemsLast ? -1 : 1)
                          : (unscoredItemsLast ? 1 : -1);
    }
    if (a.clearPriority != b.clearPriority) {
        return a.clearPriority < b.clearPriority ? -1 : 1;
    }
    return compareByLevel(a, b);
}

int
compareByScore(const SortEntry& a, const SortEntry& b, bool unscoredItemsLast)
{
    const int diff = compareNumberWithMissing(
      a.scoreRate, !a.hasScore, b.scoreRate, !b.hasScore, unscoredItemsLast);
    return diff != 0 ? diff : compareByLevel(a, b);
}

int
compareByMissCount(const SortEntry& a,
                   const SortEntry& b,
                   bool unscoredItemsLast)
{
    const int diff = compareNumberWithMissing(
      a.minBadPoor, !a.hasScore, b.minBadPoor, !b.hasScore, unscoredItemsLast);
    return diff != 0 ? diff : compareByLevel(a, b);
}

int
compareEntries(const SortEntry& a,
               const SortEntry& b,
               int sortMode,
               bool unscoredItemsLast)
{
    switch (sortMode) {
        case 1:
            return compareByTitle(a, b);
        case 2:
            return compareByArtist(a, b);
        case 3:
            return compareByBpm(a, b);
        case 4:
            return compareByLength(a, b);
        case 5:
            return compareByLevel(a, b);
        case 6:
            return compareByClear(a, b, unscoredItemsLast);
        case 7:
            return compareByScore(a, b, unscoredItemsLast);
        case 8:
            return compareByMissCount(a, b, unscoredItemsLast);
        case 9:
            return compareByTotalNotes(a, b);
        default:
            return 0;
    }
}

bool
keyFilterMatches(const SortEntry& entry, int keymodeFilter)
{
    if (entry.kind != ChartKind || keymodeFilter == 0) {
        return true;
    }
    switch (keymodeFilter) {
        case 1:
            return entry.keymode == 5 || entry.keymode == 7;
        case 2:
            return entry.keymode == 10 || entry.keymode == 14;
        case 3:
            return entry.keymode == 5;
        case 4:
            return entry.keymode == 7;
        case 5:
            return entry.keymode == 10;
        case 6:
            return entry.keymode == 14;
        default:
            return true;
    }
}

QString
difficultyGroupKey(const SortEntry& entry)
{
    return entry.directory + QChar(0x1f) + QString::number(entry.keymode);
}

QVector<SortEntry>
difficultyFilteredCharts(const QVector<SortEntry>& input, int difficultyFilter)
{
    if (difficultyFilter == 0) {
        return input;
    }

    QVector<SortEntry> result;
    result.reserve(input.size());
    QHash<QString, QVector<SortEntry>> groups;
    QVector<QString> groupOrder;
    for (const SortEntry& entry : input) {
        if (entry.kind != ChartKind) {
            result.append(entry);
            continue;
        }

        const QString key = difficultyGroupKey(entry);
        if (!groups.contains(key)) {
            groupOrder.append(key);
        }
        groups[key].append(entry);
    }

    for (const QString& key : groupOrder) {
        const QVector<SortEntry>& group = groups[key];
        QVector<SortEntry> exact;
        QVector<SortEntry> lower;
        QVector<SortEntry> higher;
        QVector<SortEntry> unknown;
        int lowerDifficulty = 0;
        int higherDifficulty = 0;
        for (const SortEntry& chart : group) {
            const int difficulty = std::max(0, chart.difficulty);
            if (difficulty == difficultyFilter) {
                exact.append(chart);
            } else if (difficulty > 0 && difficulty < difficultyFilter) {
                if (difficulty > lowerDifficulty) {
                    lower = { chart };
                    lowerDifficulty = difficulty;
                } else if (difficulty == lowerDifficulty) {
                    lower.append(chart);
                }
            } else if (difficulty > difficultyFilter) {
                if (higherDifficulty == 0 || difficulty < higherDifficulty) {
                    higher = { chart };
                    higherDifficulty = difficulty;
                } else if (difficulty == higherDifficulty) {
                    higher.append(chart);
                }
            } else {
                unknown.append(chart);
            }
        }
        if (!exact.isEmpty()) {
            result.append(exact);
        } else if (!lower.isEmpty()) {
            result.append(lower);
        } else if (!higher.isEmpty()) {
            result.append(higher);
        } else {
            result.append(unknown);
        }
    }
    return result;
}

bool
sameEntry(const SortEntry& a, const SortEntry& b)
{
    if (a.kind == ChartKind && b.kind == ChartKind) {
        return a.path == b.path;
    }
    if (a.kind == FolderKind && b.kind == FolderKind) {
        return a.rawString == b.rawString;
    }
    if (a.kind == LevelKind && b.kind == LevelKind) {
        return a.name == b.name;
    }
    if (a.kind == TableKind && b.kind == TableKind) {
        const QString aUrl = stringField(a.value, a.map, QStringLiteral("url"));
        const QString bUrl = stringField(b.value, b.map, QStringLiteral("url"));
        if (!aUrl.isEmpty() || !bUrl.isEmpty()) {
            return aUrl == bUrl;
        }
        return a.name == b.name;
    }
    if (a.kind == CourseKind && b.kind == CourseKind) {
        if (!a.identifier.isEmpty() || !b.identifier.isEmpty()) {
            return a.identifier == b.identifier;
        }
        return a.name == b.name;
    }
    if (a.kind == EntryKind && b.kind == EntryKind) {
        return !a.md5.isEmpty() && a.md5 == b.md5;
    }
    return false;
}

bool
isWordBoundary(const QString& text, int index)
{
    if (index < 0 || index >= text.size()) {
        return true;
    }
    return !text.at(index).isLetterOrNumber();
}

bool
containsWord(const QString& text, const QString& word)
{
    int index = 0;
    while ((index = text.indexOf(word, index)) >= 0) {
        if (isWordBoundary(text, index - 1) &&
            isWordBoundary(text, index + word.size())) {
            return true;
        }
        index += word.size();
    }
    return false;
}

int
difficultyHint(const FolderIndexEntry& chart)
{
    const QString text =
      chart.title.isEmpty() && chart.subtitle.isEmpty()
        ? QString()
        : (chart.title + QLatin1Char(' ') + chart.subtitle).toLower();
    if (containsWord(text, QStringLiteral("insane")))
        return 5;
    if (containsWord(text, QStringLiteral("another")))
        return 4;
    if (containsWord(text, QStringLiteral("hyper")))
        return 3;
    if (containsWord(text, QStringLiteral("normal")))
        return 2;
    if (containsWord(text, QStringLiteral("beginner")) ||
        containsWord(text, QStringLiteral("bgn"))) {
        return 1;
    }
    if (chart.playLevel <= 1)
        return 1;
    return 0;
}

FolderIndexEntry
makeFolderIndexEntry(const QVariant& value)
{
    FolderIndexEntry entry;
    entry.value = value;

    const QVariantMap map = mapFromVariant(value);
    const QVariant rawItem = map.value(QStringLiteral("rawItem"));
    const QVariant raw =
      rawItem.isValid() && !rawItem.isNull() ? rawItem : value;
    if (auto* chart = chartDataObject(raw)) {
        entry.valid = true;
        entry.folderKey = chart->getChartDirectory();
        if (entry.folderKey.isEmpty()) {
            entry.folderKey = chart->getDirectory();
        }
        entry.path = chart->getPath();
        entry.title = chart->getTitle();
        entry.subtitle = chart->getSubtitle();
        entry.keymode = static_cast<int>(chart->getKeymode());
        entry.keymodeKey = QString::number(entry.keymode);
        entry.difficulty = chart->getDifficulty();
        entry.playLevel = chart->getPlayLevel();
        entry.noteCount = chart->getNormalNoteCount() +
                          chart->getScratchCount() + chart->getLnCount() +
                          chart->getBssCount() + chart->getMineCount();
        return entry;
    }

    if (kindFor(value, map) != ChartKind) {
        return entry;
    }

    entry.valid = true;
    entry.folderKey = stringField(value, map, QStringLiteral("chartDirectory"));
    if (entry.folderKey.isEmpty()) {
        entry.folderKey = stringField(value, map, QStringLiteral("directory"));
    }
    entry.path = stringField(value, map, QStringLiteral("path"));
    entry.title = stringField(value, map, QStringLiteral("title"));
    entry.subtitle = stringField(value, map, QStringLiteral("subtitle"));
    entry.keymode = intField(value, map, QStringLiteral("keymode"));
    entry.keymodeKey = QString::number(entry.keymode);
    entry.difficulty = intField(value, map, QStringLiteral("difficulty"));
    entry.playLevel = intField(value, map, QStringLiteral("playLevel"));
    entry.noteCount = intField(value, map, QStringLiteral("normalNoteCount")) +
                      intField(value, map, QStringLiteral("scratchCount")) +
                      intField(value, map, QStringLiteral("lnCount")) +
                      intField(value, map, QStringLiteral("bssCount")) +
                      intField(value, map, QStringLiteral("mineCount"));
    return entry;
}

QHash<QString, int>
inferGroupDifficulties(QVector<FolderIndexEntry> group)
{
    QHash<QString, int> result;
    bool hasInvalid = false;
    bool allBeginner = group.length() > 1;
    bool hasNonTrivialLevel = false;
    for (const FolderIndexEntry& chart : group) {
        const int raw = chart.difficulty;
        hasInvalid = hasInvalid || raw < 1 || raw > 5;
        allBeginner = allBeginner && raw == 1;
        hasNonTrivialLevel = hasNonTrivialLevel || chart.playLevel > 1;
    }

    if (!hasInvalid && !(allBeginner && hasNonTrivialLevel)) {
        for (const FolderIndexEntry& chart : group) {
            if (!chart.path.isEmpty()) {
                result.insert(chart.path, std::max(0, chart.difficulty));
            }
        }
        return result;
    }

    std::stable_sort(group.begin(),
                     group.end(),
                     [](const FolderIndexEntry& a, const FolderIndexEntry& b) {
                         if (a.noteCount != b.noteCount) {
                             return a.noteCount < b.noteCount;
                         }
                         return a.path < b.path;
                     });

    int inferred = 1;
    for (const FolderIndexEntry& chart : group) {
        const int raw = chart.difficulty;
        const int hint = allBeginner ? difficultyHint(chart) : 0;
        if (raw >= 1 && raw <= 5 && !allBeginner) {
            inferred = raw;
        } else if (hint > 0) {
            inferred = hint;
        } else {
            inferred += 1;
            if (inferred == 5) {
                inferred = 4;
            } else if (inferred < 1) {
                inferred = 2;
            } else if (inferred > 5) {
                inferred = 5;
            }
        }
        if (!chart.path.isEmpty()) {
            result.insert(chart.path, inferred);
        }
    }
    return result;
}

} // namespace

resource_managers::ChartFolderModel::ChartFolderModel(QObject* parent)
  : QObject(parent)
{
}

int
resource_managers::ChartFolderModel::sortMode() const
{
    return m_sortMode;
}

void
resource_managers::ChartFolderModel::setSortMode(int value)
{
    if (m_sortMode == value) {
        return;
    }
    m_sortMode = value;
    emit sortModeChanged();
}

int
resource_managers::ChartFolderModel::keymodeFilter() const
{
    return m_keymodeFilter;
}

void
resource_managers::ChartFolderModel::setKeymodeFilter(int value)
{
    if (m_keymodeFilter == value) {
        return;
    }
    m_keymodeFilter = value;
    emit keymodeFilterChanged();
}

int
resource_managers::ChartFolderModel::difficultyFilter() const
{
    return m_difficultyFilter;
}

void
resource_managers::ChartFolderModel::setDifficultyFilter(int value)
{
    if (m_difficultyFilter == value) {
        return;
    }
    m_difficultyFilter = value;
    emit difficultyFilterChanged();
}

bool
resource_managers::ChartFolderModel::unscoredItemsLast() const
{
    return m_unscoredItemsLast;
}

void
resource_managers::ChartFolderModel::setUnscoredItemsLast(bool value)
{
    if (m_unscoredItemsLast == value) {
        return;
    }
    m_unscoredItemsLast = value;
    emit unscoredItemsLastChanged();
}

QVariantMap
resource_managers::ChartFolderModel::scores() const
{
    return m_scores;
}

void
resource_managers::ChartFolderModel::setScores(const QVariantMap& value)
{
    if (m_scores == value) {
        return;
    }
    m_scores = value;
    emit scoresChanged();
}

QVariantList
resource_managers::ChartFolderModel::filterAndSort(
  const QVariantList& input) const
{
    QVector<SortEntry> fixedEntries;
    QVector<SortEntry> chartEntries;
    const QVariantMap emptyScores;
    const QVariantMap& scoresForSort =
      sortModeUsesScores() ? m_scores : emptyScores;
    for (qsizetype i = 0; i < input.size(); ++i) {
        SortEntry entry = makeEntry(input.at(i),
                                    static_cast<int>(i),
                                    m_chartDifficultyByPath,
                                    scoresForSort);
        if (isSortableChartLike(entry)) {
            if (keyFilterMatches(entry, m_keymodeFilter)) {
                chartEntries.append(std::move(entry));
            }
        } else {
            fixedEntries.append(std::move(entry));
        }
    }

    chartEntries = difficultyFilteredCharts(chartEntries, m_difficultyFilter);
    if (m_sortMode != 0 && chartEntries.size() > 1) {
        std::stable_sort(chartEntries.begin(),
                         chartEntries.end(),
                         [this](const SortEntry& a, const SortEntry& b) {
                             const int result = compareEntries(
                               a, b, m_sortMode, m_unscoredItemsLast);
                             if (result != 0) {
                                 return result < 0;
                             }
                             return a.sourceIndex < b.sourceIndex;
                         });
    }

    QVariantList resultItems;
    resultItems.reserve(
      std::max<qsizetype>(1, fixedEntries.size() + chartEntries.size()));
    for (const SortEntry& entry : fixedEntries) {
        resultItems.append(entry.value);
    }
    for (const SortEntry& entry : chartEntries) {
        resultItems.append(entry.value);
    }
    if (resultItems.isEmpty()) {
        resultItems.append(QVariant());
    }
    return resultItems;
}

int
resource_managers::ChartFolderModel::indexOfItem(const QVariantList& items,
                                                 const QVariant& item) const
{
    if (!item.isValid() || item.isNull()) {
        return -1;
    }
    const QVariantMap emptyScores;
    const SortEntry needle =
      makeEntry(item, -1, m_chartDifficultyByPath, emptyScores);
    for (qsizetype i = 0; i < items.size(); ++i) {
        const SortEntry entry = makeEntry(items.at(i),
                                          static_cast<int>(i),
                                          m_chartDifficultyByPath,
                                          emptyScores);
        if (sameEntry(entry, needle)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool
resource_managers::ChartFolderModel::sortModeUsesScores(int mode) const
{
    const int effectiveMode = mode < 0 ? m_sortMode : mode;
    return effectiveMode == 6 || effectiveMode == 7 || effectiveMode == 8;
}

void
resource_managers::ChartFolderModel::rebuildFolderIndexes(
  const QVariantList& input)
{
    m_chartGroupsByFolderKeymode.clear();
    m_chartDifficultyByPath.clear();

    QHash<QString, QVector<FolderIndexEntry>> difficultyGroups;
    QVector<QString> difficultyGroupOrder;

    for (const QVariant& value : input) {
        FolderIndexEntry entry = makeFolderIndexEntry(value);
        if (!entry.valid) {
            continue;
        }

        m_chartGroupsByFolderKeymode[entry.folderKey][entry.keymodeKey].append(
          entry.value);

        const QString groupKey =
          entry.folderKey + QChar(0x1f) + entry.keymodeKey;
        if (!difficultyGroups.contains(groupKey)) {
            difficultyGroupOrder.append(groupKey);
        }
        difficultyGroups[groupKey].append(std::move(entry));
    }

    for (const QString& groupKey : difficultyGroupOrder) {
        const QHash<QString, int> groupDifficulties =
          inferGroupDifficulties(difficultyGroups.value(groupKey));
        for (auto it = groupDifficulties.cbegin();
             it != groupDifficulties.cend();
             ++it) {
            m_chartDifficultyByPath.insert(it.key(), it.value());
        }
    }
}

QVariantList
resource_managers::ChartFolderModel::chartsForSameFolderAndKeymode(
  const QVariant& chart) const
{
    const FolderIndexEntry entry = makeFolderIndexEntry(chart);
    if (!entry.valid) {
        return {};
    }

    const auto folderIt =
      m_chartGroupsByFolderKeymode.constFind(entry.folderKey);
    if (folderIt == m_chartGroupsByFolderKeymode.constEnd()) {
        return {};
    }

    const auto keymodeIt = folderIt.value().constFind(entry.keymodeKey);
    if (keymodeIt == folderIt.value().constEnd()) {
        return {};
    }
    return keymodeIt.value();
}

int
resource_managers::ChartFolderModel::difficultyForChart(
  const QVariant& chart) const
{
    const FolderIndexEntry entry = makeFolderIndexEntry(chart);
    if (!entry.valid) {
        return 0;
    }

    const auto difficultyIt = m_chartDifficultyByPath.constFind(entry.path);
    if (difficultyIt != m_chartDifficultyByPath.constEnd()) {
        return std::max(0, difficultyIt.value());
    }
    return std::max(0, entry.difficulty);
}
