#include "arena/ArenaOpponentTarget.h"
#include "arena/ArenaResultModel.h"
#include "arena/ArenaStandingsModel.h"
#include "arena/ArenaTypes.h"

#include <catch2/catch_test_macros.hpp>

#include <QHash>
#include <QMetaType>
#include <QSet>

#include <utility>

namespace {

auto
identity(QString id, QString name) -> arena::PublicIdentity
{
    return { .userId = std::move(id),
             .displayName = std::move(name),
             .avatarUrl = QStringLiteral("https://example.test/avatar.png") };
}

auto
judgements(qint64 exScore, qint64 badPoorCount = 0) -> arena::ArenaJudgements
{
    return { .perfect = exScore / 2,
             .great = exScore % 2,
             .good = 3,
             .bad = badPoorCount,
             .poor = 0,
             .emptyPoor = 0 };
}

auto
telemetry(qint64 exScore,
          qint64 sequence = 1,
          qint64 progress = 500,
          qint64 badPoorCount = 0) -> arena::TelemetrySnapshot
{
    return { .sequence = sequence,
             .exScore = exScore,
             .progressPermille = progress,
             .maxCombo = 42,
             .badPoorCount = badPoorCount,
             .judgements = judgements(exScore, badPoorCount),
             .gauge = { .type = arena::GaugeType::Hard,
                        .valueMilli = 65'000 } };
}

auto
finalResult(qint64 exScore,
            arena::ClearType clearType = arena::ClearType::Hard,
            qint64 badPoorCount = 0) -> arena::FinalResult
{
    return { .exScore = exScore,
             .maxCombo = 64,
             .badPoorCount = badPoorCount,
             .judgements = judgements(exScore, badPoorCount),
             .clearType = clearType,
             .finalGauge = { .type = arena::GaugeType::Hard,
                             .valueMilli = 80'000 } };
}

auto
active(
  QString memberId,
  std::optional<arena::TelemetrySnapshot> value,
  arena::ActiveCompetitionState state = arena::ActiveCompetitionState::Playing,
  arena::MemberStatus connection = arena::MemberStatus::Connected)
  -> arena::LiveStandingEntry
{
    return {
        .memberId = std::move(memberId),
        .connectionStatus = connection,
        .state =
          arena::LiveActiveStanding{
            .competitionState = state,
            .rank = value ? std::optional{ 1 } : std::nullopt,
            .telemetry = std::move(value),
          },
    };
}

auto
finished(QString memberId,
         qint64 exScore,
         int rank = 1,
         arena::MemberStatus connection = arena::MemberStatus::Connected)
  -> arena::LiveStandingEntry
{
    return { .memberId = std::move(memberId),
             .connectionStatus = connection,
             .state = arena::LiveFinishedStanding{
               .rank = rank,
               .result = finalResult(exScore),
             } };
}

auto
dnf(QString memberId, arena::DnfReason reason = arena::DnfReason::Aborted)
  -> arena::LiveStandingEntry
{
    return { .memberId = std::move(memberId),
             .connectionStatus = arena::MemberStatus::Reserved,
             .state = arena::LiveDnfStanding{ .reason = reason } };
}

auto
liveSnapshot(qint64 revision, QVector<arena::LiveStandingEntry> entries)
  -> arena::LiveStandingsSnapshot
{
    return { .roomId = QStringLiteral("room-1"),
             .roomGeneration = 2,
             .roundId = QStringLiteral("round-1"),
             .launchAttemptId = QStringLiteral("attempt-1"),
             .standingsRevision = revision,
             .entries = std::move(entries) };
}

auto
selection() -> arena::SelectionSnapshot
{
    return { .sha256 = QString(64, QChar(u'a')),
             .title = QStringLiteral("Competition chart"),
             .subtitle = {},
             .artist = QStringLiteral("Artist"),
             .keyMode = 7,
             .randomSequence = { 1, 2, 3 },
             .noteOrderP1 = arena::NoteOrder::Random,
             .noteOrderP2 = arena::NoteOrder::Mirror,
             .dpMode = arena::DpMode::Off,
             .laneSeed = QStringLiteral("0123456789abcdef"),
             .randomizationVersion = 1 };
}

auto
finalSnapshot(qint64 revision = 3) -> arena::RoundResultSnapshot
{
    return {
        .resultRevision = revision,
        .roundId = QStringLiteral("round-1"),
        .selectionRevision = 4,
        .finalizedAtServerMs = 500'000,
        .participantCount = 2,
        .selection = selection(),
        .winnerMemberIds = { QStringLiteral("member-a") },
        .entries = {
          arena::FinalStandingEntry{
            .memberId = QStringLiteral("member-a"),
            .identity = identity(QStringLiteral("user-a"),
                                 QStringLiteral("Alice")),
            .lobbyWinsAfter = 5,
            .state = arena::FinalFinishedStanding{
              .rank = 1,
              .result = finalResult(200, arena::ClearType::FullCombo),
            },
          },
          arena::FinalStandingEntry{
            .memberId = QStringLiteral("member-b"),
            .identity = identity(QStringLiteral("user-b"),
                                 QStringLiteral("Bob")),
            .lobbyWinsAfter = std::nullopt,
            .state = arena::FinalDnfStanding{
              .reason = arena::DnfReason::PlayDeadline,
            },
          },
        },
    };
}

auto
identities() -> QHash<QString, arena::PublicIdentity>
{
    return {
        { QStringLiteral("member-a"),
          identity(QStringLiteral("user-a"), QStringLiteral("Alice")) },
        { QStringLiteral("member-b"),
          identity(QStringLiteral("user-b"), QStringLiteral("Bob")) },
        { QStringLiteral("member-c"),
          identity(QStringLiteral("user-c"), QStringLiteral("Carol")) },
        { QStringLiteral("member-d"),
          identity(QStringLiteral("user-d"), QStringLiteral("Dan")) },
        { QStringLiteral("self"),
          identity(QStringLiteral("user-self"), QStringLiteral("Self")) },
    };
}

} // namespace

TEST_CASE(
  "ArenaCompetitionModels: standings exposes the exact value-row role contract",
  "[arena][competition-models]")
{
    using namespace arena;
    ArenaStandingsModel model;
    CHECK(model.roleNames() ==
          QHash<int, QByteArray>{
            { ArenaStandingsModel::MemberIdRole, "memberId" },
            { ArenaStandingsModel::DisplayNameRole, "displayName" },
            { ArenaStandingsModel::AvatarUrlRole, "avatarUrl" },
            { ArenaStandingsModel::ConnectedRole, "connected" },
            { ArenaStandingsModel::CompetitionStateRole, "competitionState" },
            { ArenaStandingsModel::RankRole, "rank" },
            { ArenaStandingsModel::HasScoreRole, "hasScore" },
            { ArenaStandingsModel::ExScoreRole, "exScore" },
            { ArenaStandingsModel::ProgressPermilleRole, "progressPermille" },
            { ArenaStandingsModel::MaxComboRole, "maxCombo" },
            { ArenaStandingsModel::BadPoorCountRole, "badPoorCount" },
            { ArenaStandingsModel::PerfectRole, "perfect" },
            { ArenaStandingsModel::GreatRole, "great" },
            { ArenaStandingsModel::GoodRole, "good" },
            { ArenaStandingsModel::BadRole, "bad" },
            { ArenaStandingsModel::PoorRole, "poor" },
            { ArenaStandingsModel::EmptyPoorRole, "emptyPoor" },
            { ArenaStandingsModel::GaugeTypeRole, "gaugeType" },
            { ArenaStandingsModel::GaugeValueMilliRole, "gaugeValueMilli" },
            { ArenaStandingsModel::ClearTypeRole, "clearType" },
            { ArenaStandingsModel::LobbyWinsAfterRole, "lobbyWinsAfter" },
            { ArenaStandingsModel::DnfReasonRole, "dnfReason" },
          });
    CHECK_FALSE(model.data({}, ArenaStandingsModel::MemberIdRole).isValid());
}

TEST_CASE("ArenaCompetitionModels: standings owns and replaces complete live "
          "snapshots once",
          "[arena][competition-models]")
{
    using namespace arena;
    ArenaStandingsModel model;
    auto frozenIdentities = identities();
    auto snapshot = liveSnapshot(
      1,
      { active(QStringLiteral("member-a"), telemetry(0)),
        active(QStringLiteral("member-b"),
               std::nullopt,
               ActiveCompetitionState::Loading,
               MemberStatus::Reserved),
        finished(QStringLiteral("member-c"), 180),
        dnf(QStringLiteral("member-d"), DnfReason::PlayDeadline) });
    int resets = 0;
    int snapshotChanges = 0;
    QObject::connect(
      &model, &QAbstractItemModel::modelReset, [&] { ++resets; });
    QObject::connect(&model, &ArenaStandingsModel::snapshotChanged, [&] {
        ++snapshotChanges;
    });
    REQUIRE(model.replace(snapshot, frozenIdentities));
    frozenIdentities[QStringLiteral("member-a")].displayName =
      QStringLiteral("Mutated");

    CHECK(resets == 1);
    CHECK(snapshotChanges == 1);
    CHECK(model.roundId() == QStringLiteral("round-1"));
    CHECK(model.revision() == 1);
    CHECK(model.rowCount() == 4);
    CHECK(model.rowCount(model.index(0, 0)) == 0);
    CHECK(model.index(0, 0).internalPointer() == nullptr);
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::DisplayNameRole)
            .toString() == QStringLiteral("Alice"));
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::ConnectedRole)
            .toBool());
    CHECK(
      model.data(model.index(0, 0), ArenaStandingsModel::CompetitionStateRole)
        .toString() == QStringLiteral("playing"));
    CHECK(
      model.data(model.index(0, 0), ArenaStandingsModel::RankRole).toInt() ==
      1);
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::HasScoreRole)
            .toBool());
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::ExScoreRole)
            .toLongLong() == 0);
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::GaugeTypeRole)
            .toString() == QStringLiteral("hard"));
    CHECK(model.data(model.index(1, 0), ArenaStandingsModel::ConnectedRole)
            .toBool() == false);
    CHECK(
      model.data(model.index(1, 0), ArenaStandingsModel::RankRole).toInt() ==
      0);
    CHECK_FALSE(model.data(model.index(1, 0), ArenaStandingsModel::HasScoreRole)
                  .toBool());
    CHECK(
      model.data(model.index(2, 0), ArenaStandingsModel::CompetitionStateRole)
        .toString() == QStringLiteral("finished"));
    CHECK(
      model.data(model.index(2, 0), ArenaStandingsModel::ProgressPermilleRole)
        .toLongLong() == 1000);
    CHECK(model.data(model.index(2, 0), ArenaStandingsModel::ClearTypeRole)
            .toString() == QStringLiteral("hard"));
    CHECK(
      model.data(model.index(3, 0), ArenaStandingsModel::CompetitionStateRole)
        .toString() == QStringLiteral("dnf"));
    CHECK(model.data(model.index(3, 0), ArenaStandingsModel::DnfReasonRole)
            .toString() == QStringLiteral("play_deadline"));
    CHECK(model.data(model.index(3, 0), ArenaStandingsModel::LobbyWinsAfterRole)
            .toLongLong() == -1);

    const auto firstRow = model.index(0, 0);
    for (const auto role : { ArenaStandingsModel::MemberIdRole,
                             ArenaStandingsModel::DisplayNameRole,
                             ArenaStandingsModel::AvatarUrlRole,
                             ArenaStandingsModel::CompetitionStateRole,
                             ArenaStandingsModel::GaugeTypeRole,
                             ArenaStandingsModel::ClearTypeRole,
                             ArenaStandingsModel::DnfReasonRole }) {
        CHECK(model.data(firstRow, role).typeId() == QMetaType::QString);
    }
    for (const auto role : { ArenaStandingsModel::ConnectedRole,
                             ArenaStandingsModel::HasScoreRole }) {
        CHECK(model.data(firstRow, role).typeId() == QMetaType::Bool);
    }
    CHECK(model.data(firstRow, ArenaStandingsModel::RankRole).typeId() ==
          QMetaType::Int);
    for (const auto role : { ArenaStandingsModel::ExScoreRole,
                             ArenaStandingsModel::ProgressPermilleRole,
                             ArenaStandingsModel::MaxComboRole,
                             ArenaStandingsModel::BadPoorCountRole,
                             ArenaStandingsModel::PerfectRole,
                             ArenaStandingsModel::GreatRole,
                             ArenaStandingsModel::GoodRole,
                             ArenaStandingsModel::BadRole,
                             ArenaStandingsModel::PoorRole,
                             ArenaStandingsModel::EmptyPoorRole,
                             ArenaStandingsModel::GaugeValueMilliRole,
                             ArenaStandingsModel::LobbyWinsAfterRole }) {
        CHECK(model.data(firstRow, role).typeId() == QMetaType::LongLong);
    }
}

