# Pending Reply Cancellation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace `QIfPendingReply` with a public-Qt-only cancellable reply whose returned QML handle owns cancellation, then remove Qt Interface Framework from every build and packaging path.

**Architecture:** A QML-facing `support::PendingReply` `QObject` stores JavaScript callbacks and observable terminal state. A copyable `support::PendingReplySource<T>` is captured by producers, shares a stop source and cancellation handler with the handle, and completes the handle on the application thread. `ScoreDb` and `OnlineScores` return `PendingReply*`; QML owners retain and cancel only their own replies.

**Tech Stack:** C++23, Qt 6 Core/Qml/Network/Concurrent, QML, Catch2 3, CMake, vcpkg manifests, Nix flakes.

## Global Constraints

- Remove every build and packaging dependency on Qt Interface Framework.
- Preserve `then(success, failed)`, `valid`, `resultAvailable`, `success`, and `value`.
- Cancellation for migrated reply-producing operations is invoked as
  `reply.cancel()`; remove `ScoreDb::cancelPending()` and do not introduce a
  replacement factory-level cancellation API.
- Cancellation is terminal, idempotent, reports `resultAvailable == true` and `success == false`, and invokes the existing failure callback.
- Do not expose a public `cancelled` property unless an actual caller needs to distinguish it.
- Network cancellation aborts the active `QNetworkReply`.
- SQLite work already executing may finish internally, but canceled results must never be delivered.
- Only the first success, failure, or cancellation transition wins.
- Use only public Qt APIs; do not include Qt private headers.
- Preserve unrelated worktree changes and historical design/plan documents.

---

## File Map

- Create `src/support/PendingReply.h`: typed producer source and QML handle declaration.
- Create `src/support/PendingReply.cpp`: terminal-state, JavaScript-callback, cancellation, and lifetime implementation.
- Create `test/support/PendingReply.test.cpp`: state-machine, callback-conversion, lifetime, and cancellation tests.
- Create `test/qml_components/ScoreDbAsyncApi.test.cpp`: compile-time API contract for per-reply cancellation.
- Create `test/qml_components/OnlineScores.test.cpp`: fake-network verification that reply cancellation aborts network work.
- Modify `CMakeLists.txt`: compile the replacement and stop finding/linking Interface Framework.
- Modify `test/CMakeLists.txt`: compile the three new test sources.
- Modify `RhythmGameQml/QmlForeignTypes.h`: register `PendingReply` as an anonymous QML type.
- Modify `src/qml_components/ScoreDb.h/.cpp`: return `PendingReply*`, centralize cancellable worker completion, and remove `cancelPending()`.
- Modify `src/qml_components/OnlineScores.h/.cpp`: return `PendingReply*` and connect cancellation to each live network stage.
- Modify `src/qml_components/OnlineRankingModel.h`: remove its unused `QIfPendingReply` include.
- Modify `RhythmGameQml/Lr2/Lr2SelectContext.qml`: retain/cancel this component's replies.
- Modify `share/RhythmGame/themes/Default/scripts/select/List.qml`: retain/cancel this component's replies.
- Modify `vcpkg.json`, `flake.nix`, `nix/shells/default.nix`, and `nix/packages/rhythmgame.nix`: remove the dependency and its arguments.
- Delete `vcpkgOverlayPortsWindows/qtinterfaceframework/`, `nix/packages/qtinterfaceframework.nix`, and `nix/packages/python-qface.nix`.

---

### Task 1: Add the cancellable PendingReply module

**Files:**

- Create: `src/support/PendingReply.h`
- Create: `src/support/PendingReply.cpp`
- Create: `test/support/PendingReply.test.cpp`
- Modify: `CMakeLists.txt:300-316`
- Modify: `RhythmGameQml/QmlForeignTypes.h:1-20,127-168`
- Modify: `test/CMakeLists.txt:10-55`

**Interfaces:**

- Consumes: `QJSEngine`, `QJSManagedValue`, `QQmlEngine::setObjectOwnership`, `qjsEngine(QObject*)`, `std::stop_source`.
- Produces: `support::PendingReply`, `support::PendingReplySource<T>`, `PendingReply::cancel()`, and the `finished()` signal used by later QML tasks.

- [ ] **Step 1: Add failing state-machine and QML-return tests**

Add `support/PendingReply.test.cpp` to `RhythmGame_test` in
`test/CMakeLists.txt`, then create the test with this structure:

```cpp
#include "support/PendingReply.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QEventLoop>
#include <QJSEngine>
#include <QMetaObject>
#include <QObject>
#include <QPoint>

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
```

- [ ] **Step 2: Run the test target and confirm the missing replacement fails**

Run:

```powershell
cmake --preset dev
cmake --build --preset dev --target RhythmGame_test
```

Expected: configuration succeeds, then compilation fails because
`support/PendingReply.h` does not exist.

- [ ] **Step 3: Implement the typed source and reply declaration**

Create `src/support/PendingReply.h` with the complete public contract:

