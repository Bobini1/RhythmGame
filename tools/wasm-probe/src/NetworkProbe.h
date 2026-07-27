#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringView>
#include <QtTypes>

#include <chrono>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
class QSslError;
class QTimer;
class QWebSocket;

class NetworkProbe final : public QObject
{
    Q_OBJECT

public:
    using EventCallback =
        std::function<void(QStringView, QJsonObject)>;
    using CompletionCallback =
        std::function<void(QStringView, QJsonObject)>;
    using FailureCallback =
        std::function<void(QStringView, QStringView)>;

    explicit NetworkProbe(
        EventCallback eventCallback,
        CompletionCallback completionCallback,
        FailureCallback failureCallback,
        QObject *parent = nullptr);

    void start(quint32 runNonce);

private:
    struct PendingWebSocketEvent
    {
        QString type;
        QJsonObject payload;
    };

    enum class WebSocketStep
    {
        AwaitingConnection,
        AwaitingServerMessage,
        AwaitingTextEcho,
        AwaitingBinaryEcho,
        AwaitingHeartbeat,
        AwaitingCleanClose,
        Complete,
    };

    void startQnam();
    void startWebSocket();
    void readQnamBytes();
    void finishQnam();
#ifndef QT_NO_SSL
    void sslErrorsReceived(
        QNetworkReply *reply,
        const QList<QSslError> &errors);
#endif
    void retireQnamReply();
    void webSocketConnected();
    void webSocketTextMessage(const QString &message);
    void webSocketBinaryMessage(const QByteArray &message);
    void webSocketDisconnected();
    [[nodiscard]] bool observeMainThread(QStringView handler);
    void appendWebSocketEvent(
        QStringView type,
        QJsonObject payload = {});
    void flushPendingWebSocketEvents();
    void append(QStringView type, QJsonObject payload = {});
    void fail(QStringView code, QStringView detail);
    void maybeFinish();

    static constexpr qsizetype maximumQnamResponseBytes = 4096;
    static constexpr quint64 maximumWebSocketMessageBytes = 4096;
    static constexpr qsizetype maximumPendingWebSocketEvents = 8;
    static constexpr auto networkTimeout =
        std::chrono::seconds{15};

    EventCallback m_eventCallback;
    CompletionCallback m_completionCallback;
    FailureCallback m_failureCallback;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr;
    QWebSocket *m_webSocket = nullptr;
    QTimer *m_timeout = nullptr;
    QByteArray m_qnamBody;
    QByteArray m_expectedBinary;
    QString m_requestId;
    QString m_connectionId;
    QList<PendingWebSocketEvent> m_pendingWebSocketEvents;
    quint32 m_runNonce = 0;
    WebSocketStep m_webSocketStep =
        WebSocketStep::AwaitingConnection;
    bool m_started = false;
    bool m_failed = false;
    bool m_qnamComplete = false;
    bool m_webSocketComplete = false;
    bool m_allWebSocketHandlersOnMainThread = true;
};
