#include "Lr2SkinRuntime.h"

#include "Lr2SkinElementTimerState.h"
#include "Lr2SkinTimerState.h"

#include <QVariantMap>

#include <cmath>
#include <utility>

namespace rt = lr2skin::runtime;

namespace {

bool sameReal(qreal lhs, qreal rhs) {
    return std::abs(lhs - rhs) <= 0.0001;
}

bool sourceCollectionUsesTimers(const QVariantList& sources) {
    for (const QVariant& value : sources) {
        rt::Source source;
        if (rt::readSource(value, source) && rt::sourceUsesDynamicTimer(source)) {
            return true;
        }
    }
    return false;
}

} // namespace

Lr2SkinRuntime::Lr2SkinRuntime(QObject* parent) : QObject(parent) {}

QObject* Lr2SkinRuntime::skinModel() const {
    return m_skinModel;
}

void Lr2SkinRuntime::setSkinModel(QObject* model) {
    auto* typedModel = qobject_cast<QAbstractItemModel*>(model);
    if (m_skinModel == typedModel) {
        return;
    }
    m_skinModel = typedModel;
    reconnectSkinModel();
    emit skinModelChanged();
    rebuildDescriptors();
}

QVariant Lr2SkinRuntime::runtimeActiveOptions() const {
    return m_runtimeActiveOptions;
}

void Lr2SkinRuntime::setRuntimeActiveOptions(const QVariant& value) {
    if (m_runtimeActiveOptions == value) {
        return;
    }
    m_runtimeActiveOptions = value;
    m_activeOptionSet = rt::activeOptionSet(value);
    refreshActiveOptions();
    emit runtimeActiveOptionsChanged();
}

QObject* Lr2SkinRuntime::timerState() const {
    return m_timerState;
}

void Lr2SkinRuntime::setTimerState(QObject* value) {
    auto* typedValue = qobject_cast<Lr2SkinTimerState*>(value);
    if (m_timerState == typedValue) {
        return;
    }
    m_timerState = typedValue;
    reconnectTimerState();
    emit timerStateChanged();
    rebuildDescriptors();
}

QString Lr2SkinRuntime::screenKey() const {
    return m_screenKey;
}

void Lr2SkinRuntime::setScreenKey(const QString& value) {
    if (m_screenKey == value) {
        return;
    }
    m_screenKey = value;
    emit screenKeyChanged();
    rebuildDescriptors();
}

bool Lr2SkinRuntime::gameplayScreen() const {
    return m_gameplayScreen;
}

void Lr2SkinRuntime::setGameplayScreen(bool value) {
    if (m_gameplayScreen == value) {
        return;
    }
    m_gameplayScreen = value;
    emit gameplayScreenChanged();
    rebuildDescriptors();
}

qreal Lr2SkinRuntime::selectBarElementSortBase() const {
    return m_selectBarElementSortBase;
}

void Lr2SkinRuntime::setSelectBarElementSortBase(qreal value) {
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    if (sameReal(m_selectBarElementSortBase, value)) {
        return;
    }
    m_selectBarElementSortBase = value;
    emit selectBarElementSortBaseChanged();
    rebuildDescriptors();
}

int Lr2SkinRuntime::revision() const {
    return m_revision;
}

int Lr2SkinRuntime::timerRevision() const {
    return m_timerRevision;
}

int Lr2SkinRuntime::selectInfoTimerRevision() const {
    return m_selectInfoTimerRevision;
}

int Lr2SkinRuntime::activeOptionsRevision() const {
    return m_activeOptionsRevision;
}

QVariantMap Lr2SkinRuntime::descriptor(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptorMap(*descriptor) : QVariantMap {};
}

QVariantList Lr2SkinRuntime::elementActiveOptionsForElement(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->elementActiveOptions : QVariantList {};
}

QVariant Lr2SkinRuntime::staticStateForElement(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    if (!descriptor || descriptor->staticState.isNull() || descriptor->dsts.isEmpty()) {
        return {};
    }
    if (descriptor->dstAnalysis.usesActiveOptions
            && !rt::allOpsMatch(descriptor->dsts.front(), m_activeOptionSet)) {
        return {};
    }
    return descriptor->staticState;
}

QVariant Lr2SkinRuntime::stateForElement(int index, int skinTime) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    if (!descriptor) {
        return {};
    }
    return rt::stateToVariant(rt::currentState(
        descriptor->dsts,
        skinTime,
        descriptor->timers.dstTimerFire,
        m_activeOptionSet));
}

