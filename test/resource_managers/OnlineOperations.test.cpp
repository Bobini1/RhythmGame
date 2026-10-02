#include "resource_managers/Profile.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SerializeConfig.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/ChartDataFactory.h"
#include "qml_components/OnlineRankingModel.h"
#include "qml_components/OnlineScores.h"
#include "qml_components/ScoreSyncOperation.h"
#include "gameplay_logic/BmsScore.h"
#include "gameplay_logic/BmsScoreCourse.h"
#include "Rg.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QAbstractItemModelTester>
#include <QElapsedTimer>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QThread>
#include <QUuid>
#include <cstring>

namespace resource_managers {
struct ProfileTestAccess
{
    static void signIn(Profile& profile)
    {
        profile.invalidateSession();
        profile.setOnlineUserData(OnlineUserData{ 42, "Test", {} });
        profile.setLoginState(Profile::LoginState::LoggedIn);
    }
    static void fetchUser(Profile& profile) { profile.fetchOnlineData(); }
    static void fetchTachi(Profile& profile) { profile.fetchTachiData(1); }
};
}

namespace {
void
ensureApplication()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "OnlineOperations.test";
        static char* argv[] = { name, nullptr };
        static QCoreApplication application(argc, argv);
    }
}

bool
waitUntil(const std::function<bool()>& ready)
{
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 5000) {
        QCoreApplication::processEvents();
        QThread::msleep(1);
    }
    return ready();
}

class Reply final : public QNetworkReply
{
    QByteArray bytes;
    qint64 position{};

  public:
    bool aborted{};
    explicit Reply(const QNetworkRequest& request, QObject* parent)
    {
        setParent(parent);
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }
    // Keep a completion pending to exercise a reply arriving after
    // cancellation.
    void abort() override { aborted = true; }
    void complete(QByteArray body, NetworkError error = NoError)
    {
        bytes = std::move(body);
        if (error != NoError) {
            setError(error, "Injected request failure");
        }
        setFinished(true);
        emit readyRead();
        emit finished();
    }
    qint64 bytesAvailable() const override
    {
        return bytes.size() - position + QNetworkReply::bytesAvailable();
    }
    qint64 readData(char* buffer, qint64 size) override
    {
        const auto count = std::min(size, bytes.size() - position);
        if (count <= 0)
            return -1;
        std::memcpy(
          buffer, bytes.constData() + position, static_cast<size_t>(count));
        position += count;
        return count;
    }
};

class Network final : public QNetworkAccessManager
{
  public:
    QList<QPointer<Reply>> requests;
    QList<QByteArray> bodies;
    QNetworkReply* createRequest(Operation,
                                 const QNetworkRequest& request,
                                 QIODevice* body) override
    {
        auto* reply = new Reply(request, this);
        requests.append(reply);
        bodies.append(body ? body->readAll() : QByteArray{});
        return reply;
    }
};

struct ProfileFixture
{
    QTemporaryDir directory;
    db::SqliteCppDb songs{ support::qStringToPath(
      directory.filePath("songs.sqlite")) };
    Network network;
    resource_managers::SongAssetStore assets;
    std::unique_ptr<resource_managers::Profile> profile;
    ProfileFixture()
    {
        ensureApplication();
        resource_managers::defineDb(songs);
        const auto folder = directory.filePath(
          QUuid::createUuid().toString(QUuid::WithoutBraces));
        REQUIRE(QDir().mkpath(folder));
        profile = std::make_unique<resource_managers::Profile>(
          support::qStringToPath(directory.filePath("songs.sqlite")),
          support::qStringToPath(folder + "/profile.sqlite"),
          QMap<QString, qml_components::ThemeFamily>{},
          QList<QString>{},
          &network,
          &assets);
    }
};

auto
scoreJson(const QString& guid) -> QJsonObject
{
    return { { "guid", guid },
             { "md5", "MD5" },
             { "sha256", "SHA256" },
             { "clearType", "NORMAL" },
             { "points", 2 },
             { "maxPoints", 2 },
             { "maxHits", 1 },
             { "normalNoteCount", 1 },
             { "keymode", 7 },
             { "judgementCounts", QJsonArray{ 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 } },
             { "replayData", QJsonArray{} },
             { "gaugeHistory", QJsonArray{} } };
}
}

