//
// Created by PC on 08/10/2023.
//

#ifndef RHYTHMGAME_FILEVALIDATOR_H
#define RHYTHMGAME_FILEVALIDATOR_H

#include <QObject>
#include <QUrl>
#include <QFile>
namespace qml_components {
class FileValidator : public QObject
{
    Q_OBJECT

    public:
      Q_INVOKABLE bool exists(const QString& path);
};
} // namespace qml_components

#endif // RHYTHMGAME_FILEVALIDATOR_H
