//
// Created by bobini on 27.09.23.
//

#include "Profile.h"

#include "ScanThemes.h"
#include "SerializeConfig.h"
#include "gameplay_logic/ChartData.h"
#include "support/Compress.h"
#include "support/PathToQString.h"
#include "input/GamepadManager.h"
#include "support/PathToUtfString.h"
#include <qt6keychain/keychain.h>

#include <QNetworkCookie>
#include <QJsonObject>
#include <spdlog/spdlog.h>

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
Profile::loadBearerToken() const -> QByteArray
{
    auto job = QKeychain::ReadPasswordJob(keychainService);
    job.setKey(QStringLiteral("profiles/%1/token").arg(guid));
    auto loop = QEventLoop();
    connect(
      &job, &QKeychain::ReadPasswordJob::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error()) {
        spdlog::error("Error loading bearer token from keychain: {}",
                      job.errorString().toStdString());
        return QByteArray{};
    }
    return job.binaryData();
}
void
Profile::fetchOnlineData()
{
    auto request = networkRequestFactory.createRequest("user");
    auto reply = networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Error fetching online data: {}",
                          reply->errorString().toStdString());
            reply->deleteLater();
            return;
        }
        auto data = reply->readAll();
        auto json = QJsonDocument::fromJson(data).object();
        setLoggedIn(true);
        setOnlineUsername(json["user"].toObject()["name"].toString());
        reply->deleteLater();
    });
}
void
Profile::setOnlineUsername(const QString& username)
{
    if (username == onlineUsername) {
        return;
    }
    onlineUsername = username;
    emit onlineUsernameChanged();
}
void
Profile::setLoggedIn(bool loggedIn)
{
    if (loggedIn == this->loggedIn) {
        return;
    }
    this->loggedIn = loggedIn;
    emit loggedInChanged();
}
Profile::Profile(
  const std::filesystem::path& mainDbPath,
  const std::filesystem::path& dbPath,
  const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
  QList<QString> assetsPaths,
  QObject* parent)
  : QObject(parent)
  , db(createDb(dbPath))
  , dbPath(dbPath)
  , themeConfig(
      createConfig(themeFamilies, dbPath.parent_path() / "theme_config.json")
        .release())
  , vars(this, themeFamilies, std::move(assetsPaths))
{
    db.execute("CREATE TABLE IF NOT EXISTS properties ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "key TEXT NOT NULL UNIQUE,"
               "value"
               ");");
    auto versionStmt =
      db.createStatement("SELECT value FROM properties WHERE key = 'version';");
    auto version = versionStmt.executeAndGet<int64_t>().transform(
      [](int64_t v) { return support::unpackVersion(v); });
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
    networkRequestFactory.setBaseUrl(vars.getGeneralVars()->getWebApiUri());
    auto headers = networkRequestFactory.commonHeaders();
    headers.append(QHttpHeaders::WellKnownHeader::ContentType,
                   "application/json");
    networkRequestFactory.setCommonHeaders(headers);
    if (auto token = loadBearerToken(); !token.isEmpty()) {
        networkRequestFactory.setBearerToken(token);
        fetchOnlineData();
    }
    auto attachStatement = db.createStatement("ATTACH DATABASE ? AS song_db;");
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
               "scratch_count INTEGER NOT NULL,"
               "ln_count INTEGER NOT NULL,"
               "bss_count INTEGER NOT NULL,"
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
               "dp_options INTEGER NOT NULL,"
               "game_version INTEGER NOT NULL"
               ");");
    // For migration from earlier versions that did not have scratch_count
    auto checkScratchColumn =
      db.createStatement("SELECT COUNT(*) FROM pragma_table_info('score') "
                         "WHERE name = 'scratch_count';");
    auto scratchColumnExists =
      checkScratchColumn.executeAndGet<int64_t>().value_or(0) > 0;
    if (!scratchColumnExists) {
        db.execute(
          "ALTER TABLE score ADD COLUMN scratch_count INTEGER NOT NULL "
          "DEFAULT 0;");
    }
    // For migration from earlier versions that did not have bss_count
    auto checkColumn =
      db.createStatement("SELECT COUNT(*) FROM pragma_table_info('score') "
                         "WHERE name = 'bss_count';");
    auto columnExists = checkColumn.executeAndGet<int64_t>().value_or(0) > 0;
    if (!columnExists) {
        db.execute(
          "ALTER TABLE score ADD COLUMN bss_count INTEGER NOT NULL DEFAULT 0;");
    }
    db.execute("CREATE TABLE IF NOT EXISTS score_course ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "guid TEXT NOT NULL UNIQUE,"
               "identifier TEXT NOT NULL,"
               "score_guids TEXT NOT NULL,"
               "clear_type TEXT NOT NULL,"
               "max_combo INTEGER NOT NULL,"
               "constraints TEXT NOT NULL,"
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
               "gauge_info BLOB NOT NULL,"
               "FOREIGN KEY(score_guid) REFERENCES score(guid)"
               ");");
    auto stmt = db.createStatement(
      "INSERT OR REPLACE INTO properties (key, value) VALUES "
      "('version', ?);");
    stmt.bind(1, static_cast<int64_t>(support::currentVersion));
    stmt.execute();
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
void
Profile::login(const QString& email, const QString& password)
{
    const auto request = networkRequestFactory.createRequest("signin");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QNetworkReply* reply =
      networkManager.post(request, QJsonDocument(json).toJson());

    connect(reply, &QNetworkReply::finished, [reply, this] {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject obj = doc.object();
            QString token = obj["token"].toString();
            if (!token.isEmpty()) {
                auto* job =
                  new QKeychain::WritePasswordJob(keychainService, this);
                job->setKey(QStringLiteral("profiles/%1/token").arg(guid));
                job->setBinaryData(token.toLatin1());
                connect(job, &QKeychain::Job::finished, [job] {
                    if (job->error()) {
                        spdlog::error(
                          "Failed to store token in keychain: {} - {}",
                          magic_enum::enum_name(job->error()),
                          job->errorString().toStdString());
                    }
                });
                job->start();
                networkRequestFactory.setBearerToken(token.toLatin1());
                fetchOnlineData();
            } else {
                spdlog::error("Login response did not contain a token");
            }
            reply->deleteLater();
        } else {
            spdlog::error("Login request failed: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
            reply->deleteLater();
        }
    });
}
void
Profile::logout()
{
    auto* job = new QKeychain::DeletePasswordJob(keychainService, this);
    job->setKey(QStringLiteral("profiles/%1/token").arg(guid));
    connect(job, &QKeychain::Job::finished, [job] {
        if (job->error()) {
            spdlog::error("Failed to delete token from keychain: {} - {}",
                          magic_enum::enum_name(job->error()),
                          job->errorString().toStdString());
        }
        job->deleteLater();
    });
    job->start();
    networkRequestFactory.setBearerToken({});
    setLoggedIn(false);
    setOnlineUsername({});
}
auto
Profile::getOnlineUsername() const -> QString
{
    return onlineUsername;
}
auto
Profile::getLoggedIn() const -> bool
{
    return loggedIn;
}
void
Profile::submitScore(const gameplay_logic::BmsScore& score,
                     const gameplay_logic::ChartData& chartData)
{
    QJsonObject json;
    json["scoreData"] = score.getResult()->toJson();
    json["chartData"] = chartData.toJson();
    json["replayData"] = score.getReplayData()->toJsonArray();
    json["gaugeHistory"] = score.getGaugeHistory()->toJsonArray();
    auto request = networkRequestFactory.createRequest("scores");
    auto reply = networkManager.post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Error submitting score: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
        } else {
            spdlog::info("Score submitted successfully");
        }
        reply->deleteLater();
    });
}
} // namespace resource_managers
