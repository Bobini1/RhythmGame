//
// Created by PC on 16/03/2026.
//

#include "ScoreSyncOperation.h"
#include <utility>

namespace qml_components {
ScoreSyncOperation::~ScoreSyncOperation()
{
    cancel();
}

void
ScoreSyncOperation::start(QStringList guids,
                          std::function<void(const QString&)> callback)
{
    if (finishedFlag || dispatch) {
        return;
    }
    pending = std::move(guids);
    dispatch = std::move(callback);
    setTotal(static_cast<int>(pending.size()));
    startNext();
}

void
ScoreSyncOperation::startNext()
{
    if (dispatching || finishedFlag || !dispatch) {
        return;
    }
    dispatching = true;
    while (!finishedFlag && active < concurrency && next < pending.size()) {
        const auto guid = pending[next++];
        ++active;
        // A dispatcher may complete synchronously and clear the member.
        const auto callback = dispatch;
        callback(guid);
    }
    dispatching = false;
}

void
ScoreSyncOperation::ownReply(QNetworkReply* reply)
{
    reply->setParent(this);
    replies.insert(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        replies.remove(reply);
        reply->deleteLater();
    });
}

void
ScoreSyncOperation::cancel()
{
    if (finishedFlag) {
        return;
    }
    cancelled = true;
    setFinished(true);
    const auto owned = std::exchange(replies, {});
    for (auto* reply : owned) {
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
}
ScoreSyncOperation::ScoreSyncOperation(QObject* parent)
  : QObject(parent)
{
}
auto
ScoreSyncOperation::isFinished() const -> bool
{
    return finishedFlag;
}
void
ScoreSyncOperation::setFinished(bool value)
{
    if (!value || finishedFlag) {
        return;
    }
    finishedFlag = value;
    pending.clear();
    dispatch = {};
    emit finishedChanged();
}
void
ScoreSyncOperation::setTotal(int total)
{
    if (finishedFlag) {
        return;
    }
    this->total = total;
    emit progressChanged();
    if (total == 0) {
        setFinished(true);
    }
}
void
ScoreSyncOperation::increment()
{
    if (finishedFlag) {
        return;
    }
    ++currentDone;
    if (active > 0) {
        --active;
    }
    emit progressChanged();
    if (currentDone == total) {
        setFinished(true);
    }
    startNext();
}
void
ScoreSyncOperation::reportError(const QString& message)
{
    if (finishedFlag) {
        return;
    }
    ++errors;
    lastError = message;
    emit errorsChanged();
    emit error(message);
}
} // namespace qml_components
