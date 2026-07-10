#pragma once

#include "ArenaTypes.h"

#include <QAbstractListModel>

namespace arena {

class ArenaRoomListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

  public:
    enum Roles
    {
        RoomIdRole = Qt::UserRole + 1,
        NameRole,
        PhaseRole,
        PasswordProtectedRole,
        ConnectedCountRole,
        ReservedCountRole,
        MaximumCountRole,
    };
    Q_ENUM(Roles)

    explicit ArenaRoomListModel(QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

    [[nodiscard]] auto replace(QVector<RoomSummary> rooms) -> bool;
    [[nodiscard]] auto applyDelta(const QVector<RoomSummary>& upserts,
                                  const QVector<QString>& removedRoomIds)
      -> bool;
    void upsert(RoomSummary room);
    [[nodiscard]] auto remove(QStringView roomId) -> bool;
    void clear();

  signals:
    void countChanged();

  private:
    QVector<RoomSummary> m_rooms{};
};

} // namespace arena
