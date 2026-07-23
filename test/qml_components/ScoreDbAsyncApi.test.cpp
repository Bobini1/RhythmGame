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