QVariant Lr2SkinRuntime::sliderTrackStateForElement(int index, int skinTime) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    if (!descriptor) {
        return {};
    }
    const rt::State base = rt::currentState(
        descriptor->dsts,
        skinTime,
        descriptor->timers.dstTimerFire,
        m_activeOptionSet);
    return rt::sliderTrackState(descriptor->source, base);
}

QVariant Lr2SkinRuntime::noteDstState(int index, int skinTime) const {
    return stateForLane(m_noteLaneDescriptors, index, skinTime);
}

QVariant Lr2SkinRuntime::lineDstState(int index, int skinTime) const {
    return stateForLane(m_lineLaneDescriptors, index, skinTime);
}

int Lr2SkinRuntime::dstTimerFire(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->timers.dstTimerFire : -1;
}

int Lr2SkinRuntime::srcTimerFire(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->timers.srcTimerFire : -1;
}

bool Lr2SkinRuntime::dstTimerCanFire(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->timers.dstTimerCanFire : false;
}

bool Lr2SkinRuntime::srcTimerCanFire(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->timers.srcTimerCanFire : false;
}

QObject* Lr2SkinRuntime::elementTimerState(int index) const {
    return index >= 0 && index < m_elementTimerStates.size()
        ? m_elementTimerStates.at(index)
        : nullptr;
}

bool Lr2SkinRuntime::noteFieldUsesActiveOptions() const {
    return m_noteFieldUsesActiveOptions;
}

bool Lr2SkinRuntime::noteFieldUsesTimers() const {
    return m_noteFieldUsesTimers;
}

void Lr2SkinRuntime::rebuildDescriptors() {
    m_descriptors.clear();
    m_timerDescriptorIndexes.clear();
    m_selectInfoTimerDescriptorIndexes.clear();
    m_noteLaneDescriptors.clear();
    m_lineLaneDescriptors.clear();
    m_noteFieldUsesActiveOptions = false;
    m_noteFieldUsesTimers = false;

    if (!m_skinModel) {
        resetElementTimerStates();
        bumpRevision();
        bumpTimerRevision();
        bumpSelectInfoTimerRevision();
        return;
    }

    const QVariantList noteDsts = modelListProperty("noteDsts");
    const QVariantList lineDsts = modelListProperty("lineDsts");
    m_noteLaneDescriptors.reserve(noteDsts.size());
    for (const QVariant& dsts : noteDsts) {
        m_noteLaneDescriptors.append(buildLaneDescriptor(dsts));
    }
    m_lineLaneDescriptors.reserve(lineDsts.size());
    for (const QVariant& dsts : lineDsts) {
        m_lineLaneDescriptors.append(buildLaneDescriptor(dsts));
    }

    const int rows = m_skinModel->rowCount();
    m_descriptors.reserve(rows);
    ensureElementTimerStateCount(rows);
    for (int row = 0; row < rows; ++row) {
        const int type = modelData(row, "type").toInt();
        const QVariant source = modelData(row, "src");
        const QVariantList dsts = rt::readVariantList(modelData(row, "dsts"));
        ElementDescriptor descriptor = buildDescriptor(row, type, source, dsts, noteDsts);
        const bool usesGeneralTimer = (descriptor.usesDynamicDstTimer && descriptor.dstTimer != 11)
            || (descriptor.usesDynamicSrcTimer && descriptor.srcTimer != 11);
        const bool usesSelectInfoTimer = descriptor.dstTimer == 11 || descriptor.srcTimer == 11;
        if (usesGeneralTimer) {
            m_timerDescriptorIndexes.append(row);
        }
        if (usesSelectInfoTimer) {
            m_selectInfoTimerDescriptorIndexes.append(row);
        }
        m_descriptors.append(std::move(descriptor));
        updateElementTimerState(row, m_descriptors.constLast().timers);
    }
    for (int row = rows; row < m_elementTimerStates.size(); ++row) {
        updateElementTimerState(row, TimerSnapshot {});
    }

    const QVector<LaneDescriptor>* laneCollections[] = {
        &m_noteLaneDescriptors,
        &m_lineLaneDescriptors,
    };
    for (const QVector<LaneDescriptor>* collection : laneCollections) {
        for (const LaneDescriptor& descriptor : *collection) {
            m_noteFieldUsesActiveOptions = m_noteFieldUsesActiveOptions
                || descriptor.analysis.usesActiveOptions;
            m_noteFieldUsesTimers = m_noteFieldUsesTimers
                || descriptor.analysis.usesDynamicTimer;
        }
    }

    const QVariantList sourceCollections[] = {
        modelListProperty("noteSources"),
        modelListProperty("mineSources"),
        modelListProperty("lnStartSources"),
        modelListProperty("lnEndSources"),
        modelListProperty("lnBodySources"),
        modelListProperty("lnBodyActiveSources"),
        modelListProperty("autoNoteSources"),
        modelListProperty("autoMineSources"),
        modelListProperty("autoLnStartSources"),
        modelListProperty("autoLnEndSources"),
        modelListProperty("autoLnBodySources"),
        modelListProperty("autoLnBodyActiveSources"),
        modelListProperty("lineSources"),
    };
    for (const QVariantList& collection : sourceCollections) {
        m_noteFieldUsesTimers = m_noteFieldUsesTimers
            || sourceCollectionUsesTimers(collection);
    }

    bumpRevision();
    bumpTimerRevision();
    bumpSelectInfoTimerRevision();
    bumpActiveOptionsRevision();
}

