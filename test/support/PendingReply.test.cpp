#include "support/PendingReply.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEventLoop>
#include <QJSEngine>
#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QQmlEngine>

#include <memory>
#include <optional>

namespace {

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

class ReplyFactory final : public QObject
{
    Q_OBJECT

  public:
    Q_INVOKABLE support::PendingReply* start()
    {
        source.emplace(this);
        return source->reply();
    }

    std::optional<support::PendingReplySource<int>> source;
};

class DestructionProbe final : public QObject
{
  public:
    explicit DestructionProbe(bool* destroyed)
      : destroyed(destroyed)
    {
    }

    ~DestructionProbe() override { *destroyed = true; }

  private:
    bool* destroyed;
};

} // namespace

TEST_CASE("PendingReply exposes one terminal result", "[PendingReply]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    auto* reply = source.reply();
    int finishedCount = 0;
    QObject::connect(
      reply, &support::PendingReply::finished, [&] { ++finishedCount; });

    CHECK(reply->isValid());
    CHECK_FALSE(reply->isResultAvailable());
    CHECK_FALSE(reply->isSuccessful());
    CHECK_FALSE(reply->value().isValid());

    REQUIRE(source.succeed(42));
    CHECK(reply->isResultAvailable());
    CHECK(reply->isSuccessful());
    CHECK(reply->value().toInt() == 42);
    CHECK(finishedCount == 1);
    CHECK_FALSE(source.fail());
    reply->cancel();
    CHECK(finishedCount == 1);
    delete reply;
}

TEST_CASE("PendingReply accepts queued producer completion",
          "[PendingReply][async]")
{
    ensureCoreApplication();

    SECTION("success")
    {
        QObject owner;
        support::PendingReplySource<int> source(&owner);
        auto* reply = source.reply();
        QEventLoop loop;
        QObject::connect(
          reply, &support::PendingReply::finished, &loop, &QEventLoop::quit);
        QMetaObject::invokeMethod(
          QCoreApplication::instance(),
          [source] { (void)source.succeed(17); },
          Qt::QueuedConnection);

        loop.exec();

        CHECK(reply->isSuccessful());
        CHECK(reply->value().toInt() == 17);
        delete reply;
    }

    SECTION("failure")
    {
        QObject owner;
        support::PendingReplySource<int> source(&owner);
        auto* reply = source.reply();
        QEventLoop loop;
        QObject::connect(
          reply, &support::PendingReply::finished, &loop, &QEventLoop::quit);
        QMetaObject::invokeMethod(
          QCoreApplication::instance(),
          [source] { (void)source.fail(); },
          Qt::QueuedConnection);

        loop.exec();

        CHECK_FALSE(reply->isSuccessful());
        CHECK_FALSE(reply->value().isValid());
        delete reply;
    }
}

TEST_CASE("PendingReply converts gadget and object values for JavaScript",
          "[PendingReply][qml]")
{
    ensureCoreApplication();

    SECTION("registered gadget")
    {
        QObject owner;
        support::PendingReplySource<QPoint> source(&owner);
        auto* reply = source.reply();
        QJSEngine engine;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
        engine.evaluate(
          "var received = -1;"
          "reply.then(function(value) { received = value.x + value.y; });");

        REQUIRE(source.succeed(QPoint(4, 7)));
        CHECK(engine.globalObject().property("received").toInt() == 11);
    }

    SECTION("QObject pointer")
    {
        QObject owner;
        QObject payload;
        payload.setObjectName(QStringLiteral("payload"));
        support::PendingReplySource<QObject*> source(&owner);
        auto* reply = source.reply();
        QJSEngine engine;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
        engine.evaluate(
          "var received = '';"
          "reply.then(function(value) { received = value.objectName; });");

        REQUIRE(source.succeed(&payload));
        CHECK(engine.globalObject().property("received").toString() ==
              QStringLiteral("payload"));
    }
}

TEST_CASE("PendingReply invokes callbacks registered after completion",
          "[PendingReply][qml]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    auto* reply = source.reply();
    REQUIRE(source.succeed(9));

    QJSEngine engine;
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    engine.evaluate(
      "var received = -1;"
      "reply.then(function(value) { received = value; });");

    CHECK(engine.globalObject().property("received").toInt() == 9);
}

TEST_CASE("PendingReply failure works before or after callback registration",
          "[PendingReply][qml]")
{
    ensureCoreApplication();

    SECTION("callback registered before failure")
    {
        QObject owner;
        support::PendingReplySource<int> source(&owner);
        auto* reply = source.reply();
        QJSEngine engine;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
        engine.evaluate(
          "var failureCount = 0;"
          "reply.then(undefined, function() { ++failureCount; });");

        REQUIRE(source.fail());
        CHECK(engine.globalObject().property("failureCount").toInt() == 1);
        reply->cancel();
        CHECK(engine.globalObject().property("failureCount").toInt() == 1);
    }

    SECTION("callback registered after failure")
    {
        QObject owner;
        support::PendingReplySource<int> source(&owner);
        auto* reply = source.reply();
        REQUIRE(source.fail());
        QJSEngine engine;
        engine.globalObject().setProperty("reply", engine.newQObject(reply));
        engine.evaluate(
          "var failureCount = 0;"
          "reply.then(undefined, function() { ++failureCount; });");

        CHECK(engine.globalObject().property("failureCount").toInt() == 1);
    }
}

