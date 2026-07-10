#include "db/SqliteCppDb.h"
#include "qml_components/ProfileList.h"
#include "qml_components/ThemeFamily.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QTemporaryDir>

#include <algorithm>
#include <cstring>
#include <memory>
#include <optional>

namespace resource_managers {

struct ArenaTicketOperationTestAccess
{
    static void attach(ArenaTicketOperation* operation, QNetworkReply* reply)
    {
        operation->attachReply(reply);
    }
};

} // namespace resource_managers

namespace {

class StreamingNetworkReply final : public QNetworkReply
{
  public:
    StreamingNetworkReply()
    {
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        open(QIODevice::ReadOnly);
    }

    void abort() override
    {
        aborted = true;
        setFinished(true);
        setError(QNetworkReply::OperationCanceledError,
                 QStringLiteral("cancelled"));
        emit finished();
    }

    void append(QByteArray bytes)
    {
        payload.append(std::move(bytes));
        emit readyRead();
    }

    void finish()
    {
        setFinished(true);
        emit finished();
    }

    [[nodiscard]] auto bytesAvailable() const -> qint64 override
    {
        return payload.size() - position + QNetworkReply::bytesAvailable();
    }

    bool aborted{};

  protected:
    auto readData(char* data, qint64 maximumSize) -> qint64 override
    {
        const auto available = payload.size() - position;
        const auto count = (std::min)(available, maximumSize);
        if (count <= 0) {
            return -1;
        }
        std::memcpy(data, payload.constData() + position, count);
        position += count;
        return count;
    }

  private:
    QByteArray payload;
    qint64 position{};
};

void
ensureCoreApplication()
{
    if (QCoreApplication::instance() != nullptr) {
        return;
    }
    static int argc = 1;
    static char applicationName[] = "RhythmGameArenaBattlePolicyTests";
    static char* argv[]{ applicationName, nullptr };
    static const auto application =
      std::make_unique<QCoreApplication>(argc, argv);
}

} // namespace

TEST_CASE("ArenaSession ProfileList silently enforces the generic battle gate",
          "[arena][session]")
{
    ensureCoreApplication();
    QTemporaryDir temporaryDirectory;
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto songDbPath = root / "songs.sqlite";
    db::SqliteCppDb songDb(songDbPath);
    songDb.execute("CREATE TABLE properties (key TEXT PRIMARY KEY, value)");
    QNetworkAccessManager networkManager;
    qml_components::ProfileList profiles(
      songDbPath, &songDb, {}, root / "profiles", {}, &networkManager);
    auto* secondProfile = profiles.createProfile();
    REQUIRE(secondProfile != nullptr);
    REQUIRE(profiles.getProfiles().size() >= 2);
    profiles.getBattleProfiles()->setPlayer1Profile(profiles.getProfiles()[0]);
    profiles.getBattleProfiles()->setPlayer2Profile(secondProfile);
    profiles.setBattleActive(true);
    REQUIRE(profiles.getBattleActive());

    int allowedChanges = 0;
    QObject::connect(&profiles,
                     &qml_components::ProfileList::battleAllowedChanged,
                     [&] { ++allowedChanges; });
    bool reentryAttempted = false;
    QObject::connect(
      &profiles, &qml_components::ProfileList::battleActiveChanged, [&] {
          if (!profiles.getBattleActive()) {
              reentryAttempted = true;
              profiles.setBattleActive(true);
          }
      });
    profiles.setBattleAllowed(false);
    CHECK(reentryAttempted);
    CHECK_FALSE(profiles.getBattleAllowed());
    CHECK_FALSE(profiles.getBattleActive());
    CHECK(allowedChanges == 1);

    profiles.setBattleActive(true);
    CHECK_FALSE(profiles.getBattleActive());
    profiles.setBattleAllowed(true);
    CHECK(profiles.getBattleAllowed());
    CHECK_FALSE(profiles.getBattleActive());
    CHECK(allowedChanges == 2);
}

TEST_CASE("ArenaSession Profile queues enum-only ticket failure and "
          "cancellation is silent",
          "[arena][session]")
{
    ensureCoreApplication();
    QTemporaryDir temporaryDirectory;
    REQUIRE(temporaryDirectory.isValid());
    const auto root = support::qStringToPath(temporaryDirectory.path());
    const auto songDbPath = root / "songs.sqlite";
    db::SqliteCppDb songDb(songDbPath);
    songDb.execute("CREATE TABLE properties (key TEXT PRIMARY KEY, value)");
    QNetworkAccessManager networkManager;
    qml_components::ProfileList profiles(
      songDbPath, &songDb, {}, root / "profiles", {}, &networkManager);
    auto* profile = profiles.getMainProfile();
    REQUIRE(profile != nullptr);

    std::optional<resource_managers::ArenaTicketOperation::Error> failure{
        std::nullopt
    };
    auto* operation = profile->requestArenaTicket();
    QObject::connect(operation,
                     &resource_managers::ArenaTicketOperation::failed,
                     [&](auto error) { failure = error; });
    CHECK_FALSE(failure.has_value());
    QCoreApplication::processEvents();
    REQUIRE(failure.has_value());
    CHECK(*failure ==
          resource_managers::ArenaTicketOperation::Error::NotLoggedIn);

    int terminalSignals = 0;
    auto* cancelled = profile->requestArenaTicket();
    QObject::connect(cancelled,
                     &resource_managers::ArenaTicketOperation::failed,
                     [&] { ++terminalSignals; });
    QObject::connect(cancelled,
                     &resource_managers::ArenaTicketOperation::succeeded,
                     [&] { ++terminalSignals; });
    cancelled->cancel();
    QCoreApplication::processEvents();
    CHECK(terminalSignals == 0);
}

TEST_CASE("ArenaSession ticket responses are rejected while streaming past "
          "the bounded body limit",
          "[arena][session]")
{
    ensureCoreApplication();
    auto* operation = new resource_managers::ArenaTicketOperation;
    auto* reply = new StreamingNetworkReply;
    std::optional<resource_managers::ArenaTicketOperation::Error> failure;
    QObject::connect(operation,
                     &resource_managers::ArenaTicketOperation::failed,
                     [&](auto error) { failure = error; });
    resource_managers::ArenaTicketOperationTestAccess::attach(operation, reply);

    reply->append(QByteArray(64 * 1024, 'x'));
    CHECK_FALSE(failure.has_value());
    reply->append(QByteArrayLiteral("x"));

    REQUIRE(failure.has_value());
    CHECK(*failure ==
          resource_managers::ArenaTicketOperation::Error::MalformedResponse);
    CHECK(reply->aborted);
}
