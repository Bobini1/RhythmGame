#include "Lr2SelectBarCell.h"

#include <QJSValue>
#include <QMetaProperty>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace {

qreal toReal(const QVariant& value, qreal fallback = 0.0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const qreal result = value.toReal(&ok);
    return ok ? result : fallback;
}

QVariant gadgetProperty(const QVariant& source, const char* name) {
    const QMetaObject* metaObject = source.metaType().metaObject();
    if (!metaObject) {
        return {};
    }
    const int propertyIndex = metaObject->indexOfProperty(name);
    if (propertyIndex < 0) {
        return {};
    }
    return metaObject->property(propertyIndex).readOnGadget(source.constData());
}

QVariant valueProperty(const QVariant& source, const char* name) {
    if (!source.isValid() || source.isNull()) {
        return {};
    }
    if (source.canConvert<QVariantMap>()) {
        const QVariantMap map = source.toMap();
        const auto it = map.constFind(QString::fromLatin1(name));
        return it == map.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QVariantHash>()) {
        const QVariantHash hash = source.toHash();
        const auto it = hash.constFind(QString::fromLatin1(name));
        return it == hash.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QJSValue>()) {
        const QJSValue value = source.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(QString::fromLatin1(name));
            return property.isUndefined() || property.isNull() ? QVariant() : property.toVariant();
        }
    }
    if (source.canConvert<QObject*>()) {
        if (QObject* object = source.value<QObject*>()) {
            return object->property(name);
        }
    }
    return gadgetProperty(source, name);
}

QVariantList graphValuesForType(int graphType,
                                const QVariantList& graphLamps,
                                const QVariantList& graphRanks) {
    return graphType == 0 ? graphLamps : graphRanks;
}

struct GraphBaseState {
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    QVariant a;
    QVariant r;
    QVariant g;
    QVariant b;
    QVariant blend;
    QVariant filter;
    QVariant angle;
    QVariant center;
    QVariant sortId;
    QVariant op1;
    QVariant op2;
    QVariant op3;
    QVariant op4;
};

GraphBaseState graphBaseStateFromMap(const QVariantMap& map) {
    return {
        toReal(map.value(QStringLiteral("x"))),
        toReal(map.value(QStringLiteral("y"))),
        toReal(map.value(QStringLiteral("w"))),
        toReal(map.value(QStringLiteral("h"))),
        map.value(QStringLiteral("a")),
        map.value(QStringLiteral("r")),
        map.value(QStringLiteral("g")),
        map.value(QStringLiteral("b")),
        map.value(QStringLiteral("blend")),
        map.value(QStringLiteral("filter")),
        map.value(QStringLiteral("angle")),
        map.value(QStringLiteral("center")),
        map.value(QStringLiteral("sortId")),
        map.value(QStringLiteral("op1")),
        map.value(QStringLiteral("op2")),
        map.value(QStringLiteral("op3")),
        map.value(QStringLiteral("op4")),
    };
}

GraphBaseState graphBaseStateFromHash(const QVariantHash& hash) {
    return {
        toReal(hash.value(QStringLiteral("x"))),
        toReal(hash.value(QStringLiteral("y"))),
        toReal(hash.value(QStringLiteral("w"))),
        toReal(hash.value(QStringLiteral("h"))),
        hash.value(QStringLiteral("a")),
        hash.value(QStringLiteral("r")),
        hash.value(QStringLiteral("g")),
        hash.value(QStringLiteral("b")),
        hash.value(QStringLiteral("blend")),
        hash.value(QStringLiteral("filter")),
        hash.value(QStringLiteral("angle")),
        hash.value(QStringLiteral("center")),
        hash.value(QStringLiteral("sortId")),
        hash.value(QStringLiteral("op1")),
        hash.value(QStringLiteral("op2")),
        hash.value(QStringLiteral("op3")),
        hash.value(QStringLiteral("op4")),
    };
}

