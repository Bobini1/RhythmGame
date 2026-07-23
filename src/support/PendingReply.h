#ifndef RHYTHMGAME_PENDINGREPLY_H
#define RHYTHMGAME_PENDINGREPLY_H

#include <QCoreApplication>
#include <QJSValue>
#include <QObject>
#include <QThread>
#include <QVariant>

#include <functional>
#include <memory>
#include <stop_token>
#include <type_traits>
#include <utility>

namespace support {

class PendingReply;

namespace detail {

struct PendingReplyState
{
    PendingReply* reply{};
    std::stop_source stopSource;
    std::function<void()> cancellationHandler;
};

inline void
assertApplicationThread()
{
    const auto* application = QCoreApplication::instance();
    Q_ASSERT(!application ||
             QThread::currentThread() == application->thread());
}

} // namespace detail

template<typename T>
class PendingReplySource;

class PendingReply final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT FINAL)
    Q_PROPERTY(bool resultAvailable READ isResultAvailable NOTIFY finished FINAL)
    Q_PROPERTY(bool success READ isSuccessful NOTIFY finished FINAL)
    Q_PROPERTY(QVariant value READ value NOTIFY finished FINAL)

  public:
    ~PendingReply() override;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isResultAvailable() const;
    [[nodiscard]] bool isSuccessful() const;
    [[nodiscard]] QVariant value() const;

    Q_INVOKABLE void then(const QJSValue& success,
                          const QJSValue& failed = QJSValue());
    Q_INVOKABLE void cancel();

  signals:
    void finished();

  private:
    template<typename T>
    friend class PendingReplySource;

    explicit PendingReply(std::shared_ptr<detail::PendingReplyState> state,
                          QObject* parent);
    bool settle(bool success, QVariant value, bool requestCancellation);
    void invokeCallback();
    void releaseToQml();

    std::shared_ptr<detail::PendingReplyState> state;
    QVariant result;
    QJSValue successCallback;
    QJSValue failedCallback;
    bool resultAvailable{};
    bool successful{};
};

template<typename T>
class PendingReplySource
{
  public:
    explicit PendingReplySource(QObject* owner)
      : state(std::make_shared<detail::PendingReplyState>())
    {
        detail::assertApplicationThread();
        state->reply = new PendingReply(state, owner);
    }

    [[nodiscard]] PendingReply* reply() const
    {
        detail::assertApplicationThread();
        return state->reply;
    }

    [[nodiscard]] bool succeed(const T& value) const
    {
        detail::assertApplicationThread();
        auto* pendingReply = state->reply;
        if (!pendingReply)
            return false;
        if constexpr (std::is_same_v<std::remove_cv_t<T>, QVariant>)
            return pendingReply->settle(true, value, false);
        else
            return pendingReply->settle(
              true, QVariant::fromValue(value), false);
    }

    [[nodiscard]] bool fail() const
    {
        detail::assertApplicationThread();
        auto* pendingReply = state->reply;
        return pendingReply &&
               pendingReply->settle(false, QVariant{}, false);
    }

    [[nodiscard]] std::stop_token stopToken() const
    {
        return state->stopSource.get_token();
    }

    void setCancellationHandler(std::function<void()> handler) const
    {
        detail::assertApplicationThread();
        if (state->stopSource.stop_requested()) {
            if (handler)
                handler();
            return;
        }
        if (!state->reply || state->reply->isResultAvailable())
            return;
        state->cancellationHandler = std::move(handler);
    }

  private:
    std::shared_ptr<detail::PendingReplyState> state;
};

} // namespace support

#endif // RHYTHMGAME_PENDINGREPLY_H