namespace {
struct RankingHarness
{
    Network network;
    qml_components::OnlineScores scores{ &network };
    Rg rg{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
           nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
           nullptr, nullptr, nullptr, &scores, nullptr };
    Rg* oldRg = Rg::instance;
    QNetworkAccessManager* oldNetwork =
      qml_components::OnlineRankingModel::networkManager;
    qml_components::OnlineScores* oldScores =
      qml_components::OnlineRankingModel::onlineScores;
    QScopeGuard<std::function<void()>> restore{ [this] {
        Rg::instance = oldRg;
        qml_components::OnlineRankingModel::networkManager = oldNetwork;
        qml_components::OnlineRankingModel::onlineScores = oldScores;
    } };
    std::unique_ptr<QQmlEngine> engine;
    std::unique_ptr<QObject> object;
    QStringList warnings;

    RankingHarness()
    {
        ensureApplication();
        Rg::instance = &rg;
        qml_components::OnlineRankingModel::networkManager = &network;
        qml_components::OnlineRankingModel::onlineScores = &scores;
        engine = std::make_unique<QQmlEngine>();
        QObject::connect(engine.get(),
                         &QQmlEngine::warnings,
                         engine.get(),
                         [this](const QList<QQmlError>& errors) {
                             for (const auto& error : errors)
                                 warnings.append(error.toString());
                         });
        QQmlComponent component(
          engine.get(),
          QUrl::fromLocalFile(QString::fromUtf8(RHYTHMGAME_SOURCE_DIR) +
                              "/test/qml/RankingHarness.qml"));
        INFO(component.errorString().toStdString());
        REQUIRE(component.isReady());
        object.reset(component.create());
        INFO(component.errorString().toStdString());
        REQUIRE(object);
        engine->globalObject().setProperty("h",
                                           engine->newQObject(object.get()));
        drain();
    }

    ~RankingHarness()
    {
        object.reset();
        engine.reset();
    }

    static void drain()
    {
        for (int i = 0; i < 4; ++i)
            QCoreApplication::processEvents();
    }

    void run(const QString& script, bool processEvents = true)
    {
        const auto result = engine->evaluate(script);
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        if (processEvents)
            drain();
    }

    void check(const QString& expression)
    {
        const auto result = engine->evaluate(expression);
        INFO(expression.toStdString());
        INFO(result.toString().toStdString());
        REQUIRE_FALSE(result.isError());
        CHECK(result.toBool());
        INFO(warnings.join('\n').toStdString());
        CHECK(warnings.isEmpty());
    }
};
}

TEST_CASE("Standard ranking opens profile links without loading a leaderboard",
          "[StandardRanking][qml][online]")
{
    ensureApplication();
    RankingHarness h;
    h.check("h.ranking.available && h.ranking.open()");
    h.check("h.openedUrls[0] === 'https://ranking.example/charts/chart-A'");
    h.run("h.provider = 2;");
    h.check("h.ranking.open() && h.openedUrls[1].endsWith('bmsmd5=chart-A')");
    CHECK(h.network.requests.isEmpty());
    h.run("h.ranking.url = 'https://custom.example/ranking';");
    h.check("h.ranking.open() && h.openedUrls[2] === "
            "'https://custom.example/ranking'");
    h.run("h.chart = { identifier: 'course', charts: [] };");
    h.check("!h.ranking.available && !h.ranking.open() && "
            "h.ranking.playerCount === 0");
    h.run("h.chart = {md5: 'chart-B'}; h.active = false;");
    h.check("!h.ranking.open() && h.openedUrls.length === 3");
}