GraphBaseState graphBaseState(const QVariant& source) {
    if (source.canConvert<QVariantMap>()) {
        return graphBaseStateFromMap(source.toMap());
    }
    if (source.canConvert<QVariantHash>()) {
        return graphBaseStateFromHash(source.toHash());
    }
    return {
        toReal(valueProperty(source, "x")),
        toReal(valueProperty(source, "y")),
        toReal(valueProperty(source, "w")),
        toReal(valueProperty(source, "h")),
        valueProperty(source, "a"),
        valueProperty(source, "r"),
        valueProperty(source, "g"),
        valueProperty(source, "b"),
        valueProperty(source, "blend"),
        valueProperty(source, "filter"),
        valueProperty(source, "angle"),
        valueProperty(source, "center"),
        valueProperty(source, "sortId"),
        valueProperty(source, "op1"),
        valueProperty(source, "op2"),
        valueProperty(source, "op3"),
        valueProperty(source, "op4"),
    };
}

QVariantMap segmentState(const GraphBaseState& baseState, qreal start, qreal width) {
    return {
        {QStringLiteral("x"), baseState.x + std::min<qreal>(0.0, baseState.w) + start},
        {QStringLiteral("y"), baseState.y + std::min<qreal>(0.0, baseState.h)},
        {QStringLiteral("w"), width},
        {QStringLiteral("h"), std::abs(baseState.h)},
        {QStringLiteral("a"), baseState.a},
        {QStringLiteral("r"), baseState.r},
        {QStringLiteral("g"), baseState.g},
        {QStringLiteral("b"), baseState.b},
        {QStringLiteral("blend"), baseState.blend},
        {QStringLiteral("filter"), baseState.filter},
        {QStringLiteral("angle"), baseState.angle},
        {QStringLiteral("center"), baseState.center},
        {QStringLiteral("sortId"), baseState.sortId},
        {QStringLiteral("op1"), baseState.op1},
        {QStringLiteral("op2"), baseState.op2},
        {QStringLiteral("op3"), baseState.op3},
        {QStringLiteral("op4"), baseState.op4},
    };
}

QVariant listValueAt(const QVariant& values, int index) {
    if (index < 0) {
        return {};
    }

    const QVariantList list = values.toList();
    if (!list.isEmpty()) {
        return index < list.size() ? list.at(index) : QVariant {};
    }

    if (!values.canConvert<QJSValue>()) {
        return {};
    }
    const QJSValue jsValue = values.value<QJSValue>();
    if (!jsValue.isArray()) {
        return {};
    }
    const int length = jsValue.property(QStringLiteral("length")).toInt();
    return index < length ? jsValue.property(static_cast<quint32>(index)).toVariant() : QVariant {};
}

} // namespace

Lr2SelectBarCell::Lr2SelectBarCell(QObject* parent) : QObject(parent) {}

int Lr2SelectBarCell::row() const { return m_row; }
void Lr2SelectBarCell::setRow(int value) {
    if (m_row == value) return;
    m_row = value;
    emit rowChanged();
}

QVariant Lr2SelectBarCell::entry() const { return m_entry; }
void Lr2SelectBarCell::setEntry(const QVariant& value) {
    if (m_entry == value) return;
    m_entry = value;
    emit entryChanged();
}

bool Lr2SelectBarCell::isValid() const { return m_valid; }
void Lr2SelectBarCell::setValid(bool value) {
    if (m_valid == value) return;
    m_valid = value;
    emit validChanged();
}

QString Lr2SelectBarCell::text() const { return m_text; }
void Lr2SelectBarCell::setText(const QString& value) {
    if (m_text == value) return;
    m_text = value;
    emit textChanged();
}

int Lr2SelectBarCell::titleType() const { return m_titleType; }
void Lr2SelectBarCell::setTitleType(int value) {
    if (m_titleType == value) return;
    m_titleType = value;
    emit titleTypeChanged();
}

int Lr2SelectBarCell::bodyType() const { return m_bodyType; }
void Lr2SelectBarCell::setBodyType(int value) {
    if (m_bodyType == value) return;
    m_bodyType = value;
    emit bodyTypeChanged();
}

int Lr2SelectBarCell::playLevel() const { return m_playLevel; }
void Lr2SelectBarCell::setPlayLevel(int value) {
    if (m_playLevel == value) return;
    m_playLevel = value;
    emit playLevelChanged();
}