```cpp
#ifndef RHYTHMGAME_PENDINGREPLY_H
#define RHYTHMGAME_PENDINGREPLY_H

#include <QCoreApplication>
#include <QJSValue>
#include <QObject>
#include <QThread>
#include <QVariant>

#include <functional>
#include <memory>
#include <stop_token>
#include <type_traits>
#include <utility>

namespace support {

class PendingReply;

namespace detail {

struct PendingReplyState
{
    PendingReply* reply{};
    std::stop_source stopSource;
    std::function<void()> cancellationHandler;
};

inline void
assertApplicationThread()
{
    const auto* application = QCoreApplication::instance();
    Q_ASSERT(!application ||
             QThread::currentThread() == application->thread());
}

} // namespace detail

template<typename T>
class PendingReplySource;

class PendingReply final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ isValid CONSTANT FINAL)
    Q_PROPERTY(bool resultAvailable READ isResultAvailable NOTIFY finished FINAL)
    Q_PROPERTY(bool success READ isSuccessful NOTIFY finished FINAL)
    Q_PROPERTY(QVariant value READ value NOTIFY finished FINAL)

  public:
    ~PendingReply() override;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] bool isResultAvailable() const;
    [[nodiscard]] bool isSuccessful() const;
    [[nodiscard]] QVariant value() const;

    Q_INVOKABLE void then(const QJSValue& success,
                          const QJSValue& failed = QJSValue());
    Q_INVOKABLE void cancel();

  signals:
    void finished();

  private:
    template<typename T>
    friend class PendingReplySource;

    explicit PendingReply(std::shared_ptr<detail::PendingReplyState> state,
                          QObject* parent);
    bool settle(bool success, QVariant value, bool requestCancellation);
    void invokeCallback();
    void releaseToQml();

    std::shared_ptr<detail::PendingReplyState> state;
    QVariant result;
    QJSValue successCallback;
    QJSValue failedCallback;
    bool resultAvailable{};
    bool successful{};
};

template<typename T>
class PendingReplySource
{
  public:
    explicit PendingReplySource(QObject* owner)
      : state(std::make_shared<detail::PendingReplyState>())
    {
        detail::assertApplicationThread();
        state->reply = new PendingReply(state, owner);
    }

    [[nodiscard]] PendingReply* reply() const
    {
        detail::assertApplicationThread();
        return state->reply;
    }

    [[nodiscard]] bool succeed(const T& value) const
    {
        detail::assertApplicationThread();
        auto* pendingReply = state->reply;
        if (!pendingReply)
            return false;
        if constexpr (std::is_same_v<std::remove_cv_t<T>, QVariant>)
            return pendingReply->settle(true, value, false);
        else
            return pendingReply->settle(
              true, QVariant::fromValue(value), false);
    }

    [[nodiscard]] bool fail() const
    {
        detail::assertApplicationThread();
        auto* pendingReply = state->reply;
        return pendingReply &&
               pendingReply->settle(false, QVariant{}, false);
    }

    [[nodiscard]] std::stop_token stopToken() const
    {
        return state->stopSource.get_token();
    }

    void setCancellationHandler(std::function<void()> handler) const
    {
        detail::assertApplicationThread();
        if (state->stopSource.stop_requested()) {
            if (handler)
                handler();
            return;
        }
        if (!state->reply || state->reply->isResultAvailable())
            return;
        state->cancellationHandler = std::move(handler);
    }

  private:
    std::shared_ptr<detail::PendingReplyState> state;
};

} // namespace support

#endif // RHYTHMGAME_PENDINGREPLY_H
```

- [ ] **Step 4: Implement terminal state, JavaScript invocation, and lifetime**

Create `src/support/PendingReply.cpp`:

```cpp
#include "PendingReply.h"

#include <QJSEngine>
#include <QJSManagedValue>
#include <QQmlEngine>
#include <QQmlInfo>

namespace support {

PendingReply::PendingReply(std::shared_ptr<detail::PendingReplyState> state,
                           QObject* parent)
  : QObject(parent)
  , state(std::move(state))
{
}

PendingReply::~PendingReply()
{
    detail::assertApplicationThread();
    if (!state || state->reply != this)
        return;

    state->reply = nullptr;
    if (resultAvailable)
        return;

    state->stopSource.request_stop();
    auto handler = std::move(state->cancellationHandler);
    if (handler)
        handler();
}

bool
PendingReply::isValid() const
{
    return true;
}

bool
PendingReply::isResultAvailable() const
{
    return resultAvailable;
}

bool
PendingReply::isSuccessful() const
{
    return successful;
}

QVariant
PendingReply::value() const
{
    return result;
}

void
PendingReply::then(const QJSValue& success, const QJSValue& failed)
{
    detail::assertApplicationThread();
    if (!success.isUndefined() && !success.isCallable()) {
        qmlWarning(this) << "PendingReply success callback is not callable";
        return;
    }
    if (!failed.isUndefined() && !failed.isCallable()) {
        qmlWarning(this) << "PendingReply failure callback is not callable";
        return;
    }

    successCallback = success;
    failedCallback = failed;
    if (resultAvailable)
        invokeCallback();
}

void
PendingReply::cancel()
{
    detail::assertApplicationThread();
    (void)settle(false, QVariant{}, true);
}

bool
PendingReply::settle(bool success,
                     QVariant value,
                     bool requestCancellation)
{
    detail::assertApplicationThread();
    if (resultAvailable)
        return false;

    resultAvailable = true;
    successful = success;
    result = success ? std::move(value) : QVariant{};

    auto cancellationHandler =
      requestCancellation ? std::move(state->cancellationHandler)
                          : std::function<void()>{};
    state->cancellationHandler = {};
    if (requestCancellation)
        state->stopSource.request_stop();
    if (cancellationHandler)
        cancellationHandler();

    emit finished();
    invokeCallback();
    releaseToQml();
    return true;
}

void
PendingReply::invokeCallback()
{
    auto callback = successful ? std::move(successCallback)
                               : std::move(failedCallback);
    successCallback = {};
    failedCallback = {};
    if (callback.isUndefined())
        return;

    auto* engine = qjsEngine(this);
    if (!engine) {
        qmlWarning(this) << "PendingReply has no associated QJSEngine";
        return;
    }

    QJSManagedValue managedCallback(std::move(callback), engine);
    if (successful)
        managedCallback.call({ engine->toScriptValue(result) });
    else
        managedCallback.call();
}

void
PendingReply::releaseToQml()
{
    setParent(nullptr);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::JavaScriptOwnership);
}

} // namespace support
```

- [ ] **Step 5: Add the module to CMake and the QML type registry**

Add these entries next to the other `src/support` sources in
`CMakeLists.txt`:

```cmake
        src/support/PendingReply.cpp
        src/support/PendingReply.h
```