void Lr2SkinRuntime::updateTimerFires() {
    updateTimerFiresForIndexes(m_timerDescriptorIndexes);
}

void Lr2SkinRuntime::updateSelectInfoTimerFires() {
    updateTimerFiresForIndexes(m_selectInfoTimerDescriptorIndexes);
}

void Lr2SkinRuntime::updateTimerFiresForIndexes(const QVector<int>& indexes) {
    bool generalChanged = false;
    bool selectInfoChanged = false;
    for (int index : indexes) {
        if (index < 0 || index >= m_descriptors.size()) {
            continue;
        }
        ElementDescriptor& descriptor = m_descriptors[index];
        const TimerSnapshot next = timerSnapshotFor(descriptor);
        const bool dstChanged = descriptor.timers.dstTimerCanFire != next.dstTimerCanFire
            || descriptor.timers.dstTimerFire != next.dstTimerFire;
        const bool srcChanged = descriptor.timers.srcTimerCanFire != next.srcTimerCanFire
            || descriptor.timers.srcTimerFire != next.srcTimerFire;
        if (dstChanged || srcChanged) {
            if (dstChanged) {
                if (descriptor.dstTimer == 11) {
                    selectInfoChanged = true;
                } else {
                    generalChanged = true;
                }
            }
            if (srcChanged) {
                if (descriptor.srcTimer == 11) {
                    selectInfoChanged = true;
                } else {
                    generalChanged = true;
                }
            }
            descriptor.timers = next;
            updateElementTimerState(index, descriptor.timers);
        }
    }
    if (generalChanged) {
        bumpTimerRevision();
    }
    if (selectInfoChanged) {
        bumpSelectInfoTimerRevision();
    }
}

void Lr2SkinRuntime::ensureElementTimerStateCount(int count) {
    while (m_elementTimerStates.size() < count) {
        m_elementTimerStates.append(new Lr2SkinElementTimerState(this));
    }
}

void Lr2SkinRuntime::updateElementTimerState(int index, const TimerSnapshot& snapshot) {
    if (index < 0) {
        return;
    }
    ensureElementTimerStateCount(index + 1);
    m_elementTimerStates[index]->setSnapshot(
        snapshot.dstTimerCanFire,
        snapshot.dstTimerFire,
        snapshot.srcTimerCanFire,
        snapshot.srcTimerFire);
}

void Lr2SkinRuntime::resetElementTimerStates() {
    for (auto* timerState : std::as_const(m_elementTimerStates)) {
        if (timerState) {
            timerState->setSnapshot(false, -1, false, -1);
        }
    }
}

QVariant Lr2SkinRuntime::modelData(int row, const char* roleName) const {
    if (!m_skinModel || row < 0 || row >= m_skinModel->rowCount()) {
        return {};
    }

    const QHash<int, QByteArray> roles = m_skinModel->roleNames();
    int role = -1;
    for (auto it = roles.constBegin(); it != roles.constEnd(); ++it) {
        if (it.value() == roleName) {
            role = it.key();
            break;
        }
    }
    if (role < 0) {
        return {};
    }
    return m_skinModel->data(m_skinModel->index(row, 0), role);
}

