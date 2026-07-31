#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QtTypes>

namespace arena {

struct LiveStandingsSnapshot;
struct PublicIdentity;

class ArenaOpponentTarget : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY changed FINAL)
    Q_PROPERTY(QString memberId READ memberId NOTIFY changed FINAL)
    Q_PROPERTY(QString displayName READ displayName NOTIFY changed FINAL)
    Q_PROPERTY(qint64 exScore READ exScore NOTIFY changed FINAL)
    Q_PROPERTY(bool finished READ finished NOTIFY changed FINAL)

  public:
    explicit ArenaOpponentTarget(QObject* parent = nullptr);

    auto available() const -> bool;
    auto memberId() const -> const QString&;
    auto displayName() const -> const QString&;
    auto exScore() const -> qint64;
    auto finished() const -> bool;

    void update(const LiveStandingsSnapshot& snapshot,
                QStringView selfMemberId,
                const QHash<QString, PublicIdentity>& identities);
    void clear();

  signals:
    void changed();

  private:
    bool m_available{};
    QString m_memberId{};
    QString m_displayName{};
    qint64 m_exScore{};
    bool m_finished{};
    QString m_roundId{};
    qint64 m_revision{};
};

} // namespace arena
