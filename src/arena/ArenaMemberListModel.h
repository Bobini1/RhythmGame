#pragma once

#include "ArenaTypes.h"

#include <QAbstractListModel>

namespace arena {

class ArenaMemberListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

  public:
    enum Roles
    {
        MemberIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        AvatarUrlRole,
        ConnectedRole,
        OwnerRole,
        SelfRole,
        LobbyWinsRole,
        ReadyRole,
        InventoryStateRole,
        InventoryRevisionRole,
        AvailabilityAppliedRevisionRole,
        RoundStateRole,
    };
    Q_ENUM(Roles)

    explicit ArenaMemberListModel(QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

    [[nodiscard]] auto replace(QVector<Member> members,
                               std::optional<QString> ownerMemberId,
                               QString selfMemberId) -> bool;
    void upsert(Member member);
    [[nodiscard]] auto remove(QStringView memberId) -> bool;
    void setOwnerMemberId(std::optional<QString> memberId);
    void setSelfMemberId(QString memberId);
    void clear();

  signals:
    void countChanged();

  private:
    QVector<Member> m_members{};
    std::optional<QString> m_ownerMemberId{ std::nullopt };
    QString m_selfMemberId{};
};

} // namespace arena
