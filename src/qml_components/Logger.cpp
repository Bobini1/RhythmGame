//
// Created by bobini on 08.09.23.
//

#include <QJSEngine>
#include <QQmlEngine>
#include "Logger.h"

namespace qml_components {
Logger::Logger(QObject* parent)
  : QObject(parent)
  , history(new QStringListModel(this))
{
}
void
Logger::addLog(const QString& msg)
{
    history->insertRows(history->rowCount(), 1);
    history->setData(history->index(history->rowCount() - 1), msg);
    emit logged(msg);
}
auto
Logger::getHistory() const -> QStringListModel*
{
    return history;
}
} // namespace qml_components