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

    // will be empty if not set
    Q_PROPERTY(QString chartPath READ getChartPath CONSTANT)
    Q_PROPERTY(QString avatarFolder READ getAvatarFolder CONSTANT)

    QString chartPath;
    QString avatarFolder;

  public:
    explicit ProgramSettings(QString chartPath,
                             QString avatarFolder,
                             QObject* parent = nullptr);
    [[nodiscard]] auto getChartPath() const -> QString;
    auto getAvatarFolder() const -> QString;
};

} // namespace qml_components

#endif // RHYTHMGAME_PROGRAMSETTINGS_H
