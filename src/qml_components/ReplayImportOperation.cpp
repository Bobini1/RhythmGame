// Created by Codex on 01.04.2026.

#include "ReplayImportOperation.h"
#include <algorithm>

namespace qml_components {

ReplayImportOperation::ReplayImportOperation(int total, QObject* parent)
  : QAbstractListModel(parent)
  , currentTotal(total)
{
}

void
ReplayImportOperation::setTotal(int total)
{
    currentTotal = total;
    emit progressChanged();
}

void
ReplayImportOperation::finish()
{
    if (!finishedFlag) {
        finishedFlag = true;
        emit finishedChanged();
    }
}

void
ReplayImportOperation::fail(const QString& message)
{
    setTotal(std::max(currentTotal, currentDone + 1));
    reportError(message);
    incrementErrored();
    finish();
}

void
ReplayImportOperation::incrementImported()
{
    ++importedCount;
    ++currentDone;
    emit progressChanged();
}

void
ReplayImportOperation::incrementSkipped()
{
    ++skippedCount;
    ++currentDone;
    emit progressChanged();
}

void
ReplayImportOperation::incrementErrored()
{
    ++erroredCount;
    ++currentDone;
    emit progressChanged();
}

int
ReplayImportOperation::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return errorMessages.size();
}

QVariant
ReplayImportOperation::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= errorMessages.size())
        return {};
    if (role == MessageRole)
        return errorMessages.at(index.row());
    return {};
}

QHash<int, QByteArray>
ReplayImportOperation::roleNames() const
{
    return { { MessageRole, "message" } };
}

void
ReplayImportOperation::reportError(const QString& message)
{
    beginInsertRows(QModelIndex(), errorMessages.size(), errorMessages.size());
    errorMessages.append(message);
    endInsertRows();
    emit countChanged();
    emit error(message);
}

} // namespace qml_components
