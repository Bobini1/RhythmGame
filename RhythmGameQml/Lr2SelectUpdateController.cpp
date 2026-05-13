#include "Lr2SelectUpdateController.h"

#include <QJSValue>
#include <QMetaObject>
#include <QStringList>

Lr2SelectUpdateController::Lr2SelectUpdateController(QObject* parent)
    : QObject(parent) {}

QObject* Lr2SelectUpdateController::host() const {
    return m_host;
}

void Lr2SelectUpdateController::setHost(QObject* host) {
    if (m_host == host) {
        return;
    }
    m_host = host;
    emit hostChanged();
}

QObject* Lr2SelectUpdateController::selectContext() const {
    return m_selectContext;
}

void Lr2SelectUpdateController::setSelectContext(QObject* context) {
    if (m_selectContext == context) {
        return;
    }
    m_selectContext = context;
    reconnectSelectContext();
    updateSelectRevision(false);
    emit selectContextChanged();
}

QObject* Lr2SelectUpdateController::runtimeOptions() const {
    return m_runtimeOptions;
}

void Lr2SelectUpdateController::setRuntimeOptions(QObject* options) {
    if (m_runtimeOptions == options) {
        return;
    }
    m_runtimeOptions = options;
    emit runtimeOptionsChanged();
}

QObject* Lr2SelectUpdateController::skinRuntime() const {
    return m_skinRuntime;
}

void Lr2SelectUpdateController::setSkinRuntime(QObject* runtime) {
    if (m_skinRuntime == runtime) {
        return;
    }
    m_skinRuntime = runtime;
    emit skinRuntimeChanged();
    applyRuntimeActiveOptions(hostVariant("runtimeActiveOptions"));
}

int Lr2SelectUpdateController::selectRevision() const {
    return m_selectRevision;
}

int Lr2SelectUpdateController::selectDetailRevision() const {
    return m_selectDetailRevision;
}

bool Lr2SelectUpdateController::refreshBaseActiveOptions() {
    if (!m_host || !m_runtimeOptions) {
        return false;
    }

    const QVariant nextBar = invokeRuntimeOptions("buildBarActiveOptions");
    const bool barChanged = !sameNumberArray(hostVariant("barActiveOptions"), nextBar);
    if (barChanged) {
        setHostPropertyIfChanged("barActiveOptions", nextBar);
    }

    const QVariant nextBase = invokeRuntimeOptions("buildBaseActiveOptions", nextBar);
    const bool baseChanged = !sameNumberArray(hostVariant("baseActiveOptions"), nextBase);
    if (baseChanged) {
        setHostPropertyIfChanged("baseActiveOptions", nextBase);
        setHostPropertyIfChanged("baseActiveOptionsKey", numberArrayKey(nextBase));
    }

    const QVariant nextSelectCommon = invokeRuntimeOptions("buildSelectCommonActiveOptions", nextBase);
    const bool selectCommonChanged = !sameNumberArray(hostVariant("selectCommonActiveOptions"), nextSelectCommon);
    setHostPropertyIfChanged("selectCommonActiveOptionsReady", true);
    if (selectCommonChanged) {
        setHostPropertyIfChanged("selectCommonActiveOptions", nextSelectCommon);
        setHostPropertyIfChanged("selectCommonActiveOptionsKey", numberArrayKey(nextSelectCommon));
    }

    if (baseChanged || selectCommonChanged) {
        setHostPropertyIfChanged("selectRuntimeActiveOptionsKey", QString());
    }
    if (baseChanged) {
        setHostPropertyIfChanged("gameplayRuntimeActiveOptionsKey", QString());
    }
    return barChanged || baseChanged || selectCommonChanged;
}

bool Lr2SelectUpdateController::refreshSelectRuntimeActiveOptions() {
    if (!m_host || !m_runtimeOptions || !hostBool("screenUpdatesActive")) {
        return false;
    }
    if (hostString("effectiveScreenKey") != QStringLiteral("select")) {
        const QVariant next = invokeRuntimeOptions("buildRuntimeActiveOptions", hostVariant("baseActiveOptions"));
        if (sameNumberArray(hostVariant("runtimeActiveOptions"), next)) {
            return false;
        }
        applyRuntimeActiveOptions(next);
        return true;
    }
    if (!hostBool("selectCommonActiveOptionsReady")) {
        refreshBaseActiveOptions();
    }

    const QVariant next = invokeRuntimeOptions("buildSelectRuntimeActiveOptions", hostVariant("selectCommonActiveOptions"));
    const QString nextKey = numberArrayKey(next);
    if (nextKey == hostString("selectRuntimeActiveOptionsKey")) {
        return false;
    }
    setHostPropertyIfChanged("selectRuntimeActiveOptionsKey", nextKey);
    setHostPropertyIfChanged("selectRuntimeActiveOptions", next);
    applyRuntimeActiveOptions(next);
    return true;
}

