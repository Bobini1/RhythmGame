//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_PROFILE_H
#define RHYTHMGAME_PROFILE_H

#include <QObject>
#include "db/SqliteCppDb.h"
namespace resource_managers {

class Profile : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(
      QString avatar READ getAvatar WRITE setAvatar NOTIFY avatarChanged)
    Q_PROPERTY(QString path READ getPathQString CONSTANT)
    QString name;
    QString avatar;
    db::SqliteCppDb db;
    std::filesystem::path dbPath;

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
    explicit Profile(std::filesystem::path dbPath, QObject* parent = nullptr);

    auto getName() const -> QString;
    auto getAvatar() const -> QString;
    void setName(QString newName);
    void setAvatar(QString newAvatar);
    auto getPath() const -> std::filesystem::path;
    auto getPathQString() const -> QString;
    auto getDb() -> db::SqliteCppDb&;

  signals:
    void nameChanged(QString name);
    void avatarChanged(QString avatar);
};

} // namespace resource_managers

#endif // RHYTHMGAME_PROFILE_H
