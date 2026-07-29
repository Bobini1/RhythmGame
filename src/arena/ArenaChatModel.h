#pragma once

#include "ArenaTypes.h"

#include <QAbstractListModel>
#include <QSet>

namespace arena {

class ArenaChatModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

  public:
    enum Roles
    {
        MessageIdRole = Qt::UserRole + 1,
        MemberIdRole,
        DisplayNameRole,
        TextRole,
        TimestampRole,
        SelfRole,
    };
    Q_ENUM(Roles)

    explicit ArenaChatModel(QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

    [[nodiscard]] auto replace(QVector<ChatMessage> messages,
                               QString selfMemberId,
                               QString roomId) -> bool;
    [[nodiscard]] auto containsMessage(QStringView messageId) const -> bool;
    [[nodiscard]] auto upsert(ChatMessage message) -> bool;
    [[nodiscard]] auto remove(QStringView messageId) -> bool;
    void setSelfMemberId(QString memberId);
    void resetSelfMemberIds();
    void clear();

  signals:
    void countChanged();

  private:
    [[nodiscard]] auto isSelf(const ChatMessage& message) const -> bool;

    QVector<ChatMessage> m_messages{};
    QSet<QString> m_selfMemberIds{};
    QString m_roomId{};
};

} // namespace arena
