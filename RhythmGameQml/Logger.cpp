//
// Created by bobini on 08.09.23.
//

#include <QJSEngine>
#include <QQmlEngine>
#include "Logger.h"

namespace qml_components {
Logger::Logger(QObject* parent)
  : QObject(parent)
{
}
void
Logger::setInstance(Logger* newInstance)
{
    instance = newInstance;
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
}
void
Logger::addLog(const QString& msg)
{
    history.append(msg);
    emit logged(msg);
}
auto
Logger::getHistory() const -> const QVector<QString>&
{
    return history;
}
auto
Logger::create(QQmlEngine* engine, QJSEngine* scriptEngine) -> Logger*
{
    Q_ASSERT(instance);
    return instance;
}
} // namespace qml_components