TEST_CASE("ArenaCompetitionModels: standings rejects stale invalid and "
          "other-round values atomically",
          "[arena][competition-models]")
{
    using namespace arena;
    ArenaStandingsModel model;
    const auto frozenIdentities = identities();
    const auto original =
      liveSnapshot(2, { active(QStringLiteral("member-a"), telemetry(100)) });
    int resets = 0;
    QObject::connect(
      &model, &QAbstractItemModel::modelReset, [&] { ++resets; });
    REQUIRE(model.replace(original, frozenIdentities));

    CHECK_FALSE(model.replace(original, frozenIdentities));
    auto otherRound = original;
    otherRound.roundId = QStringLiteral("round-2");
    otherRound.standingsRevision = 3;
    CHECK_FALSE(model.replace(otherRound, frozenIdentities));
    auto missingIdentity = original;
    missingIdentity.standingsRevision = 3;
    CHECK_FALSE(model.replace(missingIdentity, {}));
    auto duplicate = original;
    duplicate.standingsRevision = 3;
    duplicate.entries.push_back(duplicate.entries.front());
    CHECK_FALSE(model.replace(duplicate, frozenIdentities));
    CHECK(resets == 1);
    CHECK(model.revision() == 2);
    CHECK(model.rowCount() == 1);

    auto newer = original;
    newer.standingsRevision = 3;
    REQUIRE(model.replace(newer, frozenIdentities));
    CHECK(resets == 2);
    model.clear();
    CHECK(resets == 3);
    CHECK(model.roundId().isEmpty());
    CHECK(model.revision() == 0);
    CHECK(model.rowCount() == 0);
}

