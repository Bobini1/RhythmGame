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
#include "qml_components/ScoreSyncOperation.h"
#include <qt6keychain/keychain.h>

#include <QNetworkCookie>
#include <QJsonObject>
#include <spdlog/spdlog.h>
#include "gameplay_logic/BmsResult.h"
#include "gameplay_logic/BmsReplayData.h"
#include "gameplay_logic/BmsGaugeHistory.h"
#include "gameplay_logic/BmsScore.h"
#include <QJsonArray>
#include <qnetworkcookiejar.h>

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
constexpr int maxSqlVariables = 999;

// Fetches all local score GUIDs from the DB on a background thread,
// then calls onResult on the main thread.
void
fetchLocalGuids(db::SqliteCppDb& db,
                QThreadPool& threadPool,
                QObject* context,
                std::function<void(QSet<QString>)> onResult,
                std::function<void(QString)> onError)
{
    threadPool.start([&db, context, onResult, onError]() mutable {
        try {
            auto stmt = db.createStatement("SELECT guid FROM score");
            auto rows = stmt.executeAndGetAll<std::string>();
            QSet<QString> localGuids;
            for (const auto& r : rows)
                localGuids.insert(QString::fromStdString(r));

            QMetaObject::invokeMethod(
              context,
              [onResult, localGuids]() mutable {
                  onResult(std::move(localGuids));
              },
              Qt::QueuedConnection);
        } catch (const std::exception& e) {
            QMetaObject::invokeMethod(
              context,
              [onError, msg = std::string(e.what())]() mutable {
                  onError(QString::fromStdString(msg));
              },
              Qt::QueuedConnection);
        }
    });
}

// Fetches server-side GUIDs for a given userId,
// then calls onResult on the main thread.
void
fetchServerGuids(QNetworkAccessManager* networkManager,
                 QNetworkRequestFactory& factory,
                 int userId,
                 QObject* context,
                 std::function<void(QSet<QString>)> onResult,
                 std::function<void(QString)> onError)
{
    auto request = factory.createRequest(
      QStringLiteral("scores?fields=guid&userId=%1").arg(userId));
    auto* reply = networkManager->get(request);

    QObject::connect(
      reply,
      &QNetworkReply::finished,
      context,
      [reply, onResult, onError]() mutable {
          reply->deleteLater();
          if (reply->error() != QNetworkReply::NoError) {
              onError(reply->errorString());
              return;
          }
          auto doc = QJsonDocument::fromJson(reply->readAll());
          if (!doc.isArray()) {
              onError(QStringLiteral("Unexpected server response"));
              return;
          }
          QSet<QString> guids;
          for (const auto& v : doc.array()) {
              if (v.isObject() && v.toObject().contains("guid"))
                  guids.insert(v.toObject()["guid"].toString());
              else
                  spdlog::warn("Skipping invalid score guid entry");
          }
          onResult(std::move(guids));
      });
}
} // namespace

