//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_PROFILE_H
#define RHYTHMGAME_PROFILE_H

#include "Vars.h"
#include "db/SqliteCppDb.h"
#include "input/InputTranslator.h"
#include "qml_components/ScoreDb.h"

#include <QQmlPropertyMap>
#include "qml_components/ThemeFamily.h"
namespace resource_managers {

class Profile final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ getPathQString CONSTANT)
    Q_PROPERTY(QQmlPropertyMap* themeConfig READ getThemeConfig CONSTANT)
    Q_PROPERTY(Vars* vars READ getVars CONSTANT)
    Q_PROPERTY(qml_components::ScoreDb* scoreDb READ getScoreDb CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    QString name;
    QString avatar;
    db::SqliteCppDb db;
    std::filesystem::path dbPath;
    QQmlPropertyMap* themeConfig;
    Vars vars;
    qml_components::ScoreDb scoreDb{ &db };
    QString guid;

  public:
    /**
     * @brief Creates a profile object living in the given database.
     * If the profile doesn't exist, it will be created.
     * @param dbPath Path to the database file. Doesn't have to exist.
     * @param themeFamilies The available theme families.
     * @param avatarPath Path to the avatar folder.
     * @param parent QObject parent.
     */
    explicit Profile(
      const std::filesystem::path& mainDbPath,
      const std::filesystem::path& dbPath,
      const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
      QString avatarPath,
      QObject* parent = nullptr);

    auto getPath() const -> std::filesystem::path;
    auto getPathQString() const -> QString;
    auto getDb() -> db::SqliteCppDb&;
    auto getScoreDb() -> qml_components::ScoreDb*;
    auto getThemeConfig() const -> QQmlPropertyMap*;
    auto getVars() -> Vars*;
    auto getGuid() const -> QString;
};

} // namespace resource_managers

#endif // RHYTHMGAME_PROFILE_H
