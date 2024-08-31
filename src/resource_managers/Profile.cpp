//
// Created by bobini on 27.09.23.
//

#include "Profile.h"

#include "ScanThemes.h"
#include "SerializeConfig.h"
#include "support/Compress.h"
#include "support/PathToQString.h"
#include "input/GamepadManager.h"

namespace resource_managers {

namespace {
auto
createDb(const std::filesystem::path& dbPath) -> db::SqliteCppDb
{
    create_directories(dbPath.parent_path());
    return db::SqliteCppDb(dbPath);
}

auto
createConfig(const QMap<QString, qml_components::ThemeFamily>& availableThemes,
             const std::filesystem::path& themeConfig)
  -> std::unique_ptr<QQmlPropertyMap>
{
    auto config = std::make_unique<QQmlPropertyMap>();
    fillWithDefaults(*config, availableThemes);
    readConfig(themeConfig, *config, availableThemes);
    config->freeze();
    return config;
}
} // namespace

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
    auto updateProperty =
      db.createStatement("INSERT OR REPLACE INTO properties "
                         "(key, value) VALUES (?, ?)");
    updateProperty.bind(1, "name");
    updateProperty.bind(2, name.toStdString());
    updateProperty.execute();
    emit nameChanged();
}
void
Profile::setAvatar(QString newAvatar)
{
    if (avatar == newAvatar) {
        return;
    }
    avatar = std::move(newAvatar);
    auto updateProperty =
      db.createStatement("INSERT OR REPLACE INTO properties "
                         "(key, value) VALUES (?, ?)");
    updateProperty.bind(1, "avatar");
    updateProperty.bind(2, avatar.toStdString());
    updateProperty.execute();
    emit avatarChanged();
}
Profile::
Profile(const std::filesystem::path& dbPath,
        const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
        QObject* parent)
  : QObject(parent)
  , db(createDb(dbPath))
  , dbPath(dbPath)
  , themeConfig(
      createConfig(themeFamilies, dbPath.parent_path() / "theme_config.json")
        .release())
  , vars(this, themeFamilies)
{
    this->themeConfig->setParent(this);
    auto configPath = dbPath.parent_path() / "theme_config.json";
    connect(themeConfig,
            &QQmlPropertyMap::valueChanged,
            this,
            [this, configPath](const QString&, const QVariant&) {
                writeConfig(configPath, *themeConfig);
            });
    writeConfig(configPath, *themeConfig);
    if (!db.hasTable("properties")) {
        db.execute("CREATE TABLE properties ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "key TEXT NOT NULL UNIQUE,"
                   "value"
                   ");");
    } else {
        auto statement = db.createStatement(
          "SELECT value FROM properties WHERE key = 'avatar'");
        auto av = statement.executeAndGet<std::string>();
        avatar = QString::fromStdString(av.value_or(""));
        statement =
          db.createStatement("SELECT value FROM properties WHERE key = "
                             "'name'");
        auto n = statement.executeAndGet<std::string>();
        name = QString::fromStdString(n.value_or(""));
        statement = db.createStatement("SELECT value FROM properties WHERE "
                                       "key = 'key_config'");
        if (auto config = statement.executeAndGet<std::string>()) {
            auto serializedData = QByteArray::fromStdString(*config);
            support::decompress(serializedData, keyConfig);
        }
    }
    db.execute("CREATE TABLE IF NOT EXISTS score ("
               "id INTEGER PRIMARY KEY,"
               "sha256 TEXT NOT NULL,"
               "points INTEGER NOT NULL,"
               "max_points INTEGER NOT NULL,"
               "max_hits INTEGER NOT NULL,"
               "normal_note_count INTEGER NOT NULL,"
               "ln_count INTEGER NOT NULL,"
               "mine_count INTEGER NOT NULL,"
               "max_combo INTEGER NOT NULL,"
               "poor INTEGER NOT NULL,"
               "empty_poor INTEGER NOT NULL,"
               "bad INTEGER NOT NULL,"
               "good INTEGER NOT NULL,"
               "great INTEGER NOT NULL,"
               "perfect INTEGER NOT NULL,"
               "mine_hits INTEGER NOT NULL,"
               "clear_type TEXT NOT NULL,"
               "unix_timestamp INTEGER NOT NULL,"
               "random_sequence STRING NOT NULL"
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
auto
Profile::getThemeConfig() const -> QQmlPropertyMap*
{
    return themeConfig;
}
auto
Profile::getKeyConfig() const -> QList<input::Mapping>
{
    return keyConfig;
}
auto
Profile::setKeyConfig(const QList<input::Mapping>& keyConfig) -> void
{
    if (this->keyConfig == keyConfig) {
        return;
    }
    this->keyConfig = keyConfig;
    auto compressedData = support::compress(keyConfig);
    auto updateProperty =
      db.createStatement("INSERT OR REPLACE INTO properties "
                         "(key, value) VALUES (?, ?)");
    updateProperty.bind(1, "key_config");
    updateProperty.bind(2, compressedData.data(), compressedData.size());
    updateProperty.execute();
    emit keyConfigChanged();
}
auto
Profile::getVars() -> Vars*
{
    return &vars;
}
} // namespace resource_managers