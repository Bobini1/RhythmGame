#include "Lr2SelectUpdateController.h"

#include "Lr2SkinOptionRules.h"
#include "Lr2SkinRuntime.h"

#include <cstdlib>
#include <QStringList>

Lr2SelectUpdateController::Lr2SelectUpdateController(QObject* parent)
    : QObject(parent) {}

gameplay_logic::lr2_skin::Lr2SkinModel* Lr2SelectUpdateController::skinModel() const {
    return m_skinModel;
}

void Lr2SelectUpdateController::setSkinModel(gameplay_logic::lr2_skin::Lr2SkinModel* model) {
    if (m_skinModel == model) {
        return;
    }
    if (m_skinMetadataConnection) {
        QObject::disconnect(m_skinMetadataConnection);
        m_skinMetadataConnection = {};
    }
    m_skinModel = model;
    if (m_skinModel) {
        m_skinMetadataConnection = QObject::connect(
            m_skinModel,
            SIGNAL(skinMetadataChanged()),
            this,
            SLOT(refreshSkinOptionUsage()));
    }
    refreshSkinOptionUsage();
    emit skinModelChanged();
}

Lr2SkinRuntime* Lr2SelectUpdateController::skinRuntime() const {
    return m_skinRuntime;
}

void Lr2SelectUpdateController::setSkinRuntime(Lr2SkinRuntime* runtime) {
    if (m_skinRuntime == runtime) {
        return;
    }
    m_skinRuntime = runtime;
    emit skinRuntimeChanged();
    applyRuntimeActiveOptions(m_runtimeActiveOptions);
}

bool Lr2SelectUpdateController::screenUpdatesActive() const { return m_screenUpdatesActive; }
void Lr2SelectUpdateController::setScreenUpdatesActive(bool active) {
    if (m_screenUpdatesActive == active) {
        return;
    }
    m_screenUpdatesActive = active;
    emit screenUpdatesActiveChanged();
    refreshCurrentRuntimeActiveOptions();
}

QString Lr2SelectUpdateController::effectiveScreenKey() const { return m_effectiveScreenKey; }
void Lr2SelectUpdateController::setEffectiveScreenKey(const QString& key) {
    if (m_effectiveScreenKey == key) {
        return;
    }
    m_effectiveScreenKey = key;
    emit effectiveScreenKeyChanged();
    refreshCurrentRuntimeActiveOptions();
}

bool Lr2SelectUpdateController::componentReady() const { return m_componentReady; }
void Lr2SelectUpdateController::setComponentReady(bool ready) {
    if (m_componentReady == ready) {
        return;
    }
    m_componentReady = ready;
    emit componentReadyChanged();
    refreshCurrentRuntimeActiveOptions();
}

