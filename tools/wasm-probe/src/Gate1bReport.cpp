#include "Gate1bReport.h"

#include <QJsonDocument>
#include <QRegularExpression>

#include <algorithm>

namespace
{
const QRegularExpression &eventTypePattern()
{
    static const QRegularExpression pattern{
        QStringLiteral("^[a-z][a-z0-9]*(?:-[a-z0-9]+)*$")};
    return pattern;
}
}

Gate1bReport::Gate1bReport(QObject *parent)
    : QObject{parent}
    , m_capabilities{browserCapabilities()}
{
    m_elapsed.start();
    if (!publishGate1bSnapshot(snapshot())) {
        rejectBrowserBoundary(
            u"gate1b-snapshot-publication",
            u"browser rejected the initial fixed snapshot schema");
    }
}

void Gate1bReport::append(QStringView type, QJsonObject payload)
{
    if (m_terminal
        || !eventTypePattern().matchView(type).hasMatch()) {
        return;
    }

    const qint64 elapsedMicroseconds = m_elapsed.nsecsElapsed() / 1000;
    const qint64 lastMonotonicMicroseconds =
        m_lastMonotonicMicroseconds;
    const qint64 monotonicMicroseconds = (std::max)(
        elapsedMicroseconds,
        lastMonotonicMicroseconds + 1);
    m_lastMonotonicMicroseconds = monotonicMicroseconds;
    const qint64 sequence = m_nextSequence++;

    const QJsonObject event{
        {QStringLiteral("sequence"), sequence},
        {QStringLiteral("monotonicMicroseconds"), monotonicMicroseconds},
        {QStringLiteral("type"), type.toString()},
        {QStringLiteral("payload"), payload},
    };
    Q_ASSERT(
        !QJsonDocument{event}.toJson(QJsonDocument::Compact).isEmpty());
    if (!publishGate1bEvent(event)) {
        rejectBrowserBoundary(
            u"gate1b-event-publication",
            u"browser did not accept the committed event sequence");
    }
}

void Gate1bReport::pass(QStringView check, QJsonObject detail)
{
    if (m_terminal) {
        return;
    }
    if (m_checks.contains(check)) {
        fail(
            u"duplicate-check-completion",
            u"a core check was completed more than once");
        return;
    }

    m_checks.insert(
        check,
        QJsonObject{
            {QStringLiteral("detail"), detail},
            {QStringLiteral("passed"), true},
        });
    append(
        u"check-passed",
        QJsonObject{
            {QStringLiteral("check"), check.toString()},
            {QStringLiteral("detail"), detail},
        });
    if (m_terminal) {
        return;
    }
    if (!publishGate1bSnapshot(snapshot())) {
        rejectBrowserBoundary(
            u"gate1b-snapshot-publication",
            u"browser rejected the fixed pass snapshot schema");
    }
}

void Gate1bReport::fail(QStringView code, QStringView detail)
{
    if (m_terminal) {
        return;
    }

    const QJsonObject failure{
        {QStringLiteral("code"), code.toString()},
        {QStringLiteral("detail"), detail.toString()},
    };
    if (m_failures.size() < maximumFailureRecords) {
        m_failures.append(failure);
    }
    append(
        u"check-failed",
        QJsonObject{
            {QStringLiteral("code"), code.toString()},
            {QStringLiteral("detail"), detail.toString()},
        });
    if (!m_terminal) {
        rejectReadyOnce(code, detail);
    }
}

QJsonObject Gate1bReport::snapshot() const
{
    return {
        {QStringLiteral("phase"), m_phase},
        {QStringLiteral("checks"), m_checks},
        {QStringLiteral("capabilities"), capabilitySnapshot()},
        {QStringLiteral("failures"), m_failures},
        {QStringLiteral("cycleSummary"),
         QJsonObject{
             {QStringLiteral("completed"), 0},
             {QStringLiteral("status"), QStringLiteral("not-started")},
         }},
        {QStringLiteral("authority"), authoritySnapshot()},
    };
}

