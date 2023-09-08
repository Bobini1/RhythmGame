//
// Created by bobini on 08.09.23.
//

#ifndef RHYTHMGAME_LOGGER_H
#define RHYTHMGAME_LOGGER_H

#include <QObject>
#include <QtQmlIntegration>
namespace qml_components {

class Logger : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QStringListModel* history READ getHistory CONSTANT)

    static inline Logger* instance{};
    QStringListModel* history{};

  public:
    // Use nullptr as parent. Default ctor must not exist for Qt to prefer
    // create().
    explicit Logger(QObject* parent);
    static void setInstance(Logger* instance);
    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine) -> Logger*;
    Q_INVOKABLE void addLog(const QString& msg);
    auto getHistory() const -> QStringListModel*;

  signals:
    void logged(const QString& msg);
};

} // namespace qml_components

#endif // RHYTHMGAME_LOGGER_H
