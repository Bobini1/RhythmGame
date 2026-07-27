#include "NetworkProbe.h"

#include "BrowserRuntimeBridge.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslError>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>
#include <QWebSocket>
#include <QWebSocketProtocol>

#include <utility>

namespace
{
const QRegularExpression requestIdPattern{
    QStringLiteral("^request-[1-9][0-9]*$")};
const QRegularExpression connectionIdPattern{
    QStringLiteral("^connection-[1-9][0-9]*$")};
}

NetworkProbe::NetworkProbe(
    EventCallback eventCallback,
    CompletionCallback completionCallback,
    FailureCallback failureCallback,
    QObject *parent)
    : QObject{parent}
    , m_eventCallback{std::move(eventCallback)}
    , m_completionCallback{std::move(completionCallback)}
    , m_failureCallback{std::move(failureCallback)}
    , m_network{new QNetworkAccessManager{this}}
    , m_webSocket{new QWebSocket{
          QString{},
          QWebSocketProtocol::VersionLatest,
          this}}
    , m_timeout{new QTimer{this}}
    , m_expectedBinary{QByteArray::fromHex("010203")}
{
    m_timeout->setSingleShot(true);
    connect(m_timeout, &QTimer::timeout, this, [this] {
        fail(
            u"qt-network-timeout",
            u"QNAM or WSS did not reach a bounded terminal state");
    });
#ifndef QT_NO_SSL
    connect(
        m_network,
        &QNetworkAccessManager::sslErrors,
        this,
        &NetworkProbe::sslErrorsReceived);
#endif

    m_webSocket->setMaxAllowedIncomingFrameSize(
        maximumWebSocketMessageBytes);
    m_webSocket->setMaxAllowedIncomingMessageSize(
        maximumWebSocketMessageBytes);
    connect(
        m_webSocket,
        &QWebSocket::connected,
        this,
        &NetworkProbe::webSocketConnected);
    connect(
        m_webSocket,
        &QWebSocket::textMessageReceived,
        this,
        &NetworkProbe::webSocketTextMessage);
    connect(
        m_webSocket,
        &QWebSocket::binaryMessageReceived,
        this,
        &NetworkProbe::webSocketBinaryMessage);
    connect(
        m_webSocket,
        &QWebSocket::disconnected,
        this,
        &NetworkProbe::webSocketDisconnected);
    connect(
        m_webSocket,
        &QWebSocket::errorOccurred,
        this,
        [this](QAbstractSocket::SocketError) {
            if (m_webSocketStep != WebSocketStep::Complete) {
                fail(
                    u"qt-wss-error",
                    m_webSocket->errorString());
            }
        });
}

void NetworkProbe::start(quint32 runNonce)
{
    if (m_started || runNonce == 0 || runNonce == 0xFFFFFFFFU) {
        fail(
            u"qt-network-start-invalid",
            u"network probe must start exactly once with an owned nonce");
        return;
    }
    m_started = true;
    m_runNonce = runNonce;
    m_timeout->start(networkTimeout);
    append(
        u"qt-network-started",
        QJsonObject{{
            QStringLiteral("runNonce"),
            static_cast<qint64>(m_runNonce)}});
    startQnam();
    startWebSocket();
}

void NetworkProbe::startQnam()
{
    const QUrl url = BrowserRuntimeBridge::sameOriginUrl(
        QStringLiteral("/probe/qnam?nonce=")
        + QString::number(m_runNonce));
    const QUrl origin = BrowserRuntimeBridge::sameOriginUrl(u"/");
    if (url.scheme() != origin.scheme()
        || url.host() != origin.host()
        || url.port() != origin.port()) {
        fail(
            u"qt-qnam-origin",
            u"QNAM URL did not preserve the exact document origin");
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(
        QNetworkRequest::RedirectPolicyAttribute,
        QNetworkRequest::ManualRedirectPolicy);
    request.setMaximumRedirectsAllowed(0);
    request.setTransferTimeout(networkTimeout);
    m_reply = m_network->get(request);
    m_reply->setReadBufferSize(maximumQnamResponseBytes + 1);
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        readQnamBytes();
    });
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        finishQnam();
    });
    connect(
        m_reply,
        &QNetworkReply::errorOccurred,
        this,
        [this](QNetworkReply::NetworkError) {
            if (!m_failed && !m_qnamComplete) {
                fail(
                    u"qt-qnam-network-error",
                    u"browser-backed QNAM reported a transport error");
            }
        });
}

