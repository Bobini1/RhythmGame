//
// Created by PC on 16/03/2026.
//

#include "ScoreSyncOperation.h"

namespace qml_components {
ScoreSyncOperation::ScoreSyncOperation(QObject* parent)
  : QObject(parent)
{
}
void
ScoreSyncOperation::setTotal(int total)
{
    if (total == 0) {
        emit finished();
    }
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
        emit finished();
    }
}
void
ScoreSyncOperation::reportError(const QString& message)
{
    emit error(message);
}
} // namespace qml_components