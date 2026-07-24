#pragma once

#include <QObject>

class QNetworkAccessManager;
class QWebSocket;

class ProbeState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool exceptionPassed READ exceptionPassed NOTIFY exceptionPassedChanged)
    Q_PROPERTY(bool threadPassed READ threadPassed NOTIFY threadPassedChanged)

public:
    explicit ProbeState(QObject* parent = nullptr);

    [[nodiscard]] bool exceptionPassed() const;
    [[nodiscard]] bool threadPassed() const;

signals:
    void exceptionPassedChanged();
    void threadPassedChanged();

private:
    bool m_exceptionPassed = false;
    bool m_threadPassed = false;
    QNetworkAccessManager* m_network = nullptr;
    QWebSocket* m_webSocket = nullptr;
};