Include the type in `RhythmGameQml/QmlForeignTypes.h`:

```cpp
#include "support/PendingReply.h"
```

Register it next to the other anonymous foreign types:

```cpp
RHYTHMGAME_QML_ANONYMOUS_FOREIGN(PendingReplyForeign,
                                 support::PendingReply);
```

- [ ] **Step 6: Build and run the focused tests**

Run:

```powershell
cmake --preset dev
cmake --build --preset dev --target RhythmGame_test
& 'build/dev/bin/RhythmGame_test.exe' '[PendingReply]'
```

Expected: the target builds and all `[PendingReply]` tests pass.

- [ ] **Step 7: Commit the standalone reply module**

```powershell
git add CMakeLists.txt RhythmGameQml/QmlForeignTypes.h `
  src/support/PendingReply.h src/support/PendingReply.cpp `
  test/CMakeLists.txt test/support/PendingReply.test.cpp
git commit -m "feat: add cancellable pending replies"
```

---

### Task 2: Move ScoreDb cancellation onto returned replies

**Files:**

- Create: `test/qml_components/ScoreDbAsyncApi.test.cpp`
- Modify: `test/CMakeLists.txt`
- Modify: `src/qml_components/ScoreDb.h:8-104`
- Modify: `src/qml_components/ScoreDb.cpp:529-931`
- Modify: `RhythmGameQml/Lr2/Lr2SelectContext.qml:904-930,998-1000,1299-1363`
- Modify: `share/RhythmGame/themes/Default/scripts/select/List.qml:9-45,78-88`

**Interfaces:**

- Consumes: `support::PendingReplySource<T>` from Task 1.
- Produces: all asynchronous `ScoreDb` methods returning
  `support::PendingReply*`; QML-local `trackScoreDbReply()` and
  `cancelScoreDbReplies()` helpers; no `ScoreDb::cancelPending()`.

- [ ] **Step 1: Add a failing compile-time API contract**

Add `qml_components/ScoreDbAsyncApi.test.cpp` to `RhythmGame_test`, then create:

```cpp
#include "qml_components/ScoreDb.h"
#include "support/PendingReply.h"

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <utility>

namespace {

template<typename T>
concept HasCancelPending = requires(T& value) { value.cancelPending(); };

static_assert(!HasCancelPending<qml_components::ScoreDb>);
static_assert(
  std::same_as<
    decltype(std::declval<qml_components::ScoreDb&>().getScoresForMd5(
      std::declval<const QList<QString>&>())),
    support::PendingReply*>);
static_assert(
  std::same_as<
    decltype(std::declval<qml_components::ScoreDb&>().getTotalStats()),
    support::PendingReply*>);

} // namespace

TEST_CASE("ScoreDb async API exposes cancellation on returned replies",
          "[ScoreDb][PendingReply]")
{
    SUCCEED();
}
```

- [ ] **Step 2: Run the compile-time contract and verify it fails**

Run:

```powershell
cmake --build --preset dev --target RhythmGame_test
```

Expected: compilation fails because `ScoreDb` still has `cancelPending()` and
returns `QIfPendingReply<T>`.

- [ ] **Step 3: Change the ScoreDb header to the per-reply API**

Replace `<QIfPendingReply>` with `"support/PendingReply.h"`, remove
`stopSource`, make the thread pool non-mutable, remove `cancelPending()`, and
declare these exact public signatures:

```cpp
Q_INVOKABLE support::PendingReply* getScoresForMd5(
  const QList<QString>& md5s);
Q_INVOKABLE support::PendingReply* getScoresForCourseId(
  const QList<QString>& courseIds);
Q_INVOKABLE support::PendingReply* getScores(const QString& folder);
Q_INVOKABLE support::PendingReply* getScores(
  const resource_managers::Table& table);
Q_INVOKABLE support::PendingReply* getScores(
  const resource_managers::Level& level);
Q_INVOKABLE support::PendingReply* getScoreSummary(
  const QString& folder);
Q_INVOKABLE support::PendingReply* getScoreSummary(
  const resource_managers::Table& table);
Q_INVOKABLE support::PendingReply* getScoreSummary(
  const resource_managers::Level& level);
Q_INVOKABLE support::PendingReply* getTotalStats();
```

Update the class documentation to say:

```cpp
/**
 * @brief Provides asynchronous access to a profile's score database.
 * @details Queries execute in the thread pool. Each returned PendingReply owns
 * its cancellation; callers retain and cancel only the work they started.
 */
```

- [ ] **Step 4: Add one cancellable worker adapter in ScoreDb.cpp**

Add the standard headers used by the adapter:

```cpp
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
```

In the anonymous namespace, add:

```cpp
void
discardScoreQueryResult(qml_components::ScoreQueryResult& result)
{
    for (const auto& value : result.scores) {
        for (const auto& entry : value.toList()) {
            if (auto* score =
                  entry.value<gameplay_logic::BmsScore*>()) {
                score->deleteLater();
            } else if (auto* course =
                         entry.value<gameplay_logic::BmsScoreCourse*>()) {
                course->deleteLater();
            }
        }
    }
}

void
discardTableQueryResult(qml_components::TableQueryResult& result)
{
    discardScoreQueryResult(result.scores);
    discardScoreQueryResult(result.courseScores);
}

template<typename Result,
         typename Query,
         typename Discard = std::nullptr_t>
auto
runScoreQuery(qml_components::ScoreDb* owner,
              QThreadPool& threadPool,
              std::string operation,
              Query query,
              Discard discard = nullptr) -> support::PendingReply*
{
    auto source = support::PendingReplySource<Result>{ owner };
    auto* reply = source.reply();
    const auto stopToken = source.stopToken();

    threadPool.start(
      [source,
       stopToken,
       operation = std::move(operation),
       query = std::move(query),
       discard = std::move(discard)]() mutable {
          if (stopToken.stop_requested())
              return;

          try {
              auto result = query();
              QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [source,
                 result = std::move(result),
                 discard = std::move(discard)]() mutable {
                    if (source.succeed(result))
                        return;
                    if constexpr (!std::is_same_v<Discard, std::nullptr_t>)
                        discard(result);
                },
                Qt::QueuedConnection);
          } catch (const std::exception& exception) {
              auto error = std::string(exception.what());
              QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                [source,
                 operation = std::move(operation),
                 error = std::move(error)] {
                    if (source.fail())
                        spdlog::error("{}: {}", operation, error);
                },
                Qt::QueuedConnection);
          }
      });

    return reply;
}
```