void NetworkProbe::startWebSocket()
{
    QUrl url = BrowserRuntimeBridge::sameOriginUrl(
        QStringLiteral("/probe/ws?nonce=")
        + QString::number(m_runNonce));
    url.setScheme(QStringLiteral("wss"));
    m_webSocketStep = WebSocketStep::AwaitingConnection;
    m_webSocket->open(url);
}

void NetworkProbe::readQnamBytes()
{
    if (m_failed || m_reply == nullptr) {
        return;
    }
    const qsizetype remainingBytes =
        maximumQnamResponseBytes - m_qnamBody.size();
    const qint64 maximumReadableBytes =
        static_cast<qint64>(remainingBytes + 1);
    if (m_reply->error() != QNetworkReply::NoError) {
        fail(
            u"qt-qnam-read-error",
            u"QNAM reported an error before the bounded body read");
        return;
    }
    // The retained reply is released with deleteLater() only by
    // retireQnamReply() on a terminal success or failure path.
    Q_ASSERT(m_reply->error() == QNetworkReply::NoError);
    const QByteArray bytes = m_reply->read(maximumReadableBytes);
    if (bytes.size() > remainingBytes) {
        fail(
            u"qt-qnam-response-bound",
            u"QNAM response exceeded the fixed byte limit");
        return;
    }
    m_qnamBody += bytes;
}

