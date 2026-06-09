#include "Lr2SkinRuntime.h"

#include "Lr2SkinElementActiveOptionsState.h"
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

bool sourceHasBarDistributionGraphAnimation(const rt::Source& source) {
    const int segmentCount = source.graphType == 0 ? 11 : 28;
    const int frameCount = std::max(1, source.divX) * std::max(1, source.divY);
    return source.valid
        && source.cycle > 0
        && std::max(1, frameCount / segmentCount) > 1;
}

QVector<int> activeOptionIdsForDst(const rt::Dst& dst) {
    QVector<int> result;
    const int ops[3] = { dst.op1, dst.op2, dst.op3 };
    for (int op : ops) {
        const int id = std::abs(op);
        if (id == 0 || result.contains(id)) {
            continue;
        }
        result.append(id);
    }
    return result;
}

QVariantList activeOptionsForDst(const rt::Dst& dst, const QSet<int>& activeOptions) {
    int ids[3] = { 0, 0, 0 };
    int count = 0;
    const int ops[3] = { dst.op1, dst.op2, dst.op3 };
    for (int op : ops) {
        if (op == 0) {
            continue;
        }
        const int id = std::abs(op);
        const bool present = activeOptions.contains(id);
        if ((op > 0 && !present) || (op < 0 && present)) {
            return {};
        }
        if (op < 0) {
            continue;
        }
        bool duplicate = false;
        for (int i = 0; i < count; ++i) {
            duplicate = duplicate || ids[i] == id;
        }
        if (!duplicate && count < 3) {
            ids[count++] = id;
        }
    }

    if (count == 0) {
        return {};
    }

    std::sort(ids, ids + count);
    QVariantList result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.append(ids[i]);
    }
    return result;
}