This adapter is the only place that catches query exceptions and posts results
back to the application thread. It deliberately posts a result produced during
a cancellation race so the application thread can destroy any `QObject*`
payloads through the typed discard function.

- [ ] **Step 5: Migrate the short ScoreDb query methods**

Replace the duplicated source/thread/catch blocks with:

```cpp
auto
ScoreDb::getScoresForMd5(const QList<QString>& md5s)
  -> support::PendingReply*
{
    if (md5s.isEmpty()) {
        auto source = support::PendingReplySource<ScoreQueryResult>{ this };
        (void)source.succeed(ScoreQueryResult{});
        return source.reply();
    }
    return runScoreQuery<ScoreQueryResult>(
      this,
      threadPool,
      "Error in getScoresForMd5",
      [this, md5s] { return getScoresForMd5Impl(md5s); },
      discardScoreQueryResult);
}

auto
ScoreDb::getScoresForCourseId(const QList<QString>& courseIds)
  -> support::PendingReply*
{
    if (courseIds.isEmpty()) {
        auto source = support::PendingReplySource<ScoreQueryResult>{ this };
        (void)source.succeed(ScoreQueryResult{});
        return source.reply();
    }
    return runScoreQuery<ScoreQueryResult>(
      this,
      threadPool,
      "Error in getScoresForCourseId",
      [this, courseIds] { return getScoresForCourseIdImpl(courseIds); },
      discardScoreQueryResult);
}

auto
ScoreDb::getScores(const resource_managers::Level& level)
  -> support::PendingReply*
{
    auto md5s = QStringList{};
    for (const auto& entry : level.entries)
        md5s.append(entry.md5);
    return getScoresForMd5(md5s);
}

auto
ScoreDb::getScoreSummary(const QString& folder) -> support::PendingReply*
{
    return runScoreQuery<QVariantMap>(
      this,
      threadPool,
      "Error in getScoreSummary",
      [this, folder] { return getFolderScoreSummaryImpl(folder); });
}
```

- [ ] **Step 6: Migrate the complex ScoreDb query bodies without changing SQL**

For `getScores(QString)`, `getScores(Table)`, both aggregate summary overloads,
and `getTotalStats()`, move each existing synchronous query/result-construction
body into the `Query` lambda passed to `runScoreQuery`. Delete the nested
`try/catch`, `QMetaObject::invokeMethod`, token checks, and
`QIfPendingReply::setSuccess/setFailed` calls because the adapter now owns those
responsibilities.

The final wrappers have these exact shapes:

