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

/**
 * @brief QML-facing handle for one asynchronous result.
 *
 * @details The QML component that starts an operation owns its cancellation:
 * it retains the returned reply and calls cancel() when that operation is no
 * longer needed. PendingReplySource<T>::succeed(),
 * PendingReplySource<T>::fail(), and cancel() compete to settle the reply; the
 * first terminal result wins.
 *
 * Callback registration, cancellation, settlement, signal emission, and
 * callback invocation are application-thread operations. While pending, the
 * reply is a child of the producer QObject supplied to PendingReplySource.
 * Settlement detaches it from that producer and marks it as
 * JavaScript-owned. Destroying the reply while it is still pending, including
 * through producer teardown, requests stop and runs the current cancellation
 * handler.
 */
class PendingReply final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT FINAL)
    Q_PROPERTY(bool resultAvailable READ isResultAvailable NOTIFY finished FINAL)
    Q_PROPERTY(bool success READ isSuccessful NOTIFY finished FINAL)
    Q_PROPERTY(QVariant value READ value NOTIFY finished FINAL)

  public:
    ~PendingReply() override;

    /**
     * @return Always true for a live PendingReply.
     */
    [[nodiscard]] bool isValid() const;

    /**
     * @return Whether a success, failure, or cancellation has settled the
     * reply.
     */
    [[nodiscard]] bool isResultAvailable() const;

    /**
     * @return Whether the terminal result is successful. This is false while
     * pending and after failure or cancellation.
     */
    [[nodiscard]] bool isSuccessful() const;

    /**
     * @return The successful result, or an invalid QVariant while pending and
     * after failure or cancellation.
     */
    [[nodiscard]] QVariant value() const;

    /**
     * @brief Registers callbacks for the terminal result.
     *
     * @details A valid call while pending replaces both callbacks registered
     * by an earlier call. A valid call after settlement invokes the callback
     * matching the result synchronously. This function and the callbacks run
     * on the application thread.
     * @param success A callable receiving the successful value, or undefined.
     * @param failed A callable receiving no arguments, or undefined.
     */
    Q_INVOKABLE void then(const QJSValue& success,
                          const QJSValue& failed = QJSValue());

    /**
     * @brief Settles the reply as a cancelled terminal failure.
     *
     * @details When cancellation wins, it requests stop and invokes the
     * current cancellation handler. Calling cancel() again, or after another
     * terminal result has won, has no effect. Call this on the application
     * thread.
     */
    Q_INVOKABLE void cancel();

  signals:
    /**
     * @brief Reports that a terminal result is available.
     */
    void finished();

  private:
    template<typename T>
    friend class PendingReplySource;

    explicit PendingReply(std::shared_ptr<detail::PendingReplyState> state,
                          QObject* parent);
    bool settle(bool success, QVariant value, bool requestCancellation);
    QJSValue takeCallback();
    void invokeCallback(QJSValue callback);
    void releaseToQml();

    std::shared_ptr<detail::PendingReplyState> state;
    QVariant result;
    QJSValue successCallback;
    QJSValue failedCallback;
    bool resultAvailable = {};
    bool successful = {};
};

/**
 * @brief Producer-side control for a PendingReply.
 *
 * @details Copies share the same operation state. Construct the source and
 * call reply(), succeed(), fail(), and setCancellationHandler() on the
 * application thread. Workers may obtain and observe stopToken(), then queue
 * settlement back to the application thread.
 *
 * The reply is parented to @p owner while pending. If that owner destroys the
 * pending reply, stop is requested, the current cancellation handler runs,
 * and reply() subsequently returns nullptr. A settled reply has already been
 * detached from the owner for JavaScript to manage.
 *
 * @tparam T Type delivered on successful settlement.
 */
template<typename T>
class PendingReplySource
{
  public:
    /**
     * @brief Creates a pending reply parented to @p owner until settlement.
     * @param owner QObject parent for the pending reply.
     * @pre Called on the application thread.
     */
    explicit PendingReplySource(QObject* owner)
      : state(std::make_shared<detail::PendingReplyState>())
    {
        detail::assertApplicationThread();
        state->reply = new PendingReply(state, owner);
    }

    /**
     * @return The shared reply, or nullptr after the reply has been destroyed.
     * @pre Called on the application thread.
     */
    [[nodiscard]] PendingReply* reply() const
    {
        detail::assertApplicationThread();
        return state->reply;
    }

    /**
     * @brief Attempts to settle the reply successfully with @p value.
     * @return True only when this call supplies the first terminal result.
     * Returns false when the reply is gone or already settled.
     *
     * @note This function does not change ownership of QObject pointer values.
     * A producer transferring an owned QObject result must select its QObject
     * ownership separately and relinquish producer cleanup only when this
     * function returns true; a rejected result remains the producer's
     * responsibility.
     * @pre Called on the application thread.
     */
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

    /**
     * @brief Attempts to settle the reply as a failure without requesting
     * cancellation.
     * @return True only when this call supplies the first terminal result.
     * Returns false when the reply is gone or already settled.
     * @pre Called on the application thread.
     */
    [[nodiscard]] bool fail() const
    {
        detail::assertApplicationThread();
        auto* pendingReply = state->reply;
        return pendingReply &&
               pendingReply->settle(false, QVariant{}, false);
    }

    /**
     * @brief Returns the operation's cooperative stop token.
     *
     * @details The token is requested when cancel() wins or when the pending
     * reply is destroyed, but not by succeed() or fail(). It may be obtained
     * and observed from worker threads.
     */
    [[nodiscard]] std::stop_token stopToken() const
    {
        return state->stopSource.get_token();
    }

    /**
     * @brief Sets the cancellation handler for the producer's current stage.
     *
     * @details While pending, each call replaces the previous handler and an
     * empty handler clears it. This supports handing cancellation from one
     * stage to the next. If stop was already requested, a non-empty handler is
     * invoked synchronously instead; registrations after ordinary success or
     * failure are ignored. Registration and invocation occur on the
     * application thread.
     * @param handler Handler to install, or an empty function to clear it.
     */
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