void
Profile::loadBearerToken()
{
    auto* job = new QKeychain::ReadPasswordJob(keychainService, this);
    job->setKey(QStringLiteral("RhythmGame/profiles/%1/token").arg(guid));
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
    setLoginState(LoginState::LoggingIn);
    const auto request = networkRequestFactory.createRequest("users/me");
    auto* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Error fetching online data for user {}: {}",
                          vars.getGeneralVars()->getName().toStdString(),
                          reply->errorString().toStdString());
            setLoginState(LoginState::LoginFailed);
            return;
        }
        const auto data = reply->readAll();
        auto json = QJsonDocument::fromJson(data).object();
        auto onlineData = OnlineUserData{};
        onlineData.username = json["name"].toString();
        onlineData.userId = json["id"].toInt();
        onlineData.image = json["image"].toString();

        setLoginState(LoginState::LoggedIn);
        setOnlineUserData(onlineData);
        if (!json["tachiId"].isNull()) {
            fetchTachiData(json["tachiId"].toInt());
        }
    });
}
void
Profile::setLoginState(LoginState state)
{
    if (state == loginState) {
        return;
    }
    loginState = state;
    emit loginStateChanged();
}
void
Profile::setTachiLoginState(LoginState state)
{
    if (state == tachiLoginState) {
        return;
    }
    tachiLoginState = state;
    emit tachiLoginStateChanged();
}
void
Profile::setOnlineUserData(std::optional<OnlineUserData> userData)
{
    if (this->userData == userData) {
        return;
    }
    this->userData = userData;
    emit onlineUserDataChanged();
}
void
Profile::setTachiData(std::optional<TachiData> tachiData)
{
    if (this->tachiData == tachiData) {
        return;
    }
    this->tachiData = tachiData;
    emit tachiDataChanged();
}
Profile::Profile(
  const std::filesystem::path& mainDbPath,
  const std::filesystem::path& dbPath,
  const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
  QList<QString> assetsPaths,
  QNetworkAccessManager* networkManager,
  QObject* parent)
  : QObject(parent)
  , db(createDb(dbPath))
  , dbPath(dbPath)
  , themeConfig(
      createConfig(themeFamilies, dbPath.parent_path() / "theme_config.json")
        .release())
  , vars(this, themeFamilies, std::move(assetsPaths))
  , networkManager(networkManager)
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
    networkRequestFactory.setBaseUrl(vars.getGeneralVars()->getWebApiUrl());
    connect(
      vars.getGeneralVars(), &GeneralVars::websiteBaseUrlChanged, this, [this] {
          networkRequestFactory.setBaseUrl(
            vars.getGeneralVars()->getWebApiUrl());
          fetchOnlineData();
      });
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
               "keymode INTEGER NOT NULL,"
               "unix_timestamp INTEGER NOT NULL,"
               "length INTEGER NOT NULL,"
               "random_sequence STRING NOT NULL,"
               "random_seed INTEGER NOT NULL,"
               "note_order_algorithm INTEGER NOT NULL,"
               "note_order_algorithm_p2 INTEGER NOT NULL,"
               "dp_options INTEGER NOT NULL,"
               "game_version INTEGER NOT NULL,"
               "owner TEXT NOT NULL DEFAULT ''"
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
    // For migration from earlier versions that did not have source
    auto checkSourceColumn =
      db.createStatement("SELECT COUNT(*) FROM pragma_table_info('score') "
                         "WHERE name = 'owner';");
    auto sourceColumnExists =
      checkSourceColumn.executeAndGet<int64_t>().value_or(0) > 0;
    if (!sourceColumnExists) {
        db.execute("ALTER TABLE score ADD COLUMN owner TEXT NOT NULL DEFAULT "
                   "'';");
    }
    // For migration from earlier versions that did not have keymode
    auto checkKeymodeColumn =
      db.createStatement("SELECT COUNT(*) FROM pragma_table_info('score') "
                         "WHERE name = 'keymode';");
    auto keymodeColumnExists =
      checkKeymodeColumn.executeAndGet<int64_t>().value_or(0) > 0;
    if (!keymodeColumnExists) {
        db.execute(
          "ALTER TABLE score ADD COLUMN keymode INTEGER NOT NULL DEFAULT 0;");
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
Profile::fetchTachiData(int tachiId)
{
    const auto request = QNetworkRequest("https://boku.tachi.ac/api/v1/users/" +
                                         QString::number(tachiId));
    auto* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tachiId]() {
        if (reply->error() != QNetworkReply::NoError) {
            spdlog::error("Error fetching tachi data for user {}: {}",
                          vars.getGeneralVars()->getName().toStdString(),
                          reply->errorString().toStdString());
        }
        auto data = reply->readAll();
        auto json = QJsonDocument::fromJson(data).object();
        if (json["success"].toBool() != true) {
            spdlog::error("Tachi data response unsuccessful for user {}: {}",
                          vars.getGeneralVars()->getName().toStdString(),
                          json["description"].toString().toStdString());
        } else {
            auto body = json["body"].toObject();
            auto tachiData = TachiData{};
            tachiData.username = body["username"].toString();
            tachiData.image = "https://boku.tachi.ac/api/v1/users/" +
                              QString::number(tachiId) + "/pfp";
            tachiData.userId = tachiId;
            setTachiLoginState(LoginState::LoggedIn);
            setTachiData(std::move(tachiData));
        }
    });
}
void
Profile::login(const QString& email, const QString& password)
{
    const auto request =
      networkRequestFactory.createRequest("auth/sign-in/email");

    QJsonObject json;
    json["email"] = email;
    json["password"] = password;

    QNetworkReply* reply =
      networkManager->post(request, QJsonDocument(json).toJson());

    setLoginState(LoginState::LoggingIn);

    connect(reply, &QNetworkReply::finished, [reply, this]() mutable {
        if (reply->error() == QNetworkReply::NoError) {
            auto data = reply->readAll();
            auto doc = QJsonDocument::fromJson(data);
            auto obj = doc.object();
            auto token = obj["token"].toString();
            if (!token.isEmpty()) {
                auto* job =
                  new QKeychain::WritePasswordJob(keychainService, this);
                job->setKey(
                  QStringLiteral("RhythmGame/profiles/%1/token").arg(guid));
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
                auto onlineData = OnlineUserData{};
                auto user = obj["user"].toObject();
                onlineData.username = user["name"].toString();
                onlineData.userId = user["id"].toString().toInt();
                onlineData.image = user["image"].toString();
                setOnlineUserData(onlineData);
                fetchOnlineData();
            } else {
                spdlog::error("Login response did not contain a token");
                setLoginState(LoginState::LoginFailed);
            }
            reply->deleteLater();
        } else {
            spdlog::error("Login request failed: {} - {}",
                          magic_enum::enum_name(reply->error()),
                          reply->errorString().toStdString());
            setLoginState(LoginState::LoginFailed);
            reply->deleteLater();
        }
    });
}
void
Profile::logout()
{
    auto* job = new QKeychain::DeletePasswordJob(keychainService, this);
    auto request = networkRequestFactory.createRequest("auth/sign-out");
    auto* reply = networkManager->post(request, "{}");
    connect(
      reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
    job->setKey(QStringLiteral("RhythmGame/profiles/%1/token").arg(guid));
    connect(job, &QKeychain::Job::finished, [job] {
        if (job->error()) {
            spdlog::error("Failed to delete token from keychain: {} - {}",
                          magic_enum::enum_name(job->error()),
                          job->errorString().toStdString());
        }
        job->deleteLater();
    });
    job->start();
    networkRequestFactory.clearBearerToken();
    setLoginState(LoginState::NotLoggedIn);
    setTachiLoginState(LoginState::NotLoggedIn);
    setOnlineUserData({});
    setTachiData({});
}
auto
Profile::getLoginState() const -> LoginState
{
    return loginState;
}
auto
Profile::getTachiLoginState() const -> LoginState
{
    return tachiLoginState;
}
auto
Profile::getOnlineUserData() const -> QVariant
{
    return userData ? QVariant::fromValue(*userData)
                    : QVariant::fromValue(nullptr);
}
auto
Profile::getTachiData() const -> QVariant
{
    return tachiData ? QVariant::fromValue(*tachiData)
                     : QVariant::fromValue(nullptr);
}
auto
Profile::submitScore(const gameplay_logic::BmsScore& score,
                     const gameplay_logic::ChartData& chartData) const
  -> QNetworkReply*
{
    if (score.getResult()->getGuid().isEmpty()) {
        return nullptr;
    }
    QJsonObject json;
    auto scoreData = score.getResult()->toJson();
    scoreData["replayData"] = score.getReplayData()->toJsonArray();
    scoreData["gaugeHistory"] = score.getGaugeHistory()->toJsonArray();
    json["scoreData"] = scoreData;
    json["chartData"] = chartData.toJson();
    auto request = networkRequestFactory.createRequest("scores");
    return networkManager->post(request, QJsonDocument(json).toJson());
}
auto
Profile::uploadScores() -> qml_components::ScoreSyncOperation*
{
    auto* op = new qml_components::ScoreSyncOperation(this);
    if (!userData) {
        op->setFinished(true);
        return op;
    }
    const auto userId = userData->userId;

    fetchLocalGuids(
      db,
      threadPool,
      this,
      [this, op, userId](QSet<QString> localGuids) {
          fetchServerGuids(
            networkManager,
            networkRequestFactory,
            userId,
            this,
            [this, op, localGuids](QSet<QString> serverGuids) {
                const auto toUpload = (localGuids - serverGuids).values();
                if (toUpload.isEmpty()) {
                    op->setFinished(true);
                    return;
                }

                threadPool.start([this, op, toUpload]() mutable {
                    try {
                        auto payloads = buildUploadPayloads(toUpload);
                        QMetaObject::invokeMethod(
                          this,
                          [this, op, payloads]() mutable {
                              dispatchUploads(op, std::move(payloads));
                          },
                          Qt::QueuedConnection);
                    } catch (const std::exception& e) {
                        QMetaObject::invokeMethod(
                          this,
                          [op, msg = std::string(e.what())]() mutable {
                              op->reportError(QString::fromStdString(msg));
                              op->setFinished(true);
                          },
                          Qt::QueuedConnection);
                    }
                });
            },
            [op](const QString& err) {
                spdlog::error("scores guid fetch failed: {}",
                              err.toStdString());
                op->reportError(err);
                op->setFinished(true);
            });
      },
      [op](const QString& err) {
          spdlog::error("Error fetching local guids for upload: {}",
                        err.toStdString());
          op->reportError(err);
          op->setFinished(true);
      });

    return op;
}

auto
Profile::downloadScores() -> qml_components::ScoreSyncOperation*
{
    auto* op = new qml_components::ScoreSyncOperation(this);
    if (!userData) {
        op->setFinished(true);
        return op;
    }
    const auto userId = userData->userId;

    fetchLocalGuids(
      db,
      threadPool,
      this,
      [this, op, userId](QSet<QString> localGuids) {
          fetchServerGuids(
            networkManager,
            networkRequestFactory,
            userId,
            this,
            [this, op, localGuids](QSet<QString> serverGuids) {
                const auto toDownload = (serverGuids - localGuids).values();
                if (toDownload.isEmpty()) {
                    op->setFinished(true);
                    return;
                }
                op->setTotal(toDownload.size());
                for (const auto& guid : toDownload)
                    dispatchDownload(op, guid);
            },
            [op](const QString& err) {
                spdlog::error("scores guid fetch failed: {}",
                              err.toStdString());
                op->reportError(err);
                op->setFinished(true);
            });
      },
      [op](const QString& err) {
          spdlog::error("Error fetching local guids for download: {}",
                        err.toStdString());
          op->reportError(err);
          op->setFinished(true);
      });

    return op;
}

// Called on a background thread. Builds JSON payloads for a list of GUIDs,
// chunking the SQL query to stay under the SQLite variable limit.
auto
Profile::buildUploadPayloads(const QList<QString>& guids) -> QList<Payload>
{
    constexpr auto columns =
      "score.max_points, score.max_hits, score.normal_note_count, "
      "score.scratch_count, score.ln_count, score.bss_count, score.mine_count, "
      "score.clear_type, score.points, score.max_combo, score.poor, "
      "score.empty_poor, score.bad, score.good, score.great, score.perfect, "
      "score.mine_hits, score.guid, score.sha256, score.md5, "
      "score.unix_timestamp, score.length, score.random_sequence, "
      "score.random_seed, score.note_order_algorithm, "
      "score.note_order_algorithm_p2, score.dp_options, score.keymode, "
      "score.game_version, score.owner, replay_data.*, gauge_history.* ";

    using Row = std::tuple<gameplay_logic::BmsResult::DTO,
                           gameplay_logic::BmsReplayData::DTO,
                           gameplay_logic::BmsGaugeHistory::DTO>;

    QList<Row> rows;
    for (int i = 0; i < guids.size(); i += maxSqlVariables) {
        const auto chunk = guids.mid(i, maxSqlVariables);
        auto stmtStr =
          std::string("SELECT ") + columns +
          "FROM score "
          "JOIN replay_data ON score.guid = replay_data.score_guid "
          "JOIN gauge_history ON score.guid = gauge_history.score_guid "
          "WHERE score.guid IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() +
          ") ORDER BY score.unix_timestamp DESC";

        auto statement = db.createStatement(stmtStr);
        for (int j = 0; j < chunk.size(); ++j)
            statement.bind(j + 1, chunk[j].toStdString());

        auto chunkRows = statement.executeAndGetAll<Row>();
        rows.append(QList<Row>(chunkRows.begin(), chunkRows.end()));
    }

    QList<Payload> payloads;
    payloads.reserve(rows.size());

    for (auto& row : rows) {
        auto& resultDto = std::get<0>(row);

        auto chartStmt = db.createStatement(
          "SELECT charts.id, charts.title, charts.artist, charts.subtitle, "
          "charts.subartist, charts.genre, charts.stage_file, charts.banner, "
          "charts.back_bmp, charts.rank, charts.total, charts.play_level, "
          "charts.difficulty, charts.is_random, charts.random_sequence, "
          "charts.normal_note_count, charts.scratch_count, charts.ln_count, "
          "charts.bss_count, charts.mine_count, charts.length, "
          "charts.initial_bpm, charts.max_bpm, charts.min_bpm, "
          "charts.main_bpm, "
          "charts.avg_bpm, charts.peak_density, charts.avg_density, "
          "charts.end_density, charts.path, charts.directory, charts.sha256, "
          "charts.md5, charts.keymode, charts.game_version, "
          "h.bpms, h.histogram_data "
          "FROM song_db.charts LEFT JOIN song_db.histogram_data h "
          "ON h.chart_id = charts.id "
          "WHERE charts.md5 = ? LIMIT 1");
        chartStmt.bind(1, resultDto.md5);
        const auto chartResults =
          chartStmt.executeAndGetAll<gameplay_logic::ChartData::DTO>();
        if (chartResults.empty())
            continue;

        auto chart = gameplay_logic::ChartData::load(chartResults.front());

        if (resultDto.keymode == 0) {
            const auto chartKeymode = chart->getKeymode();
            resultDto.keymode =
              (resultDto.dpOptions == static_cast<int>(DpOptions::Battle))
                ? static_cast<int>(chartKeymode) * 2
                : static_cast<int>(chartKeymode);
        }

        auto result = gameplay_logic::BmsResult::load(std::get<0>(row));
        auto replay = gameplay_logic::BmsReplayData::load(std::get<1>(row));
        auto gauge = gameplay_logic::BmsGaugeHistory::load(std::get<2>(row));

        auto scoreData = result->toJson();
        scoreData["replayData"] = replay->toJsonArray();
        scoreData["gaugeHistory"] = gauge->toJsonArray();

        QJsonObject obj;
        obj["scoreData"] = std::move(scoreData);
        obj["chartData"] = chart->toJson();
        payloads.append({ result->getGuid(), std::move(obj) });
    }

    return payloads;
}

// Called on the main thread. Fires one POST per payload.
void
Profile::dispatchUploads(qml_components::ScoreSyncOperation* op,
                         QList<Payload> payloads)
{
    op->setTotal(payloads.size());
    if (payloads.isEmpty()) {
        op->setFinished(true);
        return;
    }
    for (auto& p : payloads) {
        auto request = networkRequestFactory.createRequest("scores");
        auto* reply =
          networkManager->post(request, QJsonDocument(p.json).toJson());
        const auto guid = p.guid;
        connect(reply, &QNetworkReply::finished, this, [reply, op, guid]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                spdlog::error("Score upload failed for {}: {}",
                              guid.toStdString(),
                              reply->errorString().toStdString());
                op->reportError(QStringLiteral("Upload failed for %1: %2")
                                  .arg(guid, reply->errorString()));
            }
            op->increment();
        });
    }
}