TEST_CASE(
  "ArenaCompetitionModels: standings converts immutable final standings",
  "[arena][competition-models]")
{
    using namespace arena;
    ArenaStandingsModel model;
    const auto result = finalSnapshot();
    REQUIRE(model.replaceFinal(result));
    CHECK(model.roundId() == result.roundId);
    CHECK(model.revision() == result.resultRevision);
    CHECK(model.rowCount() == 2);
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::DisplayNameRole)
            .toString() == QStringLiteral("Alice"));
    CHECK(
      model.data(model.index(0, 0), ArenaStandingsModel::CompetitionStateRole)
        .toString() == QStringLiteral("finished"));
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::LobbyWinsAfterRole)
            .toLongLong() == 5);
    CHECK(model.data(model.index(0, 0), ArenaStandingsModel::ClearTypeRole)
            .toString() == QStringLiteral("fc"));
    CHECK(
      model.data(model.index(1, 0), ArenaStandingsModel::RankRole).toInt() ==
      0);
    CHECK(model.data(model.index(1, 0), ArenaStandingsModel::LobbyWinsAfterRole)
            .toLongLong() == -1);
    CHECK(model.data(model.index(1, 0), ArenaStandingsModel::DnfReasonRole)
            .toString() == QStringLiteral("play_deadline"));

    CHECK_FALSE(model.replaceFinal(result));
    auto staleOther = result;
    staleOther.roundId = QStringLiteral("round-stale");
    staleOther.resultRevision = 2;
    CHECK_FALSE(model.replaceFinal(staleOther));

    auto newerOther = result;
    newerOther.roundId = QStringLiteral("round-newer");
    newerOther.resultRevision = 4;
    REQUIRE(model.replaceFinal(newerOther));
    CHECK(model.roundId() == QStringLiteral("round-newer"));
    CHECK(model.revision() == 4);
}