int Lr2SelectBarCell::difficulty() const { return m_difficulty; }
void Lr2SelectBarCell::setDifficulty(int value) {
    if (m_difficulty == value) return;
    m_difficulty = value;
    emit difficultyChanged();
}

int Lr2SelectBarCell::keymode() const { return m_keymode; }
void Lr2SelectBarCell::setKeymode(int value) {
    if (m_keymode == value) return;
    m_keymode = value;
    emit keymodeChanged();
}

bool Lr2SelectBarCell::isRanking() const { return m_ranking; }
void Lr2SelectBarCell::setRanking(bool value) {
    if (m_ranking == value) return;
    m_ranking = value;
    emit rankingChanged();
}

bool Lr2SelectBarCell::isChartLike() const { return m_chartLike; }
void Lr2SelectBarCell::setChartLike(bool value) {
    if (m_chartLike == value) return;
    m_chartLike = value;
    emit chartLikeChanged();
}

bool Lr2SelectBarCell::isEntryLike() const { return m_entryLike; }
void Lr2SelectBarCell::setEntryLike(bool value) {
    if (m_entryLike == value) return;
    m_entryLike = value;
    emit entryLikeChanged();
}

bool Lr2SelectBarCell::isFolderLike() const { return m_folderLike; }
void Lr2SelectBarCell::setFolderLike(bool value) {
    if (m_folderLike == value) return;
    m_folderLike = value;
    emit folderLikeChanged();
}

int Lr2SelectBarCell::lamp() const { return m_lamp; }
void Lr2SelectBarCell::setLamp(int value) {
    if (m_lamp == value) return;
    m_lamp = value;
    emit lampChanged();
}

int Lr2SelectBarCell::rank() const { return m_rank; }
void Lr2SelectBarCell::setRank(int value) {
    if (m_rank == value) return;
    m_rank = value;
    emit rankChanged();
}

int Lr2SelectBarCell::labelMask() const { return m_labelMask; }
void Lr2SelectBarCell::setLabelMask(int value) {
    if (m_labelMask == value) return;
    m_labelMask = value;
    emit labelMaskChanged();
}

QVariantList Lr2SelectBarCell::graphLamps() const { return m_graphLamps; }
void Lr2SelectBarCell::setGraphLamps(const QVariantList& value) {
    if (m_graphLamps == value) return;
    m_graphLamps = value;
    emit graphLampsChanged();
}

QVariantList Lr2SelectBarCell::graphRanks() const { return m_graphRanks; }
void Lr2SelectBarCell::setGraphRanks(const QVariantList& value) {
    if (m_graphRanks == value) return;
    m_graphRanks = value;
    emit graphRanksChanged();
}

int Lr2SelectBarCell::revision() const { return m_revision; }

int Lr2SelectBarCell::bodyTypeValue() const { return m_bodyType; }

QVariant Lr2SelectBarCell::bodySource(const QVariant& sources, const QVariant& fallback) const {
    QVariant source = listValueAt(sources, m_bodyType);
    if (!source.isValid() || source.isNull()) {
        source = listValueAt(sources, 0);
    }
    return source.isValid() && !source.isNull() ? source : fallback;
}

bool Lr2SelectBarCell::textVisible(int titleType) const {
    return m_valid && m_titleType == titleType;
}

QString Lr2SelectBarCell::textForTitleType(int titleType) const {
    return textVisible(titleType) ? m_text : QString();
}

bool Lr2SelectBarCell::numberVisible(int variant) const {
    if (!m_valid) {
        return false;
    }
    if (m_ranking) {
        return variant == 0 || variant == 6;
    }
    if (!m_chartLike && !m_entryLike) {
        return false;
    }
    if (m_keymode <= 0 || m_playLevel < 0) {
        return false;
    }
    return m_difficulty <= 0 ? variant == 0 : variant == m_difficulty;
}

int Lr2SelectBarCell::numberValueForVariant(int variant) const {
    return numberVisible(variant) ? m_playLevel : 0;
}

