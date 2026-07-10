#include "QtWebSocketArenaTransport.h"

#include "ArenaTypes.h"

#include <QAbstractSocket>
#include <QSslError>
#include <QtWebSockets/QWebSocket>

#include <memory>
#include <utility>

namespace arena {
namespace {

auto
mapSocketError(QAbstractSocket::SocketError error) -> ArenaTransport::Error
{
    switch (error) {
        case QAbstractSocket::RemoteHostClosedError:
            return ArenaTransport::Error::RemoteClosed;
        case QAbstractSocket::SslHandshakeFailedError:
            return ArenaTransport::Error::TlsFailed;
        case QAbstractSocket::ConnectionRefusedError:
        case QAbstractSocket::HostNotFoundError:
        case QAbstractSocket::SocketTimeoutError:
        case QAbstractSocket::NetworkError:
        case QAbstractSocket::ProxyConnectionRefusedError:
        case QAbstractSocket::ProxyConnectionClosedError:
        case QAbstractSocket::ProxyNotFoundError:
        case QAbstractSocket::ProxyConnectionTimeoutError:
            return ArenaTransport::Error::ConnectionFailed;
        default:
            return ArenaTransport::Error::Other;
    }
}

} // namespace

QtWebSocketArenaTransport::QtWebSocketArenaTransport(QObject* parent)
  : ArenaTransport(parent)
{
}

QtWebSocketArenaTransport::~QtWebSocketArenaTransport()
{
    retireCurrent();
}

void
QtWebSocketArenaTransport::retireCurrent()
{
    const auto socket = m_socket;
    m_socket.clear();
    m_generation = InvalidGeneration;
    if (!socket) {
        return;
    }
    socket->abort();
    socket->deleteLater();
}

void
QtWebSocketArenaTransport::connectTo(Generation generation, const QUrl& url)
{
    if (generation == InvalidGeneration) {
        return;
    }
    retireCurrent();
    auto* socket =
      new QWebSocket(QString{}, QWebSocketProtocol::VersionLatest, this);
    m_socket = socket;
    m_generation = generation;
    socket->setMaxAllowedIncomingFrameSize(
      static_cast<quint64>(MaxServerMessageBytes));
    socket->setMaxAllowedIncomingMessageSize(
      static_cast<quint64>(MaxServerMessageBytes));

    const auto errorEmitted = std::make_shared<bool>(false);
    const auto emitErrorOnce =
      [this, generation, errorEmitted](ArenaTransport::Error error) {
          if (std::exchange(*errorEmitted, true)) {
              return;
          }
          emit transportError(generation, error);
      };
    connect(socket, &QWebSocket::connected, this, [this, generation] {
        emit connected(generation);
    });
    connect(socket,
            &QWebSocket::textMessageReceived,
            this,
            [this, generation](const QString& message) {
                emit textReceived(generation, message);
            });
    connect(socket,
            &QWebSocket::binaryMessageReceived,
            this,
            [this, generation](const QByteArray& bytes) {
                emit binaryReceived(generation, bytes);
            });
    connect(socket,
            &QWebSocket::errorOccurred,
            this,
            [emitErrorOnce](QAbstractSocket::SocketError error) {
                emitErrorOnce(mapSocketError(error));
            });
    connect(socket,
            &QWebSocket::sslErrors,
            this,
            [emitErrorOnce](const QList<QSslError>&) {
                emitErrorOnce(ArenaTransport::Error::TlsFailed);
            });
    connect(socket,
            &QWebSocket::disconnected,
            this,
            [this, generation, emitErrorOnce] {
                emitErrorOnce(ArenaTransport::Error::RemoteClosed);
                emit disconnected(generation);
            });
    socket->open(url);
}

void
QtWebSocketArenaTransport::sendText(Generation generation,
                                    const QString& message)
{
    if (generation != m_generation || generation == InvalidGeneration ||
        !m_socket) {
        return;
    }
    m_socket->sendTextMessage(message);
}

void
QtWebSocketArenaTransport::sendBinary(Generation generation,
                                      const QByteArray& bytes)
{
    if (generation != m_generation || generation == InvalidGeneration ||
        !m_socket) {
        return;
    }
    m_socket->sendBinaryMessage(bytes);
}

void
QtWebSocketArenaTransport::close(Generation generation)
{
    if (generation != m_generation || generation == InvalidGeneration ||
        !m_socket) {
        return;
    }
    const auto socket = m_socket;
    m_socket.clear();
    m_generation = InvalidGeneration;
    socket->close();
    socket->deleteLater();
}

} // namespace arena