TEST_CASE("PendingReply replaces callbacks registered while pending",
          "[PendingReply][qml]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    auto* reply = source.reply();
    QJSEngine engine;
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    engine.evaluate(
      "var first = 0;"
      "var second = 0;"
      "reply.then(function() { ++first; });"
      "reply.then(function() { ++second; });");

    REQUIRE(source.succeed(1));
    CHECK(engine.globalObject().property("first").toInt() == 0);
    CHECK(engine.globalObject().property("second").toInt() == 1);
}

TEST_CASE("PendingReply leaves callback exceptions on the script engine",
          "[PendingReply][qml]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    auto* reply = source.reply();
    QJSEngine engine;
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    engine.evaluate(
      "reply.then(function() { throw new Error('callback failed'); });");

    REQUIRE(source.succeed(1));
    REQUIRE(engine.hasError());
    CHECK(engine.catchError().toString().contains(
      QStringLiteral("callback failed")));
}

TEST_CASE("PendingReply cancellation is terminal and uses failure callback",
          "[PendingReply][cancel]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    auto* reply = source.reply();
    int cancellationCount = 0;
    source.setCancellationHandler([&] { ++cancellationCount; });

    QJSEngine engine;
    engine.globalObject().setProperty("reply", engine.newQObject(reply));
    engine.evaluate(
      "var successCount = 0;"
      "var failureCount = 0;"
      "reply.then(function() { ++successCount; },"
      "           function() { ++failureCount; });");

    reply->cancel();
    reply->cancel();

    CHECK(source.stopToken().stop_requested());
    CHECK(reply->isResultAvailable());
    CHECK_FALSE(reply->isSuccessful());
    CHECK_FALSE(reply->value().isValid());
    CHECK(cancellationCount == 1);
    CHECK(engine.globalObject().property("successCount").toInt() == 0);
    CHECK(engine.globalObject().property("failureCount").toInt() == 1);
    CHECK_FALSE(source.succeed(42));
    CHECK_FALSE(source.fail());
}

TEST_CASE("PendingReply invokes a late cancellation handler immediately",
          "[PendingReply][cancel]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    source.reply()->cancel();
    int cancellationCount = 0;

    source.setCancellationHandler([&] { ++cancellationCount; });

    CHECK(cancellationCount == 1);
    delete source.reply();
}

TEST_CASE("PendingReply cancellation uses the latest stage handler",
          "[PendingReply][cancel]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<int> source(&owner);
    int firstStageCount = 0;
    int secondStageCount = 0;
    source.setCancellationHandler([&] { ++firstStageCount; });
    source.setCancellationHandler([&] { ++secondStageCount; });

    source.reply()->cancel();

    CHECK(firstStageCount == 0);
    CHECK(secondStageCount == 1);
    delete source.reply();
}

TEST_CASE("PendingReply producer destruction requests cancellation",
          "[PendingReply][lifetime]")
{
    ensureCoreApplication();
    std::optional<support::PendingReplySource<int>> source;
    int cancellationCount = 0;
    {
        auto owner = std::make_unique<QObject>();
        source.emplace(owner.get());
        source->setCancellationHandler([&] { ++cancellationCount; });
        REQUIRE(source->reply() != nullptr);
    }

    CHECK(source->reply() == nullptr);
    CHECK(source->stopToken().stop_requested());
    CHECK(cancellationCount == 1);
}

TEST_CASE("PendingReply lets producers clean up rejected pointer results",
          "[PendingReply][lifetime]")
{
    ensureCoreApplication();
    QObject owner;
    support::PendingReplySource<QObject*> source(&owner);
    source.reply()->cancel();
    bool destroyed = false;
    auto* value = new DestructionProbe(&destroyed);

    if (!source.succeed(value))
        delete value;

    CHECK(destroyed);
    delete source.reply();
}

TEST_CASE("PendingReply is returned to JavaScript as a cancellable object",
          "[PendingReply][qml]")
{
    ensureCoreApplication();
    ReplyFactory factory;
    QJSEngine engine;
    QQmlEngine::setObjectOwnership(&factory, QQmlEngine::CppOwnership);
    engine.globalObject().setProperty("factory", engine.newQObject(&factory));

    const auto result = engine.evaluate(
      "var reply = factory.start();"
      "var failed = 0;"
      "reply.then(undefined, function() { ++failed; });"
      "reply.cancel();"
      "reply.resultAvailable && !reply.success && failed === 1;");

    REQUIRE_FALSE(result.isError());
    CHECK(result.toBool());
    REQUIRE(factory.source.has_value());
    CHECK(factory.source->stopToken().stop_requested());
}

#include "PendingReply.test.moc"
