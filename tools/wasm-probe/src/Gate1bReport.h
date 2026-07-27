#pragma once

#include "BrowserRuntimeBridge.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringView>

class Gate1bReport final : public QObject
{
    Q_OBJECT

public:
    explicit Gate1bReport(QObject *parent = nullptr);

    void append(QStringView type, QJsonObject payload = {});
    void pass(QStringView check, QJsonObject detail = {});
    void fail(QStringView code, QStringView detail);
    [[nodiscard]] QJsonObject snapshot() const;

    void setPhase(QStringView phase);
    [[nodiscard]] bool hasPassed(QStringView check) const;
    [[nodiscard]] bool requiredCapabilitiesAvailable() const;
    void resolveReady();
    [[nodiscard]] bool isTerminal() const;

private:
    [[nodiscard]] QJsonObject capabilitySnapshot() const;
    [[nodiscard]] QJsonObject authoritySnapshot() const;
    void rejectBrowserBoundary(
        QStringView code,
        QStringView detail);
    void rejectReadyOnce(QStringView code, QStringView detail);

    static constexpr qsizetype maximumFailureRecords = 4;

    QElapsedTimer m_elapsed;
    BrowserCapabilities m_capabilities;
    QJsonObject m_checks;
    QJsonArray m_failures;
    QString m_phase = QStringLiteral("core-starting");
    qint64 m_lastMonotonicMicroseconds = -1;
    qint64 m_nextSequence = 0;
    bool m_readyResolved = false;
    bool m_readyRejectionAttempted = false;
    bool m_terminal = false;
};
