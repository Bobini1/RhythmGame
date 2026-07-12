#include "ArenaRoomListModel.h"

#include <QSet>

#include <algorithm>
#include <utility>

namespace arena {
namespace {

auto
phaseName(RoomPhase phase) -> QString
{
    switch (phase) {
        case RoomPhase::Selecting:
            return QStringLiteral("selecting");
        case RoomPhase::Loading:
            return QStringLiteral("loading");
        case RoomPhase::Playing:
            return QStringLiteral("playing");
    }
    return {};
}

auto
uniqueRoomIds(const QVector<RoomSummary>& rooms) -> bool
{
    QSet<QString> ids;
    for (const auto& room : rooms) {
        if (ids.contains(room.roomId)) {
            return false;
        }
        ids.insert(room.roomId);
    }
    return true;
}

} // namespace

ArenaRoomListModel::ArenaRoomListModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

auto
ArenaRoomListModel::rowCount(const QModelIndex& parent) const -> int
{
    return parent.isValid() ? 0 : static_cast<int>(m_rooms.size());
}

auto
ArenaRoomListModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.parent().isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_rooms.size()) {
        return {};
    }
    const auto& room = m_rooms[index.row()];
    switch (role) {
        case RoomIdRole:
            return room.roomId;
        case NameRole:
            return room.name;
        case PhaseRole:
            return phaseName(room.phase);
        case PasswordProtectedRole:
            return room.hasPassword;
        case ConnectedCountRole:
            return room.connectedCount;
        case ReservedCountRole:
            return room.reservedCount;
        case MaximumCountRole:
            return room.maxCount;
        case MembersRole: {
            QVariantList members;
            members.reserve(room.members.size());
            for (const auto& member : room.members) {
                members.push_back(QVariantMap{
                  { QStringLiteral("displayName"), member.displayName },
                  { QStringLiteral("avatarUrl"),
                    member.avatarUrl ? member.avatarUrl->toString()
                                     : QString{} },
                  { QStringLiteral("connected"), member.connected },
                });
            }
            return members;
        }
    }
    return {};
}

auto
ArenaRoomListModel::roleNames() const -> QHash<int, QByteArray>
{
    return { { RoomIdRole, "roomId" },
             { NameRole, "name" },
             { PhaseRole, "phase" },
             { PasswordProtectedRole, "passwordProtected" },
             { ConnectedCountRole, "connectedCount" },
             { ReservedCountRole, "reservedCount" },
             { MaximumCountRole, "maximumCount" },
             { MembersRole, "members" } };
}

auto
ArenaRoomListModel::replace(QVector<RoomSummary> rooms) -> bool
{
    if (!uniqueRoomIds(rooms)) {
        return false;
    }
    const auto countChangedValue = rooms.size() != m_rooms.size();
    beginResetModel();
    m_rooms = std::move(rooms);
    endResetModel();
    if (countChangedValue) {
        emit countChanged();
    }
    return true;
}

auto
ArenaRoomListModel::applyDelta(const QVector<RoomSummary>& upserts,
                               const QVector<QString>& removedRoomIds) -> bool
{
    if (!uniqueRoomIds(upserts)) {
        return false;
    }
    QSet<QString> upsertIds;
    for (const auto& room : upserts) {
        upsertIds.insert(room.roomId);
    }
    QSet<QString> removedIds;
    for (const auto& roomId : removedRoomIds) {
        if (removedIds.contains(roomId) || upsertIds.contains(roomId)) {
            return false;
        }
        removedIds.insert(roomId);
    }

    QVector<int> removalRows;
    for (qsizetype i = 0; i < m_rooms.size(); ++i) {
        if (removedIds.contains(m_rooms[i].roomId)) {
            removalRows.push_back(static_cast<int>(i));
        }
    }
    std::sort(removalRows.begin(), removalRows.end(), std::greater{});
    for (const auto row : removalRows) {
        beginRemoveRows({}, row, row);
        m_rooms.removeAt(row);
        endRemoveRows();
        emit countChanged();
    }
    for (const auto& room : upserts) {
        upsert(room);
    }
    return true;
}

void
ArenaRoomListModel::upsert(RoomSummary room)
{
    const auto found =
      std::find_if(m_rooms.begin(), m_rooms.end(), [&](const RoomSummary& row) {
          return row.roomId == room.roomId;
      });
    if (found == m_rooms.end()) {
        const auto row = static_cast<int>(m_rooms.size());
        beginInsertRows({}, row, row);
        m_rooms.push_back(std::move(room));
        endInsertRows();
        emit countChanged();
        return;
    }

    QList<int> changedRoles;
    if (found->name != room.name) {
        changedRoles.push_back(NameRole);
    }
    if (found->phase != room.phase) {
        changedRoles.push_back(PhaseRole);
    }
    if (found->hasPassword != room.hasPassword) {
        changedRoles.push_back(PasswordProtectedRole);
    }
    if (found->connectedCount != room.connectedCount) {
        changedRoles.push_back(ConnectedCountRole);
    }
    if (found->reservedCount != room.reservedCount) {
        changedRoles.push_back(ReservedCountRole);
    }
    if (found->maxCount != room.maxCount) {
        changedRoles.push_back(MaximumCountRole);
    }
    if (found->members != room.members) {
        changedRoles.push_back(MembersRole);
    }
    if (changedRoles.isEmpty()) {
        return;
    }
    const auto row = static_cast<int>(std::distance(m_rooms.begin(), found));
    *found = std::move(room);
    emit dataChanged(index(row, 0), index(row, 0), changedRoles);
}

auto
ArenaRoomListModel::remove(QStringView roomId) -> bool
{
    const auto found = std::find_if(
      m_rooms.begin(), m_rooms.end(), [&](const RoomSummary& room) {
          return room.roomId == roomId;
      });
    if (found == m_rooms.end()) {
        return false;
    }
    const auto row = static_cast<int>(std::distance(m_rooms.begin(), found));
    beginRemoveRows({}, row, row);
    m_rooms.erase(found);
    endRemoveRows();
    emit countChanged();
    return true;
}

void
ArenaRoomListModel::clear()
{
    if (m_rooms.isEmpty()) {
        return;
    }
    beginResetModel();
    m_rooms.clear();
    endResetModel();
    emit countChanged();
}

} // namespace arena