TEST_CASE("Standard ranking merges local LR2IR scores on select and result",
          "[StandardRanking][qml][online]")
{
    ensureApplication();
    RankingHarness h;
    h.run(
      "h.provider = 2; h.savedResults = {'CHART-A': [{ guid: 'old', "
      "points: 80, maxPoints: 200, clearType: 'HARD', unixTimestamp: 100 }]}; "
      "h.showResult = true; h.loadEntries = true;");
    REQUIRE_FALSE(h.network.requests.isEmpty());
    h.network.requests.back()->complete(
      R"({"chart":{},"leaderboard":[{"player_id":1,"score":200,"score_max":200,"clear_type":"HARD"},{"player_id":2,"score":100,"score_max":200,"clear":"NORMAL"}],"total_pages":1})");
    h.drain();
    h.check("h.ranking.playerCount === 3 && h.ranking.position === 2 && "
            "h.ranking.previousPosition === 3");
    h.check("h.ranking.entries[1].bestPoints === 180 && "
            "h.ranking.entries[1].bestClearType === 'HARD'");
    h.run("h.showResult = false; h.savedResults = {};");
    h.check("h.ranking.playerCount === 2 && h.ranking.position === 0 && "
            "h.ranking.previousPosition === 0");
    h.run("h.savedResults = {'CHART-A': [{guid:'saved', points:175, "
          "maxPoints:200, clearType:'HARD'}]};");
    h.check("h.ranking.position === 2 && !h.ranking.loading");
    h.run("h.savedResults = {'CHART-A': [{guid:'saved', points:50, "
          "maxPoints:200, clearType:'HARD'}]};");
    h.check("h.ranking.position === 3 && !h.ranking.loading");
    h.run("h.provider = 0;");
    h.check("h.ranking.bestPointsResult.guid === 'saved' && "
            "h.ranking.bestClearTypeResult.guid === 'saved'");
}

TEST_CASE(
  "Standard result ranking tracks the earlier score and submission refresh",
  "[StandardRanking][qml][online]")
{
    ensureApplication();
    RankingHarness h;
    h.run("h.loadEntries = true; h.showResult = true;");
    REQUIRE(h.network.requests.size() == 2);
    auto history = h.network.requests.back();
    CHECK(history->url().query().contains("date_lte=199"));
    history->complete(R"([{"userId":42,"bestPoints":80,"maxPoints":200}])");
    h.network.requests.front()->complete(
      R"([{"userId":1,"bestPoints":200},{"userId":42,"bestPoints":180},{"userId":2,"bestPoints":100}])");
    h.drain();
    h.check("h.ranking.position === 2 && h.ranking.previousPosition === 3 && "
            "!h.ranking.loading");
    CHECK(h.network.requests.size() == 2);
    h.run("h.resultScore.submissionState = 1;");
    CHECK(h.network.requests.size() == 4);
    h.run("h.active = false;");
    h.check("h.ranking.playerCount === 0 && !h.ranking.loading");
    CHECK(h.network.requests[2]->aborted);
    CHECK(h.network.requests[3]->aborted);
}

TEST_CASE("Standard ranking resolves a Tachi link without fetching scores",
          "[StandardRanking][qml][online]")
{
    ensureApplication();
    RankingHarness h;
    h.run("h.provider = 1; h.ranking.open(); h.ranking.open();");
    REQUIRE(h.network.requests.size() == 1);
    h.check("h.ranking.opening");
    h.network.requests.back()->complete("{}",
                                        QNetworkReply::ContentNotFoundError);
    REQUIRE(h.network.requests.size() == 2);
    CHECK(h.network.requests.back()->url().path().contains("bms-14k"));
    h.network.requests.back()->complete(
      R"({"body":{"chart":{"chartID":"42","data":{"notecount":100}}}})");
    h.drain();
    h.check(
      "!h.ranking.opening && h.openedUrls.length === 1 && h.openedUrls[0] === "
      "'https://boku.tachi.ac/games/bms-14k/charts/42'");
    CHECK(h.network.requests.size() == 2);
}

TEST_CASE("Standard ranking cancels a pending open when its selection changes",
          "[StandardRanking][qml][online]")
{
    ensureApplication();
    RankingHarness h;
    h.run("h.provider = 1; h.ranking.open();");
    REQUIRE_FALSE(h.network.requests.isEmpty());
    auto old = h.network.requests.back();
    h.run("h.chart = {md5: 'chart-B'};", false);
    REQUIRE(old);
    CHECK(old->aborted);
    old->complete(R"({"body":{"chart":{"chartID":"old","data":{}}}})");
    h.drain();
    h.check(
      "h.openedUrls.length === 0 && h.failures === 0 && !h.ranking.opening");
    h.run("h.ranking.open(); h.active = false;", false);
    CHECK(h.network.requests.back()->aborted);
    h.drain();
    h.check("h.openedUrls.length === 0 && h.failures === 0");
}