```cpp
auto
ScoreDb::getScores(const QString& folder) -> support::PendingReply*
{
    return runScoreQuery<ScoreQueryResult>(
      this,
      threadPool,
      "Error in getScores",
      [this, folder] {
          auto countQuery = scoreDb->createStatement(
            "SELECT COUNT(*) "
            "FROM song_db.charts "
            "WHERE path LIKE ? || '%' "
            "AND NOT EXISTS ("
            "  SELECT 1 FROM score WHERE score.md5 = song_db.charts.md5"
            ")");
          countQuery.bind(1, folder.toStdString());
          const auto unplayedCount =
            countQuery.executeAndGet<int>().value_or(0);

          auto query = scoreDb->createStatement(
            "SELECT score.max_points, score.max_hits, "
            "score.normal_note_count, score.scratch_count, score.ln_count, "
            "score.bss_count, "
            "score.mine_count, score.clear_type, score.points, "
            "score.max_combo, score.poor, score.empty_poor, score.bad, "
            "score.good, score.great, score.perfect, score.mine_hits, "
            "score.guid, score.sha256, score.md5, score.unix_timestamp, "
            "score.length, score.random_sequence, score.random_seed, "
            "score.note_order_algorithm, score.note_order_algorithm_p2, "
            "score.dp_options, score.keymode, score.game_version, "
            "score.owner, "
            "replay_data.*, gauge_history.* "
            "FROM score "
            "JOIN replay_data ON score.guid = replay_data.score_guid "
            "JOIN gauge_history ON score.guid = gauge_history.score_guid "
            "JOIN song_db.charts ON score.md5 = song_db.charts.md5 "
            "WHERE song_db.charts.path LIKE ? || '%' ");
          query.bind(1, folder.toStdString());

          const auto rows = query.executeAndGetAll<
            std::tuple<gameplay_logic::BmsResult::DTO,
                       gameplay_logic::BmsReplayData::DTO,
                       gameplay_logic::BmsGaugeHistory::DTO>>();
          auto* mainThread = QCoreApplication::instance()->thread();
          QMap<QString, QVariantList> groupedScores;
          for (const auto& row : rows) {
              const auto md5 =
                QString::fromStdString(std::get<0>(row).md5);
              auto* score = new gameplay_logic::BmsScore{
                  gameplay_logic::BmsResult::load(std::get<0>(row)),
                  gameplay_logic::BmsReplayData::load(std::get<1>(row)),
                  gameplay_logic::BmsGaugeHistory::load(std::get<2>(row))
              };
              score->moveToThread(mainThread);
              groupedScores[md5].append(QVariant::fromValue(score));
          }

          auto groupedVariantScores = QVariantMap{};
          for (auto& [md5, scores] : groupedScores.asKeyValueRange())
              groupedVariantScores[md5] = QVariant::fromValue(
                std::move(scores));

          return ScoreQueryResult{
            .unplayed = unplayedCount,
            .scores = std::move(groupedVariantScores),
          };
      },
      discardScoreQueryResult);
}

auto
ScoreDb::getScores(const resource_managers::Table& table)
  -> support::PendingReply*
{
    return runScoreQuery<TableQueryResult>(
      this,
      threadPool,
      "Error in getScores(table)",
      [this, table] {
          auto md5s = QStringList{};
          for (const auto& level : table.levels) {
              for (const auto& entry : level.entries)
                  md5s.append(entry.md5);
          }
          auto courseIds = QStringList{};
          for (const auto& courseList : table.courses) {
              for (const auto& course : courseList)
                  courseIds.append(course.getIdentifier());
          }
          auto scores = getScoresForMd5Impl(md5s);
          auto courseScores = getScoresForCourseIdImpl(courseIds);
          return TableQueryResult{
            .courseScores = std::move(courseScores),
            .scores = std::move(scores),
          };
      },
      discardTableQueryResult);
}

auto
ScoreDb::getScoreSummary(const resource_managers::Table& table)
  -> support::PendingReply*
{
    auto md5s = QStringList{};
    for (const auto& level : table.levels) {
        for (const auto& entry : level.entries)
            md5s.append(entry.md5);
    }
    return runScoreQuery<QVariantMap>(
      this,
      threadPool,
      "Error in getScoreSummary(table)",
      [this, md5s] { return getScoreSummaryForMd5Impl(md5s); });
}

auto
ScoreDb::getScoreSummary(const resource_managers::Level& level)
  -> support::PendingReply*
{
    auto md5s = QStringList{};
    for (const auto& entry : level.entries)
        md5s.append(entry.md5);
    return runScoreQuery<QVariantMap>(
      this,
      threadPool,
      "Error in getScoreSummary(level)",
      [this, md5s] { return getScoreSummaryForMd5Impl(md5s); });
}

auto
ScoreDb::getTotalStats() -> support::PendingReply*
{
    return runScoreQuery<ScoreStatsResult>(
      this,
      threadPool,
      "Error in getTotalStats",
      [this] {
          auto statement = scoreDb->createStatement(
            "SELECT "
            "COUNT(*), "
            "COALESCE(SUM(CASE WHEN clear_type NOT IN ('FAILED', 'NOPLAY') "
            "THEN 1 ELSE 0 END), 0), "
            "COALESCE(SUM(CASE WHEN clear_type = 'FAILED' THEN 1 ELSE 0 "
            "END), 0), "
            "COALESCE(SUM(perfect), 0), "
            "COALESCE(SUM(great), 0), "
            "COALESCE(SUM(good), 0), "
            "COALESCE(SUM(bad), 0), "
            "COALESCE(SUM(poor + empty_poor), 0), "
            "COALESCE(MAX(max_combo), 0) "
            "FROM score");
          const auto row =
            statement.executeAndGet<ScoreStatsRow>().value_or(ScoreStatsRow{});
          return ScoreStatsResult{
            .playCount = row.playCount,
            .clearCount = row.clearCount,
            .failCount = row.failCount,
            .perfectCount = row.perfectCount,
            .greatCount = row.greatCount,
            .goodCount = row.goodCount,
            .badCount = row.badCount,
            .poorCount = row.poorCount,
            .maxCombo = row.maxCombo,
          };
      });
}
```

Delete `ScoreDb::cancelPending()` completely.

- [ ] **Step 7: Add local reply ownership to Lr2SelectContext.qml**

Add these members near the other request-token state:

```qml
property var pendingScoreDbReplies: []

function trackScoreDbReply(reply: var) : var {
    if (!reply || reply.resultAvailable) {
        return reply;
    }
    pendingScoreDbReplies.push(reply);
    let forget = function() {
        reply.finished.disconnect(forget);
        let index = pendingScoreDbReplies.indexOf(reply);
        if (index >= 0) {
            pendingScoreDbReplies.splice(index, 1);
            pendingScoreDbReplies = pendingScoreDbReplies.slice();
        }
    };
    reply.finished.connect(forget);
    return reply;
}

function cancelScoreDbReplies() {
    let replies = pendingScoreDbReplies;
    pendingScoreDbReplies = [];
    for (let reply of replies) {
        if (reply && !reply.resultAvailable) {
            reply.cancel();
        }
    }
}
```

Replace both calls to `scoreDb.cancelPending()`/`ScoreDb.cancelPending()` with
`cancelScoreDbReplies()`. Change each asynchronous call in this component to
pass through the tracker:

```qml
trackScoreDbReply(db.getScoreSummary(item)).then((result) => {
trackScoreDbReply(scoreDb.getScoresForMd5(md5s)).then((result) => {
trackScoreDbReply(scoreDb.getScoresForCourseId(courseIds)).then((courseResult) => {
trackScoreDbReply(scoreDb.getScores(folder)).then((result) => {
trackScoreDbReply(db.getTotalStats()).then((result) => {
```

Call `cancelScoreDbReplies()` from `Component.onDestruction`.

- [ ] **Step 8: Add local reply ownership to Default List.qml**

Add the same `pendingScoreDbReplies`, `trackScoreDbReply()`, and
`cancelScoreDbReplies()` implementation from Step 7. Replace both
`scoreDb.cancelPending()` calls with `cancelScoreDbReplies()`, and wrap the
three reply-returning calls:

```qml
trackScoreDbReply(
    Rg.profileList.mainProfile.scoreDb.getScoresForMd5(md5s)
).then((result) => {

trackScoreDbReply(
    Rg.profileList.mainProfile.scoreDb.getScores(
        historyStack[historyStack.length - 1])
).then((result) => {

trackScoreDbReply(
    Rg.profileList.mainProfile.scoreDb.getScoreSummary(folder)
).then((result) => {
```