int Lr2SelectBarCell::numberValueOrInvisibleForVariant(int variant) const {
    return numberVisible(variant) ? m_playLevel : -2147483648;
}

bool Lr2SelectBarCell::lampVisibleForKind(int kind) const {
    return m_valid && kind == 3 && m_lamp > 0;
}

int Lr2SelectBarCell::lampForKind(int kind) const {
    return kind == 3 ? m_lamp : 0;
}

bool Lr2SelectBarCell::rankingForKind(int kind) const {
    return m_valid && kind == 6 && m_ranking;
}

bool Lr2SelectBarCell::rankVisibleForKind(int kind) const {
    return m_valid && kind == 6 && m_ranking && m_rank > 0;
}

int Lr2SelectBarCell::rankForKind(int kind) const {
    return kind == 6 && m_ranking ? m_rank : 0;
}

bool Lr2SelectBarCell::overlayVisibleForKind(int kind, int variant) const {
    if (!m_valid) {
        return false;
    }
    if (kind == 3) {
        return m_lamp == variant;
    }
    if (kind == 6) {
        return m_ranking && m_rank == variant;
    }
    if (kind == 8) {
        return variant >= 0 && variant < 31 && (m_labelMask & (1 << variant)) != 0;
    }
    return false;
}

qreal Lr2SelectBarCell::graphValueForType(int graphType, int segment) const {
    if (segment < 0) {
        return 0.0;
    }
    const QVariantList& values = graphType == 0 ? m_graphLamps : m_graphRanks;
    return segment < values.size() ? std::max<qreal>(0.0, toReal(values.at(segment))) : 0.0;
}

QVariantList Lr2SelectBarCell::graphSegmentModel(int graphType,
                                                 int segmentCount,
                                                 int frameCount,
                                                 const QVariant& state) const {
    if (m_graphSegmentCacheRevision == m_revision
            && m_graphSegmentCacheGraphType == graphType
            && m_graphSegmentCacheSegmentCount == segmentCount
            && m_graphSegmentCacheFrameCount == frameCount
            && m_graphSegmentCacheState == state) {
        return m_graphSegmentCacheModel;
    }

    auto cacheAndReturn = [this, graphType, segmentCount, frameCount, &state](QVariantList model) {
        m_graphSegmentCacheRevision = m_revision;
        m_graphSegmentCacheGraphType = graphType;
        m_graphSegmentCacheSegmentCount = segmentCount;
        m_graphSegmentCacheFrameCount = frameCount;
        m_graphSegmentCacheState = state;
        m_graphSegmentCacheModel = std::move(model);
        return m_graphSegmentCacheModel;
    };

    if (!m_valid || !m_folderLike || !state.isValid() || state.isNull()) {
        return cacheAndReturn({});
    }

    const QVariantList values = graphValuesForType(graphType, m_graphLamps, m_graphRanks);
    if (values.isEmpty()) {
        return cacheAndReturn({});
    }

    qreal total = 0.0;
    const int effectiveSegmentCount = std::max(0, segmentCount);
    for (int i = 0; i < effectiveSegmentCount && i < values.size(); ++i) {
        total += std::max<qreal>(0.0, toReal(values.at(i)));
    }
    if (total <= 0.0) {
        return cacheAndReturn({});
    }

    const GraphBaseState baseState = graphBaseState(state);
    const qreal fullW = std::abs(baseState.w);
    const qreal fullH = std::abs(baseState.h);
    if (fullW <= 0.0 || fullH <= 0.0) {
        return cacheAndReturn({});
    }

    QVariantList result;
    result.reserve(effectiveSegmentCount);

    std::vector<qreal> starts(static_cast<size_t>(effectiveSegmentCount), 0.0);
    std::vector<qreal> widths(static_cast<size_t>(effectiveSegmentCount), 0.0);

    qreal accumulated = 0.0;
    for (int segment = effectiveSegmentCount - 1; segment >= 0; --segment) {
        const qreal amount = segment < values.size()
            ? std::max<qreal>(0.0, toReal(values.at(segment)))
            : 0.0;
        if (amount <= 0.0) {
            continue;
        }
        starts[static_cast<size_t>(segment)] = accumulated * fullW / total;
        widths[static_cast<size_t>(segment)] = amount * fullW / total;
        accumulated += amount;
    }

    for (int segment = 0; segment < effectiveSegmentCount; ++segment) {
        const bool visible = widths[static_cast<size_t>(segment)] > 0.0;
        result.append(QVariantMap {
            {QStringLiteral("segment"), segment},
            {QStringLiteral("frameOffset"), std::min(std::max(1, frameCount) - 1, segment)},
            {QStringLiteral("visible"), visible},
            {QStringLiteral("state"), visible
                ? segmentState(baseState,
                               starts[static_cast<size_t>(segment)],
                               widths[static_cast<size_t>(segment)])
                : QVariantMap {}},
        });
    }
    return cacheAndReturn(std::move(result));
}