TEST_CASE("Account replies cannot revive a logged out or replaced session",
          "[Profile][online]")
{
    ensureApplication();
    ProfileFixture fixture;
    auto& profile = *fixture.profile;
    profile.login("test@example.invalid", "synthetic");
    auto old = fixture.network.requests.back();
    SECTION("logout")
    {
        profile.logout();
        REQUIRE(old->aborted);
        old->complete(R"({"token":"ignored"})");
        CHECK(profile.getLoginState() ==
              resource_managers::Profile::LoginState::NotLoggedIn);
    }
    SECTION("replacement login")
    {
        profile.login("second@example.invalid", "synthetic");
        REQUIRE(old->aborted);
        old->complete("{}", QNetworkReply::AuthenticationRequiredError);
        CHECK(profile.getLoginState() ==
              resource_managers::Profile::LoginState::LoggingIn);
    }
    SECTION("endpoint change")
    {
        profile.getVars()->getGeneralVars()->setWebsiteBaseUrl(
          "https://other.example.invalid/");
        REQUIRE(old->aborted);
        old->complete(R"({"token":"ignored"})");
        CHECK(profile.getLoginState() ==
              resource_managers::Profile::LoginState::NotLoggedIn);
        CHECK(fixture.network.requests.size() ==
              2); // Login and sign-out on the original server.
    }
    SECTION("profile deletion")
    {
        fixture.profile.reset();
        CHECK(old.isNull());
    }
}

TEST_CASE("Late account metadata cannot restore the session",
          "[Profile][online]")
{
    ensureApplication();
    ProfileFixture fixture;
    resource_managers::ProfileTestAccess::signIn(*fixture.profile);
    resource_managers::ProfileTestAccess::fetchUser(*fixture.profile);
    auto old = fixture.network.requests.back();
    fixture.profile->logout();
    old->complete(R"({"id":42,"name":"Old session","tachiId":1})");
    CHECK_FALSE(fixture.profile->getOnlineUserDataValue());
    CHECK(fixture.profile->getTachiLoginState() ==
          resource_managers::Profile::LoginState::NotLoggedIn);
}

TEST_CASE("Tachi completion respects logout during data notification",
          "[Profile][online]")
{
    ensureApplication();
    ProfileFixture fixture;
    auto& profile = *fixture.profile;
    resource_managers::ProfileTestAccess::signIn(profile);
    resource_managers::ProfileTestAccess::fetchTachi(profile);
    auto reply = fixture.network.requests.back();
    SECTION("logout while the request is pending")
    {
        profile.logout();
    }
    SECTION("logout from the data notification")
    {
        QObject::connect(&profile,
                         &resource_managers::Profile::tachiDataChanged,
                         &profile,
                         [&] {
                             if (!profile.getTachiData().isNull())
                                 profile.logout();
                         });
    }
    reply->complete(R"({"success":true,"body":{"username":"Test"}})");
    CHECK(profile.getTachiLoginState() ==
          resource_managers::Profile::LoginState::NotLoggedIn);
    CHECK(profile.getTachiData().isNull());
}

TEST_CASE("Score sync limits work and finishes once after partial failure",
          "[ScoreSync]")
{
    qml_components::ScoreSyncOperation operation;
    QStringList guids;
    for (int i = 0; i < 20; ++i)
        guids.append(QString::number(i));
    int dispatched = 0;
    int completions = 0;
    QObject::connect(&operation,
                     &qml_components::ScoreSyncOperation::finishedChanged,
                     [&] { ++completions; });
    operation.start(guids, [&](const QString&) { ++dispatched; });
    CHECK(dispatched == operation.concurrency);
    operation.reportError("one score failed");
    for (int i = 0; i < guids.size(); ++i) {
        CHECK(dispatched - operation.getDone() <= operation.concurrency);
        operation.increment();
    }
    CHECK(operation.isFinished());
    CHECK(operation.getDone() == 20);
    CHECK(operation.getErrorCount() == 1);
    CHECK(operation.getLastError() == "one score failed");
    operation.increment();
    operation.setFinished(false);
    operation.cancel();
    CHECK(operation.getDone() == 20);
    CHECK(completions == 1);
}

TEST_CASE("Score sync cancellation drops queued work and aborts replies",
          "[ScoreSync]")
{
    ensureApplication();
    qml_components::ScoreSyncOperation operation;
    int dispatched = 0;
    operation.start({ "1", "2", "3", "4", "5" },
                    [&](const QString&) { ++dispatched; });
    auto reply = QPointer(
      new Reply(QNetworkRequest(QUrl("https://example.invalid")), nullptr));
    operation.ownReply(reply);
    operation.cancel();
    CHECK(reply->aborted);
    CHECK(operation.isCancelled());
    operation.increment();
    CHECK(dispatched == operation.concurrency);
    CHECK(operation.getDone() == 0);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(reply.isNull());
}

