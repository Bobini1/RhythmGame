#include "ArenaMemberListModel.h"

#include <QSet>

#include <algorithm>
#include <utility>

namespace arena {
namespace {

auto
inventoryStateName(InventoryState state) -> QString
{
    switch (state) {
        case InventoryState::Missing:
            return QStringLiteral("missing");
        case InventoryState::Syncing:
            return QStringLiteral("syncing");
        case InventoryState::Ready:
            return QStringLiteral("ready");
    }
    return {};
}

auto
roundStateName(MemberRoundState state) -> QString
{
    switch (state) {
        case MemberRoundState::Eligible:
            return QStringLiteral("eligible");
        case MemberRoundState::Waiting:
            return QStringLiteral("waiting");
        case MemberRoundState::Probing:
            return QStringLiteral("probing");
        case MemberRoundState::Loading:
            return QStringLiteral("loading");
        case MemberRoundState::Loaded:
            return QStringLiteral("loaded");
        case MemberRoundState::Playing:
            return QStringLiteral("playing");
    }
    return {};
}

auto
uniqueMemberIds(const QVector<Member>& members) -> bool
{
    QSet<QString> ids;
    for (const auto& member : members) {
        if (ids.contains(member.memberId)) {
            return false;
        }
        ids.insert(member.memberId);
    }
    return true;
}

} // namespace

ArenaMemberListModel::ArenaMemberListModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

auto
ArenaMemberListModel::rowCount(const QModelIndex& parent) const -> int
{
    return parent.isValid() ? 0 : static_cast<int>(m_members.size());
}

auto
ArenaMemberListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.parent().isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_members.size()) {
        return {};
    }
    const auto& member = m_members[index.row()];
    switch (role) {
        case MemberIdRole:
            return member.memberId;
        case DisplayNameRole:
            return member.identity.displayName;
        case AvatarUrlRole:
            return member.identity.avatarUrl.value_or(QString{});
        case ConnectedRole:
            return member.status == MemberStatus::Connected;
        case OwnerRole:
            return m_ownerMemberId && member.memberId == *m_ownerMemberId;
        case SelfRole:
            return member.memberId == m_selfMemberId;
        case LobbyWinsRole:
            return member.lobbyWins;
        case ReadyRole:
            return member.ready;
        case InventoryStateRole:
            return inventoryStateName(member.inventoryState);
        case InventoryRevisionRole:
            return member.inventoryRevision;
        case AvailabilityAppliedRevisionRole:
            return member.availabilityAppliedRevision;
        case RoundStateRole:
            return roundStateName(member.roundState);
    }
    return {};
}

auto
ArenaMemberListModel::roleNames() const -> QHash<int, QByteArray>
{
    return {
        { MemberIdRole, "memberId" },
        { DisplayNameRole, "displayName" },
        { AvatarUrlRole, "avatarUrl" },
        { ConnectedRole, "connected" },
        { OwnerRole, "owner" },
        { SelfRole, "self" },
        { LobbyWinsRole, "lobbyWins" },
        { ReadyRole, "ready" },
        { InventoryStateRole, "inventoryState" },
        { InventoryRevisionRole, "inventoryRevision" },
        { AvailabilityAppliedRevisionRole, "availabilityAppliedRevision" },
        { RoundStateRole, "roundState" },
    };
}

auto
ArenaMemberListModel::replace(QVector<Member> members,
                              std::optional<QString> ownerMemberId,
                              QString selfMemberId) -> bool
{
    if (!uniqueMemberIds(members)) {
        return false;
    }
    const auto countChangedValue = members.size() != m_members.size();
    beginResetModel();
    m_members = std::move(members);
    m_ownerMemberId = std::move(ownerMemberId);
    m_selfMemberId = std::move(selfMemberId);
    endResetModel();
    if (countChangedValue) {
        emit countChanged();
    }
    return true;
}