// Called on the main thread. Fires one GET and saves the result.
void
Profile::dispatchDownload(qml_components::ScoreSyncOperation* op,
                          const QString& guid)
{
    auto request = networkRequestFactory.createRequest(
      QStringLiteral("scores/%1").arg(guid));
    auto* reply = networkManager->get(request);

    connect(
      reply, &QNetworkReply::finished, this, [this, reply, op, guid]() mutable {
          reply->deleteLater();
          if (reply->error() != QNetworkReply::NoError) {
              spdlog::error("Score download failed for {}: {}",
                            guid.toStdString(),
                            reply->errorString().toStdString());
              op->reportError(QStringLiteral("Download failed for %1: %2")
                                .arg(guid, reply->errorString()));
              op->increment();
              return;
          }
          auto data = reply->readAll();
          threadPool.start([this, op, guid, data]() mutable {
              try {
                  auto doc = QJsonDocument::fromJson(data);
                  auto scoreObj = doc.object();
                  auto result = gameplay_logic::BmsResult::fromJson(scoreObj);
                  auto replayData =
                    std::make_unique<gameplay_logic::BmsReplayData>(
                      gameplay_logic::BmsReplayData::fromJsonArray(
                        scoreObj["replayData"].toArray()),
                      result->getGuid());
                  auto gaugeHistory =
                    std::make_unique<gameplay_logic::BmsGaugeHistory>(
                      gameplay_logic::BmsGaugeHistory::fromJsonArray(
                        scoreObj["gaugeHistory"].toArray()),
                      result->getGuid());
                  gameplay_logic::BmsScore(std::move(result),
                                           std::move(replayData),
                                           std::move(gaugeHistory))
                    .save(db);

                  QMetaObject::invokeMethod(
                    this, [op]() { op->increment(); }, Qt::QueuedConnection);
              } catch (const std::exception& e) {
                  spdlog::error("Error saving downloaded score {}: {}",
                                guid.toStdString(),
                                e.what());
                  QMetaObject::invokeMethod(
                    this,
                    [op, msg = std::string(e.what())]() mutable {
                        op->reportError(QString::fromStdString(msg));
                        op->increment();
                    },
                    Qt::QueuedConnection);
              }
          });
      });
}

} // namespace resource_managers
