//
// Created by bobini on 27.09.23.
//

#include "Profile.h"

#include "ScanThemes.h"
#include "SerializeConfig.h"
#include "support/Compress.h"
#include "support/PathToQString.h"
#include "input/GamepadManager.h"
#include "support/PathToUtfString.h"

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

Profile::Profile(
  const std::filesystem::path& mainDbPath,
  const std::filesystem::path& dbPath,
  const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
  QString avatarPath,
  QObject* parent)
  : QObject(parent)
  , db(createDb(dbPath))
  , dbPath(dbPath)
  , themeConfig(
      createConfig(themeFamilies, dbPath.parent_path() / "theme_config.json")
        .release())
  , vars(this, themeFamilies, std::move(avatarPath))
{
    auto attachStatement = db.createStatement(
      "ATTACH DATABASE ? AS song_db;");
    attachStatement.bind(1, support::pathToUtfString(mainDbPath));
    attachStatement.execute();
    this->themeConfig->setParent(this);
    auto configPath = dbPath.parent_path() / "theme_config.json";
    connect(themeConfig,
            &QQmlPropertyMap::valueChanged,
            this,
            [this, configPath](const QString&, const QVariant&) {
                writeConfig(configPath, *themeConfig);
            });
    writeConfig(configPath, *themeConfig);
    db.execute("CREATE TABLE IF NOT EXISTS score ("
               "id INTEGER PRIMARY KEY,"
               "guid TEXT NOT NULL UNIQUE,"
               "sha256 TEXT NOT NULL,"
               "md5 TEXT NOT NULL,"
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
               "length INTEGER NOT NULL,"
               "random_sequence STRING NOT NULL,"
               "random_seed INTEGER NOT NULL,"
               "note_order_algorithm INTEGER NOT NULL,"
               "note_order_algorithm_p2 INTEGER NOT NULL,"
               "game_version INTEGER NOT NULL"
               ");");
    db.execute("CREATE TABLE IF NOT EXISTS score_course ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "guid TEXT NOT NULL UNIQUE,"
                "identifier TEXT NOT NULL,"
                "score_guids TEXT NOT NULL,"
                "clear_type TEXT NOT NULL,"
                "max_combo INTEGER NOT NULL,"
                "constraints TEXT NOT NULL,"
                "trophies TEXT NOT NULL,"
                "unix_timestamp INTEGER NOT NULL,"
                "game_version INTEGER NOT NULL);");
    db.execute("CREATE TABLE IF NOT EXISTS replay_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "score_guid TEXT NOT NULL UNIQUE,"
               "replay_data BLOB NOT NULL,"
               "FOREIGN KEY(score_guid) REFERENCES score(guid)"
               ");");
    db.execute("CREATE TABLE IF NOT EXISTS gauge_history ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "score_guid TEXT NOT NULL UNIQUE,"
               "gauge_history BLOB NOT NULL,"
               "gauge_info BLOB NOT NULL,"
               "FOREIGN KEY(score_guid) REFERENCES score(guid)"
               ");");
    db.execute("CREATE TABLE IF NOT EXISTS properties ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "key TEXT NOT NULL UNIQUE,"
               "value"
               ");");
    const auto folderName = dbPath.parent_path().filename();
    auto statement = db.createStatement("INSERT OR IGNORE INTO properties "
                                        "(key, value) VALUES ('guid', ?);");
    statement.bind(1, folderName.string());
    statement.execute();
    auto getGuid = db.createStatement("SELECT value FROM properties "
                                      "WHERE key = 'guid';");
    auto guidResult =
      getGuid.executeAndGet<std::string>().value_or(std::string{});
    guid = QString::fromStdString(guidResult);
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
Profile::getScoreDb() -> qml_components::ScoreDb*
{
    return &scoreDb;
}
auto
Profile::getThemeConfig() const -> QQmlPropertyMap*
{
    return themeConfig;
}
auto
Profile::getVars() -> Vars*
{
    return &vars;
}
auto
Profile::getGuid() const -> QString
{
    return guid;
}
} // namespace resource_managers