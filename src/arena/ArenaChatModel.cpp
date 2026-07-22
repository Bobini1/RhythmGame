#include "ArenaChatModel.h"

#include <QSet>

#include <algorithm>
#include <utility>

namespace arena {
namespace {

auto
validReplacement(const QVector<ChatMessage>& messages) -> bool
{
    if (messages.size() > MaxWireChatBacklog) {
        return false;
    }
    QSet<QString> ids;
    for (const auto& message : messages) {
        if (ids.contains(message.messageId)) {
            return false;
        }
        ids.insert(message.messageId);
    }
    return true;
}

} // namespace

ArenaChatModel::ArenaChatModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

auto
ArenaChatModel::rowCount(const QModelIndex& parent) const -> int
{
    return parent.isValid() ? 0 : static_cast<int>(m_messages.size());
}

auto
ArenaChatModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.parent().isValid() || index.column() != 0 ||
        index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }
    const auto& message = m_messages[index.row()];
    switch (role) {
        case MessageIdRole:
            return message.messageId;
        case MemberIdRole:
            return message.authorMemberId;
        case DisplayNameRole:
            return message.authorDisplayName;
        case TextRole:
            return message.text;
        case TimestampRole:
            return message.sentAtMs;
        case SelfRole:
            return message.authorMemberId == m_selfMemberId;
    }
    return {};
}

auto
ArenaChatModel::roleNames() const -> QHash<int, QByteArray>
{
    return { { MessageIdRole, "messageId" },     { MemberIdRole, "memberId" },
             { DisplayNameRole, "displayName" }, { TextRole, "text" },
             { TimestampRole, "timestamp" },     { SelfRole, "self" } };
}

auto
ArenaChatModel::replace(QVector<ChatMessage> messages, QString selfMemberId)
  -> bool
{
    if (!validReplacement(messages)) {
        return false;
    }
    const auto countChangedValue = messages.size() != m_messages.size();
    beginResetModel();
    m_messages = std::move(messages);
    m_selfMemberId = std::move(selfMemberId);
    endResetModel();
    if (countChangedValue) {
        emit countChanged();
    }
    return true;
}

auto
ArenaChatModel::containsMessage(QStringView messageId) const -> bool
{
    return std::any_of(
      m_messages.cbegin(), m_messages.cend(), [&](const ChatMessage& message) {
          return message.messageId == messageId;
      });
}

auto
ArenaChatModel::upsert(ChatMessage message) -> bool
{
    const auto found = std::find_if(
      m_messages.begin(), m_messages.end(), [&](const ChatMessage& row) {
          return row.messageId == message.messageId;
      });
    if (found != m_messages.end()) {
        QList<int> changedRoles;
        if (found->authorMemberId != message.authorMemberId) {
            changedRoles.push_back(MemberIdRole);
            const auto oldSelf = found->authorMemberId == m_selfMemberId;
            const auto newSelf = message.authorMemberId == m_selfMemberId;
            if (oldSelf != newSelf) {
                changedRoles.push_back(SelfRole);
            }
        }
        if (found->authorDisplayName != message.authorDisplayName) {
            changedRoles.push_back(DisplayNameRole);
        }
        if (found->text != message.text) {
            changedRoles.push_back(TextRole);
        }
        if (found->sentAtMs != message.sentAtMs) {
            changedRoles.push_back(TimestampRole);
        }
        if (changedRoles.isEmpty()) {
            return false;
        }
        const auto row =
          static_cast<int>(std::distance(m_messages.begin(), found));
        *found = std::move(message);
        emit dataChanged(index(row, 0), index(row, 0), changedRoles);
        return false;
    }

    const auto wasFull = m_messages.size() == MaxWireChatBacklog;
    if (wasFull) {
        beginRemoveRows({}, 0, 0);
        m_messages.removeFirst();
        endRemoveRows();
    }
    const auto row = static_cast<int>(m_messages.size());
    beginInsertRows({}, row, row);
    m_messages.push_back(std::move(message));
    endInsertRows();
    if (!wasFull) {
        emit countChanged();
    }
    return true;
}

auto
ArenaChatModel::remove(QStringView messageId) -> bool
{
    const auto found = std::find_if(
      m_messages.begin(), m_messages.end(), [&](const ChatMessage& message) {
          return message.messageId == messageId;
      });
    if (found == m_messages.end()) {
        return false;
    }
    const auto row = static_cast<int>(std::distance(m_messages.begin(), found));
    beginRemoveRows({}, row, row);
    m_messages.erase(found);
    endRemoveRows();
    emit countChanged();
    return true;
}

void
ArenaChatModel::setSelfMemberId(QString memberId)
{
    if (m_selfMemberId == memberId) {
        return;
    }
    const auto old = m_selfMemberId;
    m_selfMemberId = std::move(memberId);
    for (qsizetype i = 0; i < m_messages.size(); ++i) {
        const auto& author = m_messages[i].authorMemberId;
        if (author == old || author == m_selfMemberId) {
            emit dataChanged(index(static_cast<int>(i), 0),
                             index(static_cast<int>(i), 0),
                             { SelfRole });
        }
    }
}

void
ArenaChatModel::clear()
{
    if (m_messages.isEmpty()) {
        m_selfMemberId.clear();
        return;
    }
    beginResetModel();
    m_messages.clear();
    m_selfMemberId.clear();
    endResetModel();
    emit countChanged();
}

} // namespace arena
