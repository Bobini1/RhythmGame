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

    Q_PROPERTY(QStringListModel* history READ getHistory CONSTANT)
    QStringListModel* history{};

  public:
    explicit Logger(QObject* parent = nullptr);
    Q_INVOKABLE void addLog(const QString& msg);
    auto getHistory() const -> QStringListModel*;

  signals:
    void logged(const QString& msg);
};

} // namespace qml_components

#endif // RHYTHMGAME_LOGGER_H
