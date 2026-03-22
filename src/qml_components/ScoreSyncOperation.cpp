//
// Created by PC on 16/03/2026.
//

#include "ScoreSyncOperation.h"

namespace qml_components {
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
    if (finishedFlag == value) {
        return;
    }
    finishedFlag = value;
    emit finishedChanged();
}
void
ScoreSyncOperation::setTotal(int total)
{
    if (total == this->total) {
        return;
    }
    this->total = total;
    emit progressChanged();
}
void
ScoreSyncOperation::increment()
{
    ++currentDone;
    emit progressChanged();
    if (currentDone == total) {
        setFinished(true);
    }
}
void
ScoreSyncOperation::reportError(const QString& message)
{
    emit error(message);
}
} // namespace qml_components