TEST_CASE("Downloads stay bounded and malformed scores are counted as failures",
          "[Profile][ScoreSync]")
{
    ensureApplication();
    ProfileFixture fixture;
    resource_managers::ProfileTestAccess::signIn(*fixture.profile);
    auto* operation = fixture.profile->downloadScores();
    REQUIRE(waitUntil([&] { return fixture.network.requests.size() == 1; }));
    QJsonArray guids;
    for (int i = 0; i < 12; ++i)
        guids.append(QJsonObject{ { "guid", QString::number(i) } });
    fixture.network.requests[0]->complete(QJsonDocument(guids).toJson());
    REQUIRE(fixture.network.requests.size() == 1 + operation->concurrency);
    bool malformedSent = false;
    REQUIRE(waitUntil([&] {
        int inFlight = 0;
        const auto requests = fixture.network.requests;
        for (qsizetype i = 1; i < requests.size(); ++i) {
            auto reply = requests[i];
            if (reply && !reply->isFinished()) {
                ++inFlight;
                const auto guid = reply->url().path().section('/', -1);
                reply->complete(malformedSent
                                  ? QJsonDocument(scoreJson(guid)).toJson()
                                  : QByteArray("{}"));
                malformedSent = true;
            }
        }
        CHECK(inFlight <= operation->concurrency);
        return operation->isFinished();
    }));
    CHECK(operation->getDone() == 12);
    CHECK(operation->getErrorCount() == 1);
    CHECK(fixture.profile->getDb()
            .createStatement("SELECT count(*) FROM score")
            .executeAndGet<int>() == 11);
}

TEST_CASE("Logout cancels score discovery before it can dispatch requests",
          "[Profile][ScoreSync]")
{
    ensureApplication();
    ProfileFixture fixture;
    resource_managers::ProfileTestAccess::signIn(*fixture.profile);
    auto* operation = fixture.profile->downloadScores();
    REQUIRE(waitUntil([&] { return fixture.network.requests.size() == 1; }));
    auto reply = fixture.network.requests[0];
    fixture.profile->logout();
    REQUIRE(operation->isCancelled());
    REQUIRE(reply->aborted);
    reply->complete(R"([{"guid":"late-score"}])");
    CHECK(fixture.network.requests.size() == 2);
}

TEST_CASE("Uploads prepare only a bounded batch of scores",
          "[Profile][ScoreSync]")
{
    ensureApplication();
    ProfileFixture fixture;
    resource_managers::ProfileTestAccess::signIn(*fixture.profile);
    const resource_managers::ChartDataFactory factory;
    auto chart = factory.loadChartData(
      "#TITLE Upload\n#BPM 120\n#00111:01\n",
      "upload.bms",
      [](auto) { return 1; },
      -1);
    chart.chartData->save(fixture.songs);
    for (int i = 0; i < 12; ++i) {
        const auto guid = QString::number(i);
        auto object = scoreJson(guid);
        object["md5"] = chart.chartData->getMd5();
        object["sha256"] = chart.chartData->getSha256();
        auto score = gameplay_logic::BmsScore::fromRemoteJson(
          QJsonDocument(object).toJson(), guid);
        score->save(fixture.profile->getDb());
    }
    auto* operation = fixture.profile->uploadScores();
    REQUIRE(waitUntil([&] { return fixture.network.requests.size() == 1; }));
    fixture.network.requests[0]->complete("[]");
    REQUIRE(waitUntil([&] {
        return fixture.network.requests.size() == 1 + operation->concurrency;
    }));
    REQUIRE(waitUntil([&] {
        const auto requests = fixture.network.requests;
        int inFlight = 0;
        for (qsizetype i = 1; i < requests.size(); ++i) {
            auto reply = requests[i];
            if (reply && !reply->isFinished()) {
                ++inFlight;
                CHECK(QJsonDocument::fromJson(fixture.network.bodies[i])
                        .object()
                        .contains("scoreData"));
                reply->complete("{}",
                                i == 1 ? QNetworkReply::InternalServerError
                                       : QNetworkReply::NoError);
            }
        }
        CHECK(inFlight <= operation->concurrency);
        return operation->isFinished();
    }));
    CHECK(operation->getDone() == 12);
    CHECK(operation->getErrorCount() == 1);
}