QVariantList Lr2SkinRuntime::modelListProperty(const char* name) const {
    return m_skinModel ? m_skinModel->property(name).toList() : QVariantList {};
}

QVariantMap Lr2SkinRuntime::descriptorMap(const ElementDescriptor& descriptor) const {
    QVariantMap value;
    value.insert(QStringLiteral("index"), descriptor.index);
    value.insert(QStringLiteral("type"), descriptor.type);
    value.insert(QStringLiteral("z"), descriptor.z);
    value.insert(QStringLiteral("usesActiveOptions"), descriptor.dstAnalysis.usesActiveOptions);
    value.insert(QStringLiteral("usesSkinTime"), descriptor.usesSkinTime);
    value.insert(QStringLiteral("usesElementSkinTime"), descriptor.usesElementSkinTime);
    value.insert(QStringLiteral("useDirectElementSkinClock"), descriptor.useDirectElementSkinClock);
    value.insert(QStringLiteral("needsManualElementSkinTime"), descriptor.needsManualElementSkinTime);
    value.insert(QStringLiteral("elementSkinClockMode"), descriptor.elementSkinClockMode);
    value.insert(QStringLiteral("sourceHasFrameAnimation"), descriptor.sourceHasFrameAnimation);
    value.insert(QStringLiteral("usesSelectHeldButtonTimer"), descriptor.usesSelectHeldButtonTimer);
    value.insert(QStringLiteral("usesLiveDstClock"), descriptor.usesLiveDstClock);
    value.insert(QStringLiteral("usesLiveSourceClock"), descriptor.usesLiveSourceClock);
    value.insert(QStringLiteral("usesLiveSelectClock"), descriptor.usesLiveSelectClock);
    value.insert(QStringLiteral("usesDynamicDstTimer"), descriptor.usesDynamicDstTimer);
    value.insert(QStringLiteral("usesDynamicSrcTimer"), descriptor.usesDynamicSrcTimer);
    value.insert(QStringLiteral("spriteUsesDirectSkinClock"), descriptor.spriteUsesDirectSkinClock);
    value.insert(QStringLiteral("spriteSkinClockMode"), descriptor.spriteSkinClockMode);
    value.insert(QStringLiteral("spriteSourceSkinClockMode"), descriptor.spriteSourceSkinClockMode);
    value.insert(QStringLiteral("spriteStateOverrideKind"), descriptor.spriteStateOverrideKind);
    value.insert(QStringLiteral("usesSpriteStateOverride"), descriptor.usesSpriteStateOverride);
    value.insert(QStringLiteral("usesSpriteForceHidden"), descriptor.usesSpriteForceHidden);
    value.insert(QStringLiteral("usesButtonFrameOverride"), descriptor.usesButtonFrameOverride);
    value.insert(QStringLiteral("sourceMouseCursor"), descriptor.sourceMouseCursor);
    value.insert(QStringLiteral("dstOffsetsEnabled"), descriptor.dstOffsetsEnabled);
    value.insert(QStringLiteral("dstOffsetSide"), descriptor.dstOffsetSide);
    value.insert(QStringLiteral("scratchRotationSide"), descriptor.scratchRotationSide);
    value.insert(QStringLiteral("dstTimer"), descriptor.dstTimer);
    value.insert(QStringLiteral("srcTimer"), descriptor.srcTimer);
    value.insert(QStringLiteral("selectScrollSlider"), descriptor.selectScrollSlider);
    value.insert(QStringLiteral("genericSlider"), descriptor.genericSlider);
    value.insert(QStringLiteral("gameplayProgressSlider"), descriptor.gameplayProgressSlider);
    value.insert(QStringLiteral("gameplayLaneCoverSlider"), descriptor.gameplayLaneCoverSlider);
    value.insert(QStringLiteral("numberRefSlider"), descriptor.numberRefSlider);
    value.insert(QStringLiteral("buttonId"), descriptor.buttonId);
    return value;
}

Lr2SkinRuntime::LaneDescriptor Lr2SkinRuntime::buildLaneDescriptor(const QVariant& dsts) const {
    LaneDescriptor descriptor;
    descriptor.dsts = rt::readDsts(dsts);
    descriptor.analysis = rt::analyzeDsts(descriptor.dsts);
    descriptor.staticState = descriptor.analysis.canUseStaticState && !descriptor.dsts.isEmpty()
        ? rt::stateToVariant(rt::copyDstAsState(descriptor.dsts.front(), descriptor.dsts.front()))
        : QVariant {};
    return descriptor;
}

