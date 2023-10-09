//
// Created by bobini on 27.09.23.
//

#include "Profile.h"
#include "support/PathToQString.h"

namespace resource_managers {

auto
createDb(const std::filesystem::path& dbPath) -> db::SqliteCppDb
{
    std::filesystem::create_directories(dbPath.parent_path());
#ifdef _WIN32
    return db::SqliteCppDb(
      QString::fromStdWString(dbPath.wstring()).toStdString());
#else
    return db::SqliteCppDb(dbPath);
#endif
}

auto
Profile::save() -> void
{
    // get count of rows in profiles table
    auto countStatement = db.createStatement("SELECT COUNT(*) FROM profiles");
    auto result = countStatement.executeAndGet<int>();
    if (!result.has_value()) {
        throw std::runtime_error(
          "Failed to get count of rows in profiles table");
    }
    if (result.value() == 0) {
        // insert new row
        auto statement =
          db.createStatement("INSERT INTO profiles (id, name, avatar) "
                             "VALUES (1, ?, ?)");
        statement.bind(1, name.toStdString());
        statement.bind(2, avatar.toStdString());
        statement.execute();
        return;
    }
    auto statement =
      db.createStatement("UPDATE profiles SET name = ?, avatar = ? "
                         "WHERE id = 1");
    statement.bind(1, name.toStdString());
    statement.bind(2, avatar.toStdString());
}
auto
Profile::getName() const -> QString
{
    return name;
}
auto
Profile::getAvatar() const -> QString
{
    return avatar;
}

void
Profile::setName(QString newName)
{
    if (name == newName) {
        return;
    }
    name = std::move(newName);
    save();
    emit nameChanged(name);
}
void
Profile::setAvatar(QString newAvatar)
{
    if (avatar == newAvatar) {
        return;
    }
    avatar = std::move(newAvatar);
    save();
    emit avatarChanged(avatar);
}
Profile::Profile(std::filesystem::path dbPath, QObject* parent)
  : QObject(parent)
  , db(createDb(dbPath))
  , dbPath(std::move(dbPath))
{
    if (!db.hasTable("profiles")) {
        db.execute("CREATE TABLE profiles ("
                   "id INTEGER PRIMARY KEY CHECK (id = 1),"
                   "name TEXT NOT NULL,"
                   "avatar TEXT NOT NULL"
                   ");");
        name = "Player";
        avatar = "mascot.png";
        save();
    } else {
        auto statement = db.createStatement("SELECT * FROM profiles");
        auto result = statement.executeAndGet<ProfileDTO>();
        if (!result.has_value()) {
            // this is very unlikely to happen
            name = "Player";
            avatar = "mascot.png";
            save();
        } else {
            name = QString::fromStdString(result->name);
            avatar = QString::fromStdString(result->avatar);
        }
    }
    db.execute("CREATE TABLE IF NOT EXISTS score ("
               "id INTEGER PRIMARY KEY,"
               "sha256 TEXT NOT NULL,"
               "points INTEGER NOT NULL,"
               "max_points INTEGER NOT NULL,"
               "max_hits INTEGER NOT NULL,"
               "max_combo INTEGER NOT NULL,"
               "poor INTEGER NOT NULL,"
               "empty_poor INTEGER NOT NULL,"
               "bad INTEGER NOT NULL,"
               "good INTEGER NOT NULL,"
               "great INTEGER NOT NULL,"
               "perfect INTEGER NOT NULL,"
               "clear_type TEXT NOT NULL,"
               "unix_timestamp INTEGER NOT NULL"
               ");");
    db.execute("CREATE TABLE IF NOT EXISTS replay_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "score_id INTEGER NOT NULL,"
               "replay_data BLOB NOT NULL,"
               "FOREIGN KEY(score_id) REFERENCES score(id)"
               ");");
    db.execute("CREATE TABLE IF NOT EXISTS gauge_history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "score_id INTEGER NOT NULL,"
               "gauge_history BLOB NOT NULL,"
               "FOREIGN KEY(score_id) REFERENCES score(id)"
               ");");
}
auto
Profile::getPath() const -> std::filesystem::path
{
    return dbPath;
}
auto
Profile::getPathQString() const -> QString
{
    return support::pathToQString(dbPath);
}
auto
Profile::getDb() -> db::SqliteCppDb&
{
    return db;
}
} // namespace resource_managers