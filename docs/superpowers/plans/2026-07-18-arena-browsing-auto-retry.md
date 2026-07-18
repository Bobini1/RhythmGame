# Arena Browsing Automatic Retry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make anonymous Arena lobby browsing recover indefinitely from stalled or failed network attempts without requiring a manual Retry action.

**Architecture:** `ArenaSession` owns a browsing-specific attempt timeout, retry task, and exponential backoff. A browsing attempt remains incomplete until the first directory snapshot; transport failures and ten-second readiness timeouts schedule another anonymous connection, while protocol failures retain their existing terminal state.

**Tech Stack:** C++20, Qt 6 signals and `QWebSocket` transport abstraction, `ArenaScheduler`, Catch2 with fake scheduler and transport.

## Global Constraints

- Each anonymous attempt has 10 seconds to receive its first directory snapshot.
- Retry delays are 500, 1000, 2000, 4000, and 8000 milliseconds, capped at 8000 milliseconds indefinitely.
- A directory snapshot resets the next retry delay to 500 milliseconds.
- Leaving Arena cancels the browsing timeout and retry task.
- Authenticated admission, in-room reconnect, and terminal protocol-error behavior remain unchanged.
- Add no QML/UI tests and no tests that parse source files as text.

---

### Task 1: Add the anonymous browsing readiness loop

**Files:**
- Modify: `src/arena/ArenaSession.h`
- Modify: `src/arena/ArenaSession.cpp`
- Test: `test/arena/ArenaSession.test.cpp`

**Interfaces:**
- Consumes: `ArenaScheduler::scheduleOnce`, `ArenaScheduler::cancel`, `ArenaTransport::Generation`, and the existing anonymous `HandshakeKind`.
- Produces: private `startAnonymousBrowseAttempt()`, `scheduleAnonymousBrowseRetry()`, and `cancelAnonymousBrowseTasks()` session helpers plus browsing-specific scheduler state.

- [ ] **Step 1: Write focused failing state-machine tests**

Add Catch2 sections beside the existing anonymous browsing test. Drive only `Fixture`, `FakeArenaScheduler`, and `FakeArenaTransport`:

```cpp
TEST_CASE("ArenaSession automatically retries stalled anonymous browsing with bounded backoff",
          "[arena][session]")
{
    ensureCoreApplication();
    Fixture fixture;
    fixture.session.connectForBrowsing();

    const std::array<qint64, 6> delays{ 500, 1'000, 2'000, 4'000, 8'000, 8'000 };
    for (const auto delay : delays) {
        const auto connectCount = fixture.transport.connectCalls.size();
        fixture.scheduler.advanceBy(9'999);
        CHECK(fixture.transport.connectCalls.size() == connectCount);
        fixture.scheduler.advanceBy(1);
        CHECK(fixture.transport.connectCalls.size() == connectCount);
        fixture.scheduler.advanceBy(delay - 1);
        CHECK(fixture.transport.connectCalls.size() == connectCount);
        fixture.scheduler.advanceBy(1);
        REQUIRE(fixture.transport.connectCalls.size() == connectCount + 1);
        CHECK(fixture.session.getState() == arena::ArenaSession::State::Disconnected);
        CHECK(fixture.session.getErrorCode().isEmpty());
    }
}
```

Add a second test that completes hello plus directory, disconnects, reconnects after 500 ms, completes a second directory, disconnects again, and proves the delay is reset to 500 ms. End it with `exitArena()`, advance beyond all deadlines, and prove no further `connectTo` calls occur. Add a protocol-failure section using malformed server text and prove it remains `State::Error` with no scheduled retry.

- [ ] **Step 2: Run the focused test and verify RED**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target RhythmGame_test --config RelWithDebInfo --parallel 1
build/dev-relwithdebinfo/test/bin/RhythmGame_test.exe "ArenaSession automatically retries*"
```

Expected: the new test fails because the first stalled anonymous connection never schedules another `connectTo` call.

- [ ] **Step 3: Add browsing-specific scheduler state and helpers**

In `ArenaSession.h`, add independent state so in-room reconnect timing is not overloaded:

```cpp
ArenaScheduler::TaskId m_browseAttemptTimeoutTask{};
ArenaScheduler::TaskId m_browseRetryTask{};
qint64 m_nextBrowseBackoffMs{ InitialBackoffMs };

void startAnonymousBrowseAttempt();
void scheduleAnonymousBrowseRetry();
void cancelAnonymousBrowseTasks();
```

`startAnonymousBrowseAttempt()` must cancel stale browsing tasks, call `startTransport(HandshakeKind::AnonymousBrowse)`, capture the lifecycle and transport generation, and schedule a ten-second callback. The callback retries only when the session is still active, the lifecycle and transport generation still match, no directory is ready, the session has no room, and the connection is anonymous.

`scheduleAnonymousBrowseRetry()` must invalidate the current transport, clear recoverable transport errors, keep `State::Disconnected`, schedule the current delay, and double the next delay up to `MaximumBackoffMs`. Its callback calls `startAnonymousBrowseAttempt()` only if the same active lifecycle is still browsing anonymously.

`cancelAnonymousBrowseTasks()` cancels both browsing task IDs. Call it before authenticated admission, from async/session cleanup, and from terminal protocol failure paths.

- [ ] **Step 4: Complete and reset browsing attempts at the correct boundaries**

Change `openAnonymousBrowsing()` to reset the browsing backoff and call `startAnonymousBrowseAttempt()`.

For an anonymous connection with no pending admission, route `handleDisconnected()` and `handleTransportError()` to `scheduleAnonymousBrowseRetry()` instead of `State::Error`. Preserve the existing reconnect, admission, and in-room branches.

In `handleDirectorySnapshot()`, after accepting the snapshot:

```cpp
cancelAnonymousBrowseTasks();
m_nextBrowseBackoffMs = InitialBackoffMs;
setDirectoryReady(true);
```

Do not cancel the browsing readiness timeout merely on `server_hello`; the first directory snapshot is the completion boundary.

- [ ] **Step 5: Run the focused tests and Arena session slice**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target RhythmGame_test --config RelWithDebInfo --parallel 1
build/dev-relwithdebinfo/test/bin/RhythmGame_test.exe "[arena][session]"
```

Expected: all `[arena][session]` tests pass, including bounded automatic retries, reset-on-directory, exit cancellation, and terminal protocol errors.

- [ ] **Step 6: Build the application and check the diff**

Run:

```powershell
cmake --build build/dev-relwithdebinfo --target RhythmGame_exe --config RelWithDebInfo --parallel 1
git diff --check -- src/arena/ArenaSession.h src/arena/ArenaSession.cpp test/arena/ArenaSession.test.cpp
```

Expected: build exits 0 and `git diff --check` reports no errors.

- [ ] **Step 7: Commit the implementation**

```powershell
git add -- src/arena/ArenaSession.h src/arena/ArenaSession.cpp test/arena/ArenaSession.test.cpp
git commit -m "fix: automatically retry Arena browsing"
```