void NetworkProbe::finishQnam()
{
    if (m_failed || m_reply == nullptr || m_qnamComplete) {
        return;
    }
    readQnamBytes();
    if (m_failed) {
        return;
    }

    const QVariant redirect = m_reply->attribute(
        QNetworkRequest::RedirectionTargetAttribute);
    const int status = m_reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString contentType = m_reply->header(
        QNetworkRequest::ContentTypeHeader).toString();
    const bool corsHeaderPresent =
        m_reply->hasRawHeader("Access-Control-Allow-Origin");
    if (m_reply->error() != QNetworkReply::NoError
        || redirect.isValid()
        || status != 200
        || contentType != u"application/json; charset=utf-8"
        || corsHeaderPresent) {
        fail(
            u"qt-qnam-response-contract",
            u"QNAM status, MIME, redirect, or CORS contract failed");
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(m_qnamBody, &parseError);
    if (parseError.error != QJsonParseError::NoError
        || !document.isObject()) {
        fail(
            u"qt-qnam-json",
            u"QNAM response was not a bounded JSON object");
        return;
    }
    const QJsonObject object = document.object();
    const QStringList expectedKeys{
        QStringLiteral("nonce"),
        QStringLiteral("ok"),
        QStringLiteral("requestId"),
        QStringLiteral("transport"),
    };
    if (object.keys() != expectedKeys
        || object.value(u"nonce").toInteger()
            != static_cast<qint64>(m_runNonce)
        || object.value(u"ok").toBool() != true
        || object.value(u"transport").toString() != u"https"
        || !requestIdPattern.match(
            object.value(u"requestId").toString()).hasMatch()) {
        fail(
            u"qt-qnam-body",
            u"QNAM body did not match the owned nonce and schema");
        return;
    }
    const QByteArray exactBody =
        QJsonDocument{object}.toJson(QJsonDocument::Compact) + '\n';
    if (m_qnamBody != exactBody) {
        fail(
            u"qt-qnam-body-bytes",
            u"QNAM response bytes differed from the exact expected body");
        return;
    }

    m_requestId = object.value(u"requestId").toString();
    m_qnamComplete = true;
    m_completionCallback(
        u"qt-qnam-same-origin",
        QJsonObject{
            {QStringLiteral("contentType"), contentType},
            {QStringLiteral("corsHeaderPresent"), corsHeaderPresent},
            {QStringLiteral("redirected"), redirect.isValid()},
            {QStringLiteral("requestId"), m_requestId},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
            {QStringLiteral("status"), status},
        });
    retireQnamReply();
    maybeFinish();
}

#ifndef QT_NO_SSL
void NetworkProbe::sslErrorsReceived(
    QNetworkReply *reply,
    const QList<QSslError> &errors)
{
    if (m_failed
        || reply == nullptr
        || reply != m_reply
        || errors.isEmpty()) {
        return;
    }
    fail(
        u"qt-qnam-ssl-errors",
        u"QNAM rejected one or more TLS certificate errors");
}
#endif

void NetworkProbe::retireQnamReply()
{
    QNetworkReply *const reply = std::exchange(m_reply, nullptr);
    if (reply != nullptr) {
        reply->deleteLater();
    }
}

void NetworkProbe::webSocketConnected()
{
    if (!observeMainThread(u"connected")
        || m_webSocketStep != WebSocketStep::AwaitingConnection) {
        return;
    }
    m_webSocketStep = WebSocketStep::AwaitingServerMessage;
    appendWebSocketEvent(
        u"qt-wss-opened",
        QJsonObject{{
            QStringLiteral("runNonce"),
            static_cast<qint64>(m_runNonce)}});
}

void NetworkProbe::webSocketTextMessage(const QString &message)
{
    if (!observeMainThread(u"textMessageReceived") || m_failed) {
        return;
    }
    const QByteArray messageUtf8 = message.toUtf8();
    if (messageUtf8.size()
        > static_cast<qsizetype>(maximumWebSocketMessageBytes)) {
        fail(
            u"qt-wss-message-bound",
            u"WSS text message exceeded the fixed byte limit");
        return;
    }

    if (m_webSocketStep == WebSocketStep::AwaitingServerMessage) {
        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(messageUtf8, &parseError);
        if (parseError.error != QJsonParseError::NoError
            || !document.isObject()) {
            fail(
                u"qt-wss-server-message",
                u"WSS server greeting was not valid JSON");
            return;
        }
        const QJsonObject object = document.object();
        const QStringList expectedKeys{
            QStringLiteral("connectionId"),
            QStringLiteral("nonce"),
            QStringLiteral("type"),
            QStringLiteral("value"),
        };
        if (object.keys() != expectedKeys
            || object.value(u"nonce").toInteger()
                != static_cast<qint64>(m_runNonce)
            || object.value(u"type").toString() != u"server-message"
            || object.value(u"value").toString() != u"connected"
            || !connectionIdPattern.match(
                object.value(u"connectionId").toString()).hasMatch()) {
            fail(
                u"qt-wss-server-message-contract",
                u"WSS greeting did not match the nonce and schema");
            return;
        }
        m_connectionId = object.value(u"connectionId").toString();
        flushPendingWebSocketEvents();
        m_webSocketStep = WebSocketStep::AwaitingTextEcho;
        m_webSocket->sendTextMessage(
            QStringLiteral("text-echo:")
            + QString::number(m_runNonce));
        return;
    }

    if (m_webSocketStep == WebSocketStep::AwaitingTextEcho) {
        const QString expected =
            QStringLiteral("text-echo:")
            + QString::number(m_runNonce);
        if (message != expected) {
            fail(u"qt-wss-text-echo", u"WSS text echo mismatched");
            return;
        }
        m_webSocketStep = WebSocketStep::AwaitingBinaryEcho;
        m_webSocket->sendBinaryMessage(m_expectedBinary);
        return;
    }

    if (m_webSocketStep == WebSocketStep::AwaitingHeartbeat) {
        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(messageUtf8, &parseError);
        const QJsonObject object = document.object();
        if (parseError.error != QJsonParseError::NoError
            || !document.isObject()
            || object.keys() != QStringList{
                QStringLiteral("nonce"),
                QStringLiteral("type")}
            || object.value(u"nonce").toInteger()
                != static_cast<qint64>(m_runNonce)
            || object.value(u"type").toString() != u"heartbeat") {
            fail(
                u"qt-wss-heartbeat",
                u"WSS application heartbeat mismatched");
            return;
        }
        m_webSocketStep = WebSocketStep::AwaitingCleanClose;
        m_webSocket->sendTextMessage(QStringLiteral("close"));
        return;
    }

    fail(
        u"qt-wss-text-order",
        u"WSS text message arrived in an invalid protocol state");
}

void NetworkProbe::webSocketBinaryMessage(const QByteArray &message)
{
    if (!observeMainThread(u"binaryMessageReceived") || m_failed) {
        return;
    }
    if (m_webSocketStep != WebSocketStep::AwaitingBinaryEcho
        || message != m_expectedBinary) {
        fail(
            u"qt-wss-binary-echo",
            u"WSS binary echo mismatched or arrived out of order");
        return;
    }
    m_webSocketStep = WebSocketStep::AwaitingHeartbeat;
    m_webSocket->sendTextMessage(
        QStringLiteral("heartbeat:")
        + QString::number(m_runNonce));
}

void NetworkProbe::webSocketDisconnected()
{
    if (!observeMainThread(u"disconnected") || m_failed) {
        return;
    }
    if (m_webSocketStep != WebSocketStep::AwaitingCleanClose
        || m_webSocket->closeCode()
            != QWebSocketProtocol::CloseCodeNormal
        || m_webSocket->closeReason() != u"probe-complete") {
        fail(
            u"qt-wss-clean-close",
            u"WSS did not complete its owned normal close");
        return;
    }
    m_webSocketStep = WebSocketStep::Complete;
    m_webSocketComplete = true;
    m_completionCallback(
        u"qt-wss-main-thread",
        QJsonObject{
            {QStringLiteral("allHandlersOnMainThread"),
             m_allWebSocketHandlersOnMainThread},
            {QStringLiteral("closeCode"),
             static_cast<int>(m_webSocket->closeCode())},
            {QStringLiteral("closeReason"),
             m_webSocket->closeReason()},
            {QStringLiteral("connectionId"), m_connectionId},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    maybeFinish();
}

bool NetworkProbe::observeMainThread(QStringView handler)
{
    const bool mainThread =
        QThread::currentThread() == qApp->thread();
    m_allWebSocketHandlersOnMainThread &=
        mainThread;
    appendWebSocketEvent(
        u"qt-wss-handler",
        QJsonObject{
            {QStringLiteral("handler"), handler.toString()},
            {QStringLiteral("mainThread"), mainThread},
            {QStringLiteral("runNonce"),
             static_cast<qint64>(m_runNonce)},
        });
    if (!mainThread) {
        fail(
            u"qt-wss-handler-thread",
            u"a QWebSocket handler ran outside qApp->thread()");
    }
    return mainThread;
}

void NetworkProbe::appendWebSocketEvent(
    QStringView type,
    QJsonObject payload)
{
    if (m_connectionId.isEmpty()) {
        if (m_pendingWebSocketEvents.size()
            >= maximumPendingWebSocketEvents) {
            fail(
                u"qt-wss-pending-event-bound",
                u"WSS produced too many events before connection identity");
            return;
        }
        m_pendingWebSocketEvents.append(
            PendingWebSocketEvent{
                type.toString(),
                std::move(payload),
            });
        return;
    }
    payload.insert(QStringLiteral("connectionId"), m_connectionId);
    append(type, std::move(payload));
}

void NetworkProbe::flushPendingWebSocketEvents()
{
    if (m_connectionId.isEmpty()) {
        fail(
            u"qt-wss-connection-identity",
            u"native WSS events cannot publish without connection identity");
        return;
    }
    QList<PendingWebSocketEvent> pending =
        std::exchange(m_pendingWebSocketEvents, {});
    for (PendingWebSocketEvent &event : pending) {
        event.payload.insert(
            QStringLiteral("connectionId"),
            m_connectionId);
        append(event.type, std::move(event.payload));
    }
}

void NetworkProbe::append(QStringView type, QJsonObject payload)
{
    if (m_eventCallback) {
        m_eventCallback(type, std::move(payload));
    }
}

void NetworkProbe::fail(QStringView code, QStringView detail)
{
    if (m_failed || (m_qnamComplete && m_webSocketComplete)) {
        return;
    }
    m_failed = true;
    m_timeout->stop();
    if (m_reply != nullptr) {
        m_reply->abort();
        retireQnamReply();
    }
    if (m_webSocket->state() != QAbstractSocket::UnconnectedState) {
        m_webSocket->abort();
    }
    if (m_failureCallback) {
        m_failureCallback(code, detail);
    }
}

void NetworkProbe::maybeFinish()
{
    if (!m_failed && m_qnamComplete && m_webSocketComplete) {
        m_timeout->stop();
        append(
            u"qt-network-complete",
            QJsonObject{
                {QStringLiteral("connectionId"), m_connectionId},
                {QStringLiteral("requestId"), m_requestId},
                {QStringLiteral("runNonce"),
                 static_cast<qint64>(m_runNonce)},
            });
    }
}