TEST_CASE("ArenaCompetitionModels: standings gives a same-round final "
          "precedence over live revisions",
          "[arena][competition-models]")
{
    using namespace arena;
    ArenaStandingsModel model;
    const auto frozenIdentities = identities();
    REQUIRE(model.replace(
      liveSnapshot(20,
                   { active(QStringLiteral("member-a"), telemetry(100)),
                     active(QStringLiteral("member-b"), telemetry(90)) }),
      frozenIdentities));

    auto result = finalSnapshot(1);
    REQUIRE(model.replaceFinal(result));
    CHECK(model.revision() == 1);
    CHECK(
      model.data(model.index(0, 0), ArenaStandingsModel::CompetitionStateRole)
        .toString() == QStringLiteral("finished"));

    auto lateLive =
      liveSnapshot(21,
                   { active(QStringLiteral("member-a"), telemetry(220)),
                     active(QStringLiteral("member-b"), telemetry(210)) });
    CHECK_FALSE(model.replace(lateLive, frozenIdentities));
    CHECK(model.revision() == 1);
}

TEST_CASE("ArenaCompetitionModels: result transitions from pending to final "
          "without leaking options",
          "[arena][competition-models]")
{
    using namespace arena;
    ArenaResultModel model;
    int changes = 0;
    QObject::connect(&model, &ArenaResultModel::changed, [&] { ++changes; });
    const auto summary = QStringLiteral("P1 Random | P2 Mirror | DP Off");
    REQUIRE(model.setPending(QStringLiteral("round-1"),
                             2,
                             QStringLiteral("Competition chart"),
                             summary,
                             false));
    CHECK(model.valid());
    CHECK_FALSE(model.finalized());
    CHECK(model.resultRevision() == 0);
    CHECK(model.participantCount() == 2);
    CHECK(model.localRank() == 0);
    CHECK_FALSE(model.localDnf());
    CHECK_FALSE(model.localWinner());
    CHECK(model.selectionOptionsSummary() == summary);
    CHECK_FALSE(model.selectionOptionsSummary().contains(
      QStringLiteral("0123456789abcdef")));
    CHECK(model.standings()->rowCount() == 0);

    auto wrongPendingRound = finalSnapshot(4);
    wrongPendingRound.roundId = QStringLiteral("round-other");
    CHECK_FALSE(model.replaceFinal(
      wrongPendingRound, QStringLiteral("member-a"), summary));

    REQUIRE(
      model.replaceFinal(finalSnapshot(), QStringLiteral("member-a"), summary));
    CHECK(model.finalized());
    CHECK(model.resultRevision() == 3);
    CHECK(model.winnerMemberIds() == QStringList{ QStringLiteral("member-a") });
    CHECK(model.winnerNames() == QStringList{ QStringLiteral("Alice") });
    CHECK(model.localRank() == 1);
    CHECK_FALSE(model.localDnf());
    CHECK(model.localWinner());
    CHECK(model.standings()->rowCount() == 2);
    CHECK(changes == 2);

    auto stale = finalSnapshot(2);
    CHECK_FALSE(model.replaceFinal(stale, QStringLiteral("member-a"), summary));
    auto newer = finalSnapshot(4);
    newer.roundId = QStringLiteral("round-newer");
    REQUIRE(model.replaceFinal(newer, QStringLiteral("member-a"), summary));
    CHECK(model.roundId() == QStringLiteral("round-newer"));
    CHECK(changes == 3);

    model.clear();
    CHECK_FALSE(model.valid());
    CHECK_FALSE(model.finalized());
    CHECK(model.roundId().isEmpty());
    CHECK(model.standings()->rowCount() == 0);
    CHECK(changes == 4);
}

