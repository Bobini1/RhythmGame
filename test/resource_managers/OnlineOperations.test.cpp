#include "resource_managers/Profile.h"
#include "resource_managers/DefineDb.h"
#include "resource_managers/SerializeConfig.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/ChartDataFactory.h"
#include "qml_components/OnlineRankingModel.h"
#include "qml_components/OnlineScores.h"
#include "qml_components/ScoreSyncOperation.h"
#include "gameplay_logic/BmsScore.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
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
    auto firstPage = network.requests.back();
    REQUIRE(firstPage->parent() == model.get());
    firstPage->complete(R"({"body":{"pbs":[{}],"users":[]}})");
    auto nextPage = network.requests.back();
    REQUIRE(nextPage != firstPage);
    CHECK(nextPage->parent() == model.get());
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