Call `cancelScoreDbReplies()` from `Component.onDestruction`.

- [ ] **Step 9: Build and verify the ScoreDb API and QML modules**

Run:

```powershell
cmake --build --preset dev --target RhythmGame_test RhythmGame_qmlplugin RhythmGame_lr2_qmlplugin
& 'build/dev/bin/RhythmGame_test.exe' '[ScoreDb][PendingReply]'
rg -n -S 'cancelPending\(' src/qml_components/ScoreDb.h `
  src/qml_components/ScoreDb.cpp `
  RhythmGameQml/Lr2/Lr2SelectContext.qml `
  share/RhythmGame/themes/Default/scripts/select/List.qml
```

Expected: build succeeds, the API test passes, and `rg` reports no
`ScoreDb::cancelPending()` declaration, implementation, or QML call.

- [ ] **Step 10: Commit the ScoreDb migration**

```powershell
git add src/qml_components/ScoreDb.h src/qml_components/ScoreDb.cpp `
  RhythmGameQml/Lr2/Lr2SelectContext.qml `
  share/RhythmGame/themes/Default/scripts/select/List.qml `
  test/CMakeLists.txt test/qml_components/ScoreDbAsyncApi.test.cpp
git commit -m "refactor: cancel score queries by reply"
```

---

### Task 3: Make OnlineScores replies cancel their network work

**Files:**

- Create: `test/qml_components/OnlineScores.test.cpp`
- Modify: `test/CMakeLists.txt`
- Modify: `src/qml_components/OnlineScores.h:4-60`
- Modify: `src/qml_components/OnlineScores.cpp:1-519`

**Interfaces:**

- Consumes: `support::PendingReplySource<T>` and `PendingReply::cancel()`.
- Produces: `OnlineScores::getScoreByGuid()` and
  `getRankingEntryAtTimestamp()` returning `support::PendingReply*`, with the
  cancellation handler following the currently active network stage.

- [ ] **Step 1: Add a fake-network cancellation test**

Add `qml_components/OnlineScores.test.cpp` to `RhythmGame_test`, then create:

```cpp
#include "qml_components/OnlineScores.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>

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
        aborted = true;
        setError(OperationCanceledError, QStringLiteral("cancelled"));
        setFinished(true);
        emit finished();
    }

    bool aborted{};

  protected:
    qint64 readData(char*, qint64) override { return -1; }
};

class FakeNetworkAccessManager final : public QNetworkAccessManager
{
  public:
    QPointer<FakeNetworkReply> lastReply;

  protected:
    QNetworkReply* createRequest(Operation,
                                 const QNetworkRequest& request,
                                 QIODevice*) override
    {
        lastReply = new FakeNetworkReply(request, this);
        return lastReply;
    }
};

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

TEST_CASE("OnlineScores Tachi reply cancellation aborts chart resolution",
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
```

- [ ] **Step 2: Build and verify the old API fails the test**

Run:

```powershell
cmake --build --preset dev --target RhythmGame_test
```

Expected: compilation fails because the methods still return
`QIfPendingReply<T>` without `cancel()`.

- [ ] **Step 3: Change OnlineScores to return PendingReply pointers**

Replace the Interface Framework include with:

```cpp
#include "support/PendingReply.h"
```

Add explicit implementation includes for the public APIs used by cancellation:

```cpp
#include <QPointer>
#include <QQmlEngine>
```

Use these declarations:

```cpp
Q_INVOKABLE support::PendingReply* getScoreByGuid(
  const QString& webApiUrl,
  const QString& guid);

Q_INVOKABLE support::PendingReply* getRankingEntryAtTimestamp(
  QString webApiUrl,
  qint64 userId,
  QString md5,
  qint64 timestamp = QDateTime::currentSecsSinceEpoch(),
  OnlineRankingModel::Provider provider =
    OnlineRankingModel::Provider::RhythmGame);
```

Remove `const` from both definitions so `this` can own the pending handle, and
remove `mutable` from `threadPool` because the asynchronous methods are no
longer const.

- [ ] **Step 4: Migrate getScoreByGuid with request and parser cancellation**

Create `support::PendingReplySource<gameplay_logic::BmsScore*> source(this)` at
the start and return `source.reply()`. Rename the raw network pointer to
`networkReply`. Immediately after starting the GET, install:

```cpp
source.setCancellationHandler(
  [reply = QPointer<QNetworkReply>(networkReply)] {
      if (reply)
          reply->abort();
  });
```

Capture `source` instead of `outer` in every continuation. In the network
finished handler:

```cpp
if (networkReply->error() == QNetworkReply::OperationCanceledError &&
    source.stopToken().stop_requested()) {
    networkReply->deleteLater();
    return;
}
if (networkReply->error() != QNetworkReply::NoError) {
    spdlog::error("getScoreByGuid failed: {} - {}",
                  magic_enum::enum_name(networkReply->error()),
                  networkReply->errorString().toStdString());
    (void)source.fail();
    networkReply->deleteLater();
    return;
}
auto data = networkReply->readAll();
networkReply->deleteLater();
source.setCancellationHandler({});
```

The parser lambda checks `source.stopToken().stop_requested()` before parsing
and before allocating `BmsScore`. Once a score has been allocated, always post
it to the application thread so a cancellation race can delete it. Post
completion with:

```cpp
QMetaObject::invokeMethod(
  QCoreApplication::instance(),
  [source, score]() mutable {
      QQmlEngine::setObjectOwnership(score,
                                     QQmlEngine::JavaScriptOwnership);
      if (!source.succeed(score))
          score->deleteLater();
  },
  Qt::QueuedConnection);
```

Copy an exception's `what()` string before leaving the catch block and post
parse failures as:

```cpp
QMetaObject::invokeMethod(
  QCoreApplication::instance(),
  [source, error = std::move(error)] {
      if (source.fail())
          spdlog::error("Error parsing getScoreByGuid response: {}", error);
  },
  Qt::QueuedConnection);
```

