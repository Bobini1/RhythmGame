#pragma once

#include "ArenaTransport.h"

#include <QPointer>

class QWebSocket;

namespace arena {

class QtWebSocketArenaTransport final : public ArenaTransport
{
    Q_OBJECT
  public:
    explicit QtWebSocketArenaTransport(QObject* parent = nullptr);
    ~QtWebSocketArenaTransport() override;

    void connectTo(Generation generation, const QUrl& url) override;
    void sendText(Generation generation, const QString& message) override;
    void sendBinary(Generation generation, const QByteArray& bytes) override;
    void close(Generation generation) override;

  private:
    QPointer<QWebSocket> m_socket;
    Generation m_generation{ InvalidGeneration };

    void retireCurrent();
};

} // namespace arena
