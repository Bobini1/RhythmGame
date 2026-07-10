#include "ArenaResultModel.h"

#include "ArenaTypes.h"

#include <QHash>

#include <algorithm>
#include <utility>

namespace arena {

ArenaResultModel::ArenaResultModel(QObject* parent)
  : QObject(parent)
  , m_standings(this)
{
}

auto
ArenaResultModel::valid() const -> bool
{
    return m_valid;
}

auto
ArenaResultModel::finalized() const -> bool
{
    return m_finalized;
}

auto
ArenaResultModel::roundId() const -> const QString&
{
    return m_roundId;
}

auto
ArenaResultModel::resultRevision() const -> qint64
{
    return m_resultRevision;
}

auto
ArenaResultModel::participantCount() const -> int
{
    return m_participantCount;
}

auto
ArenaResultModel::winnerMemberIds() const -> const QStringList&
{
    return m_winnerMemberIds;
}

auto
ArenaResultModel::winnerNames() const -> const QStringList&
{
    return m_winnerNames;
}

auto
ArenaResultModel::localRank() const -> int
{
    return m_localRank;
}

auto
ArenaResultModel::localDnf() const -> bool
{
    return m_localDnf;
}

auto
ArenaResultModel::localWinner() const -> bool
{
    return m_localWinner;
}

auto
ArenaResultModel::selectionTitle() const -> const QString&
{
    return m_selectionTitle;
}

auto
ArenaResultModel::selectionOptionsSummary() const -> const QString&
{
    return m_selectionOptionsSummary;
}

auto
ArenaResultModel::standings() -> ArenaStandingsModel*
{
    return &m_standings;
}

auto
ArenaResultModel::setPending(QString roundId,
                             int participantCount,
                             QString selectionTitle,
                             QString selectionOptionsSummary,
                             bool localDnf) -> bool
{
    if (roundId.isEmpty() || participantCount < 1 ||
        participantCount > RoomCapacity ||
        (m_valid && (m_finalized || m_roundId != roundId))) {
        return false;
    }
    if (m_valid && m_participantCount == participantCount &&
        m_selectionTitle == selectionTitle &&
        m_selectionOptionsSummary == selectionOptionsSummary &&
        m_localDnf == localDnf) {
        return false;
    }

    m_standings.clear();
    m_valid = true;
    m_finalized = false;
    m_roundId = std::move(roundId);
    m_resultRevision = 0;
    m_participantCount = participantCount;
    m_winnerMemberIds.clear();
    m_winnerNames.clear();
    m_localRank = 0;
    m_localDnf = localDnf;
    m_localWinner = false;
    m_selectionTitle = std::move(selectionTitle);
    m_selectionOptionsSummary = std::move(selectionOptionsSummary);
    emit changed();
    return true;
}

auto
ArenaResultModel::replaceFinal(const RoundResultSnapshot& snapshot,
                               QStringView localMemberId,
                               QString selectionOptionsSummary) -> bool
{
    if (m_valid && !m_finalized && m_roundId != snapshot.roundId) {
        return false;
    }
    if (m_valid && m_finalized && snapshot.resultRevision <= m_resultRevision) {
        return false;
    }

    ArenaStandingsModel validatedStandings;
    if (!validatedStandings.replaceFinal(snapshot)) {
        return false;
    }

    QHash<QString, QString> namesByMemberId;
    namesByMemberId.reserve(snapshot.entries.size());
    int localRankValue = 0;
    bool localDnfValue = false;
    for (const auto& entry : snapshot.entries) {
        namesByMemberId.insert(entry.memberId, entry.identity.displayName);
        if (entry.memberId != localMemberId) {
            continue;
        }
        if (const auto* finished =
              std::get_if<FinalFinishedStanding>(&entry.state)) {
            localRankValue = finished->rank;
        } else if (std::get_if<FinalDnfStanding>(&entry.state) != nullptr) {
            localDnfValue = true;
        }
    }

    QStringList winnerMemberIdsValue;
    QStringList winnerNamesValue;
    winnerMemberIdsValue.reserve(snapshot.winnerMemberIds.size());
    winnerNamesValue.reserve(snapshot.winnerMemberIds.size());
    for (const auto& winnerId : snapshot.winnerMemberIds) {
        const auto name = namesByMemberId.constFind(winnerId);
        if (name == namesByMemberId.cend()) {
            return false;
        }
        winnerMemberIdsValue.push_back(winnerId);
        winnerNamesValue.push_back(*name);
    }

    if (!m_standings.replaceFinal(snapshot)) {
        return false;
    }
    m_valid = true;
    m_finalized = true;
    m_roundId = snapshot.roundId;
    m_resultRevision = snapshot.resultRevision;
    m_participantCount = snapshot.participantCount;
    m_winnerMemberIds = std::move(winnerMemberIdsValue);
    m_winnerNames = std::move(winnerNamesValue);
    m_localRank = localRankValue;
    m_localDnf = localDnfValue;
    m_localWinner = std::any_of(
      m_winnerMemberIds.cbegin(),
      m_winnerMemberIds.cend(),
      [&](const QString& winnerId) { return winnerId == localMemberId; });
    m_selectionTitle = snapshot.selection.title;
    m_selectionOptionsSummary = std::move(selectionOptionsSummary);
    emit changed();
    return true;
}

void
ArenaResultModel::clear()
{
    if (!m_valid && !m_finalized && m_roundId.isEmpty() &&
        m_resultRevision == 0 && m_participantCount == 0 &&
        m_winnerMemberIds.isEmpty() && m_winnerNames.isEmpty() &&
        m_localRank == 0 && !m_localDnf && !m_localWinner &&
        m_selectionTitle.isEmpty() && m_selectionOptionsSummary.isEmpty() &&
        m_standings.rowCount() == 0) {
        return;
    }

    m_standings.clear();
    m_valid = false;
    m_finalized = false;
    m_roundId.clear();
    m_resultRevision = 0;
    m_participantCount = 0;
    m_winnerMemberIds.clear();
    m_winnerNames.clear();
    m_localRank = 0;
    m_localDnf = false;
    m_localWinner = false;
    m_selectionTitle.clear();
    m_selectionOptionsSummary.clear();
    emit changed();
}

} // namespace arena
