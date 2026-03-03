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
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsScore.h"
#include <QJsonArray>

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

void
Profile::loadBearerToken()
{
    auto* job = new QKeychain::ReadPasswordJob(keychainService, this);
    job->setKey(QStringLiteral("profiles/%1/token").arg(guid));
    connect(job, &QKeychain::Job::finished, this, [this, job]() {
        job->deleteLater();
        if (job->error()) {
            return;
        }
        const auto token = job->binaryData();
        if (!token.isEmpty()) {
            networkRequestFactory.setBearerToken(token);
            fetchOnlineData();
        }
    });
    job->start();
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
    loadBearerToken();
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
auto
Profile::login(const QString& email, const QString& password)
  -> QIfPendingReply<void>
{
    auto outerReply = QIfPendingReply<void>{};
    const auto request = networkRequestFactory.createRequest("signin");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QNetworkReply* reply =
      networkManager.post(request, QJsonDocument(json).toJson());

    connect(
      reply, &QNetworkReply::finished, [reply, this, outerReply]() mutable {
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
                  outerReply.setSuccess();
              } else {
                  spdlog::error("Login response did not contain a token");
                  outerReply.setFailed();
              }
              reply->deleteLater();
          } else {
              spdlog::error("Login request failed: {} - {}",
                            magic_enum::enum_name(reply->error()),
                            reply->errorString().toStdString());
              outerReply.setFailed();
              reply->deleteLater();
          }
      });
    return outerReply;
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
auto
Profile::submitScore(const gameplay_logic::BmsScore& score,
                     const gameplay_logic::ChartData& chartData)
  -> QNetworkReply*
{
    if (score.getResult()->getGuid().isEmpty()) {
        return nullptr;
    }
    QJsonObject json;
    json["scoreData"] = score.getResult()->toJson();
    json["chartData"] = chartData.toJson();
    json["replayData"] = score.getReplayData()->toJsonArray();
    json["gaugeHistory"] = score.getGaugeHistory()->toJsonArray();
    auto request = networkRequestFactory.createRequest("user/scores");
    return networkManager.post(request, QJsonDocument(json).toJson());
}
auto
Profile::uploadScores() -> QIfPendingReply<int>
{
    auto outerReply = QIfPendingReply<int>{};

    scoreDb.runOnDbThread([this, outerReply]() mutable {
        try {
            auto stmt = db.createStatement("SELECT guid FROM score");
            auto rows = stmt.executeAndGetAll<std::string>();
            QJsonArray guidsArray;
            for (const auto& r : rows) {
                guidsArray.append(QString::fromStdString(r));
            }

            QMetaObject::invokeMethod(
              this,
              [this, outerReply, guidsArray]() mutable {
                  auto request =
                    networkRequestFactory.createRequest("user/scores/unknown");
                  QNetworkReply* unknownReply = networkManager.post(
                    request, QJsonDocument(guidsArray).toJson());

                  connect(
                    unknownReply,
                    &QNetworkReply::finished,
                    [this, unknownReply, outerReply]() mutable {
                        if (unknownReply->error() != QNetworkReply::NoError) {
                            spdlog::error(
                              "user/scores/unknown failed: {} - {}",
                              magic_enum::enum_name(unknownReply->error()),
                              unknownReply->errorString().toStdString());
                            outerReply.setFailed();
                            unknownReply->deleteLater();
                            return;
                        }
                        auto data = unknownReply->readAll();
                        auto doc = QJsonDocument::fromJson(data);
                        if (!doc.isArray()) {
                            spdlog::error("scores/unknown returned non-array");
                            outerReply.setFailed();
                            unknownReply->deleteLater();
                            return;
                        }
                        auto array = doc.array();
                        auto guids = QList<QString>{};
                        for (const auto& v : array) {
                            if (v.isString())
                                guids.append(v.toString());
                        }
                        unknownReply->deleteLater();

                        if (guids.isEmpty()) {
                            outerReply.setSuccess(0);
                            return;
                        }

                        scoreDb.runOnDbThread(
                          [this, guids, outerReply]() mutable {
                              try {
                                  constexpr auto columns =
                                    "score.max_points, score.max_hits, "
                                    "score.normal_note_count, "
                                    "score.scratch_count, score.ln_count, "
                                    "score.bss_count, score.mine_count, "
                                    "score.clear_type, score.points, "
                                    "score.max_combo, score.poor, "
                                    "score.empty_poor, score.bad, score.good, "
                                    "score.great, score.perfect, "
                                    "score.mine_hits, score.guid, score.sha256,"
                                    " score.md5, score.unix_timestamp, "
                                    "score.length, score.random_sequence, "
                                    "score.random_seed, "
                                    "score.note_order_algorithm, "
                                    "score.note_order_algorithm_p2, "
                                    "score.dp_options, score.game_version, "
                                    "replay_data.*, gauge_history.* ";

                                  auto stmtStr =
                                    std::string("SELECT ") + columns +
                                    "FROM score JOIN replay_data ON "
                                    "score.guid = replay_data.score_guid "
                                    "JOIN gauge_history ON score.guid = "
                                    "gauge_history.score_guid "
                                    "WHERE score.guid IN (" +
                                    QString("?, ")
                                      .repeated(guids.size())
                                      .chopped(2)
                                      .toStdString() +
                                    ") ORDER BY score.unix_timestamp DESC";

                                  auto statement = db.createStatement(stmtStr);
                                  for (int i = 0; i < guids.size(); ++i) {
                                      statement.bind(i + 1,
                                                     guids[i].toStdString());
                                  }
                                  const auto results =
                                    statement.executeAndGetAll<std::tuple<
                                      gameplay_logic::BmsResult::DTO,
                                      gameplay_logic::BmsReplayData::DTO,
                                      gameplay_logic::BmsGaugeHistory::DTO>>();

                                  QJsonArray payload;
                                  for (const auto& row : results) {
                                      auto result =
                                        gameplay_logic::BmsResult::load(
                                          std::get<0>(row));
                                      auto replay =
                                        gameplay_logic::BmsReplayData::load(
                                          std::get<1>(row));
                                      auto gauge =
                                        gameplay_logic::BmsGaugeHistory::load(
                                          std::get<2>(row));
                                      QJsonObject obj;
                                      obj["scoreData"] = result->toJson();
                                      auto chartStmt = db.createStatement(
                                        "SELECT charts.id, charts.title, "
                                        "charts.artist, charts.subtitle, "
                                        "charts.subartist, charts.genre, "
                                        "charts.stage_file, charts.banner, "
                                        "charts.back_bmp, charts.rank, "
                                        "charts.total, charts.play_level, "
                                        "charts.difficulty, charts.is_random, "
                                        "charts.random_sequence, "
                                        "charts.normal_note_count, "
                                        "charts.scratch_count, charts.ln_count,"
                                        " charts.bss_count, charts.mine_count, "
                                        "charts.length, charts.initial_bpm, "
                                        "charts.max_bpm, charts.min_bpm, "
                                        "charts.main_bpm, charts.avg_bpm, "
                                        "charts.peak_density, "
                                        "charts.avg_density, "
                                        "charts.end_density, charts.path, "
                                        "charts.directory, charts.sha256, "
                                        "charts.md5, charts.keymode, "
                                        "h.bpms, h.histogram_data "
                                        "FROM song_db.charts LEFT JOIN "
                                        "song_db.histogram_data h "
                                        "ON h.chart_id = charts.id "
                                        "WHERE charts.md5 = ? LIMIT 1");
                                      chartStmt.bind(1, std::get<0>(row).md5);
                                      const auto chartResults =
                                        chartStmt.executeAndGetAll<
                                          gameplay_logic::ChartData::DTO>();
                                      if (chartResults.empty())
                                          continue;
                                      auto chart =
                                        gameplay_logic::ChartData::load(
                                          chartResults.front());
                                      obj["chartData"] = chart->toJson();
                                      obj["replayData"] = replay->toJsonArray();
                                      obj["gaugeHistory"] =
                                        gauge->toJsonArray();
                                      payload.append(obj);
                                  }

                                  if (payload.isEmpty()) {
                                      QMetaObject::invokeMethod(
                                        this,
                                        [outerReply]() mutable {
                                            outerReply.setSuccess(0);
                                        },
                                        Qt::QueuedConnection);
                                      return;
                                  }

                                  const auto uploadedCount = payload.size();
                                  QMetaObject::invokeMethod(
                                    this,
                                    [this,
                                     payload = std::move(payload),
                                     outerReply,
                                     uploadedCount]() mutable {
                                        auto bulkRequest =
                                          networkRequestFactory.createRequest(
                                            "user/scores/bulk");
                                        auto* bulkReply = networkManager.post(
                                          bulkRequest,
                                          QJsonDocument(payload).toJson());
                                        connect(
                                          bulkReply,
                                          &QNetworkReply::finished,
                                          [bulkReply,
                                           outerReply,
                                           uploadedCount]() mutable {
                                              if (bulkReply->error() !=
                                                  QNetworkReply::NoError) {
                                                  spdlog::error(
                                                    "user/scores/bulk failed: "
                                                    "{} - {}",
                                                    magic_enum::enum_name(
                                                      bulkReply->error()),
                                                    bulkReply->errorString()
                                                      .toStdString());
                                                  outerReply.setFailed();
                                              } else {
                                                  outerReply.setSuccess(
                                                    static_cast<int>(
                                                      uploadedCount));
                                              }
                                              bulkReply->deleteLater();
                                          });
                                    },
                                    Qt::QueuedConnection);
                              } catch (const std::exception& e) {
                                  spdlog::error(
                                    "Error building upload payload: {}",
                                    e.what());
                                  QMetaObject::invokeMethod(
                                    this,
                                    [outerReply]() mutable {
                                        outerReply.setFailed();
                                    },
                                    Qt::QueuedConnection);
                              }
                          });
                    });
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
            spdlog::error("Error in uploadScores: {}", e.what());
            QMetaObject::invokeMethod(
              this,
              [outerReply]() mutable { outerReply.setFailed(); },
              Qt::QueuedConnection);
        }
    });
    return outerReply;
}
auto
Profile::downloadScores() -> QIfPendingReply<int>
{
    auto outerReply = QIfPendingReply<int>{};

    scoreDb.runOnDbThread([this, outerReply]() mutable {
        try {
            auto stmt = db.createStatement("SELECT guid FROM score");
            auto rows = stmt.executeAndGetAll<std::string>();
            QJsonArray guidsArray;
            for (const auto& r : rows) {
                guidsArray.append(QString::fromStdString(r));
            }

            QMetaObject::invokeMethod(
              this,
              [this, outerReply, guidsArray]() mutable {
                  auto request =
                    networkRequestFactory.createRequest("user/scores/missing");
                  QNetworkReply* missingReply = networkManager.post(
                    request, QJsonDocument(guidsArray).toJson());

                  connect(
                    missingReply,
                    &QNetworkReply::finished,
                    [this, missingReply, outerReply]() mutable {
                        if (missingReply->error() != QNetworkReply::NoError) {
                            spdlog::error(
                              "user/scores/missing failed: {} - {}",
                              magic_enum::enum_name(missingReply->error()),
                              missingReply->errorString().toStdString());
                            outerReply.setFailed();
                            missingReply->deleteLater();
                            return;
                        }
                        auto data = missingReply->readAll();
                        auto doc = QJsonDocument::fromJson(data);
                        if (!doc.isArray()) {
                            spdlog::error("scores/missing returned non-array");
                            outerReply.setFailed();
                            missingReply->deleteLater();
                            return;
                        }
                        auto array = doc.array();
                        missingReply->deleteLater();

                        if (array.isEmpty()) {
                            outerReply.setSuccess(0);
                            return;
                        }

                        scoreDb.runOnDbThread([this,
                                               array,
                                               outerReply]() mutable {
                            try {
                                int processed = 0;
                                for (const auto& v : array) {
                                    if (!v.isObject())
                                        continue;
                                    auto o = v.toObject();
                                    auto scoreObj = o["scoreData"].toObject();
                                    auto chartObj = o["chartData"].toObject();
                                    auto replayArr = o["replayData"].toArray();
                                    auto gaugeArr = o["gaugeHistory"].toArray();

                                    if (!chartObj.isEmpty()) {
                                        QString title =
                                          chartObj["title"].toString();
                                        QString artist =
                                          chartObj["artist"].toString();
                                        QString subtitle =
                                          chartObj["subtitle"].toString();
                                        QString subartist =
                                          chartObj["subartist"].toString();
                                        QString genre =
                                          chartObj["genre"].toString();
                                        QString stageFile =
                                          chartObj["stageFile"].toString();
                                        QString banner =
                                          chartObj["banner"].toString();
                                        QString backBmp =
                                          chartObj["backBmp"].toString();
                                        int rank = chartObj["rank"].toInt();
                                        double total =
                                          chartObj["total"].toDouble();
                                        int playLevel =
                                          chartObj["playLevel"].toInt();
                                        int difficulty =
                                          chartObj["difficulty"].toInt();
                                        int normalNoteCount =
                                          chartObj["normalNoteCount"].toInt();
                                        int scratchCount =
                                          chartObj["scratchCount"].toInt();
                                        int lnCount =
                                          chartObj["lnCount"].toInt();
                                        int bssCount =
                                          chartObj["bssCount"].toInt();
                                        int mineCount =
                                          chartObj["mineCount"].toInt();
                                        int64_t length = static_cast<int64_t>(
                                          chartObj["length"]
                                            .toVariant()
                                            .toLongLong());
                                        double initialBpm =
                                          chartObj["initialBpm"].toDouble();
                                        double maxBpm =
                                          chartObj["maxBpm"].toDouble();
                                        double minBpm =
                                          chartObj["minBpm"].toDouble();
                                        double mainBpm =
                                          chartObj["mainBpm"].toDouble();
                                        double avgBpm =
                                          chartObj["avgBpm"].toDouble();
                                        double peakDensity =
                                          chartObj["peakDensity"].toDouble();
                                        double avgDensity =
                                          chartObj["avgDensity"].toDouble();
                                        double endDensity =
                                          chartObj["endDensity"].toDouble();
                                        QString path =
                                          chartObj["path"].toString();
                                        int64_t directory = -1;
                                        if (chartObj.contains("directory")) {
                                            directory = static_cast<int64_t>(
                                              chartObj["directory"]
                                                .toVariant()
                                                .toLongLong());
                                        }
                                        QString sha256 =
                                          chartObj["sha256"].toString();
                                        QString md5 =
                                          chartObj["md5"].toString();
                                        bool isRandom =
                                          chartObj["isRandom"].toBool();
                                        QList<qint64> randomSequence;
                                        if (chartObj.contains(
                                              "randomSequence") &&
                                            chartObj["randomSequence"]
                                              .isArray()) {
                                            for (const auto& rv :
                                                 chartObj["randomSequence"]
                                                   .toArray()) {
                                                randomSequence.append(
                                                  static_cast<qint64>(
                                                    rv.toVariant()
                                                      .toLongLong()));
                                            }
                                        }
                                        auto keymode = gameplay_logic::
                                          ChartData::Keymode::K7;
                                        if (chartObj.contains("keymode")) {
                                            keymode =
                                              static_cast<gameplay_logic::
                                                            ChartData::Keymode>(
                                                chartObj["keymode"].toInt(
                                                  static_cast<int>(
                                                    gameplay_logic::ChartData::
                                                      Keymode::K7)));
                                        }
                                        QList<QList<qint64>> histogramData;
                                        if (chartObj.contains(
                                              "histogramData") &&
                                            chartObj["histogramData"]
                                              .isArray()) {
                                            for (const auto& listV :
                                                 chartObj["histogramData"]
                                                   .toArray()) {
                                                QList<qint64> sub;
                                                if (listV.isArray()) {
                                                    for (const auto& item :
                                                         listV.toArray()) {
                                                        sub.append(
                                                          static_cast<qint64>(
                                                            item.toVariant()
                                                              .toLongLong()));
                                                    }
                                                }
                                                histogramData.append(sub);
                                            }
                                        }
                                        QList<gameplay_logic::BpmChange>
                                          bpmChanges;
                                        if (chartObj.contains("bpmChanges") &&
                                            chartObj["bpmChanges"].isArray()) {
                                            for (const auto& bv :
                                                 chartObj["bpmChanges"]
                                                   .toArray()) {
                                                if (!bv.isObject())
                                                    continue;
                                                auto bo = bv.toObject();
                                                gameplay_logic::BpmChange bc;
                                                bc.time.timestamp =
                                                  static_cast<int64_t>(
                                                    bo["time"]
                                                      .toVariant()
                                                      .toLongLong());
                                                bc.time.position =
                                                  bo["position"].toDouble();
                                                bc.bpm = bo["bpm"].toDouble();
                                                bpmChanges.append(bc);
                                            }
                                        }
                                        auto chart = std::make_unique<
                                          gameplay_logic::ChartData>(
                                          std::move(title),
                                          std::move(artist),
                                          std::move(subtitle),
                                          std::move(subartist),
                                          std::move(genre),
                                          std::move(stageFile),
                                          std::move(banner),
                                          std::move(backBmp),
                                          rank,
                                          total,
                                          playLevel,
                                          difficulty,
                                          isRandom,
                                          std::move(randomSequence),
                                          normalNoteCount,
                                          scratchCount,
                                          lnCount,
                                          bssCount,
                                          mineCount,
                                          length,
                                          initialBpm,
                                          maxBpm,
                                          minBpm,
                                          mainBpm,
                                          avgBpm,
                                          peakDensity,
                                          avgDensity,
                                          endDensity,
                                          std::move(path),
                                          directory,
                                          std::move(sha256),
                                          std::move(md5),
                                          keymode,
                                          std::move(histogramData),
                                          std::move(bpmChanges));
                                        if (chart)
                                            chart->save(db);
                                    }

                                    auto result =
                                      gameplay_logic::BmsResult::fromJson(
                                        scoreObj);
                                    auto hits = gameplay_logic::BmsReplayData::
                                      fromJsonArray(replayArr);
                                    auto replay = std::make_unique<
                                      gameplay_logic::BmsReplayData>(
                                      hits, result->getGuid());
                                    auto gauges = gameplay_logic::
                                      BmsGaugeHistory::fromJsonArray(gaugeArr);
                                    auto gaugeHistory = std::make_unique<
                                      gameplay_logic::BmsGaugeHistory>(
                                      gauges, result->getGuid());
                                    auto score = gameplay_logic::BmsScore(
                                      std::move(result),
                                      std::move(replay),
                                      std::move(gaugeHistory));
                                    score.save(db);
                                    ++processed;
                                }

                                QMetaObject::invokeMethod(
                                  this,
                                  [outerReply, processed]() mutable {
                                      outerReply.setSuccess(processed);
                                  },
                                  Qt::QueuedConnection);
                            } catch (const std::exception& e) {
                                spdlog::error(
                                  "Error in downloadScores processing: {}",
                                  e.what());
                                QMetaObject::invokeMethod(
                                  this,
                                  [outerReply]() mutable {
                                      outerReply.setFailed();
                                  },
                                  Qt::QueuedConnection);
                            }
                        });
                    });
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
            spdlog::error("Error in downloadScores: {}", e.what());
            QMetaObject::invokeMethod(
              this,
              [outerReply]() mutable { outerReply.setFailed(); },
              Qt::QueuedConnection);
        }
    });
    return outerReply;
}
} // namespace resource_managers
