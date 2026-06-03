#include "Lr2SelectUpdateController.h"

#include "Lr2SkinOptionRules.h"
#include "Lr2SkinRuntime.h"

#include <cstdlib>
#include <QJSValue>
#include <QStringList>

Lr2SelectUpdateController::Lr2SelectUpdateController(QObject* parent)
    : QObject(parent) {}

namespace {

int keymodeOptionFor(int keymode, int baseOption) {
    switch (keymode) {
    case 7:
        return baseOption;
    case 5:
        return baseOption + 1;
    case 14:
        return baseOption + 2;
    case 10:
        return baseOption + 3;
    case 9:
        return baseOption + 4;
    case 24:
        return 1160;
    case 48:
        return 1161;
    default:
        return 0;
    }
}

}

QObject* Lr2SelectUpdateController::skinModel() const {
    return m_skinModel;
}

void Lr2SelectUpdateController::setSkinModel(QObject* model) {
    if (m_skinModel == model) {
        return;
    }
    m_skinModel = model;
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
}

int Lr2SelectUpdateController::scoreRevision() const { return m_scoreRevision; }
void Lr2SelectUpdateController::setScoreRevision(int revision) {
    if (m_scoreRevision == revision) {
        return;
    }
    m_scoreRevision = revision;
    emit scoreRevisionChanged();
    updateSelectRevision(true);
}

int Lr2SelectUpdateController::focusRevision() const { return m_focusRevision; }
void Lr2SelectUpdateController::setFocusRevision(int revision) {
    if (m_focusRevision == revision) {
        return;
    }
    m_focusRevision = revision;
    emit focusRevisionChanged();
    updateSelectRevision(true);
}

int Lr2SelectUpdateController::selectPanel() const { return m_selectPanel; }
void Lr2SelectUpdateController::setSelectPanel(int panel) {
    if (m_selectPanel == panel) {
        return;
    }
    m_selectPanel = panel;
    emit selectPanelChanged();
    if (refreshBaseActiveOptions()) {
        refreshSelectRuntimeActiveOptions();
    }
}

QString Lr2SelectUpdateController::lr2RankingMd5() const { return m_lr2RankingMd5; }
void Lr2SelectUpdateController::setLr2RankingMd5(const QString& md5) {
    if (m_lr2RankingMd5 == md5) {
        return;
    }
    m_lr2RankingMd5 = md5;
    emit lr2RankingMd5Changed();
}

QString Lr2SelectUpdateController::lr2RankingRequestMd5() const { return m_lr2RankingRequestMd5; }
void Lr2SelectUpdateController::setLr2RankingRequestMd5(const QString& md5) {
    if (m_lr2RankingRequestMd5 == md5) {
        return;
    }
    m_lr2RankingRequestMd5 = md5;
    emit lr2RankingRequestMd5Changed();
}

QVariant Lr2SelectUpdateController::parseActiveOptions() const { return m_parseActiveOptions; }
void Lr2SelectUpdateController::setParseActiveOptions(const QVariant& options) {
    if (m_parseActiveOptions == options) {
        return;
    }
    m_parseActiveOptions = options;
    emit parseActiveOptionsChanged();
}

QVariant Lr2SelectUpdateController::runtimeActiveOptions() const { return m_runtimeActiveOptions; }
void Lr2SelectUpdateController::setRuntimeActiveOptions(const QVariant& options) {
    if (m_runtimeActiveOptions == options) {
        return;
    }
    m_runtimeActiveOptions = options;
    emit runtimeActiveOptionsChanged();
}

QVariant Lr2SelectUpdateController::barActiveOptions() const { return m_barActiveOptions; }
void Lr2SelectUpdateController::setBarActiveOptions(const QVariant& options) {
    if (m_barActiveOptions == options) {
        return;
    }
    m_barActiveOptions = options;
    emit barActiveOptionsChanged();
}

QVariant Lr2SelectUpdateController::baseActiveOptions() const { return m_baseActiveOptions; }
void Lr2SelectUpdateController::setBaseActiveOptions(const QVariant& options) {
    if (m_baseActiveOptions == options) {
        return;
    }
    m_baseActiveOptions = options;
    emit baseActiveOptionsChanged();
}

QString Lr2SelectUpdateController::baseActiveOptionsKey() const { return m_baseActiveOptionsKey; }
void Lr2SelectUpdateController::setBaseActiveOptionsKey(const QString& key) {
    if (m_baseActiveOptionsKey == key) {
        return;
    }
    m_baseActiveOptionsKey = key;
    emit baseActiveOptionsKeyChanged();
}