TEST_CASE(
  "ArenaCompetitionModels: result exposes pending and finalized local DNF",
  "[arena][competition-models]")
{
    using namespace arena;
    ArenaResultModel model;
    REQUIRE(model.setPending(QStringLiteral("round-1"),
                             2,
                             QStringLiteral("Competition chart"),
                             QStringLiteral("options"),
                             true));
    CHECK(model.localDnf());
    REQUIRE(model.replaceFinal(
      finalSnapshot(), QStringLiteral("member-b"), QStringLiteral("options")));
    CHECK(model.finalized());
    CHECK(model.localRank() == 0);
    CHECK(model.localDnf());
    CHECK_FALSE(model.localWinner());
}

TEST_CASE(
  "ArenaCompetitionModels: target derives a stable strongest other score",
  "[arena][competition-models]")
{
    using namespace arena;
    ArenaOpponentTarget target;
    const auto frozenIdentities = identities();
    int changes = 0;
    QObject::connect(
      &target, &ArenaOpponentTarget::changed, [&] { ++changes; });

    auto first =
      liveSnapshot(1,
                   { active(QStringLiteral("self"), telemetry(300)),
                     active(QStringLiteral("member-a"), telemetry(200)),
                     active(QStringLiteral("member-b"), telemetry(210)),
                     active(QStringLiteral("member-c"), std::nullopt),
                     dnf(QStringLiteral("member-d")) });
    target.update(first, u"self", frozenIdentities);
    CHECK(target.available());
    CHECK(target.memberId() == QStringLiteral("member-b"));
    CHECK(target.displayName() == QStringLiteral("Bob"));
    CHECK(target.exScore() == 210);
    CHECK_FALSE(target.finished());

    auto tied = first;
    tied.standingsRevision = 2;
    std::get<LiveActiveStanding>(tied.entries[1].state).telemetry =
      telemetry(210);
    target.update(tied, u"self", frozenIdentities);
    CHECK(target.memberId() == QStringLiteral("member-b"));

    auto overtaken = tied;
    overtaken.standingsRevision = 3;
    std::get<LiveActiveStanding>(overtaken.entries[1].state).telemetry =
      telemetry(220);
    target.update(overtaken, u"self", frozenIdentities);
    CHECK(target.memberId() == QStringLiteral("member-a"));
    CHECK(target.exScore() == 220);

    auto final = overtaken;
    final.standingsRevision = 4;
    final.entries[1] = finished(QStringLiteral("member-a"), 230);
    target.update(final, u"self", frozenIdentities);
    CHECK(target.memberId() == QStringLiteral("member-a"));
    CHECK(target.exScore() == 230);
    CHECK(target.finished());

    target.update(first, u"self", frozenIdentities);
    CHECK(target.exScore() == 230);
    CHECK(changes == 3);
}