bool selectNumberUsesFocusedState(int num) {
    return num == 42 || num == 96
        || (num >= 45 && num <= 49)
        || (num >= 70 && num <= 116)
        || num == 128
        || num == 150 || num == 152 || num == 154
        || (num >= 179 && num <= 182)
        || (num >= 200 && num <= 242)
        || num == 290 || num == 291
        || num == 300 || (num >= 320 && num <= 330)
        || (num >= 350 && num <= 368)
        || (num >= 410 && num <= 427)
        || num == 1163 || num == 1164
        || (num >= 1312 && num <= 1327);
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

QList<int> Lr2SkinRuntime::runtimeActiveOptions() const {
    return m_runtimeActiveOptions;
}

void Lr2SkinRuntime::setRuntimeActiveOptions(const QList<int>& value) {
    if (m_runtimeActiveOptions == value) {
        return;
    }
    const QSet<int> previousActiveOptionSet = m_activeOptionSet;
    const QSet<int> nextActiveOptionSet = rt::activeOptionSet(value);
    QSet<int> changedOptionIds = previousActiveOptionSet;
    changedOptionIds.unite(nextActiveOptionSet);
    QSet<int> unchangedOptionIds = previousActiveOptionSet;
    unchangedOptionIds.intersect(nextActiveOptionSet);
    changedOptionIds.subtract(unchangedOptionIds);

    m_runtimeActiveOptions = value;
    m_activeOptionSet = nextActiveOptionSet;
    if (!changedOptionIds.isEmpty()) {
        refreshActiveOptions(changedOptionIds);
    }
    emit runtimeActiveOptionsChanged();
}

Lr2SkinTimerState* Lr2SkinRuntime::timerState() const {
    return m_timerState;
}

void Lr2SkinRuntime::setTimerState(Lr2SkinTimerState* value) {
    if (m_timerState == value) {
        return;
    }
    m_timerState = value;
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

QVariantList Lr2SkinRuntime::elementDescriptors() const {
    QVariantList result;
    result.reserve(m_descriptors.size());
    for (const ElementDescriptor& descriptor : m_descriptors) {
        result.append(QVariant::fromValue(
            static_cast<const Lr2SkinElementDescriptorValue&>(descriptor)));
    }
    return result;
}

int Lr2SkinRuntime::descriptorRevision() const {
    return m_descriptorRevision;
}

QVariantList Lr2SkinRuntime::elementTimerStates() const {
    QVariantList result;
    result.reserve(m_elementTimerStates.size());
    for (Lr2SkinElementTimerState* state : m_elementTimerStates) {
        result.append(QVariant::fromValue(static_cast<QObject*>(state)));
    }
    return result;
}

QVariantList Lr2SkinRuntime::noteDstTimerFires() const {
    return m_noteDstTimerFires;
}

QVariantList Lr2SkinRuntime::lineDstTimerFires() const {
    return m_lineDstTimerFires;
}

Lr2SkinElementDescriptorValue Lr2SkinRuntime::descriptor(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? static_cast<const Lr2SkinElementDescriptorValue&>(*descriptor)
                      : Lr2SkinElementDescriptorValue {};
}

QVariantList Lr2SkinRuntime::elementActiveOptionsForElement(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    return descriptor ? descriptor->elementActiveOptions : QVariantList {};
}

Lr2SkinElementActiveOptionsState* Lr2SkinRuntime::elementActiveOptionsState(int index) const {
    return index >= 0 && index < m_elementActiveOptionsStates.size()
        ? m_elementActiveOptionsStates.at(index)
        : nullptr;
}

Lr2TimelineStateValue Lr2SkinRuntime::staticStateForElement(int index) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    if (!descriptor || !descriptor->staticState.valid || descriptor->dsts.isEmpty()) {
        return {};
    }
    if (descriptor->dstAnalysis.usesActiveOptions
            && !rt::allOpsMatch(descriptor->dsts.front(), m_activeOptionSet)) {
        return {};
    }
    return descriptor->staticState;
}

Lr2TimelineStateValue Lr2SkinRuntime::stateForElement(int index, int skinTime) const {
    const ElementDescriptor* descriptor = descriptorAt(index);
    if (!descriptor) {
        return {};
    }
    return rt::currentState(
        descriptor->dsts,
        skinTime,
        descriptor->timers.dstTimerFire,
        m_activeOptionSet);
}

Lr2TimelineStateValue Lr2SkinRuntime::sliderTrackStateForElement(int index, int skinTime) const {
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

Lr2TimelineStateValue Lr2SkinRuntime::noteDstState(int index, int skinTime) const {
    return stateForLane(m_noteLaneDescriptors, m_noteDstTimerFires, index, skinTime);
}

Lr2TimelineStateValue Lr2SkinRuntime::lineDstState(int index, int skinTime) const {
    return stateForLane(m_lineLaneDescriptors, m_lineDstTimerFires, index, skinTime);
}

Lr2TimelineStateValue Lr2SkinRuntime::noteDstStateForTimerFire(
    int index,
    int skinTime,
    int timerFire) const {
    return stateForLaneWithTimerFire(m_noteLaneDescriptors, index, skinTime, timerFire);
}

Lr2TimelineStateValue Lr2SkinRuntime::lineDstStateForTimerFire(
    int index,
    int skinTime,
    int timerFire) const {
    return stateForLaneWithTimerFire(m_lineLaneDescriptors, index, skinTime, timerFire);
}

bool Lr2SkinRuntime::noteDstStateUsesSkinTime(int index) const {
    return laneStateUsesSkinTime(m_noteLaneDescriptors, index);
}

bool Lr2SkinRuntime::lineDstStateUsesSkinTime(int index) const {
    return laneStateUsesSkinTime(m_lineLaneDescriptors, index);
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

void Lr2SkinRuntime::rebuildDescriptors() {
    const bool previousNoteFieldUsesActiveOptions = m_noteFieldUsesActiveOptions;
    m_descriptors.clear();
    m_activeOptionDescriptorIndexes.clear();
    m_activeOptionDescriptorIndexesByOption.clear();
    m_timerDescriptorIndexes.clear();
    m_selectInfoTimerDescriptorIndexes.clear();
    m_timerDescriptorIndexesByTimer.clear();
    m_noteFieldTimerIds.clear();
    m_noteLaneDescriptors.clear();
    m_lineLaneDescriptors.clear();
    m_noteFieldUsesActiveOptions = false;

    if (!m_skinModel) {
        resetElementTimerStates();
        resetElementActiveOptionsStates();
        if (m_noteFieldUsesActiveOptions != previousNoteFieldUsesActiveOptions) {
            emit noteFieldUsesActiveOptionsChanged();
        }
        notifyElementDataChanged();
        updateNoteFieldTimerFires();
        return;
    }

    const QVariantList noteDsts = modelListProperty("noteDsts");
    const QVariantList lineDsts = modelListProperty("lineDsts");
    m_noteLaneDescriptors.reserve(noteDsts.size());
    for (const QVariant& dsts : noteDsts) {
        LaneDescriptor descriptor = buildLaneDescriptor(dsts);
        if (descriptor.analysis.usesDynamicTimer && descriptor.analysis.firstTimer != 0) {
            m_noteFieldTimerIds.insert(descriptor.analysis.firstTimer);
        }
        m_noteLaneDescriptors.append(std::move(descriptor));
    }
    m_lineLaneDescriptors.reserve(lineDsts.size());
    for (const QVariant& dsts : lineDsts) {
        LaneDescriptor descriptor = buildLaneDescriptor(dsts);
        if (descriptor.analysis.usesDynamicTimer && descriptor.analysis.firstTimer != 0) {
            m_noteFieldTimerIds.insert(descriptor.analysis.firstTimer);
        }
        m_lineLaneDescriptors.append(std::move(descriptor));
    }

    const int rows = m_skinModel->rowCount();
    m_descriptors.reserve(rows);
    ensureElementTimerStateCount(rows);
    ensureElementActiveOptionsStateCount(rows);
    for (int row = 0; row < rows; ++row) {
        const int type = modelData(row, "type").toInt();
        const QVariant source = modelData(row, "src");
        const QVariantList dsts = rt::readVariantList(modelData(row, "dsts"));
        ElementDescriptor descriptor = buildDescriptor(row, type, source, dsts, noteDsts);
        const bool usesGeneralTimer = (descriptor.usesDynamicDstTimer && descriptor.dstTimer != 11)
            || (descriptor.usesDynamicSrcTimer && descriptor.srcTimer != 11);
        const bool usesSelectInfoTimer = descriptor.dstTimer == 11 || descriptor.srcTimer == 11;
        if (descriptor.dstAnalysis.usesActiveOptions && !descriptor.dsts.isEmpty()) {
            m_activeOptionDescriptorIndexes.append(row);
            for (int optionId : activeOptionIdsForDst(descriptor.dsts.front())) {
                m_activeOptionDescriptorIndexesByOption[optionId].append(row);
            }
        }
        if (usesGeneralTimer) {
            m_timerDescriptorIndexes.append(row);
            if (descriptor.usesDynamicDstTimer && descriptor.dstTimer != 11) {
                m_timerDescriptorIndexesByTimer[descriptor.dstTimer].append(row);
            }
            if (descriptor.usesDynamicSrcTimer && descriptor.srcTimer != 11) {
                m_timerDescriptorIndexesByTimer[descriptor.srcTimer].append(row);
            }
        }
        if (usesSelectInfoTimer) {
            m_selectInfoTimerDescriptorIndexes.append(row);
        }
        m_descriptors.append(std::move(descriptor));
        updateElementTimerState(row, m_descriptors.constLast().timers);
        updateElementActiveOptionsState(row,
                                        m_descriptors.constLast().elementActiveOptions,
                                        m_descriptors.constLast().elementActive);
    }
    for (int row = rows; row < m_elementTimerStates.size(); ++row) {
        updateElementTimerState(row, TimerSnapshot {});
    }
    for (int row = rows; row < m_elementActiveOptionsStates.size(); ++row) {
        updateElementActiveOptionsState(row, QVariantList {}, false);
    }

    const QVector<LaneDescriptor>* laneCollections[] = {
        &m_noteLaneDescriptors,
        &m_lineLaneDescriptors,
    };
    for (const QVector<LaneDescriptor>* collection : laneCollections) {
        for (const LaneDescriptor& descriptor : *collection) {
            m_noteFieldUsesActiveOptions = m_noteFieldUsesActiveOptions
                || descriptor.analysis.usesActiveOptions;
        }
    }

    if (m_noteFieldUsesActiveOptions != previousNoteFieldUsesActiveOptions) {
        emit noteFieldUsesActiveOptionsChanged();
    }
    notifyElementDataChanged();
    updateNoteFieldTimerFires();
}

void Lr2SkinRuntime::updateTimerFires() {
    updateTimerFiresForIndexes(m_timerDescriptorIndexes);
    updateNoteFieldTimerFires();
}

void Lr2SkinRuntime::updateGameplayTimerFires() {
    if (!m_timerState) {
        updateTimerFires();
        return;
    }

    bool fullRefresh = false;
    const QSet<int> changedTimers = m_timerState->takeCommittedGameplayTimerChanges(&fullRefresh);
    if (fullRefresh || changedTimers.isEmpty()) {
        updateTimerFires();
        return;
    }

    QSet<int> seenIndexes;
    QVector<int> indexes;
    bool noteFieldChanged = false;
    for (int timer : changedTimers) {
        noteFieldChanged = noteFieldChanged || m_noteFieldTimerIds.contains(timer);
        const QVector<int> timerIndexes = m_timerDescriptorIndexesByTimer.value(timer);
        for (int index : timerIndexes) {
            if (seenIndexes.contains(index)) {
                continue;
            }
            seenIndexes.insert(index);
            indexes.append(index);
        }
    }
    updateTimerFiresForIndexes(indexes);
    if (noteFieldChanged) {
        updateNoteFieldTimerFires();
    }
}

void Lr2SkinRuntime::updateSelectInfoTimerFires() {
    updateTimerFiresForIndexes(m_selectInfoTimerDescriptorIndexes);
    if (m_noteFieldTimerIds.contains(11)) {
        updateNoteFieldTimerFires();
    }
}

void Lr2SkinRuntime::updateTimerFiresForIndexes(const QVector<int>& indexes) {
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
            descriptor.timers = next;
            updateElementTimerState(index, descriptor.timers);
        }
    }
}

void Lr2SkinRuntime::ensureElementTimerStateCount(int count) {
    while (m_elementTimerStates.size() < count) {
        m_elementTimerStates.append(new Lr2SkinElementTimerState(this));
    }
}

void Lr2SkinRuntime::ensureElementActiveOptionsStateCount(int count) {
    while (m_elementActiveOptionsStates.size() < count) {
        m_elementActiveOptionsStates.append(new Lr2SkinElementActiveOptionsState(this));
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

bool Lr2SkinRuntime::updateElementActiveOptionsState(int index,
                                                    const QVariantList& activeOptions,
                                                    bool active) {
    if (index < 0) {
        return false;
    }
    ensureElementActiveOptionsStateCount(index + 1);
    return m_elementActiveOptionsStates[index]->setActiveOptions(activeOptions, active);
}

void Lr2SkinRuntime::resetElementTimerStates() {
    for (auto* timerState : std::as_const(m_elementTimerStates)) {
        if (timerState) {
            timerState->setSnapshot(false, -1, false, -1);
        }
    }
}

void Lr2SkinRuntime::resetElementActiveOptionsStates() {
    for (auto* activeOptionsState : std::as_const(m_elementActiveOptionsStates)) {
        if (activeOptionsState) {
            activeOptionsState->setActiveOptions(QVariantList {}, false);
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

Lr2SkinRuntime::LaneDescriptor Lr2SkinRuntime::buildLaneDescriptor(const QVariant& dsts) const {
    LaneDescriptor descriptor;
    descriptor.dsts = rt::readDsts(dsts);
    descriptor.analysis = rt::analyzeDsts(descriptor.dsts);
    descriptor.staticState = descriptor.analysis.canUseStaticState && !descriptor.dsts.isEmpty()
        ? rt::copyDstAsState(descriptor.dsts.front(), descriptor.dsts.front())
        : Lr2TimelineStateValue {};
    return descriptor;
}

Lr2SkinRuntime::ElementDescriptor Lr2SkinRuntime::buildDescriptor(
    int index,
    int type,
    const QVariant& sourceValue,
    const QVariantList& dstValues,
    const QVariantList& noteDsts) const {
    ElementDescriptor descriptor;
    descriptor.valid = true;
    descriptor.index = index;
    descriptor.type = type;
    rt::readSource(sourceValue, descriptor.source);
    descriptor.dsts = rt::readDsts(dstValues);
    descriptor.dstAnalysis = rt::analyzeDsts(descriptor.dsts);
    descriptor.usesActiveOptions = descriptor.dstAnalysis.usesActiveOptions;

    const bool selectScreen = m_screenKey == QStringLiteral("select");
    const bool sourceCycles = rt::sourceCyclesContinuously(descriptor.source);
    const bool selectInfoDstTimer = selectScreen && descriptor.dstAnalysis.firstTimer == 11;
    const bool selectInfoSrcTimer = selectScreen && descriptor.source.valid && descriptor.source.timer == 11;
    const bool selectPanelTimer = (descriptor.dstAnalysis.firstTimer >= 21 && descriptor.dstAnalysis.firstTimer <= 26)
        || (descriptor.dstAnalysis.firstTimer >= 31 && descriptor.dstAnalysis.firstTimer <= 36);
    const bool selectHeldDstTimer = selectScreen && m_timerState
        && m_timerState->isSelectHeldButtonTimer(descriptor.dstAnalysis.firstTimer);
    const bool selectHeldSrcTimer = selectScreen && m_timerState
        && descriptor.source.valid
        && m_timerState->isSelectHeldButtonTimer(descriptor.source.timer);
    constexpr int selectIntroClockLimit = 3200;
    const bool selectDelayedPersistentDst = selectScreen
        && !descriptor.dsts.isEmpty()
        && descriptor.dsts.front().valid
        && descriptor.dstAnalysis.firstTimer == 0
        && descriptor.dsts.front().loop >= 0
        && descriptor.dsts.front().loop >= descriptor.dsts.back().time
        && descriptor.dsts.back().time > selectIntroClockLimit;

    const bool usesElementActiveOptions =
        descriptor.dstAnalysis.usesActiveOptions && !descriptor.dsts.isEmpty();
    descriptor.elementActive = !usesElementActiveOptions
        || rt::allOpsMatch(descriptor.dsts.front(), m_activeOptionSet);
    descriptor.elementActiveOptions = usesElementActiveOptions && descriptor.elementActive
        ? activeOptionsForDst(descriptor.dsts.front(), m_activeOptionSet)
        : QVariantList {};
    descriptor.staticState = descriptor.dstAnalysis.canUseStaticState && !descriptor.dsts.isEmpty()
        ? rt::copyDstAsState(descriptor.dsts.front(), descriptor.dsts.front())
        : Lr2TimelineStateValue {};
    descriptor.sourceHasFrameAnimation = sourceCycles;
    descriptor.sourceTreeHasFrameAnimation = rt::sourceTreeCyclesContinuously(sourceValue);
    descriptor.sourceTreeUsesChartAsset = rt::sourceTreeUsesChartAsset(sourceValue);
    descriptor.directChartAssetSourceType = rt::chartAssetSourceType(descriptor.source);
    descriptor.barDistributionGraphSourceHasFrameAnimation =
        sourceHasBarDistributionGraphAnimation(descriptor.source);
    descriptor.usesLiveDstClock = selectScreen
        && (selectPanelTimer
            || selectHeldDstTimer
            || descriptor.dstAnalysis.loopsContinuously
            || selectDelayedPersistentDst);
    descriptor.usesLiveSourceClock = selectScreen && (sourceCycles || selectHeldSrcTimer);
    descriptor.usesLiveSelectClock = descriptor.usesLiveDstClock || descriptor.usesLiveSourceClock;
    descriptor.usesSelectHeldButtonTimer = selectHeldDstTimer || selectHeldSrcTimer;
    descriptor.usesDstSkinTime = !descriptor.dstAnalysis.canUseStaticState;
    descriptor.usesSkinTime = descriptor.usesDstSkinTime
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
    descriptor.sourceTextId = descriptor.source.valid ? descriptor.source.st : -1;
    descriptor.usesDynamicDstTimer = descriptor.dstTimer != 0;
    descriptor.usesDynamicSrcTimer = descriptor.srcTimer != 0;
    descriptor.selectScrollSlider = descriptor.spriteStateOverrideKind == rt::SelectScrollSpriteStateOverride;
    descriptor.genericSlider = descriptor.spriteStateOverrideKind == rt::GenericSliderSpriteStateOverride;
    descriptor.gameplayProgressSlider = descriptor.spriteStateOverrideKind == rt::GameplayProgressSpriteStateOverride;
    descriptor.gameplayLaneCoverSlider = descriptor.spriteStateOverrideKind == rt::GameplayLaneCoverSpriteStateOverride;
    descriptor.numberRefSlider = descriptor.spriteStateOverrideKind == rt::NumberRefSpriteStateOverride;
    descriptor.buttonId = descriptor.source.buttonId;
    descriptor.numberUsesFocusedSelectState = selectNumberUsesFocusedState(descriptor.source.num);

    if (rt::isSelectBarElement(type, descriptor.source)) {
        descriptor.z = descriptor.dstAnalysis.firstSortId
            + rt::selectBarElementLayer(type, descriptor.source) * 0.001
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

Lr2TimelineStateValue Lr2SkinRuntime::stateForLane(const QVector<LaneDescriptor>& lanes,
                                                   const QVariantList& timerFires,
                                                   int index,
                                                   int skinTime) const {
    const int timerFire = index >= 0 && index < timerFires.size()
        ? timerFires.at(index).toInt()
        : -1;
    return stateForLaneWithTimerFire(lanes, index, skinTime, timerFire);
}

Lr2TimelineStateValue Lr2SkinRuntime::stateForLaneWithTimerFire(
    const QVector<LaneDescriptor>& lanes,
    int index,
    int skinTime,
    int timerFire) const {
    if (index < 0 || index >= lanes.size()) {
        return {};
    }

    const LaneDescriptor& lane = lanes.at(index);
    if (lane.staticState.valid
            && (!lane.analysis.usesActiveOptions || rt::allOpsMatch(lane.dsts.front(), m_activeOptionSet))) {
        return lane.staticState;
    }

    if (timerFire < 0 && lane.analysis.firstTimer == 0) {
        timerFire = 0;
    }

    return rt::currentState(
        lane.dsts,
        skinTime,
        timerFire,
        m_activeOptionSet);
}

bool Lr2SkinRuntime::laneStateUsesSkinTime(const QVector<LaneDescriptor>& lanes, int index) const {
    if (index < 0 || index >= lanes.size()) {
        return false;
    }

    const LaneDescriptor& lane = lanes.at(index);
    return !(lane.staticState.valid
        && (!lane.analysis.usesActiveOptions || rt::allOpsMatch(lane.dsts.front(), m_activeOptionSet)));
}

int Lr2SkinRuntime::laneTimerFire(const LaneDescriptor& lane) const {
    const int timer = lane.analysis.firstTimer;
    if (timer == 0) {
        return 0;
    }
    if (!m_timerState || !m_timerState->skinTimerCanFire(timer)) {
        return -1;
    }
    return m_timerState->skinTimerFireTime(timer, false);
}

QVariantList Lr2SkinRuntime::timerFiresForLanes(const QVector<LaneDescriptor>& lanes) const {
    QVariantList result;
    result.reserve(lanes.size());
    for (const LaneDescriptor& lane : lanes) {
        result.append(laneTimerFire(lane));
    }
    return result;
}

void Lr2SkinRuntime::updateNoteFieldTimerFires() {
    const QVariantList nextNoteTimerFires = timerFiresForLanes(m_noteLaneDescriptors);
    if (m_noteDstTimerFires != nextNoteTimerFires) {
        m_noteDstTimerFires = nextNoteTimerFires;
        emit noteDstTimerFiresChanged();
    }

    const QVariantList nextLineTimerFires = timerFiresForLanes(m_lineLaneDescriptors);
    if (m_lineDstTimerFires != nextLineTimerFires) {
        m_lineDstTimerFires = nextLineTimerFires;
        emit lineDstTimerFiresChanged();
    }
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
        &Lr2SkinTimerState::timerFireTimesChanged,
        this,
        &Lr2SkinRuntime::updateTimerFires));
    m_timerStateConnections.append(QObject::connect(
        m_timerState,
        &Lr2SkinTimerState::gameplayTimerValuesCommitted,
        this,
        &Lr2SkinRuntime::updateGameplayTimerFires));
    m_timerStateConnections.append(QObject::connect(
        m_timerState,
        &Lr2SkinTimerState::selectInfoTimerFireTimesChanged,
        this,
        &Lr2SkinRuntime::updateSelectInfoTimerFires));
}

void Lr2SkinRuntime::refreshActiveOptions(const QSet<int>& changedOptionIds) {
    bool anyChanged = false;
    QVector<int> indexes;
    if (changedOptionIds.isEmpty()) {
        indexes = m_activeOptionDescriptorIndexes;
    } else {
        QSet<int> seenIndexes;
        for (int optionId : changedOptionIds) {
            const QVector<int> optionIndexes = m_activeOptionDescriptorIndexesByOption.value(optionId);
            for (int index : optionIndexes) {
                if (seenIndexes.contains(index)) {
                    continue;
                }
                seenIndexes.insert(index);
                indexes.append(index);
            }
        }
    }

    for (int index : std::as_const(indexes)) {
        if (index < 0 || index >= m_descriptors.size()) {
            continue;
        }
        ElementDescriptor& descriptor = m_descriptors[index];
        const bool usesElementActiveOptions =
            descriptor.dstAnalysis.usesActiveOptions && !descriptor.dsts.isEmpty();
        descriptor.elementActive = !usesElementActiveOptions
            || rt::allOpsMatch(descriptor.dsts.front(), m_activeOptionSet);
        descriptor.elementActiveOptions = usesElementActiveOptions && descriptor.elementActive
            ? activeOptionsForDst(descriptor.dsts.front(), m_activeOptionSet)
            : QVariantList {};
        anyChanged = updateElementActiveOptionsState(descriptor.index,
                                                    descriptor.elementActiveOptions,
                                                    descriptor.elementActive)
            || anyChanged;
    }
    if (!anyChanged) {
        return;
    }
}

void Lr2SkinRuntime::notifyElementDataChanged() {
    ++m_descriptorRevision;
    emit elementDescriptorsChanged();
    emit elementTimerStatesChanged();
}