TEST_CASE(
  "Tachi query cancellation and model destruction release their children",
  "[OnlineRanking]")
{
    ensureApplication();
    Network network;
    qml_components::OnlineScores scores(&network);
    auto* previousNetwork = qml_components::OnlineRankingModel::networkManager;
    auto* previousScores = qml_components::OnlineRankingModel::onlineScores;
    qml_components::OnlineRankingModel::networkManager = &network;
    qml_components::OnlineRankingModel::onlineScores = &scores;
    const auto restore = qScopeGuard([&] {
        qml_components::OnlineRankingModel::networkManager = previousNetwork;
        qml_components::OnlineRankingModel::onlineScores = previousScores;
    });
    auto model = std::make_unique<qml_components::OnlineRankingModel>();
    model->setProvider(qml_components::OnlineRankingModel::Provider::Tachi);
    model->setMd5("old");
    model->setMd5("current");
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CHECK(model->findChildren<qml_components::TachiResolveHandle*>().size() ==
          1);
    network.requests.back()->complete(
      R"({"body":{"chart":{"chartID":"42","data":{"notecount":1}}}})");
    CHECK(model->getChartId() == "42");
    CHECK(model->getGameId() == "bms-7k");
    auto firstPage = network.requests.back();
    REQUIRE(firstPage->parent() == model.get());
    QJsonArray page;
    for (int i = 0; i < 100; ++i)
        page.append(QJsonObject{});
    firstPage->complete(
      QJsonDocument(QJsonObject{ { "body",
                                   QJsonObject{ { "pbs", page },
                                                { "users", QJsonArray{} } } } })
        .toJson());
    auto nextPage = network.requests.back();
    REQUIRE(nextPage != firstPage);
    CHECK(nextPage->parent() == model.get());
    model->setMd5("next-chart");
    CHECK(model->getChartId().isEmpty());
    CHECK(model->getGameId().isEmpty());
    CHECK(nextPage->aborted);
    model.reset();
    CHECK(nextPage.isNull());
}

TEST_CASE("Leaderboard ignores stale completions and owns paginated replies",
          "[OnlineRanking]")
{
    ensureApplication();
    Network network;
    auto* previous = qml_components::OnlineRankingModel::networkManager;
    qml_components::OnlineRankingModel::networkManager = &network;
    const auto restore = qScopeGuard(
      [&] { qml_components::OnlineRankingModel::networkManager = previous; });
    auto model = std::make_unique<qml_components::OnlineRankingModel>();
    model->setProvider(qml_components::OnlineRankingModel::Provider::LR2IR);
    model->setMd5("first-chart");
    REQUIRE(network.requests.size() == 1);
    auto old = network.requests[0];
    model->setMd5("second-chart");
    REQUIRE(old->aborted);
    old->complete(R"({"chart":{},"leaderboard":[{}],"total_pages":1})");
    CHECK(model->rowCount() == 0);
    REQUIRE(network.requests.size() == 2);
    network.requests[1]->complete(
      R"({"chart":{},"leaderboard":[{}],"total_pages":2,"total_rows":2})");
    REQUIRE(network.requests.size() == 3);
    network.requests[2]->complete(R"({"chart":{},"leaderboard":[{}]})");
    CHECK(model->rowCount() == 2);
    CHECK(model->getScoreCount() == -1);
    CHECK_FALSE(model->isLoading());
    model->refresh();
    auto pending = network.requests.back();
    model.reset();
    CHECK(pending.isNull());
}

TEST_CASE(
  "Configuration writes preserve malformed input and drain the latest snapshot",
  "[config]")
{
    QTemporaryDir directory;
    const auto path = support::qStringToPath(directory.filePath("vars.json"));
    QFile malformed(path);
    REQUIRE(malformed.open(QIODevice::WriteOnly));
    malformed.write("{broken");
    malformed.close();
    CHECK_THROWS(resource_managers::readJsonConfig(path));
    resource_managers::writeJsonConfig(path, { { "value", 1 } });
    const auto backups =
      QDir(directory.path()).entryList({ "vars.json.invalid-*" }, QDir::Files);
    REQUIRE(backups.size() == 1);
    QFile backup(directory.filePath(backups[0]));
    REQUIRE(backup.open(QIODevice::ReadOnly));
    CHECK(backup.readAll() == "{broken");
    {
        resource_managers::ConfigWriter writer;
        for (int i = 0; i < 1000; ++i)
            writer.write(path, { { "value", i } });
    }
    CHECK(resource_managers::readJsonConfig(path)["value"].toInt() == 999);
    resource_managers::writeJsonConfig(path, { { "other", 2 } }, true);
    CHECK(resource_managers::readJsonConfig(path)["value"].toInt() == 999);
    CHECK(resource_managers::readJsonConfig(path)["other"].toInt() == 2);
}