void Gate1bReport::setPhase(QStringView phase)
{
    if (m_terminal || phase.isEmpty() || m_phase == phase) {
        return;
    }
    m_phase = phase.toString();
    if (!publishGate1bSnapshot(snapshot())) {
        rejectBrowserBoundary(
            u"gate1b-snapshot-publication",
            u"browser rejected the fixed phase snapshot schema");
    }
}

bool Gate1bReport::hasPassed(QStringView check) const
{
    return m_checks.value(check).toObject().value(u"passed").toBool(false);
}

bool Gate1bReport::requiredCapabilitiesAvailable() const
{
    return m_capabilities.secureContext
        && m_capabilities.crossOriginIsolated
        && m_capabilities.sharedArrayBuffer
        && m_capabilities.jspiApi
        && m_capabilities.webGl2Api
        && m_capabilities.audioWorklet
        && m_capabilities.opfs
        && m_capabilities.fileSystemAccess;
}

void Gate1bReport::resolveReady()
{
    if (m_terminal || m_readyResolved) {
        return;
    }
    m_readyResolved = true;
    const QJsonObject readySnapshot = snapshot();
    if (!publishGate1bSnapshot(readySnapshot)) {
        rejectBrowserBoundary(
            u"gate1b-snapshot-publication",
            u"browser rejected the readiness snapshot schema");
        return;
    }
    if (!resolveGate1bReady(readySnapshot)) {
        rejectBrowserBoundary(
            u"gate1b-ready-publication",
            u"browser rejected the readiness snapshot");
    }
}

bool Gate1bReport::isTerminal() const
{
    return m_terminal;
}

QJsonObject Gate1bReport::capabilitySnapshot() const
{
    return {
        {QStringLiteral("secureContext"), m_capabilities.secureContext},
        {QStringLiteral("crossOriginIsolated"),
         m_capabilities.crossOriginIsolated},
        {QStringLiteral("sharedArrayBuffer"),
         m_capabilities.sharedArrayBuffer},
        {QStringLiteral("jspiApi"), m_capabilities.jspiApi},
        {QStringLiteral("webGl2Api"), m_capabilities.webGl2Api},
        {QStringLiteral("audioWorklet"), m_capabilities.audioWorklet},
        {QStringLiteral("opfs"), m_capabilities.opfs},
        {QStringLiteral("fileSystemAccess"),
         m_capabilities.fileSystemAccess},
    };
}

QJsonObject Gate1bReport::authoritySnapshot() const
{
    return {
        {QStringLiteral("gate1bTechnicalPassed"), false},
        {QStringLiteral("gate0Satisfied"), false},
        {QStringLiteral("formalGate1EntryAuthorized"), false},
        {QStringLiteral("gate1Passed"), false},
        {QStringLiteral("productionPortAuthorized"), false},
    };
}

void Gate1bReport::rejectBrowserBoundary(
    QStringView code,
    QStringView detail)
{
    if (m_readyRejectionAttempted) {
        return;
    }
    if (m_failures.size() < maximumFailureRecords) {
        m_failures.append(
            QJsonObject{
                {QStringLiteral("code"), code.toString()},
                {QStringLiteral("detail"), detail.toString()},
            });
    }
    rejectReadyOnce(code, detail);
}

void Gate1bReport::rejectReadyOnce(
    QStringView code,
    QStringView detail)
{
    if (m_readyRejectionAttempted) {
        return;
    }
    m_readyRejectionAttempted = true;
    m_terminal = true;
    m_phase = QStringLiteral("core-failed");
    // Publication failures are terminal inputs here, not recursive calls.
    // The direct ready rejection remains the final browser boundary.
    static_cast<void>(publishGate1bSnapshot(snapshot()));
    rejectGate1bReady(
        code,
        QJsonObject{{QStringLiteral("message"), detail.toString()}});
}