When `doc.isObject()` is false, post failure and return immediately so parsing
cannot continue with an invalid document. Logging is conditional on `fail()`
winning, so a parse exception racing with cancellation does not log a canceled
operation as an error.

- [ ] **Step 5: Migrate the RhythmGame ranking request**

Create one `support::PendingReplySource<QVariant> source(this)` before input
validation. Fail invalid input immediately and return `source.reply()`.

For the RhythmGame provider, install a `QPointer<QNetworkReply>` abort handler
after the GET. Capture `source` in the finished lambda and use:

```cpp
if (networkReply->error() == QNetworkReply::OperationCanceledError &&
    source.stopToken().stop_requested()) {
    return;
}
if (networkReply->error() == QNetworkReply::ContentNotFoundError ||
    networkReply->error() == QNetworkReply::OperationCanceledError) {
    (void)source.succeed(QVariant{});
    return;
}
```

Replace each remaining `pendingReply.setFailed()` with `(void)source.fail()`
and each `pendingReply.setSuccess(value)` with `(void)source.succeed(value)`.
Return `source.reply()` after the switch.

- [ ] **Step 6: Move Tachi cancellation across both request stages**

Immediately after `resolveTachiChartId()`, install:

```cpp
source.setCancellationHandler(
  [handle = QPointer<TachiResolveHandle>(handle)] {
      if (!handle)
          return;
      emit handle->cancel();
      handle->deleteLater();
  });
```

At the start of the `resolved` lambda, return after deleting the handle if
`source.stopToken().stop_requested()`. Before starting the score request, clear
the resolution handler, then install a `QPointer<QNetworkReply>` abort handler
for `scoresReply`.

In the score-request finished lambda, treat a cancellation caused by
`source.stopToken()` as an ignored late completion, keep
`ContentNotFoundError` as a successful empty result, and replace all other
terminal calls with `source.succeed()`/`source.fail()`.

Capture `source` in the Tachi `failed` lambda and keep "chart not on Tachi" as
`source.succeed(QVariant{})`. The unsupported-provider branch calls
`source.fail()`.

- [ ] **Step 7: Run focused network cancellation tests**

Run:

```powershell
cmake --build --preset dev --target RhythmGame_test
& 'build/dev/bin/RhythmGame_test.exe' '[OnlineScores][PendingReply]'
```

Expected: both fake-network cancellation tests pass, with each active fake
reply reporting `aborted == true`.

- [ ] **Step 8: Commit the OnlineScores migration**

```powershell
git add src/qml_components/OnlineScores.h `
  src/qml_components/OnlineScores.cpp test/CMakeLists.txt `
  test/qml_components/OnlineScores.test.cpp
git commit -m "refactor: cancel online scores by reply"
```

---

### Task 4: Remove Qt Interface Framework from build and packaging

**Files:**

- Modify: `CMakeLists.txt:68,400`
- Modify: `src/qml_components/OnlineRankingModel.h:1-8`
- Modify: `vcpkg.json:68-70`
- Modify: `flake.nix:24-52`
- Modify: `nix/shells/default.nix:1-72`
- Modify: `nix/packages/rhythmgame.nix:1-76`
- Delete: `vcpkgOverlayPortsWindows/qtinterfaceframework/port.data.cmake`
- Delete: `vcpkgOverlayPortsWindows/qtinterfaceframework/portfile.cmake`
- Delete: `vcpkgOverlayPortsWindows/qtinterfaceframework/requirements_minimal.txt`
- Delete: `vcpkgOverlayPortsWindows/qtinterfaceframework/vcpkg.json`
- Delete: `nix/packages/qtinterfaceframework.nix`
- Delete: `nix/packages/python-qface.nix`

**Interfaces:**

- Consumes: migrated sources with no `QIfPendingReply`.
- Produces: CMake, vcpkg, and Nix graphs with no Interface Framework or
  Interface-Framework-only qface package.

- [ ] **Step 1: Remove the CMake component and link target**

Change the Qt component search to:

```cmake
find_package(Qt6 COMPONENTS ShaderTools Quick Concurrent Multimedia Svg LinguistTools QuickControls2 WebSockets REQUIRED)
```

Remove `Qt6::InterfaceFramework` from `target_link_libraries(RhythmGame_lib ...)`
without reordering the remaining libraries.

- [ ] **Step 2: Remove the stale source include**

Delete the unused `#include <QIfPendingReply>` from
`src/qml_components/OnlineRankingModel.h`. Do not otherwise alter the model.

- [ ] **Step 3: Remove the vcpkg dependency and overlay**

Delete the `"qtinterfaceframework",` entry from `vcpkg.json`. Delete all four
tracked files under `vcpkgOverlayPortsWindows/qtinterfaceframework/`, leaving
the surrounding Windows overlay ports untouched.

Validate the manifest:

```powershell
Get-Content -Raw vcpkg.json | ConvertFrom-Json | Out-Null
```

Expected: exit code 0 and no JSON parsing error.

- [ ] **Step 4: Remove the Nix package plumbing**

In `flake.nix`, delete:

```nix
qface = pkgs.callPackage ./nix/packages/python-qface.nix {};

qtinterfaceframework = pkgs.kdePackages.callPackage ./nix/packages/qtinterfaceframework.nix {
  inherit qface;
  inherit stdenv;
};
```

Change the package and shell calls to pass only `ned14-llfio`:

```nix
rhythmgame = pkgs.kdePackages.callPackage ./nix/packages/rhythmgame.nix {
  inherit ned14-llfio;
  lexy = nur-foolnotion.foonathan-lexy;
  inherit stdenv;
};
inherit ned14-llfio;

devShells.default = pkgs.kdePackages.callPackage ./nix/shells/default.nix {
  inherit ned14-llfio;
  lexy = nur-foolnotion.foonathan-lexy;
  inherit (pkgs.kdePackages) qtdeclarative qtwebsockets qtsvg qtshadertools qtwayland qtmultimedia qttools qtkeychain;
  mkShell = pkgs.mkShell.override {inherit stdenv;};
};
```

