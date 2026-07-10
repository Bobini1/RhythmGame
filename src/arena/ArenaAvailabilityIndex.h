#pragma once

#include "ArenaBinaryProtocol.h"

#include <QByteArray>
#include <QObject>
#include <QStringView>
#include <QtTypes>

namespace arena {

class ArenaAvailabilityIndex : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY changed FINAL)
    Q_PROPERTY(qint64 revision READ revision NOTIFY changed FINAL)

  public:
    enum class State
    {
        NotApplicable,
        Syncing,
        Ready,
    };
    Q_ENUM(State)

    enum class Availability
    {
        NotApplicable,
        Syncing,
        AvailableToAll,
        UnavailableToSome,
    };
    Q_ENUM(Availability)

    explicit ArenaAvailabilityIndex(QObject* parent = nullptr);

    [[nodiscard]] auto state() const -> State;
    [[nodiscard]] auto revision() const -> qint64;
    [[nodiscard]] auto availability(QStringView sha256Hex) const
      -> Availability;
    Q_INVOKABLE [[nodiscard]] auto availabilityFor(
      const QString& sha256Hex) const -> Availability;
    [[nodiscard]] auto applyReset(qint64 targetRevision, QByteArray packed)
      -> bool;
    [[nodiscard]] auto applyDelta(qint64 baseRevision,
                                  qint64 targetRevision,
                                  QByteArray added,
                                  QByteArray removed) -> bool;
    void setSyncing();
    void clear();

  signals:
    void changed();

  private:
    State m_state{ State::NotApplicable };
    qint64 m_revision{};
    QByteArray m_packedHashes{};
};

} // namespace arena