QVariant Lr2SelectUpdateController::selectCommonActiveOptions() const { return m_selectCommonActiveOptions; }
void Lr2SelectUpdateController::setSelectCommonActiveOptions(const QVariant& options) {
    if (m_selectCommonActiveOptions == options) {
        return;
    }
    m_selectCommonActiveOptions = options;
    emit selectCommonActiveOptionsChanged();
}

QString Lr2SelectUpdateController::selectCommonActiveOptionsKey() const { return m_selectCommonActiveOptionsKey; }
void Lr2SelectUpdateController::setSelectCommonActiveOptionsKey(const QString& key) {
    if (m_selectCommonActiveOptionsKey == key) {
        return;
    }
    m_selectCommonActiveOptionsKey = key;
    emit selectCommonActiveOptionsKeyChanged();
}

bool Lr2SelectUpdateController::selectCommonActiveOptionsReady() const { return m_selectCommonActiveOptionsReady; }
void Lr2SelectUpdateController::setSelectCommonActiveOptionsReady(bool ready) {
    if (m_selectCommonActiveOptionsReady == ready) {
        return;
    }
    m_selectCommonActiveOptionsReady = ready;
    emit selectCommonActiveOptionsReadyChanged();
}

QVariant Lr2SelectUpdateController::selectRuntimeActiveOptions() const { return m_selectRuntimeActiveOptions; }
void Lr2SelectUpdateController::setSelectRuntimeActiveOptions(const QVariant& options) {
    if (m_selectRuntimeActiveOptions == options) {
        return;
    }
    m_selectRuntimeActiveOptions = options;
    emit selectRuntimeActiveOptionsChanged();
}

QString Lr2SelectUpdateController::selectRuntimeActiveOptionsKey() const { return m_selectRuntimeActiveOptionsKey; }
void Lr2SelectUpdateController::setSelectRuntimeActiveOptionsKey(const QString& key) {
    if (m_selectRuntimeActiveOptionsKey == key) {
        return;
    }
    m_selectRuntimeActiveOptionsKey = key;
    emit selectRuntimeActiveOptionsKeyChanged();
}

QVariant Lr2SelectUpdateController::gameplayRuntimeActiveOptions() const { return m_gameplayRuntimeActiveOptions; }
void Lr2SelectUpdateController::setGameplayRuntimeActiveOptions(const QVariant& options) {
    if (m_gameplayRuntimeActiveOptions == options) {
        return;
    }
    m_gameplayRuntimeActiveOptions = options;
    emit gameplayRuntimeActiveOptionsChanged();
}

QString Lr2SelectUpdateController::gameplayRuntimeActiveOptionsKey() const { return m_gameplayRuntimeActiveOptionsKey; }
void Lr2SelectUpdateController::setGameplayRuntimeActiveOptionsKey(const QString& key) {
    if (m_gameplayRuntimeActiveOptionsKey == key) {
        return;
    }
    m_gameplayRuntimeActiveOptionsKey = key;
    emit gameplayRuntimeActiveOptionsKeyChanged();
}