Remove `qtinterfaceframework` from the argument lists and `buildInputs` in
`nix/shells/default.nix` and `nix/packages/rhythmgame.nix`. Delete both
`nix/packages/qtinterfaceframework.nix` and the now-unreferenced
`nix/packages/python-qface.nix`.

- [ ] **Step 5: Prove that no live dependency reference remains**

Run:

```powershell
git grep -n -i -e 'QIfPendingReply' -e 'qtinterfaceframework' `
  -e 'Qt6::InterfaceFramework' -e 'InterfaceFramework' `
  -- ':!docs/superpowers/**'
git grep -n -i -e 'qface' -- flake.nix nix
git ls-files 'vcpkgOverlayPortsWindows/qtinterfaceframework/**' `
  'nix/packages/qtinterfaceframework.nix' `
  'nix/packages/python-qface.nix'
```

Expected: all commands produce no output. Historical documents under
`docs/superpowers` are intentionally excluded.

- [ ] **Step 6: Reconfigure and build without the package requirement**

Run:

```powershell
cmake --preset dev
cmake --build --preset dev --target RhythmGame_test RhythmGame_exe
```

Expected: CMake does not search for `Qt6InterfaceFramework`, and both targets
build successfully.

- [ ] **Step 7: Evaluate the Nix flake when Nix is available**

Run:

```powershell
if (Get-Command nix -ErrorAction SilentlyContinue) {
    nix flake check --no-build
} else {
    Write-Host 'SKIP: nix is unavailable on this Windows host'
}
```

Expected: `nix flake check --no-build` exits 0, or the explicit skip is
reported for later Linux CI verification.

- [ ] **Step 8: Commit dependency removal**

```powershell
git add CMakeLists.txt src/qml_components/OnlineRankingModel.h `
  vcpkg.json flake.nix `
  nix/shells/default.nix nix/packages/rhythmgame.nix
git add -u -- vcpkgOverlayPortsWindows/qtinterfaceframework `
  nix/packages/qtinterfaceframework.nix nix/packages/python-qface.nix
git commit -m "build: remove Qt Interface Framework"
```

---

### Task 5: Full verification and final cleanup

**Files:**

- Modify only if verification reveals a defect in files already listed above.

**Interfaces:**

- Consumes: all prior tasks.
- Produces: fresh build/test/dependency evidence and a narrow final diff.

- [ ] **Step 1: Inspect the complete scoped diff**

Run:

```powershell
git status --short
git diff d7649e510 --check
git diff d7649e510 --stat
git diff d7649e510 -- `
  CMakeLists.txt vcpkg.json flake.nix nix `
  vcpkgOverlayPortsWindows/qtinterfaceframework `
  src/support src/qml_components/ScoreDb.h src/qml_components/ScoreDb.cpp `
  src/qml_components/OnlineScores.h src/qml_components/OnlineScores.cpp `
  src/qml_components/OnlineRankingModel.h `
  RhythmGameQml/QmlForeignTypes.h `
  RhythmGameQml/Lr2/Lr2SelectContext.qml `
  share/RhythmGame/themes/Default/scripts/select/List.qml `
  test/CMakeLists.txt test/support test/qml_components
```

Expected: no whitespace errors; only the intended replacement, migrations,
tests, and dependency removals appear. Ignore pre-existing unrelated worktree
changes.

- [ ] **Step 2: Run all focused regression tests**

Run:

```powershell
& 'build/dev/bin/RhythmGame_test.exe' '[PendingReply]'
& 'build/dev/bin/RhythmGame_test.exe' '[ScoreDb][PendingReply]'
& 'build/dev/bin/RhythmGame_test.exe' '[OnlineScores][PendingReply]'
```

Expected: all focused test runs report zero failures.

- [ ] **Step 3: Run the full test suite**

Run:

```powershell
ctest --preset dev
```

Expected: all discovered tests pass with zero failures.

- [ ] **Step 4: Rebuild the application-facing targets**

Before building, confirm there is no other active compiler or vcpkg process:

```powershell
Get-Process | Where-Object {
    $_.ProcessName -match 'cmake|ninja|msbuild|cl|link|vcpkg'
} | Select-Object ProcessName,Id,StartTime
```

Then run:

```powershell
cmake --build --preset dev --target RhythmGame_exe `
  RhythmGame_qmlplugin RhythmGame_lr2_qmlplugin
```

Expected: all targets build successfully.

- [ ] **Step 5: Repeat the dependency audit**

Run:

```powershell
git grep -n -i -e 'QIfPendingReply' -e 'qtinterfaceframework' `
  -e 'Qt6::InterfaceFramework' -e 'InterfaceFramework' `
  -- ':!docs/superpowers/**'
git grep -n -i -e 'qface' -- flake.nix nix
git ls-files 'vcpkgOverlayPortsWindows/qtinterfaceframework/**' `
  'nix/packages/qtinterfaceframework.nix' `
  'nix/packages/python-qface.nix'
```

Expected: no output.

- [ ] **Step 6: Review requirements against the approved spec**

Confirm from fresh evidence:

- every asynchronous `ScoreDb`/`OnlineScores` method returns `PendingReply*`;
- QML cancellation calls target retained replies, never `ScoreDb`;
- cancellation invokes failure exactly once and rejects later completion;
- network cancellation aborts the current request;
- database workers check their reply's stop token;
- no public `cancelled` state was added;
- no Qt private API or Interface Framework reference remains;
- all focused tests, the full suite, and application-facing targets passed.

If a check fails, fix only the responsible task's files and repeat that task's
focused verification plus Steps 1-5 above before reporting completion.
