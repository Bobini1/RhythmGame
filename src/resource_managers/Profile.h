//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_PROFILE_H
#define RHYTHMGAME_PROFILE_H

#include "db/SqliteCppDb.h"

#include <QQmlPropertyMap>
#include "qml_components/ThemeFamily.h"
namespace resource_managers {

class Profile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(
      QString avatar READ getAvatar WRITE setAvatar NOTIFY avatarChanged)
    Q_PROPERTY(QString path READ getPathQString CONSTANT)
    Q_PROPERTY(QQmlPropertyMap* themeConfig READ getThemeConfig CONSTANT)
    QString name;
    QString avatar;
    db::SqliteCppDb db;
    std::filesystem::path dbPath;
    QQmlPropertyMap* themeConfig;

    struct ProfileDTO
    {
        int64_t id;
        std::string name;
        std::string avatar;
    };

    auto save() -> void;

  public:
    /**
     * @brief Creates a profiles object living in the given database.
     * If the profiles doesn't exist, it will be created.
     * @param dbPath Path to the database file. Doesn't have to exist.
     * @param parent QObject parent.
     */
    explicit Profile(
      const std::filesystem::path& dbPath,
      const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
      QObject* parent = nullptr);

    auto getName() const -> QString;
    auto getAvatar() const -> QString;
    void setName(QString newName);
    void setAvatar(QString newAvatar);
    auto getPath() const -> std::filesystem::path;
    auto getPathQString() const -> QString;
    auto getDb() -> db::SqliteCppDb&;
    auto getThemeConfig() const -> QQmlPropertyMap*;

  signals:
    void nameChanged(QString name);
    void avatarChanged(QString avatar);
};

} // namespace resource_managers

#endif // RHYTHMGAME_PROFILE_H
