#pragma once

#include "arena/ArenaTransport.h"

#include <QVector>

#include <utility>

namespace arena::test {

class FakeArenaTransport final : public ArenaTransport
{
  public:
    struct ConnectCall
    {
        Generation generation;
        QUrl url;
        bool operator==(const ConnectCall&) const = default;
    };
    struct TextCall
    {
        Generation generation;
        QString message;
        bool operator==(const TextCall&) const = default;
    };
    struct BinaryCall
    {
        Generation generation;
        QByteArray bytes;
        bool operator==(const BinaryCall&) const = default;
    };

    using ArenaTransport::ArenaTransport;

    QVector<ConnectCall> connectCalls{};
    QVector<TextCall> textCalls{};
    QVector<BinaryCall> binaryCalls{};
    QVector<Generation> closeCalls{};

    void connectTo(Generation generation, const QUrl& url) override
    {
        connectCalls.push_back({ generation, url });
    }
    void sendText(Generation generation, const QString& message) override
    {
        textCalls.push_back({ generation, message });
    }
    void sendBinary(Generation generation, const QByteArray& bytes) override
    {
        binaryCalls.push_back({ generation, bytes });
    }
    void close(Generation generation) override
    {
        closeCalls.push_back(generation);
    }

    void injectConnected(Generation generation) { emit connected(generation); }
    void injectDisconnected(Generation generation)
    {
        emit disconnected(generation);
    }
    void injectText(Generation generation, QString message)
    {
        emit textReceived(generation, std::move(message));
    }
    void injectBinary(Generation generation, QByteArray bytes)
    {
        emit binaryReceived(generation, std::move(bytes));
    }
    void injectError(Generation generation, Error error)
    {
        emit transportError(generation, error);
    }
};

} // namespace arena::test