void
ArenaMemberListModel::upsert(Member member)
{
    const auto found =
      std::find_if(m_members.begin(), m_members.end(), [&](const Member& row) {
          return row.memberId == member.memberId;
      });
    if (found == m_members.end()) {
        const auto row = static_cast<int>(m_members.size());
        beginInsertRows({}, row, row);
        m_members.push_back(std::move(member));
        endInsertRows();
        emit countChanged();
        return;
    }

    QList<int> changedRoles;
    if (found->identity.displayName != member.identity.displayName) {
        changedRoles.push_back(DisplayNameRole);
    }
    if (found->identity.avatarUrl != member.identity.avatarUrl) {
        changedRoles.push_back(AvatarUrlRole);
    }
    if (found->status != member.status) {
        changedRoles.push_back(ConnectedRole);
    }
    if (found->lobbyWins != member.lobbyWins) {
        changedRoles.push_back(LobbyWinsRole);
    }
    if (found->ready != member.ready) {
        changedRoles.push_back(ReadyRole);
    }
    if (found->inventoryState != member.inventoryState) {
        changedRoles.push_back(InventoryStateRole);
    }
    if (found->inventoryRevision != member.inventoryRevision) {
        changedRoles.push_back(InventoryRevisionRole);
    }
    if (found->availabilityAppliedRevision !=
        member.availabilityAppliedRevision) {
        changedRoles.push_back(AvailabilityAppliedRevisionRole);
    }
    if (found->roundState != member.roundState) {
        changedRoles.push_back(RoundStateRole);
    }
    if (changedRoles.isEmpty()) {
        // userId is intentionally not a visible role, but retain the latest
        // authoritative value in the owned row.
        found->identity.userId = std::move(member.identity.userId);
        return;
    }
    const auto row = static_cast<int>(std::distance(m_members.begin(), found));
    *found = std::move(member);
    emit dataChanged(index(row, 0), index(row, 0), changedRoles);
}

auto
ArenaMemberListModel::remove(QStringView memberId) -> bool
{
    const auto found = std::find_if(
      m_members.begin(), m_members.end(), [&](const Member& member) {
          return member.memberId == memberId;
      });
    if (found == m_members.end()) {
        return false;
    }
    const auto row = static_cast<int>(std::distance(m_members.begin(), found));
    beginRemoveRows({}, row, row);
    m_members.erase(found);
    endRemoveRows();
    emit countChanged();
    return true;
}

void
ArenaMemberListModel::setOwnerMemberId(std::optional<QString> memberId)
{
    if (m_ownerMemberId == memberId) {
        return;
    }
    const auto old = m_ownerMemberId;
    m_ownerMemberId = std::move(memberId);
    for (qsizetype i = 0; i < m_members.size(); ++i) {
        const auto& id = m_members[i].memberId;
        if ((old && id == *old) ||
            (m_ownerMemberId && id == *m_ownerMemberId)) {
            emit dataChanged(index(static_cast<int>(i), 0),
                             index(static_cast<int>(i), 0),
                             { OwnerRole });
        }
    }
}

void
ArenaMemberListModel::setSelfMemberId(QString memberId)
{
    if (m_selfMemberId == memberId) {
        return;
    }
    const auto old = m_selfMemberId;
    m_selfMemberId = std::move(memberId);
    for (qsizetype i = 0; i < m_members.size(); ++i) {
        const auto& id = m_members[i].memberId;
        if (id == old || id == m_selfMemberId) {
            emit dataChanged(index(static_cast<int>(i), 0),
                             index(static_cast<int>(i), 0),
                             { SelfRole });
        }
    }
}

void
ArenaMemberListModel::clear()
{
    if (m_members.isEmpty()) {
        m_ownerMemberId.reset();
        m_selfMemberId.clear();
        return;
    }
    beginResetModel();
    m_members.clear();
    m_ownerMemberId.reset();
    m_selfMemberId.clear();
    endResetModel();
    emit countChanged();
}

} // namespace arena
