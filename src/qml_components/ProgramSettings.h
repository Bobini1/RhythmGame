//
// Created by bobini on 17.08.23.
//

#ifndef RHYTHMGAME_PROGRAMSETTINGS_H
#define RHYTHMGAME_PROGRAMSETTINGS_H

#include <QQmlEngine>
namespace qml_components {

class ProgramSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString avatarFolder READ getAvatarFolder CONSTANT)

    QString chartPath;
    QString avatarFolder;

  public:
    explicit ProgramSettings(QString avatarFolder,
                             QObject* parent = nullptr);
    auto getAvatarFolder() const -> QString;
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
