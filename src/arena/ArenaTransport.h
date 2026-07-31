#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUrl>

namespace arena {

class ArenaTransport : public QObject
{
    Q_OBJECT
  public:
    using Generation = quint64;
    static constexpr Generation InvalidGeneration = 0;

    enum class Error
    {
        ConnectionFailed,
        RemoteClosed,
        TlsFailed,
        Other
    };
    Q_ENUM(Error)

    using QObject::QObject;
    ~ArenaTransport() override = default;

    virtual void connectTo(Generation generation, const QUrl& url) = 0;
    virtual void sendText(Generation generation, const QString& message) = 0;
    virtual void sendBinary(Generation generation, const QByteArray& bytes) = 0;
    virtual void close(Generation generation) = 0;

  signals:
    void connected(Generation generation);
    void disconnected(Generation generation);
    void textReceived(Generation generation, const QString& message);
    void binaryReceived(Generation generation, const QByteArray& bytes);
    void transportError(Generation generation,
                        arena::ArenaTransport::Error error);
};

} // namespace arena