TEST_CASE("ArenaCompetitionModels: target handles zero no-data DNF new rounds "
          "and clear",
          "[arena][competition-models]")
{
    using namespace arena;
    ArenaOpponentTarget target;
    const auto frozenIdentities = identities();

    auto noScores =
      liveSnapshot(1,
                   { active(QStringLiteral("self"), telemetry(10)),
                     active(QStringLiteral("member-a"), std::nullopt),
                     dnf(QStringLiteral("member-b")) });
    target.update(noScores, u"self", frozenIdentities);
    CHECK_FALSE(target.available());

    auto zero = noScores;
    zero.standingsRevision = 2;
    zero.entries[1] = active(QStringLiteral("member-a"), telemetry(0));
    target.update(zero, u"self", frozenIdentities);
    CHECK(target.available());
    CHECK(target.memberId() == QStringLiteral("member-a"));
    CHECK(target.exScore() == 0);

    auto lostScore = zero;
    lostScore.standingsRevision = 3;
    lostScore.entries[1] = active(QStringLiteral("member-a"), std::nullopt);
    target.update(lostScore, u"self", frozenIdentities);
    CHECK_FALSE(target.available());

    auto restored = lostScore;
    restored.standingsRevision = 4;
    restored.entries[1] = active(QStringLiteral("member-a"), telemetry(5));
    target.update(restored, u"self", frozenIdentities);
    REQUIRE(target.available());

    auto becameDnf = restored;
    becameDnf.standingsRevision = 5;
    becameDnf.entries[1] = dnf(QStringLiteral("member-a"));
    target.update(becameDnf, u"self", frozenIdentities);
    CHECK_FALSE(target.available());

    auto newRound =
      liveSnapshot(1,
                   { active(QStringLiteral("self"), telemetry(10)),
                     active(QStringLiteral("member-b"), telemetry(0)),
                     active(QStringLiteral("member-a"), telemetry(0)) });
    newRound.roundId = QStringLiteral("round-2");
    target.update(newRound, u"self", frozenIdentities);
    CHECK(target.memberId() == QStringLiteral("member-b"));

    target.clear();
    CHECK_FALSE(target.available());
    CHECK(target.memberId().isEmpty());
    CHECK(target.displayName().isEmpty());
    CHECK(target.exScore() == 0);
    CHECK_FALSE(target.finished());
}
