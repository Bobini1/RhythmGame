#include "Lr2ResolvedText.h"
#include "Lr2ResolvedTextRegistry.h"

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QJSEngine>

#include <memory>

namespace {

void
ensureCoreApplication()
{
    static int argc = 1;
    static char appName[] = "RhythmGame_test";
    static char* argv[] = { appName, nullptr };
    static std::unique_ptr<QCoreApplication> app;
    if (!QCoreApplication::instance()) {
        app = std::make_unique<QCoreApplication>(argc, argv);
    }
}

} // namespace

TEST_CASE("LR2 resolved text registry refreshes queued active ids",
          "[lr2][text]")
{
    ensureCoreApplication();

    Lr2ResolvedTextRegistry registry;
    Lr2ResolvedText first;
    Lr2ResolvedText second;
    first.setRegistry(&registry);
    second.setRegistry(&registry);
    first.setSourceTextId(10);
    second.setSourceTextId(20);

    QJSEngine engine;
    QJSValue resolver = engine.evaluate(
      QStringLiteral("(function(id) { return 'text-' + id; })"));

    registry.queueTextRefreshIds({ 10, 20, 999 });

    REQUIRE(registry.refreshQueuedTexts(resolver));
    REQUIRE(first.text() == QStringLiteral("text-10"));
    REQUIRE(second.text() == QStringLiteral("text-20"));
}

TEST_CASE("LR2 resolved text registry full refresh uses active text ids",
          "[lr2][text]")
{
    ensureCoreApplication();

    Lr2ResolvedTextRegistry registry;
    Lr2ResolvedText active;
    active.setRegistry(&registry);
    active.setSourceTextId(30);

    QJSEngine engine;
    QJSValue resolver =
      engine.evaluate(QStringLiteral("(function(id) { return 'all-' + id; })"));

    registry.queueAllTextRefresh();

    REQUIRE(registry.refreshQueuedTexts(resolver));
    REQUIRE(active.text() == QStringLiteral("all-30"));
    REQUIRE_FALSE(registry.refreshQueuedTexts(resolver));
}
