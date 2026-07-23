#include "qml_components/OnlineScores.h"
#include "support/PendingReply.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QThread>

#include <algorithm>
#include <concepts>
#include <cstring>
#include <memory>
#include <utility>

namespace {

static_assert(
  std::same_as<decltype(std::declval<qml_components::OnlineScores&>()
                          .getScoreByGuid(std::declval<const QString&>(),
                                          std::declval<const QString&>())),
               support::PendingReply*>);
static_assert(
  std::same_as<
    decltype(std::declval<qml_components::OnlineScores&>()
               .getRankingEntryAtTimestamp(
                 std::declval<QString>(),
                 std::declval<qint64>(),
                 std::declval<QString>(),
                 std::declval<qint64>(),
                 std::declval<qml_components::OnlineRankingModel::Provider>())),
    support::PendingReply*>);

void
ensureCoreApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    if (!QCoreApplication::instance()) {
        [[maybe_unused]] static auto* app = new QCoreApplication(argc, argv);
    }
}

class FakeNetworkReply final : public QNetworkReply
{
  public:
    FakeNetworkReply(const QNetworkRequest& request, QObject* parent)
      : QNetworkReply(parent)
    {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }

    void abort() override
    {
        if (isFinished())
            return;
        aborted = true;
        setError(OperationCanceledError, QStringLiteral("cancelled"));
        setFinished(true);
        emit finished();
    }

    void complete(QByteArray response)
    {
        payload = std::move(response);
        setFinished(true);
        emit finished();
    }

    bool aborted{};

    qint64 bytesAvailable() const override
    {
        return payload.size() - offset + QNetworkReply::bytesAvailable();
    }

  protected:
    qint64 readData(char* data, qint64 maxSize) override
    {
        if (offset >= payload.size())
            return -1;
        const auto available = payload.size() - offset;
        const auto bytesToCopy = std::min(maxSize, available);
        std::memcpy(data,
                    payload.constData() + offset,
                    static_cast<std::size_t>(bytesToCopy));
        offset += bytesToCopy;
        return bytesToCopy;
    }

  private:
    QByteArray payload;
    qint64 offset{};
};

class FakeNetworkAccessManager final : public QNetworkAccessManager
{
  public:
    QList<QPointer<FakeNetworkReply>> replies;
    QPointer<FakeNetworkReply> lastReply;

  protected:
    QNetworkReply* createRequest(Operation,
                                 const QNetworkRequest& request,
                                 QIODevice*) override
    {
        lastReply = new FakeNetworkReply(request, this);
        replies.append(lastReply);
        return lastReply;
    }
};

constexpr auto resolvedTachiChart =
  R"({"body":{"chart":{"chartID":"chart-id","data":{"notecount":100}}}})";

constexpr auto minimalScore =
  R"({"guid":"score-id","replayData":[],"gaugeHistory":[]})";

} // namespace

TEST_CASE("OnlineScores score reply cancellation aborts the request",
          "[OnlineScores][PendingReply][cancel]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    qml_components::OnlineScores onlineScores(&network);

    auto* operation = onlineScores.getScoreByGuid(
      QStringLiteral("https://example.invalid/"), QStringLiteral("score-id"));
    REQUIRE(network.lastReply);

    operation->cancel();

    CHECK(network.lastReply->aborted);
    CHECK(operation->isResultAvailable());
    CHECK_FALSE(operation->isSuccessful());
}

TEST_CASE("OnlineScores RhythmGame cancellation aborts the ranking request",
          "[OnlineScores][PendingReply][cancel]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    qml_components::OnlineScores onlineScores(&network);

    auto* operation = onlineScores.getRankingEntryAtTimestamp(
      QStringLiteral("https://example.invalid/"),
      42,
      QStringLiteral("0123456789abcdef0123456789abcdef"),
      1,
      qml_components::OnlineRankingModel::Provider::RhythmGame);
    REQUIRE(network.lastReply);

    operation->cancel();

    CHECK(network.lastReply->aborted);
    CHECK(operation->isResultAvailable());
    CHECK_FALSE(operation->isSuccessful());
}

TEST_CASE("OnlineScores Tachi cancellation aborts chart resolution",
          "[OnlineScores][PendingReply][cancel]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    qml_components::OnlineScores onlineScores(&network);

    auto* operation = onlineScores.getRankingEntryAtTimestamp(
      QStringLiteral("https://example.invalid/"),
      42,
      QStringLiteral("0123456789abcdef0123456789abcdef"),
      1,
      qml_components::OnlineRankingModel::Provider::Tachi);
    REQUIRE(network.lastReply);

    operation->cancel();

    CHECK(network.lastReply->aborted);
    CHECK(operation->isResultAvailable());
    CHECK_FALSE(operation->isSuccessful());
}

TEST_CASE("OnlineScores Tachi cancellation follows the score request",
          "[OnlineScores][PendingReply][cancel]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    qml_components::OnlineScores onlineScores(&network);

    auto* operation = onlineScores.getRankingEntryAtTimestamp(
      QStringLiteral("https://example.invalid/"),
      42,
      QStringLiteral("0123456789abcdef0123456789abcdef"),
      1,
      qml_components::OnlineRankingModel::Provider::Tachi);
    REQUIRE(network.lastReply);
    network.lastReply->complete(QByteArray(resolvedTachiChart));
    REQUIRE(network.replies.size() == 2);
    const auto scoresReply = network.lastReply;
    REQUIRE(scoresReply);

    operation->cancel();

    CHECK(scoresReply->aborted);
    CHECK(operation->isResultAvailable());
    CHECK_FALSE(operation->isSuccessful());
}

TEST_CASE("OnlineScores destruction cancels active network replies",
          "[OnlineScores][PendingReply][cancel][lifetime]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    auto operation = QPointer<support::PendingReply>{};
    {
        auto onlineScores =
          std::make_unique<qml_components::OnlineScores>(&network);
        operation = onlineScores->getScoreByGuid(
          QStringLiteral("https://example.invalid/"),
          QStringLiteral("score-id"));
        REQUIRE(network.lastReply);
    }

    CHECK(network.lastReply->aborted);
    CHECK(operation.isNull());
}

TEST_CASE("OnlineScores destruction drains queued parser delivery",
          "[OnlineScores][PendingReply][cancel][lifetime]")
{
    ensureCoreApplication();
    FakeNetworkAccessManager network;
    auto operation = QPointer<support::PendingReply>{};
    {
        auto onlineScores =
          std::make_unique<qml_components::OnlineScores>(&network);
        operation = onlineScores->getScoreByGuid(
          QStringLiteral("https://example.invalid/"),
          QStringLiteral("score-id"));
        REQUIRE(network.lastReply);
        network.lastReply->complete(QByteArray(minimalScore));
        QThread::msleep(20);
    }

    CHECK(operation.isNull());
    QCoreApplication::sendPostedEvents(QCoreApplication::instance(),
                                       QEvent::MetaCall);
}
