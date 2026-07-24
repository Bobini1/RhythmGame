#include "PendingReply.h"

#include <QJSEngine>
#include <QJSManagedValue>
#include <QPointer>
#include <QQmlEngine>
#include <QQmlInfo>

namespace support {

PendingReply::PendingReply(std::shared_ptr<detail::PendingReplyState> state,
                           QObject* parent)
  : QObject(parent)
  , state(std::move(state))
{
}

PendingReply::~PendingReply()
{
    detail::assertApplicationThread();
    if (!state || state->reply != this)
        return;

    state->reply = nullptr;
    if (resultAvailable)
        return;

    state->stopSource.request_stop();
    auto handler = std::move(state->cancellationHandler);
    if (handler)
        handler();
}

bool
PendingReply::isValid() const
{
    return true;
}

bool
PendingReply::isResultAvailable() const
{
    return resultAvailable;
}

bool
PendingReply::isSuccessful() const
{
    return successful;
}

QVariant
PendingReply::value() const
{
    return result;
}

void
PendingReply::then(const QJSValue& success, const QJSValue& failed)
{
    detail::assertApplicationThread();
    if (!success.isUndefined() && !success.isCallable()) {
        qmlWarning(this) << "PendingReply success callback is not callable";
        return;
    }
    if (!failed.isUndefined() && !failed.isCallable()) {
        qmlWarning(this) << "PendingReply failure callback is not callable";
        return;
    }

    successCallback = success;
    failedCallback = failed;
    if (resultAvailable)
        invokeCallback(takeCallback());
}

void
PendingReply::cancel()
{
    detail::assertApplicationThread();
    (void)settle(false, QVariant{}, true);
}

bool
PendingReply::settle(bool success, QVariant value, bool requestCancellation)
{
    detail::assertApplicationThread();
    if (resultAvailable)
        return false;

    resultAvailable = true;
    successful = success;
    result = success ? std::move(value) : QVariant{};

    auto cancellationHandler = requestCancellation
                                 ? std::move(state->cancellationHandler)
                                 : std::function<void()>{};
    state->cancellationHandler = {};
    auto callback = takeCallback();

    QPointer replyGuard(this);
    releaseToQml();
    if (!replyGuard)
        return true;

    if (requestCancellation)
        state->stopSource.request_stop();
    if (!replyGuard)
        return true;

    if (cancellationHandler)
        cancellationHandler();
    if (!replyGuard)
        return true;

    emit finished();
    if (!replyGuard)
        return true;

    invokeCallback(std::move(callback));
    return true;
}

QJSValue
PendingReply::takeCallback()
{
    auto callback =
      successful ? std::move(successCallback) : std::move(failedCallback);
    successCallback = {};
    failedCallback = {};
    return callback;
}

void
PendingReply::invokeCallback(QJSValue callback)
{
    if (callback.isUndefined())
        return;

    const auto replySuccessful = successful;
    auto* engine = qjsEngine(this);
    if (!engine) {
        qmlWarning(this) << "PendingReply has no associated QJSEngine";
        return;
    }

    QJSManagedValue managedCallback(std::move(callback), engine);
    if (replySuccessful)
        managedCallback.call({ engine->toScriptValue(result) });
    else
        managedCallback.call();
}

void
PendingReply::releaseToQml()
{
    setParent(nullptr);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::JavaScriptOwnership);
}

} // namespace support