bool Lr2SelectUpdateController::refreshGameplayRuntimeActiveOptions() {
    bool gameplayScreen = false;
    if (m_host) {
        QVariant result;
        if (QMetaObject::invokeMethod(m_host, "isGameplayScreen", Q_RETURN_ARG(QVariant, result))) {
            gameplayScreen = result.toBool();
        }
    }
    if (!m_host || !m_runtimeOptions || !gameplayScreen) {
        return false;
    }

    const QVariant next = invokeRuntimeOptions("buildRuntimeActiveOptions", hostVariant("baseActiveOptions"));
    const QString nextKey = numberArrayKey(next);
    if (nextKey == hostString("gameplayRuntimeActiveOptionsKey")) {
        return false;
    }
    setHostPropertyIfChanged("gameplayRuntimeActiveOptionsKey", nextKey);
    setHostPropertyIfChanged("gameplayRuntimeActiveOptions", next);
    applyRuntimeActiveOptions(next);
    return true;
}

void Lr2SelectUpdateController::handleCommittedSelectState() {
    if (!m_host) {
        return;
    }
    const QString rankingMd5 = hostString("lr2RankingMd5");
    const bool rankingRequestChanged = hostString("lr2RankingRequestMd5") != rankingMd5;
    if (rankingRequestChanged) {
        setHostPropertyIfChanged("lr2RankingRequestMd5", rankingMd5);
    }
    if (rankingRequestChanged) {
        invokeHostVoid("applyRankingStatsToSelectContext");
    }
    invokeHostVoid("updateSelectSideEffects");
}

void Lr2SelectUpdateController::handleSelectRevisionChanged() {
    handleCommittedSelectState();
    refreshSelectRuntimeActiveOptions();
}


void Lr2SelectUpdateController::selectRevisionDependencyChanged() {
    updateSelectRevision(true);
}

QVariant Lr2SelectUpdateController::invokeRuntimeOptions(const char* method) const {
    QVariant result;
    if (m_runtimeOptions) {
        QMetaObject::invokeMethod(m_runtimeOptions, method, Q_RETURN_ARG(QVariant, result));
    }
    return result;
}

QVariant Lr2SelectUpdateController::invokeRuntimeOptions(const char* method, const QVariant& arg) const {
    QVariant result;
    if (m_runtimeOptions) {
        QMetaObject::invokeMethod(m_runtimeOptions, method, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, arg));
    }
    return result;
}

void Lr2SelectUpdateController::invokeHostVoid(const char* method) const {
    if (m_host) {
        QMetaObject::invokeMethod(m_host, method);
    }
}

int Lr2SelectUpdateController::contextInt(const char* name, int fallback) const {
    if (!m_selectContext) {
        return fallback;
    }
    bool ok = false;
    const int value = m_selectContext->property(name).toInt(&ok);
    return ok ? value : fallback;
}

bool Lr2SelectUpdateController::hostBool(const char* name, bool fallback) const {
    if (!m_host) {
        return fallback;
    }
    const QVariant value = m_host->property(name);
    return value.isValid() ? value.toBool() : fallback;
}

QString Lr2SelectUpdateController::hostString(const char* name) const {
    return m_host ? m_host->property(name).toString() : QString();
}

QVariant Lr2SelectUpdateController::hostVariant(const char* name) const {
    return m_host ? m_host->property(name) : QVariant();
}

void Lr2SelectUpdateController::setHostPropertyIfChanged(const char* name, const QVariant& value) const {
    if (!m_host || m_host->property(name) == value) {
        return;
    }
    m_host->setProperty(name, value);
}

void Lr2SelectUpdateController::applyRuntimeActiveOptions(const QVariant& value) const {
    setHostPropertyIfChanged("runtimeActiveOptions", value);
    if (m_skinRuntime && m_skinRuntime->property("runtimeActiveOptions") != value) {
        m_skinRuntime->setProperty("runtimeActiveOptions", value);
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

    if (!values.canConvert<QJSValue>()) {
        return result;
    }
    const QJSValue jsValue = values.value<QJSValue>();
    if (!jsValue.isArray()) {
        return result;
    }
    const int length = jsValue.property(QStringLiteral("length")).toInt();
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result.append(jsValue.property(static_cast<quint32>(i)).toInt());
    }
    return result;
}

bool Lr2SelectUpdateController::updateSelectRevision(bool runSideEffects) {
    const int nextScoreRevision = contextInt("scoreRevision");
    const int nextFocusRevision = contextInt("focusRevision");
    const int next = nextScoreRevision + nextFocusRevision;
    if (m_selectRevision == next) {
        return false;
    }
    m_selectRevision = next;
    emit selectRevisionChanged();
    setSelectDetailRevision(next);
    if (runSideEffects && hostBool("componentReady")) {
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

void Lr2SelectUpdateController::reconnectSelectContext() {
    if (m_focusRevisionConnection) {
        QObject::disconnect(m_focusRevisionConnection);
        m_focusRevisionConnection = {};
    }
    if (m_scoreRevisionConnection) {
        QObject::disconnect(m_scoreRevisionConnection);
        m_scoreRevisionConnection = {};
    }
    if (!m_selectContext) {
        return;
    }
    m_focusRevisionConnection = QObject::connect(
        m_selectContext, SIGNAL(focusRevisionChanged()), this, SLOT(selectRevisionDependencyChanged()));
    m_scoreRevisionConnection = QObject::connect(
        m_selectContext, SIGNAL(scoreRevisionChanged()), this, SLOT(selectRevisionDependencyChanged()));
}