bool Lr2SelectUpdateController::rankingMode() const { return m_rankingMode; }
void Lr2SelectUpdateController::setRankingMode(bool active) {
    if (m_rankingMode == active) {
        return;
    }
    m_rankingMode = active;
    emit rankingModeChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

int Lr2SelectUpdateController::selectPanel() const { return m_selectPanel; }
void Lr2SelectUpdateController::setSelectPanel(int panel) {
    if (m_selectPanel == panel) {
        return;
    }
    m_selectPanel = panel;
    emit selectPanelChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::parseActiveOptions() const { return m_parseActiveOptions; }
void Lr2SelectUpdateController::setParseActiveOptions(const QList<int>& options) {
    if (m_parseActiveOptions == options) {
        return;
    }
    m_parseActiveOptions = options;
    emit parseActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::runtimeActiveOptions() const { return m_runtimeActiveOptions; }
void Lr2SelectUpdateController::setRuntimeActiveOptions(const QList<int>& options) {
    if (m_runtimeActiveOptions == options) {
        return;
    }
    m_runtimeActiveOptions = options;
    emit runtimeActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::barActiveOptions() const { return m_barActiveOptions; }
void Lr2SelectUpdateController::setBarActiveOptions(const QList<int>& options) {
    if (m_barActiveOptions == options) {
        return;
    }
    m_barActiveOptions = options;
    emit barActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::baseActiveOptions() const { return m_baseActiveOptions; }
void Lr2SelectUpdateController::setBaseActiveOptions(const QList<int>& options) {
    if (m_baseActiveOptions == options) {
        return;
    }
    m_baseActiveOptions = options;
    emit baseActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::selectCommonActiveOptions() const { return m_selectCommonActiveOptions; }
void Lr2SelectUpdateController::setSelectCommonActiveOptions(const QList<int>& options) {
    if (m_selectCommonActiveOptions == options) {
        return;
    }
    m_selectCommonActiveOptions = options;
    emit selectCommonActiveOptionsChanged();
}

bool Lr2SelectUpdateController::selectCommonActiveOptionsReady() const { return m_selectCommonActiveOptionsReady; }
void Lr2SelectUpdateController::setSelectCommonActiveOptionsReady(bool ready) {
    if (m_selectCommonActiveOptionsReady == ready) {
        return;
    }
    m_selectCommonActiveOptionsReady = ready;
    emit selectCommonActiveOptionsReadyChanged();
}

QList<int> Lr2SelectUpdateController::selectRuntimeActiveOptions() const { return m_selectRuntimeActiveOptions; }
void Lr2SelectUpdateController::setSelectRuntimeActiveOptions(const QList<int>& options) {
    if (m_selectRuntimeActiveOptions == options) {
        return;
    }
    m_selectRuntimeActiveOptions = options;
    emit selectRuntimeActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::gameplayRuntimeActiveOptions() const { return m_gameplayRuntimeActiveOptions; }
void Lr2SelectUpdateController::setGameplayRuntimeActiveOptions(const QList<int>& options) {
    if (m_gameplayRuntimeActiveOptions == options) {
        return;
    }
    m_gameplayRuntimeActiveOptions = options;
    emit gameplayRuntimeActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::barRuntimeActiveOptions() const {
    return m_barRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setBarRuntimeActiveOptions(const QList<int>& options) {
    if (m_barRuntimeActiveOptions == options) {
        return;
    }
    m_barRuntimeActiveOptions = options;
    emit barRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::baseRuntimeActiveOptions() const {
    return m_baseRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setBaseRuntimeActiveOptions(const QList<int>& options) {
    if (m_baseRuntimeActiveOptions == options) {
        return;
    }
    m_baseRuntimeActiveOptions = options;
    emit baseRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::selectCommonRuntimeActiveOptions() const {
    return m_selectCommonRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setSelectCommonRuntimeActiveOptions(const QList<int>& options) {
    if (m_selectCommonRuntimeActiveOptions == options) {
        return;
    }
    m_selectCommonRuntimeActiveOptions = options;
    emit selectCommonRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::selectRequiredRuntimeActiveOptions() const {
    return m_selectRequiredRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setSelectRequiredRuntimeActiveOptions(const QList<int>& options) {
    if (m_selectRequiredRuntimeActiveOptions == options) {
        return;
    }
    m_selectRequiredRuntimeActiveOptions = options;
    emit selectRequiredRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QList<int> Lr2SelectUpdateController::selectRuntimeGeneratedActiveOptions() const {
    return m_selectRuntimeGeneratedActiveOptions;
}

void Lr2SelectUpdateController::setSelectRuntimeGeneratedActiveOptions(const QList<int>& options) {
    if (m_selectRuntimeGeneratedActiveOptions == options) {
        return;
    }
    m_selectRuntimeGeneratedActiveOptions = options;
    emit selectRuntimeGeneratedActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::selectDetailRuntimeActiveOptions() const {
    return m_selectDetailRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setSelectDetailRuntimeActiveOptions(const QList<int>& options) {
    if (m_selectDetailRuntimeActiveOptions == options) {
        return;
    }
    m_selectDetailRuntimeActiveOptions = options;
    emit selectDetailRuntimeActiveOptionsChanged();
}

QList<int> Lr2SelectUpdateController::screenRuntimeActiveOptions() const {
    return m_screenRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setScreenRuntimeActiveOptions(const QList<int>& options) {
    if (m_screenRuntimeActiveOptions == options) {
        return;
    }
    m_screenRuntimeActiveOptions = options;
    emit screenRuntimeActiveOptionsChanged();
    refreshCurrentRuntimeActiveOptions();
}

bool Lr2SelectUpdateController::gameplayScreen() const {
    return m_gameplayScreen;
}

void Lr2SelectUpdateController::setGameplayScreen(bool active) {
    if (m_gameplayScreen == active) {
        return;
    }
    m_gameplayScreen = active;
    emit gameplayScreenChanged();
    refreshCurrentRuntimeActiveOptions();
}

bool Lr2SelectUpdateController::selectReplayOptionsUsed() const {
    return m_selectReplayOptionsUsed;
}

bool Lr2SelectUpdateController::selectScoreOptionIdsUsed() const {
    return m_selectScoreOptionIdsUsed;
}

bool Lr2SelectUpdateController::selectEntryStatusOptionsUsed() const {
    return m_selectEntryStatusOptionsUsed;
}

bool Lr2SelectUpdateController::selectDifficultyBarOptionsUsed() const {
    return m_selectDifficultyBarOptionsUsed;
}

bool Lr2SelectUpdateController::selectDifficultyLampOptionsUsed() const {
    return m_selectDifficultyLampOptionsUsed;
}

bool Lr2SelectUpdateController::selectDifficultyStateUsed() const {
    return m_selectDifficultyStateUsed;
}

bool Lr2SelectUpdateController::selectCourseDetailOptionsUsed() const {
    return m_selectCourseDetailOptionsUsed;
}

bool Lr2SelectUpdateController::selectRankingStatusOptionsUsed() const {
    return m_selectRankingStatusOptionsUsed;
}

bool Lr2SelectUpdateController::refreshBaseActiveOptions() {
    const QList<int> nextBar = mergedNumberArray(
        selectStaticOptions(true, false),
        m_barRuntimeActiveOptions);
    const bool barChanged = m_barActiveOptions != nextBar;
    if (barChanged) {
        setBarActiveOptions(nextBar);
    }

    const QList<int> nextBase = mergedNumberArray(
        mergedNumberArray(nextBar, selectStaticOptions(false, true)),
        m_baseRuntimeActiveOptions);
    const bool baseChanged = m_baseActiveOptions != nextBase;
    if (baseChanged) {
        setBaseActiveOptions(nextBase);
    }

    const QList<int> nextSelectCommonBase = mergedNumberArray(
        nextBase,
        m_selectCommonRuntimeActiveOptions);
    const QList<int> nextSelectCommon = mergedNumberArray(
        nextSelectCommonBase,
        m_selectRequiredRuntimeActiveOptions);
    const bool selectCommonChanged = m_selectCommonActiveOptions != nextSelectCommon;
    setSelectCommonActiveOptionsReady(true);
    if (selectCommonChanged) {
        setSelectCommonActiveOptions(nextSelectCommon);
    }

    return barChanged || baseChanged || selectCommonChanged;
}

bool Lr2SelectUpdateController::refreshSelectRuntimeActiveOptions() {
    if (!m_screenUpdatesActive) {
        return false;
    }
    if (m_effectiveScreenKey != QStringLiteral("select")) {
        const QList<int> next = mergedNumberArray(
            m_baseActiveOptions,
            m_screenRuntimeActiveOptions);
        if (m_runtimeActiveOptions == next) {
            return false;
        }
        applyRuntimeActiveOptions(next);
        return true;
    }
    if (!m_selectCommonActiveOptionsReady) {
        refreshBaseActiveOptions();
    }

    QList<int> next = m_selectCommonActiveOptions;
    next.reserve(m_selectCommonActiveOptions.size()
        + m_selectRuntimeGeneratedActiveOptions.size()
        + m_selectDetailRuntimeActiveOptions.size());
    for (int value : m_selectRuntimeGeneratedActiveOptions) {
        appendUniqueOption(next, value);
    }
    for (int value : m_selectDetailRuntimeActiveOptions) {
        appendUniqueOption(next, value);
    }
    if (m_runtimeActiveOptions == next) {
        return false;
    }
    setSelectRuntimeActiveOptions(next);
    applyRuntimeActiveOptions(next);
    return true;
}

bool Lr2SelectUpdateController::refreshGameplayRuntimeActiveOptions() {
    if (!m_gameplayScreen) {
        return false;
    }

    const QList<int> next = mergedNumberArray(
        m_baseActiveOptions,
        m_screenRuntimeActiveOptions);
    if (m_runtimeActiveOptions == next) {
        return false;
    }
    setGameplayRuntimeActiveOptions(next);
    applyRuntimeActiveOptions(next);
    return true;
}

void Lr2SelectUpdateController::refreshCurrentRuntimeActiveOptions() {
    if (!m_screenUpdatesActive) {
        return;
    }
    refreshSelectRuntimeActiveOptions();
    refreshGameplayRuntimeActiveOptions();
}

void Lr2SelectUpdateController::applyRuntimeActiveOptions(const QList<int>& value) {
    setRuntimeActiveOptions(value);
    if (m_skinRuntime) {
        m_skinRuntime->setRuntimeActiveOptions(value);
    }
}

QList<int> Lr2SelectUpdateController::mergedNumberArray(const QList<int>& first, const QList<int>& second) const {
    if (first.isEmpty()) {
        return second;
    }
    if (second.isEmpty()) {
        return first;
    }

    QList<int> result = first;
    result.reserve(first.size() + second.size());
    for (int value : second) {
        appendUniqueOption(result, value);
    }
    return result;
}
QList<int> Lr2SelectUpdateController::selectStaticOptions(
    bool includeRankingOption,
    bool includePanelOption) const {
    QList<int> result;
    const QList<int> source = m_effectiveSkinActiveOptions.isEmpty()
        ? m_parseActiveOptions
        : m_effectiveSkinActiveOptions;
    for (int option : source) {
        if (!Lr2SkinOptionRules::isRuntimeOwnedOptionValue(option)) {
            appendUniqueSkinOption(result, option);
        }
    }

    appendDefaultOptionIfMissing(result, { 910, 911, 912, 913 }, 910);
    appendDefaultOptionIfMissing(result, { 920, 921, 922 }, 920);

    appendUniqueSkinOption(result, 20);
    appendUniqueSkinOption(result, 46);
    appendUniqueSkinOption(result, 52);
    appendUniqueSkinOption(result, 572);
    appendUniqueSkinOption(result, 622);
    appendUniqueSkinOption(result, 624);

    if (includeRankingOption) {
        appendUniqueSkinOption(result, m_rankingMode ? 621 : 620);
    }
    if (includePanelOption) {
        appendUniqueSkinOption(result, m_selectPanel > 0 ? 20 + m_selectPanel : 20);
    }
    return result;
}

bool Lr2SelectUpdateController::skinUsesOption(int option) const {
    if (!m_skinOptionFilterActive) {
        return true;
    }
    return m_usedSkinOptions.contains(std::abs(option));
}

bool Lr2SelectUpdateController::skinUsesAnyOption(std::initializer_list<int> options) const {
    if (!m_skinOptionFilterActive) {
        return true;
    }
    for (int option : options) {
        if (m_usedSkinOptions.contains(std::abs(option))) {
            return true;
        }
    }
    return false;
}

bool Lr2SelectUpdateController::skinUsesOptionRange(int first, int last) const {
    if (!m_skinOptionFilterActive) {
        return true;
    }
    for (int option = first; option <= last; ++option) {
        if (m_usedSkinOptions.contains(std::abs(option))) {
            return true;
        }
    }
    return false;
}

bool Lr2SelectUpdateController::skinUsesAnySelectElementOption(std::initializer_list<int> options) const {
    if (!m_selectElementOptionsAvailable) {
        return skinUsesAnyOption(options);
    }
    for (int option : options) {
        if (m_usedSelectElementOptions.contains(std::abs(option))) {
            return true;
        }
    }
    return false;
}

bool Lr2SelectUpdateController::skinUsesSelectElementOptionRange(int first, int last) const {
    if (!m_selectElementOptionsAvailable) {
        return skinUsesOptionRange(first, last);
    }
    for (int option = first; option <= last; ++option) {
        if (m_usedSelectElementOptions.contains(std::abs(option))) {
            return true;
        }
    }
    return false;
}

void Lr2SelectUpdateController::refreshSkinOptionUsage() {
    auto* model = m_skinModel.data();
    const QVariantList usedOptionsValue = model ? model->usedOptions() : QVariantList {};
    const QVariantList usedElementOptionsValue = model ? model->usedElementOptions() : QVariantList {};
    const QList<int> nextEffectiveSkinActiveOptions = intList(
        model ? model->effectiveActiveOptions() : QVariantList {});
    const bool nextSkinUsesSelectDifficultySource = model && model->usesSelectDifficultySource();

    QSet<int> nextUsedSkinOptions;
    for (int option : intList(usedOptionsValue)) {
        nextUsedSkinOptions.insert(std::abs(option));
    }

    QSet<int> nextUsedSelectElementOptions;
    for (int option : intList(usedElementOptionsValue)) {
        const int id = std::abs(option);
        nextUsedSkinOptions.insert(id);
        nextUsedSelectElementOptions.insert(id);
    }

    const bool nextSkinOptionFilterActive = !nextUsedSkinOptions.isEmpty();
    const bool nextSelectElementOptionsAvailable = model != nullptr;
    const bool metadataChanged = m_usedSkinOptions != nextUsedSkinOptions
        || m_usedSelectElementOptions != nextUsedSelectElementOptions
        || m_effectiveSkinActiveOptions != nextEffectiveSkinActiveOptions
        || m_skinUsesSelectDifficultySource != nextSkinUsesSelectDifficultySource
        || m_skinOptionFilterActive != nextSkinOptionFilterActive
        || m_selectElementOptionsAvailable != nextSelectElementOptionsAvailable;
    if (!metadataChanged) {
        return;
    }

    m_usedSkinOptions = nextUsedSkinOptions;
    m_usedSelectElementOptions = nextUsedSelectElementOptions;
    m_effectiveSkinActiveOptions = nextEffectiveSkinActiveOptions;
    m_skinUsesSelectDifficultySource = nextSkinUsesSelectDifficultySource;
    m_skinOptionFilterActive = nextSkinOptionFilterActive;
    m_selectElementOptionsAvailable = nextSelectElementOptionsAvailable;

    const bool nextReplayOptionsUsed = skinUsesAnySelectElementOption({
        196, 197, 1196, 1197, 1199, 1200,
        1202, 1203, 1205, 1206, 1207, 1208
    });
    const bool nextScoreOptionIdsUsed = skinUsesSelectElementOptionRange(118, 130)
        || skinUsesAnySelectElementOption({ 144, 145, 1128 });
    const bool nextEntryStatusOptionsUsed = skinUsesSelectElementOptionRange(100, 130)
        || skinUsesSelectElementOptionRange(200, 207)
        || skinUsesAnySelectElementOption({ 1100, 1101, 1102, 1103, 1104 });
    const bool nextDifficultyBarOptionsUsed = skinUsesSelectElementOptionRange(70, 79)
        || skinUsesSelectElementOptionRange(500, 570);
    const bool nextDifficultyLampOptionsUsed = skinUsesSelectElementOptionRange(520, 570);
    const bool nextDifficultyStateUsed = nextDifficultyBarOptionsUsed
        || m_skinUsesSelectDifficultySource;
    const bool nextCourseDetailOptionsUsed = skinUsesAnySelectElementOption({ 290, 293 })
        || skinUsesSelectElementOptionRange(580, 589)
        || skinUsesSelectElementOptionRange(700, 755);
    const bool nextRankingStatusOptionsUsed = skinUsesSelectElementOptionRange(600, 616);

    bool usageChanged = false;
    const auto assignUsage = [&usageChanged](bool& current, bool next) {
        if (current == next) {
            return;
        }
        current = next;
        usageChanged = true;
    };

    assignUsage(m_selectReplayOptionsUsed, nextReplayOptionsUsed);
    assignUsage(m_selectScoreOptionIdsUsed, nextScoreOptionIdsUsed);
    assignUsage(m_selectEntryStatusOptionsUsed, nextEntryStatusOptionsUsed);
    assignUsage(m_selectDifficultyBarOptionsUsed, nextDifficultyBarOptionsUsed);
    assignUsage(m_selectDifficultyLampOptionsUsed, nextDifficultyLampOptionsUsed);
    assignUsage(m_selectDifficultyStateUsed, nextDifficultyStateUsed);
    assignUsage(m_selectCourseDetailOptionsUsed, nextCourseDetailOptionsUsed);
    assignUsage(m_selectRankingStatusOptionsUsed, nextRankingStatusOptionsUsed);

    if (usageChanged) {
        emit selectOptionUsageChanged();
    }

    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

void Lr2SelectUpdateController::appendUniqueOption(QList<int>& options, int option) const {
    for (int existing : options) {
        if (existing == option) {
            return;
        }
    }
    options.append(option);
}

void Lr2SelectUpdateController::appendUniqueSkinOption(QList<int>& options, int option) const {
    if (!skinUsesOption(option)) {
        return;
    }
    appendUniqueOption(options, option);
}

void Lr2SelectUpdateController::appendDefaultOptionIfMissing(
    QList<int>& options,
    const QList<int>& choices,
    int fallback) const {
    bool anyUsed = false;
    for (int choice : choices) {
        if (skinUsesOption(choice)) {
            anyUsed = true;
            break;
        }
    }
    if (!anyUsed) {
        return;
    }

    for (int choice : choices) {
        for (int option : options) {
            if (std::abs(option) == std::abs(choice)) {
                return;
            }
        }
    }
    appendUniqueSkinOption(options, fallback);
}

QList<int> Lr2SelectUpdateController::intList(const QVariantList& values) const {
    QList<int> result;
    result.reserve(values.size());
    for (const QVariant& value : values) {
        result.append(value.toInt());
    }
    return result;
}
