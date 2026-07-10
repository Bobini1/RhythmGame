#include "ArenaOpponentTarget.h"

#include "ArenaTypes.h"

#include <QSet>

#include <algorithm>
#include <optional>
#include <utility>

namespace arena {
namespace {

struct Candidate
{
    QString memberId;
    QString displayName;
    qint64 exScore{};
    bool finished{};
};

auto
candidateFor(const LiveStandingEntry& entry, const PublicIdentity& identity)
  -> std::optional<Candidate>
{
    if (const auto* active = std::get_if<LiveActiveStanding>(&entry.state)) {
        if (!active->telemetry) {
            return std::nullopt;
        }
        return Candidate{ .memberId = entry.memberId,
                          .displayName = identity.displayName,
                          .exScore = active->telemetry->exScore,
                          .finished = false };
    }
    if (const auto* finished =
          std::get_if<LiveFinishedStanding>(&entry.state)) {
        return Candidate{ .memberId = entry.memberId,
                          .displayName = identity.displayName,
                          .exScore = finished->result.exScore,
                          .finished = true };
    }
    return std::nullopt;
}

} // namespace

ArenaOpponentTarget::ArenaOpponentTarget(QObject* parent)
  : QObject(parent)
{
}

auto
ArenaOpponentTarget::available() const -> bool
{
    return m_available;
}

auto
ArenaOpponentTarget::memberId() const -> const QString&
{
    return m_memberId;
}

auto
ArenaOpponentTarget::displayName() const -> const QString&
{
    return m_displayName;
}

auto
ArenaOpponentTarget::exScore() const -> qint64
{
    return m_exScore;
}

auto
ArenaOpponentTarget::finished() const -> bool
{
    return m_finished;
}

void
ArenaOpponentTarget::update(const LiveStandingsSnapshot& snapshot,
                            QStringView selfMemberId,
                            const QHash<QString, PublicIdentity>& identities)
{
    const auto sameRound = m_roundId == snapshot.roundId;
    if (snapshot.roundId.isEmpty() || snapshot.standingsRevision < 1 ||
        snapshot.entries.isEmpty() || snapshot.entries.size() > RoomCapacity ||
        (sameRound && snapshot.standingsRevision <= m_revision)) {
        return;
    }

    QVector<Candidate> candidates;
    candidates.reserve(snapshot.entries.size());
    QSet<QString> memberIds;
    for (const auto& entry : snapshot.entries) {
        const auto identity = identities.constFind(entry.memberId);
        if (entry.memberId.isEmpty() || memberIds.contains(entry.memberId) ||
            identity == identities.cend() || identity->displayName.isEmpty()) {
            return;
        }
        memberIds.insert(entry.memberId);
        if (entry.memberId == selfMemberId) {
            continue;
        }
        if (const auto candidate = candidateFor(entry, *identity)) {
            candidates.push_back(*candidate);
        }
    }

    std::optional<Candidate> selected{ std::nullopt };
    if (!candidates.isEmpty()) {
        const auto strongest =
          std::max_element(candidates.cbegin(),
                           candidates.cend(),
                           [](const Candidate& left, const Candidate& right) {
                               return left.exScore < right.exScore;
                           });
        selected = *strongest;
        if (sameRound && m_available) {
            const auto retained =
              std::find_if(candidates.cbegin(),
                           candidates.cend(),
                           [&](const Candidate& candidate) {
                               return candidate.memberId == m_memberId &&
                                      candidate.exScore == strongest->exScore;
                           });
            if (retained != candidates.cend()) {
                selected = *retained;
            }
        }
    }

    const auto availableValue = selected.has_value();
    const auto memberIdValue = selected ? selected->memberId : QString{};
    const auto displayNameValue = selected ? selected->displayName : QString{};
    const auto exScoreValue = selected ? selected->exScore : 0;
    const auto finishedValue = selected && selected->finished;
    const auto visibleChanged =
      m_available != availableValue || m_memberId != memberIdValue ||
      m_displayName != displayNameValue || m_exScore != exScoreValue ||
      m_finished != finishedValue;

    m_available = availableValue;
    m_memberId = memberIdValue;
    m_displayName = displayNameValue;
    m_exScore = exScoreValue;
    m_finished = finishedValue;
    m_roundId = snapshot.roundId;
    m_revision = snapshot.standingsRevision;
    if (visibleChanged) {
        emit changed();
    }
}

void
ArenaOpponentTarget::clear()
{
    const auto visibleChanged = m_available || !m_memberId.isEmpty() ||
                                !m_displayName.isEmpty() || m_exScore != 0 ||
                                m_finished;
    m_available = false;
    m_memberId.clear();
    m_displayName.clear();
    m_exScore = 0;
    m_finished = false;
    m_roundId.clear();
    m_revision = 0;
    if (visibleChanged) {
        emit changed();
    }
}

} // namespace arena
