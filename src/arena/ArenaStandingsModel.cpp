#include "ArenaStandingsModel.h"

#include "ArenaTypes.h"

#include <QSet>

#include <utility>

namespace arena {
namespace {

auto
gaugeTypeName(GaugeType type) -> QString
{
    switch (type) {
        case GaugeType::Fc:
            return QStringLiteral("fc");
        case GaugeType::ExHard:
            return QStringLiteral("exhard");
        case GaugeType::Hard:
            return QStringLiteral("hard");
        case GaugeType::Normal:
            return QStringLiteral("normal");
        case GaugeType::Easy:
            return QStringLiteral("easy");
        case GaugeType::AssistEasy:
            return QStringLiteral("aeasy");
    }
    return {};
}

auto
clearTypeName(ClearType type) -> QString
{
    switch (type) {
        case ClearType::Max:
            return QStringLiteral("max");
        case ClearType::Perfect:
            return QStringLiteral("perfect");
        case ClearType::FullCombo:
            return QStringLiteral("fc");
        case ClearType::ExHard:
            return QStringLiteral("exhard");
        case ClearType::Hard:
            return QStringLiteral("hard");
        case ClearType::Normal:
            return QStringLiteral("normal");
        case ClearType::Easy:
            return QStringLiteral("easy");
        case ClearType::AssistEasy:
            return QStringLiteral("aeasy");
        case ClearType::Failed:
            return QStringLiteral("failed");
    }
    return {};
}

auto
dnfReasonName(DnfReason reason) -> QString
{
    switch (reason) {
        case DnfReason::Aborted:
            return QStringLiteral("aborted");
        case DnfReason::ResultUnavailable:
            return QStringLiteral("result_unavailable");
        case DnfReason::Left:
            return QStringLiteral("left");
        case DnfReason::Kicked:
            return QStringLiteral("kicked");
        case DnfReason::GraceExpired:
            return QStringLiteral("grace_expired");
        case DnfReason::PlayDeadline:
            return QStringLiteral("play_deadline");
    }
    return {};
}

auto
activeStateName(ActiveCompetitionState state) -> QString
{
    switch (state) {
        case ActiveCompetitionState::Loading:
            return QStringLiteral("loading");
        case ActiveCompetitionState::Playing:
            return QStringLiteral("playing");
    }
    return {};
}

auto
validCounter(qint64 value) -> bool
{
    return value >= 0 && value <= MaxScoreCounter;
}

auto
validJudgements(const ArenaJudgements& value) -> bool
{
    return validCounter(value.perfect) && validCounter(value.great) &&
           validCounter(value.good) && validCounter(value.bad) &&
           validCounter(value.poor) && validCounter(value.emptyPoor);
}

auto
validGauge(const GaugeSnapshot& value) -> bool
{
    return !gaugeTypeName(value.type).isEmpty() && value.valueMilli >= 0 &&
           value.valueMilli <= 100'000;
}

auto
validScore(qint64 exScore,
           qint64 maxCombo,
           qint64 badPoorCount,
           const ArenaJudgements& judgements) -> bool
{
    return validCounter(exScore) && validCounter(maxCombo) &&
           validCounter(badPoorCount) && validJudgements(judgements) &&
           exScore == 2 * judgements.perfect + judgements.great &&
           badPoorCount ==
             judgements.bad + judgements.poor + judgements.emptyPoor;
}

auto
validTelemetry(const TelemetrySnapshot& value) -> bool
{
    return value.sequence >= 1 && value.sequence <= MaxUInt32 &&
           value.progressPermille >= 0 && value.progressPermille <= 1'000 &&
           validScore(value.exScore,
                      value.maxCombo,
                      value.badPoorCount,
                      value.judgements) &&
           validGauge(value.gauge);
}

auto
validFinalResult(const FinalResult& value) -> bool
{
    return !clearTypeName(value.clearType).isEmpty() &&
           validScore(value.exScore,
                      value.maxCombo,
                      value.badPoorCount,
                      value.judgements) &&
           validGauge(value.finalGauge);
}

auto
validIdentity(const PublicIdentity& value) -> bool
{
    return !value.userId.isEmpty() && !value.displayName.isEmpty();
}

} // namespace

ArenaStandingsModel::ArenaStandingsModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

auto
ArenaStandingsModel::rowCount(const QModelIndex& parent) const -> int
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

auto
ArenaStandingsModel::data(const QModelIndex& index, int role) const -> QVariant
{
    const auto rowIndex = static_cast<qsizetype>(index.row());
    if (!index.isValid() || index.parent().isValid() || index.column() != 0 ||
        rowIndex < 0 || rowIndex >= m_rows.size()) {
        return {};
    }
    const auto& row = m_rows[rowIndex];
    switch (role) {
        case MemberIdRole:
            return row.memberId;
        case DisplayNameRole:
            return row.displayName;
        case AvatarUrlRole:
            return row.avatarUrl;
        case ConnectedRole:
            return row.connected;
        case CompetitionStateRole:
            return row.competitionState;
        case RankRole:
            return row.rank;
        case HasScoreRole:
            return row.hasScore;
        case ExScoreRole:
            return row.exScore;
        case ProgressPermilleRole:
            return row.progressPermille;
        case MaxComboRole:
            return row.maxCombo;
        case BadPoorCountRole:
            return row.badPoorCount;
        case PerfectRole:
            return row.perfect;
        case GreatRole:
            return row.great;
        case GoodRole:
            return row.good;
        case BadRole:
            return row.bad;
        case PoorRole:
            return row.poor;
        case EmptyPoorRole:
            return row.emptyPoor;
        case GaugeTypeRole:
            return row.gaugeType;
        case GaugeValueMilliRole:
            return row.gaugeValueMilli;
        case ClearTypeRole:
            return row.clearType;
        case LobbyWinsAfterRole:
            return row.lobbyWinsAfter;
        case DnfReasonRole:
            return row.dnfReason;
    }
    return {};
}

auto
ArenaStandingsModel::roleNames() const -> QHash<int, QByteArray>
{
    return {
        { MemberIdRole, "memberId" },
        { DisplayNameRole, "displayName" },
        { AvatarUrlRole, "avatarUrl" },
        { ConnectedRole, "connected" },
        { CompetitionStateRole, "competitionState" },
        { RankRole, "rank" },
        { HasScoreRole, "hasScore" },
        { ExScoreRole, "exScore" },
        { ProgressPermilleRole, "progressPermille" },
        { MaxComboRole, "maxCombo" },
        { BadPoorCountRole, "badPoorCount" },
        { PerfectRole, "perfect" },
        { GreatRole, "great" },
        { GoodRole, "good" },
        { BadRole, "bad" },
        { PoorRole, "poor" },
        { EmptyPoorRole, "emptyPoor" },
        { GaugeTypeRole, "gaugeType" },
        { GaugeValueMilliRole, "gaugeValueMilli" },
        { ClearTypeRole, "clearType" },
        { LobbyWinsAfterRole, "lobbyWinsAfter" },
        { DnfReasonRole, "dnfReason" },
    };
}

auto
ArenaStandingsModel::roundId() const -> const QString&
{
    return m_roundId;
}

auto
ArenaStandingsModel::revision() const -> qint64
{
    return m_revision;
}

auto
ArenaStandingsModel::replace(const LiveStandingsSnapshot& snapshot,
                             const QHash<QString, PublicIdentity>& identities)
  -> bool
{
    if (snapshot.roundId.isEmpty() || snapshot.standingsRevision < 1 ||
        m_kind == SnapshotKind::Final ||
        (m_kind == SnapshotKind::Live && m_roundId != snapshot.roundId) ||
        (m_kind == SnapshotKind::Live &&
         snapshot.standingsRevision <= m_revision)) {
        return false;
    }
    auto rows = liveRows(snapshot, identities);
    if (!rows) {
        return false;
    }
    install(std::move(*rows),
            snapshot.roundId,
            snapshot.standingsRevision,
            SnapshotKind::Live);
    return true;
}

auto
ArenaStandingsModel::replaceFinal(const RoundResultSnapshot& snapshot) -> bool
{
    if (snapshot.roundId.isEmpty() || snapshot.resultRevision < 1 ||
        (m_kind == SnapshotKind::Live && m_roundId != snapshot.roundId) ||
        (m_kind == SnapshotKind::Final &&
         snapshot.resultRevision <= m_revision)) {
        return false;
    }
    auto rows = finalRows(snapshot);
    if (!rows) {
        return false;
    }
    install(std::move(*rows),
            snapshot.roundId,
            snapshot.resultRevision,
            SnapshotKind::Final);
    return true;
}

void
ArenaStandingsModel::clear()
{
    if (m_kind == SnapshotKind::None && m_rows.isEmpty() &&
        m_roundId.isEmpty() && m_revision == 0) {
        return;
    }
    beginResetModel();
    m_rows.clear();
    m_roundId.clear();
    m_revision = 0;
    m_kind = SnapshotKind::None;
    endResetModel();
    emit snapshotChanged();
}

auto
ArenaStandingsModel::liveRows(const LiveStandingsSnapshot& snapshot,
                              const QHash<QString, PublicIdentity>& identities)
  -> std::optional<QVector<StandingRow>>
{
    if (snapshot.entries.isEmpty() || snapshot.entries.size() > RoomCapacity) {
        return std::nullopt;
    }

    QVector<StandingRow> rows;
    rows.reserve(snapshot.entries.size());
    QSet<QString> memberIds;
    for (const auto& entry : snapshot.entries) {
        const auto identity = identities.constFind(entry.memberId);
        if (entry.memberId.isEmpty() || memberIds.contains(entry.memberId) ||
            identity == identities.cend() || !validIdentity(*identity)) {
            return std::nullopt;
        }
        memberIds.insert(entry.memberId);

        StandingRow row{
            .memberId = entry.memberId,
            .displayName = identity->displayName,
            .avatarUrl = identity->avatarUrl.value_or(QString{}),
            .connected = entry.connectionStatus == MemberStatus::Connected,
        };
        if (const auto* active =
              std::get_if<LiveActiveStanding>(&entry.state)) {
            row.competitionState = activeStateName(active->competitionState);
            if (row.competitionState.isEmpty() ||
                active->rank.has_value() != active->telemetry.has_value() ||
                (active->rank && (*active->rank < 1 ||
                                  *active->rank > snapshot.entries.size()))) {
                return std::nullopt;
            }
            row.rank = active->rank.value_or(0);
            if (active->telemetry) {
                if (!validTelemetry(*active->telemetry)) {
                    return std::nullopt;
                }
                applyTelemetry(row, *active->telemetry);
            }
        } else if (const auto* finished =
                     std::get_if<LiveFinishedStanding>(&entry.state)) {
            if (finished->rank < 1 ||
                finished->rank > snapshot.entries.size() ||
                !validFinalResult(finished->result)) {
                return std::nullopt;
            }
            row.competitionState = QStringLiteral("finished");
            row.rank = finished->rank;
            applyFinalResult(row, finished->result);
        } else if (const auto* dnf =
                     std::get_if<LiveDnfStanding>(&entry.state)) {
            row.competitionState = QStringLiteral("dnf");
            row.dnfReason = dnfReasonName(dnf->reason);
            if (row.dnfReason.isEmpty()) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

auto
ArenaStandingsModel::finalRows(const RoundResultSnapshot& snapshot)
  -> std::optional<QVector<StandingRow>>
{
    if (snapshot.participantCount < 1 ||
        snapshot.participantCount > RoomCapacity ||
        snapshot.entries.isEmpty() ||
        snapshot.entries.size() != snapshot.participantCount) {
        return std::nullopt;
    }

    QVector<StandingRow> rows;
    rows.reserve(snapshot.entries.size());
    QVector<QString> expectedWinners;
    QSet<QString> memberIds;
    for (const auto& entry : snapshot.entries) {
        if (entry.memberId.isEmpty() || memberIds.contains(entry.memberId) ||
            !validIdentity(entry.identity) ||
            (entry.lobbyWinsAfter && (*entry.lobbyWinsAfter < 0 ||
                                      *entry.lobbyWinsAfter > MaxUInt32))) {
            return std::nullopt;
        }
        memberIds.insert(entry.memberId);

        StandingRow row{
            .memberId = entry.memberId,
            .displayName = entry.identity.displayName,
            .avatarUrl = entry.identity.avatarUrl.value_or(QString{}),
            // Final snapshots do not carry connection state.
            .connected = false,
            .lobbyWinsAfter = entry.lobbyWinsAfter.value_or(-1),
        };
        if (const auto* finished =
              std::get_if<FinalFinishedStanding>(&entry.state)) {
            if (finished->rank < 1 ||
                finished->rank > snapshot.participantCount ||
                !validFinalResult(finished->result)) {
                return std::nullopt;
            }
            row.competitionState = QStringLiteral("finished");
            row.rank = finished->rank;
            applyFinalResult(row, finished->result);
            if (finished->rank == 1) {
                expectedWinners.push_back(entry.memberId);
            }
        } else if (const auto* dnf =
                     std::get_if<FinalDnfStanding>(&entry.state)) {
            row.competitionState = QStringLiteral("dnf");
            row.dnfReason = dnfReasonName(dnf->reason);
            if (row.dnfReason.isEmpty()) {
                return std::nullopt;
            }
        } else {
            return std::nullopt;
        }
        rows.push_back(std::move(row));
    }
    if (snapshot.winnerMemberIds != expectedWinners) {
        return std::nullopt;
    }
    return rows;
}

void
ArenaStandingsModel::applyTelemetry(StandingRow& row,
                                    const TelemetrySnapshot& telemetry)
{
    row.hasScore = true;
    row.exScore = telemetry.exScore;
    row.progressPermille = telemetry.progressPermille;
    row.maxCombo = telemetry.maxCombo;
    row.badPoorCount = telemetry.badPoorCount;
    row.perfect = telemetry.judgements.perfect;
    row.great = telemetry.judgements.great;
    row.good = telemetry.judgements.good;
    row.bad = telemetry.judgements.bad;
    row.poor = telemetry.judgements.poor;
    row.emptyPoor = telemetry.judgements.emptyPoor;
    row.gaugeType = gaugeTypeName(telemetry.gauge.type);
    row.gaugeValueMilli = telemetry.gauge.valueMilli;
}

void
ArenaStandingsModel::applyFinalResult(StandingRow& row,
                                      const FinalResult& result)
{
    row.hasScore = true;
    row.exScore = result.exScore;
    row.progressPermille = 1'000;
    row.maxCombo = result.maxCombo;
    row.badPoorCount = result.badPoorCount;
    row.perfect = result.judgements.perfect;
    row.great = result.judgements.great;
    row.good = result.judgements.good;
    row.bad = result.judgements.bad;
    row.poor = result.judgements.poor;
    row.emptyPoor = result.judgements.emptyPoor;
    row.gaugeType = gaugeTypeName(result.finalGauge.type);
    row.gaugeValueMilli = result.finalGauge.valueMilli;
    row.clearType = clearTypeName(result.clearType);
}

void
ArenaStandingsModel::install(QVector<StandingRow> rows,
                             QString roundId,
                             qint64 revision,
                             SnapshotKind kind)
{
    beginResetModel();
    m_rows = std::move(rows);
    m_roundId = std::move(roundId);
    m_revision = revision;
    m_kind = kind;
    endResetModel();
    emit snapshotChanged();
}

} // namespace arena