TEST_CASE("Profile destruction flushes delayed settings changes",
          "[Profile][config]")
{
    ensureApplication();
    ProfileFixture fixture;
    const auto path =
      fixture.profile->getPath().parent_path() / "generalVars.json";
    for (int i = 0; i < 100; ++i)
        fixture.profile->getVars()->getGeneralVars()->setName(
          QString::number(i));
    fixture.profile.reset();
    CHECK(resource_managers::readJsonConfig(path)["name"].toString() == "99");
}

TEST_CASE("Leaderboard filters and paging expose a consistent model",
          "[OnlineRanking]")
{
    ensureApplication();
    using Model = qml_components::OnlineRankingModel;
    Network network;
    auto* previous = Model::networkManager;
    Model::networkManager = &network;
    const auto restore = qScopeGuard([&] { Model::networkManager = previous; });
    Model model;
    QAbstractItemModelTester modelTester(
      &model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.setProvider(Model::Provider::LR2IR);
    model.setLimit(1);
    model.setOffset(1);
    model.setMissCountLte(0);
    model.setSortBy(Model::SortableColumn::Combo);
    model.setSortDir(Model::SortDirection::Desc);
    int resets = 0;
    QObject::connect(&model, &QAbstractItemModel::modelReset, [&] {
        ++resets;
        CHECK((model.rowCount() <= model.getLimit() || model.getLimit() == 0));
        for (int row = 0; row < model.rowCount(); ++row)
            CHECK(model.data(model.index(row), Model::UserNameRole).isValid());
    });
    QObject::connect(&model, &QAbstractItemModel::rowsInserted, [&] {
        FAIL("Pagination must reset its filtered slice");
    });
    model.setMd5("chart");
    network.requests.back()->complete(
      R"({"chart":{"play_people":3,"play_count":12,"hard_count":3},"total_pages":2,"leaderboard":[{"player_id":1,"player_name":"A","combo":5,"bad_poor":0,"score":10,"score_max":20,"clear_type":"HARD"}]})");
    CHECK(model.rowCount() == 0);
    network.requests.back()->complete(
      R"({"chart":{},"leaderboard":[{"player_id":2,"player_name":"B","combo":10,"bad_poor":0,"score":20,"score_max":20,"clear_type":"HARD"},{"player_id":3,"player_name":"C","combo":15,"bad_poor":1,"score":30,"score_max":40,"clear_type":"HARD"}]})");
    REQUIRE_FALSE(model.isLoading());
    CHECK(model.rowCount() == 1);
    CHECK(model.data(model.index(0), Model::UserNameRole).toString() == "A");
    CHECK(model.getRankingEntries().size() == 2);
    CHECK(model.getPlayerCount() == 3);
    CHECK(model.getScoreCount() == 12);
    CHECK(model.getClearCounts().value("HARD").toInt() == 2);
    model.setOffset(20);
    CHECK(model.rowCount() == 0);
    model.setOffset(0);
    model.setLimit(0);
    CHECK(model.rowCount() == 2);
    CHECK(resets >= 4);
}

TEST_CASE(
  "Leaderboard totals remain unavailable when any player count is missing",
  "[OnlineRanking]")
{
    ensureApplication();
    using Model = qml_components::OnlineRankingModel;
    Network network;
    auto* previous = Model::networkManager;
    Model::networkManager = &network;
    const auto restore = qScopeGuard([&] { Model::networkManager = previous; });
    for (bool unavailable : { false, true }) {
        Model model;
        model.setWebApiUrl("https://example.test/api/");
        const auto previousRequests = network.requests.size();
        model.setMd5("chart");
        REQUIRE(network.requests.size() == previousRequests + 1);
        network.requests.back()->complete(
          unavailable
            ? R"([{"userId":1,"scoreCount":3},{"userId":2}])"
            : R"([{"userId":1,"scoreCount":3},{"userId":2,"scoreCount":0}])");
        CHECK_FALSE(model.isLoading());
        CHECK(model.getPlayerCount() == 2);
        CHECK(model.getScoreCount() == (unavailable ? -1 : 3));
    }
}

TEST_CASE("Tachi applies filters even with provider ordering and keeps "
          "unavailable counts distinct",
          "[OnlineRanking]")
{
    ensureApplication();
    using Model = qml_components::OnlineRankingModel;
    Network network;
    qml_components::OnlineScores scores(&network);
    auto* oldNetwork = Model::networkManager;
    auto* oldScores = Model::onlineScores;
    Model::networkManager = &network;
    Model::onlineScores = &scores;
    const auto restore = qScopeGuard([&] {
        Model::networkManager = oldNetwork;
        Model::onlineScores = oldScores;
    });
    Model model;
    QAbstractItemModelTester modelTester(
      &model, QAbstractItemModelTester::FailureReportingMode::Fatal);
    model.setProvider(Model::Provider::Tachi);
    model.setComboLte(0);
    model.setLimit(1);
    model.setMd5("chart");
    network.requests.back()->complete(
      R"({"body":{"chart":{"chartID":"42","data":{"notecount":10}}}})");
    network.requests.back()->complete(
      R"({"body":{"pbs":[{"userID":1,"scoreData":{"score":20,"optional":{"maxCombo":10}}},{"userID":2,"scoreData":{"score":0,"optional":{"maxCombo":0}}}],"users":[{"id":1,"username":"A"},{"id":2,"username":"B"}]}})");
    REQUIRE_FALSE(model.isLoading());
    REQUIRE(model.rowCount() == 1);
    CHECK(model.data(model.index(0), Model::UserNameRole).toString() == "B");
    CHECK(model.getPlayerCount() == 2);
    CHECK(model.getScoreCount() == -1);
    CHECK(model.data(model.index(0), Model::ScoreCountRole).toInt() == -1);
}

TEST_CASE(
  "Bokutachi imports refresh cached results before reporting completion",
  "[Profile][ScoreDb][OnlineOperations]")
{
    ensureApplication();
    ProfileFixture fixture;
    resource_managers::ProfileTestAccess::signIn(*fixture.profile);
    resource_managers::ProfileTestAccess::fetchTachi(*fixture.profile);
    fixture.network.requests.back()->complete(
      R"({"success":true,"body":{"username":"Test"}})");
    auto components = resource_managers::ChartDataFactory{}.loadChartData(
      "#TITLE Imported\n#BPM 120\n#00111:01\n", "imported.bms", [](auto) {
          return 1;
      });
    components.chartData->save(fixture.songs);
    const auto md5 = components.chartData->getMd5();
    const auto sha256 = components.chartData->getSha256();
    auto* scores = fixture.profile->getScoreDb();
    CHECK(scores->getChartResults().value(md5).toList().isEmpty());
    const auto firstRequest = fixture.network.requests.size();
    auto* operation = fixture.profile->importBokutachiScores();
    REQUIRE_FALSE(operation->isFinished());
    REQUIRE(fixture.network.requests.size() == firstRequest + 2);
    bool completed = false;
    QObject::connect(
      operation, &qml_components::ScoreSyncOperation::finishedChanged, [&] {
          if (operation->isFinished()) {
              completed = true;
              CHECK(scores->getChartResults().value(md5).toList().size() == 1);
          }
      });
    auto payload = QByteArray(
      R"({"body":{"charts":[{"chartID":"chart","data":{"hashMD5":"%MD5%","hashSHA256":"%SHA%"}}],"pbs":[{"game":"bms-7k","chartID":"chart","timeAchieved":1234000,"scoreData":{"enumIndexes":{"lamp":7},"score":2,"judgements":{"pgreat":1,"great":0,"good":0,"bad":0,"poor":0},"optional":{"maxCombo":1}}}]}})");
    payload.replace("%MD5%", md5.toUtf8()).replace("%SHA%", sha256.toUtf8());
    fixture.network.requests[firstRequest]->complete(payload);
    fixture.network.requests[firstRequest + 1]->complete(
      R"({"body":{"charts":[],"pbs":[]}})");
    REQUIRE(waitUntil([&] { return operation->isFinished(); }));
    CHECK(completed);
    CHECK(scores->getChartResults().value(md5).toList().size() == 1);
}