QVariant Lr2SelectUpdateController::barRuntimeActiveOptions() const {
    return m_barRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setBarRuntimeActiveOptions(const QVariant& options) {
    if (m_barRuntimeActiveOptions == options) {
        return;
    }
    m_barRuntimeActiveOptions = options;
    emit barRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QVariant Lr2SelectUpdateController::baseRuntimeActiveOptions() const {
    return m_baseRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setBaseRuntimeActiveOptions(const QVariant& options) {
    if (m_baseRuntimeActiveOptions == options) {
        return;
    }
    m_baseRuntimeActiveOptions = options;
    emit baseRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QVariant Lr2SelectUpdateController::selectCommonRuntimeActiveOptions() const {
    return m_selectCommonRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setSelectCommonRuntimeActiveOptions(const QVariant& options) {
    if (m_selectCommonRuntimeActiveOptions == options) {
        return;
    }
    m_selectCommonRuntimeActiveOptions = options;
    emit selectCommonRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QVariant Lr2SelectUpdateController::selectRequiredRuntimeActiveOptions() const {
    return m_selectRequiredRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setSelectRequiredRuntimeActiveOptions(const QVariant& options) {
    if (m_selectRequiredRuntimeActiveOptions == options) {
        return;
    }
    m_selectRequiredRuntimeActiveOptions = options;
    emit selectRequiredRuntimeActiveOptionsChanged();
    refreshBaseActiveOptions();
    refreshCurrentRuntimeActiveOptions();
}

QVariant Lr2SelectUpdateController::selectRuntimeGeneratedActiveOptions() const {
    return m_selectRuntimeGeneratedActiveOptions;
}

void Lr2SelectUpdateController::setSelectRuntimeGeneratedActiveOptions(const QVariant& options) {
    if (m_selectRuntimeGeneratedActiveOptions == options) {
        return;
    }
    m_selectRuntimeGeneratedActiveOptions = options;
    emit selectRuntimeGeneratedActiveOptionsChanged();
    refreshCurrentRuntimeActiveOptions();
}

QVariant Lr2SelectUpdateController::screenRuntimeActiveOptions() const {
    return m_screenRuntimeActiveOptions;
}

void Lr2SelectUpdateController::setScreenRuntimeActiveOptions(const QVariant& options) {
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

int Lr2SelectUpdateController::selectRevision() const {
    return m_selectRevision;
}

int Lr2SelectUpdateController::selectDetailRevision() const {
    return m_selectDetailRevision;
}

int Lr2SelectUpdateController::selectedKeymode() const {
    return m_selectedKeymode;
}

void Lr2SelectUpdateController::setSelectedKeymode(int keymode) {
    if (m_selectedKeymode == keymode) {
        return;
    }
    m_selectedKeymode = keymode;
    emit selectedKeymodeChanged();
}

int Lr2SelectUpdateController::selectedJudgeOption() const {
    return m_selectedJudgeOption;
}

void Lr2SelectUpdateController::setSelectedJudgeOption(int option) {
    if (m_selectedJudgeOption == option) {
        return;
    }
    m_selectedJudgeOption = option;
    emit selectedJudgeOptionChanged();
    refreshCurrentRuntimeActiveOptions();
}

bool Lr2SelectUpdateController::spToDpActive() const {
    return m_spToDpActive;
}

void Lr2SelectUpdateController::setSpToDpActive(bool active) {
    if (m_spToDpActive == active) {
        return;
    }
    m_spToDpActive = active;
    emit spToDpActiveChanged();
}

bool Lr2SelectUpdateController::battleModeActive() const {
    return m_battleModeActive;
}

void Lr2SelectUpdateController::setBattleModeActive(bool active) {
    if (m_battleModeActive == active) {
        return;
    }
    m_battleModeActive = active;
    emit battleModeActiveChanged();
}

bool Lr2SelectUpdateController::refreshBaseActiveOptions() {
    const QVariant nextBar = mergedNumberArray(
        selectStaticOptions(true, false),
        normalizedNumberArray(m_barRuntimeActiveOptions));
    const bool barChanged = !sameNumberArray(m_barActiveOptions, nextBar);
    if (barChanged) {
        setBarActiveOptions(nextBar);
    }

    const QVariant nextBase = mergedNumberArray(
        mergedNumberArray(nextBar, selectStaticOptions(false, true)),
        normalizedNumberArray(m_baseRuntimeActiveOptions));
    const bool baseChanged = !sameNumberArray(m_baseActiveOptions, nextBase);
    if (baseChanged) {
        setBaseActiveOptions(nextBase);
        setBaseActiveOptionsKey(numberArrayKey(nextBase));
    }

    const QVariant nextSelectCommonBase = mergedNumberArray(
        nextBase,
        normalizedNumberArray(m_selectCommonRuntimeActiveOptions));
    const QVariant nextSelectCommon = mergedNumberArray(
        nextSelectCommonBase,
        normalizedNumberArray(m_selectRequiredRuntimeActiveOptions));
    const bool selectCommonChanged = !sameNumberArray(m_selectCommonActiveOptions, nextSelectCommon);
    setSelectCommonActiveOptionsReady(true);
    if (selectCommonChanged) {
        setSelectCommonActiveOptions(nextSelectCommon);
        setSelectCommonActiveOptionsKey(numberArrayKey(nextSelectCommon));
    }

    if (baseChanged || selectCommonChanged) {
        setSelectRuntimeActiveOptionsKey(QString());
    }
    if (baseChanged) {
        setGameplayRuntimeActiveOptionsKey(QString());
    }
    return barChanged || baseChanged || selectCommonChanged;
}

bool Lr2SelectUpdateController::refreshSelectRuntimeActiveOptions() {
    if (!m_screenUpdatesActive) {
        return false;
    }
    if (m_effectiveScreenKey != QStringLiteral("select")) {
        const QVariant next = mergedNumberArray(
            m_baseActiveOptions,
            normalizedNumberArray(m_screenRuntimeActiveOptions));
        if (sameNumberArray(m_runtimeActiveOptions, next)) {
            return false;
        }
        applyRuntimeActiveOptions(next);
        return true;
    }
    if (!m_selectCommonActiveOptionsReady) {
        refreshBaseActiveOptions();
    }

    QVariantList next = mergedNumberArray(
        m_selectCommonActiveOptions,
        normalizedNumberArray(m_selectRuntimeGeneratedActiveOptions));
    replaceSelectJudgeOption(next);
    appendSelectKeymodeOptions(next);
    const QString nextKey = numberArrayKey(next);
    if (nextKey == m_selectRuntimeActiveOptionsKey
            && sameNumberArray(m_runtimeActiveOptions, next)) {
        return false;
    }
    setSelectRuntimeActiveOptionsKey(nextKey);
    setSelectRuntimeActiveOptions(next);
    applyRuntimeActiveOptions(next);
    return true;
}

bool Lr2SelectUpdateController::refreshGameplayRuntimeActiveOptions() {
    if (!m_gameplayScreen) {
        return false;
    }

    const QVariant next = mergedNumberArray(
        m_baseActiveOptions,
        normalizedNumberArray(m_screenRuntimeActiveOptions));
    const QString nextKey = numberArrayKey(next);
    if (nextKey == m_gameplayRuntimeActiveOptionsKey
            && sameNumberArray(m_runtimeActiveOptions, next)) {
        return false;
    }
    setGameplayRuntimeActiveOptionsKey(nextKey);
    setGameplayRuntimeActiveOptions(next);
    applyRuntimeActiveOptions(next);
    return true;
}

void Lr2SelectUpdateController::handleCommittedSelectState() {
    const bool rankingRequestChanged = m_lr2RankingRequestMd5 != m_lr2RankingMd5;
    if (rankingRequestChanged) {
        setLr2RankingRequestMd5(m_lr2RankingMd5);
    }
    if (rankingRequestChanged) {
        emit rankingStatsApplyRequested();
    }
    emit selectSideEffectsUpdateRequested();
}

void Lr2SelectUpdateController::handleSelectRevisionChanged() {
    handleCommittedSelectState();
    refreshSelectRuntimeActiveOptions();
}

void Lr2SelectUpdateController::refreshCurrentRuntimeActiveOptions() {
    if (!m_screenUpdatesActive) {
        return;
    }
    refreshSelectRuntimeActiveOptions();
    refreshGameplayRuntimeActiveOptions();
}

void Lr2SelectUpdateController::applyRuntimeActiveOptions(const QVariant& value) {
    setRuntimeActiveOptions(value);
    if (m_skinRuntime && m_skinRuntime->runtimeActiveOptions() != value) {
        m_skinRuntime->setRuntimeActiveOptions(value);
    }
}

bool Lr2SelectUpdateController::sameNumberArray(const QVariant& lhs, const QVariant& rhs) const {
    return numberList(lhs) == numberList(rhs);
}

QString Lr2SelectUpdateController::numberArrayKey(const QVariant& values) const {
    const QList<int> list = numberList(values);
    if (list.isEmpty()) {
        return {};
    }
    QStringList parts;
    parts.reserve(list.size());
    for (int value : list) {
        parts.append(QString::number(value));
    }
    return parts.join(QLatin1Char(','));
}

QVariant Lr2SelectUpdateController::normalizedNumberArray(const QVariant& values) const {
    QVariantList result;
    const QList<int> numbers = numberList(values);
    result.reserve(numbers.size());
    for (int value : numbers) {
        result.append(value);
    }
    return result;
}

QVariantList Lr2SelectUpdateController::mergedNumberArray(const QVariant& first, const QVariant& second) const {
    QVariantList result;
    const QList<int> firstList = numberList(first);
    const QList<int> secondList = numberList(second);
    result.reserve(firstList.size() + secondList.size());
    for (int value : firstList) {
        appendUniqueOption(result, value);
    }
    for (int value : secondList) {
        appendUniqueOption(result, value);
    }
    return result;
}

QVariantList Lr2SelectUpdateController::selectStaticOptions(
    bool includeRankingOption,
    bool includePanelOption) const {
    QVariantList result;
    QObject* model = skinModelObject();
    QVariant source = model ? model->property("effectiveActiveOptions") : QVariant {};
    if (numberList(source).isEmpty()) {
        source = m_parseActiveOptions;
    }

    for (int option : numberList(source)) {
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

QObject* Lr2SelectUpdateController::skinModelObject() const {
    return m_skinModel;
}

bool Lr2SelectUpdateController::skinUsesOption(int option) const {
    QObject* model = skinModelObject();
    const QList<int> usedOptions = numberList(model ? model->property("usedOptions") : QVariant {});
    const QList<int> usedElementOptions = numberList(model ? model->property("usedElementOptions") : QVariant {});
    if (usedOptions.isEmpty() && usedElementOptions.isEmpty()) {
        return true;
    }
    const int id = std::abs(option);
    for (int used : usedOptions) {
        if (std::abs(used) == id) {
            return true;
        }
    }
    for (int used : usedElementOptions) {
        if (std::abs(used) == id) {
            return true;
        }
    }
    return false;
}

int Lr2SelectUpdateController::selectModeKeymode(int keymode) const {
    if (!m_spToDpActive && !m_battleModeActive) {
        return keymode;
    }
    if (keymode == 7) {
        return 14;
    }
    if (keymode == 5) {
        return 10;
    }
    return keymode;
}

void Lr2SelectUpdateController::appendSelectKeymodeOptions(QVariantList& options) const {
    const int keymode = selectedKeymode();
    if (keymode <= 0) {
        return;
    }

    const int effectiveKeymode = selectModeKeymode(keymode);
    const int selectOption = keymodeOptionFor(effectiveKeymode, 160);
    const int afterOption = keymodeOptionFor(effectiveKeymode, 165);
    if (selectOption > 0) {
        appendUniqueSkinOption(options, selectOption);
    }
    if (afterOption > 0) {
        appendUniqueSkinOption(options, afterOption);
    }
}

int Lr2SelectUpdateController::compatibleSelectJudgeOption() const {
    int option = m_selectedJudgeOption;
    if (option < 180 || option > 184) {
        option = 182;
    }

    if (skinUsesOption(option)) {
        return option;
    }

    for (int offset = 1; offset <= 4; ++offset) {
        const int lower = option - offset;
        if (lower >= 180 && skinUsesOption(lower)) {
            return lower;
        }
        const int upper = option + offset;
        if (upper <= 184 && skinUsesOption(upper)) {
            return upper;
        }
    }

    return 0;
}

void Lr2SelectUpdateController::replaceSelectJudgeOption(QVariantList& options) const {
    qsizetype write = 0;
    for (qsizetype read = 0; read < options.size(); ++read) {
        const int option = options.at(read).toInt();
        const int id = std::abs(option);
        if (id >= 180 && id <= 184) {
            continue;
        }
        if (write != read) {
            options[write] = options.at(read);
        }
        ++write;
    }
    options.erase(options.begin() + write, options.end());

    const int judgeOption = compatibleSelectJudgeOption();
    if (judgeOption > 0) {
        appendUniqueOption(options, judgeOption);
    }
}

void Lr2SelectUpdateController::appendUniqueOption(QVariantList& options, int option) const {
    for (const QVariant& existing : options) {
        if (existing.toInt() == option) {
            return;
        }
    }
    options.append(option);
}

void Lr2SelectUpdateController::appendUniqueSkinOption(QVariantList& options, int option) const {
    if (!skinUsesOption(option)) {
        return;
    }
    appendUniqueOption(options, option);
}

void Lr2SelectUpdateController::appendDefaultOptionIfMissing(
    QVariantList& options,
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

    const QList<int> current = numberList(options);
    for (int choice : choices) {
        for (int option : current) {
            if (std::abs(option) == std::abs(choice)) {
                return;
            }
        }
    }
    appendUniqueSkinOption(options, fallback);
}

QList<int> Lr2SelectUpdateController::numberList(const QVariant& values) const {
    QList<int> result;
    const QVariantList list = values.toList();
    if (!list.isEmpty()) {
        result.reserve(list.size());
        for (const QVariant& value : list) {
            result.append(value.toInt());
        }
        return result;
    }

    if (values.canConvert<QJSValue>()) {
        const QJSValue array = values.value<QJSValue>();
        const int length = array.property(QStringLiteral("length")).toInt();
        if (length <= 0) {
            return result;
        }
        result.reserve(length);
        for (int i = 0; i < length; ++i) {
            const QJSValue entry = array.property(static_cast<quint32>(i));
            if (entry.isNumber()) {
                result.append(entry.toInt());
            }
        }
        return result;
    }

    return result;
}

bool Lr2SelectUpdateController::updateSelectRevision(bool runSideEffects) {
    const int next = m_scoreRevision + m_focusRevision;
    if (m_selectRevision == next) {
        return false;
    }
    m_selectRevision = next;
    emit selectRevisionChanged();
    setSelectDetailRevision(next);
    if (runSideEffects && m_componentReady) {
        handleSelectRevisionChanged();
    }
    return true;
}

void Lr2SelectUpdateController::setSelectDetailRevision(int revision) {
    if (m_selectDetailRevision == revision) {
        return;
    }
    m_selectDetailRevision = revision;
    emit selectDetailRevisionChanged();
}