Lr2SkinRuntime::ElementDescriptor Lr2SkinRuntime::buildDescriptor(
    int index,
    int type,
    const QVariant& sourceValue,
    const QVariantList& dstValues,
    const QVariantList& noteDsts) const {
    ElementDescriptor descriptor;
    descriptor.index = index;
    descriptor.type = type;
    rt::readSource(sourceValue, descriptor.source);
    descriptor.dsts = rt::readDsts(dstValues);
    descriptor.dstAnalysis = rt::analyzeDsts(descriptor.dsts);

    const bool selectScreen = m_screenKey == QStringLiteral("select");
    const bool sourceCycles = rt::sourceCyclesContinuously(descriptor.source);
    const bool selectInfoDstTimer = selectScreen && descriptor.dstAnalysis.firstTimer == 11;
    const bool selectInfoSrcTimer = selectScreen && descriptor.source.valid && descriptor.source.timer == 11;
    const bool selectPanelTimer = (descriptor.dstAnalysis.firstTimer >= 21 && descriptor.dstAnalysis.firstTimer <= 26)
        || (descriptor.dstAnalysis.firstTimer >= 31 && descriptor.dstAnalysis.firstTimer <= 36);

    descriptor.elementActiveOptions = descriptor.dstAnalysis.usesActiveOptions && !descriptor.dsts.isEmpty()
        ? rt::activeOptionsForDsts(descriptor.dsts.front(), m_runtimeActiveOptions)
        : QVariantList {};
    descriptor.staticState = descriptor.dstAnalysis.canUseStaticState && !descriptor.dsts.isEmpty()
        ? rt::stateToVariant(rt::copyDstAsState(descriptor.dsts.front(), descriptor.dsts.front()))
        : QVariant {};
    descriptor.sourceHasFrameAnimation = sourceCycles;
    descriptor.usesLiveDstClock = selectScreen && (selectPanelTimer || descriptor.dstAnalysis.loopsContinuously);
    descriptor.usesLiveSourceClock = selectScreen && sourceCycles;
    descriptor.usesLiveSelectClock = descriptor.usesLiveDstClock || descriptor.usesLiveSourceClock;
    descriptor.usesSelectHeldButtonTimer = m_timerState
        ? m_timerState->isSelectHeldButtonTimer(descriptor.dstAnalysis.firstTimer)
        : false;
    descriptor.usesSkinTime = !descriptor.dstAnalysis.canUseStaticState
        || sourceCycles
        || descriptor.source.resultChartType > 0;
    descriptor.usesElementSkinTime = descriptor.usesSkinTime
        && type != 0
        && type != 3
        && type != 4
        && type != 5
        && type != 8
        && type != 9;
    descriptor.useDirectElementSkinClock = descriptor.usesElementSkinTime;
    descriptor.needsManualElementSkinTime = descriptor.usesElementSkinTime
        && (type == 7
            || type == 10
            || type == 11
            || type == 12
            || (type == 1 && sourceCycles));
    const int dstClockMode = selectInfoDstTimer
        ? rt::SelectInfoClock
        : (descriptor.usesLiveDstClock ? rt::SelectSourceClock : rt::RenderClock);
    const int sourceClockMode = selectInfoSrcTimer
        ? rt::SelectInfoClock
        : (descriptor.usesLiveSourceClock ? rt::SelectSourceClock : rt::RenderClock);
    descriptor.elementSkinClockMode = descriptor.usesElementSkinTime
        ? ((selectInfoDstTimer || (sourceCycles && selectInfoSrcTimer))
            ? rt::SelectInfoClock
            : (descriptor.usesLiveSelectClock ? rt::SelectSourceClock : rt::RenderClock))
        : rt::ManualClock;
    descriptor.spriteUsesDirectSkinClock = descriptor.usesSkinTime && !descriptor.usesSelectHeldButtonTimer;
    descriptor.spriteSkinClockMode = descriptor.spriteUsesDirectSkinClock
        ? dstClockMode
        : rt::ManualClock;
    descriptor.spriteSourceSkinClockMode = descriptor.spriteUsesDirectSkinClock && sourceCycles
        ? sourceClockMode
        : rt::ManualClock;
    descriptor.spriteStateOverrideKind = rt::spriteStateOverrideKind(
        m_screenKey,
        m_gameplayScreen,
        descriptor.source);
    descriptor.usesSpriteStateOverride = descriptor.spriteStateOverrideKind != rt::NoSpriteStateOverride;
    descriptor.usesSpriteForceHidden = descriptor.source.onMouse;
    descriptor.usesButtonFrameOverride = selectScreen && descriptor.source.button;
    descriptor.sourceMouseCursor = descriptor.source.mouseCursor;
    descriptor.dstOffsetsEnabled = m_gameplayScreen
        && !descriptor.dsts.isEmpty()
        && !descriptor.dsts.front().offsets.isEmpty();
    descriptor.dstOffsetSide = descriptor.source.side == 2 ? 2 : 1;
    descriptor.scratchRotationSide = descriptor.dstAnalysis.scratchRotationSide;
    descriptor.dstTimer = descriptor.dstAnalysis.firstTimer;
    descriptor.srcTimer = descriptor.source.valid ? descriptor.source.timer : 0;
    descriptor.usesDynamicDstTimer = descriptor.dstTimer != 0;
    descriptor.usesDynamicSrcTimer = descriptor.srcTimer != 0;
    descriptor.selectScrollSlider = descriptor.spriteStateOverrideKind == rt::SelectScrollSpriteStateOverride;
    descriptor.genericSlider = descriptor.spriteStateOverrideKind == rt::GenericSliderSpriteStateOverride;
    descriptor.gameplayProgressSlider = descriptor.spriteStateOverrideKind == rt::GameplayProgressSpriteStateOverride;
    descriptor.gameplayLaneCoverSlider = descriptor.spriteStateOverrideKind == rt::GameplayLaneCoverSpriteStateOverride;
    descriptor.numberRefSlider = descriptor.spriteStateOverrideKind == rt::NumberRefSpriteStateOverride;
    descriptor.buttonId = descriptor.source.buttonId;

    if (rt::isSelectBarElement(type, descriptor.source)) {
        descriptor.z = m_selectBarElementSortBase
            + rt::selectBarElementLayer(type, descriptor.source)
            + descriptor.dstAnalysis.firstSortId * 0.000001
            + index * 0.000000001;
    } else if (type == 8) {
        descriptor.z = rt::staticNoteElementSortId(noteDsts) + index * 0.000001;
    } else {
        descriptor.z = descriptor.dstAnalysis.firstSortId + index * 0.000001;
    }

    descriptor.timers = timerSnapshotFor(descriptor);
    return descriptor;
}

