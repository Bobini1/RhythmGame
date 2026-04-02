// Created by Codex on 01.04.2026.

#include "ReplayImportOperation.h"

namespace qml_components {

ReplayImportOperation::ReplayImportOperation(int total, QObject* parent)
  : QObject(parent)
  , currentTotal(total)
{
    if (total <= 0) {
        finishedFlag = true;
    }
}

void
ReplayImportOperation::checkFinished()
{
    if (currentDone >= currentTotal) {
        finishedFlag = true;
        emit finishedChanged();
    }
}

void
ReplayImportOperation::incrementImported()
{
    ++importedCount;
    ++currentDone;
    emit progressChanged();
    checkFinished();
}

void
ReplayImportOperation::incrementSkipped()
{
    ++skippedCount;
    ++currentDone;
    emit progressChanged();
    checkFinished();
}

void
ReplayImportOperation::incrementErrored()
{
    ++currentDone;
    emit progressChanged();
    checkFinished();
}

void
ReplayImportOperation::reportError(const QString& message)
{
    emit error(message);
}

} // namespace qml_components