void Lr2SelectBarCell::setCore(int row,
                               bool valid,
                               const QString& text,
                               int titleType,
                               int bodyType,
                               int playLevel,
                               int difficulty,
                               int keymode,
                               bool ranking,
                               bool chartLike,
                               bool entryLike,
                               bool folderLike,
                               int lamp,
                               int rank,
                               int labelMask,
                               const QVariantList& graphLamps,
                               const QVariantList& graphRanks) {
    const bool rowChanged = m_row != row;
    const bool validChanged = m_valid != valid;
    const bool textChanged = m_text != text;
    const bool titleTypeChanged = m_titleType != titleType;
    const bool bodyTypeChanged = m_bodyType != bodyType;
    const bool playLevelChanged = m_playLevel != playLevel;
    const bool difficultyChanged = m_difficulty != difficulty;
    const bool keymodeChanged = m_keymode != keymode;
    const bool rankingChanged = m_ranking != ranking;
    const bool chartLikeChanged = m_chartLike != chartLike;
    const bool entryLikeChanged = m_entryLike != entryLike;
    const bool folderLikeChanged = m_folderLike != folderLike;
    const bool lampChanged = m_lamp != lamp;
    const bool rankChanged = m_rank != rank;
    const bool labelMaskChanged = m_labelMask != labelMask;
    const bool graphLampsChanged = m_graphLamps != graphLamps;
    const bool graphRanksChanged = m_graphRanks != graphRanks;

    if (!rowChanged
            && !validChanged
            && !textChanged
            && !titleTypeChanged
            && !bodyTypeChanged
            && !playLevelChanged
            && !difficultyChanged
            && !keymodeChanged
            && !rankingChanged
            && !chartLikeChanged
            && !entryLikeChanged
            && !folderLikeChanged
            && !lampChanged
            && !rankChanged
            && !labelMaskChanged
            && !graphLampsChanged
            && !graphRanksChanged) {
        return;
    }

    m_row = row;
    m_valid = valid;
    m_text = text;
    m_titleType = titleType;
    m_bodyType = bodyType;
    m_playLevel = playLevel;
    m_difficulty = difficulty;
    m_keymode = keymode;
    m_ranking = ranking;
    m_chartLike = chartLike;
    m_entryLike = entryLike;
    m_folderLike = folderLike;
    m_lamp = lamp;
    m_rank = rank;
    m_labelMask = labelMask;
    m_graphLamps = graphLamps;
    m_graphRanks = graphRanks;
    ++m_revision;

    Q_UNUSED(rowChanged);
    Q_UNUSED(validChanged);
    Q_UNUSED(textChanged);
    Q_UNUSED(titleTypeChanged);
    Q_UNUSED(bodyTypeChanged);
    Q_UNUSED(playLevelChanged);
    Q_UNUSED(difficultyChanged);
    Q_UNUSED(keymodeChanged);
    Q_UNUSED(rankingChanged);
    Q_UNUSED(chartLikeChanged);
    Q_UNUSED(entryLikeChanged);
    Q_UNUSED(folderLikeChanged);
    Q_UNUSED(lampChanged);
    Q_UNUSED(rankChanged);
    Q_UNUSED(labelMaskChanged);
    Q_UNUSED(graphLampsChanged);
    Q_UNUSED(graphRanksChanged);
    emit revisionChanged();
    emit coreChanged();
}