Lr2SkinRuntime::TimerSnapshot Lr2SkinRuntime::timerSnapshotFor(
    const ElementDescriptor& descriptor) const {
    TimerSnapshot snapshot;
    if (!m_timerState) {
        snapshot.dstTimerFire = descriptor.dstTimer == 0 ? 0 : -1;
        snapshot.srcTimerFire = descriptor.srcTimer == 0 ? 0 : -1;
        return snapshot;
    }

    snapshot.dstTimerCanFire = m_timerState->skinTimerCanFire(descriptor.dstTimer);
    snapshot.srcTimerCanFire = m_timerState->skinTimerCanFire(descriptor.srcTimer);
    const bool dstUsesSelectInfoClock = descriptor.dstTimer == 11
        && (descriptor.elementSkinClockMode == rt::SelectInfoClock
            || descriptor.spriteSkinClockMode == rt::SelectInfoClock);
    const bool srcUsesSelectInfoClock = descriptor.srcTimer == 11
        && (descriptor.elementSkinClockMode == rt::SelectInfoClock
            || descriptor.spriteSourceSkinClockMode == rt::SelectInfoClock);

    if (descriptor.dstTimer == 0 || dstUsesSelectInfoClock) {
        snapshot.dstTimerFire = 0;
    } else {
        snapshot.dstTimerFire = snapshot.dstTimerCanFire
            ? m_timerState->skinTimerFireTime(descriptor.dstTimer, descriptor.usesLiveDstClock)
            : -1;
    }

    if (descriptor.srcTimer == 0 || srcUsesSelectInfoClock) {
        snapshot.srcTimerFire = 0;
    } else {
        snapshot.srcTimerFire = snapshot.srcTimerCanFire
            ? m_timerState->skinTimerFireTime(descriptor.srcTimer, descriptor.usesLiveSourceClock)
            : -1;
    }
    return snapshot;
}

QVariant Lr2SkinRuntime::stateForLane(const QVector<LaneDescriptor>& lanes,
                                      int index,
                                      int skinTime) const {
    if (index < 0 || index >= lanes.size()) {
        return {};
    }

    const LaneDescriptor& lane = lanes.at(index);
    if (lane.staticState.isValid()
            && !lane.staticState.isNull()
            && (!lane.analysis.usesActiveOptions || rt::allOpsMatch(lane.dsts.front(), m_activeOptionSet))) {
        return lane.staticState;
    }

    int timerFire = lane.analysis.firstTimer == 0 ? 0 : -1;
    if (lane.analysis.firstTimer != 0 && m_timerState
            && m_timerState->skinTimerCanFire(lane.analysis.firstTimer)) {
        timerFire = m_timerState->skinTimerFireTime(lane.analysis.firstTimer, false);
    }

    return rt::stateToVariant(rt::currentState(
        lane.dsts,
        skinTime,
        timerFire,
        m_activeOptionSet));
}

const Lr2SkinRuntime::ElementDescriptor* Lr2SkinRuntime::descriptorAt(int index) const {
    if (index < 0 || index >= m_descriptors.size()) {
        return nullptr;
    }
    const ElementDescriptor& descriptor = m_descriptors.at(index);
    return descriptor.index == index ? &descriptor : nullptr;
}

void Lr2SkinRuntime::reconnectSkinModel() {
    for (const QMetaObject::Connection& connection : m_skinModelConnections) {
        QObject::disconnect(connection);
    }
    m_skinModelConnections.clear();

    if (!m_skinModel) {
        return;
    }

    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        &QAbstractItemModel::modelReset,
        this,
        &Lr2SkinRuntime::rebuildDescriptors));
    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        &QAbstractItemModel::rowsInserted,
        this,
        &Lr2SkinRuntime::rebuildDescriptors));
    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        &QAbstractItemModel::rowsRemoved,
        this,
        &Lr2SkinRuntime::rebuildDescriptors));
    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        &QAbstractItemModel::dataChanged,
        this,
        &Lr2SkinRuntime::rebuildDescriptors));
    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        SIGNAL(skinMetadataChanged()),
        this,
        SLOT(rebuildDescriptors())));
    m_skinModelConnections.append(QObject::connect(
        m_skinModel,
        &QObject::destroyed,
        this,
        [this]() {
            m_skinModel = nullptr;
            rebuildDescriptors();
        }));
}

void Lr2SkinRuntime::reconnectTimerState() {
    for (const QMetaObject::Connection& connection : m_timerStateConnections) {
        QObject::disconnect(connection);
    }
    m_timerStateConnections.clear();

    if (!m_timerState) {
        return;
    }

    m_timerStateConnections.append(QObject::connect(
        m_timerState,
        &Lr2SkinTimerState::revisionChanged,
        this,
        &Lr2SkinRuntime::updateTimerFires));
    m_timerStateConnections.append(QObject::connect(
        m_timerState,
        &Lr2SkinTimerState::selectInfoRevisionChanged,
        this,
        &Lr2SkinRuntime::updateSelectInfoTimerFires));
}

void Lr2SkinRuntime::refreshActiveOptions() {
    for (ElementDescriptor& descriptor : m_descriptors) {
        descriptor.elementActiveOptions = descriptor.dstAnalysis.usesActiveOptions && !descriptor.dsts.isEmpty()
            ? rt::activeOptionsForDsts(descriptor.dsts.front(), m_runtimeActiveOptions)
            : QVariantList {};
    }
    bumpActiveOptionsRevision();
}

void Lr2SkinRuntime::bumpRevision() {
    ++m_revision;
    emit revisionChanged();
}

void Lr2SkinRuntime::bumpTimerRevision() {
    ++m_timerRevision;
    emit timerRevisionChanged();
}

void Lr2SkinRuntime::bumpSelectInfoTimerRevision() {
    ++m_selectInfoTimerRevision;
    emit selectInfoTimerRevisionChanged();
}

void Lr2SkinRuntime::bumpActiveOptionsRevision() {
    ++m_activeOptionsRevision;
    emit activeOptionsRevisionChanged();
}
