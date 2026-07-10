#pragma once

#include "ArenaStandingsModel.h"

#include <QObject>
#include <QStringList>

namespace arena {

struct RoundResultSnapshot;

class ArenaResultModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool valid READ valid NOTIFY changed FINAL)
    Q_PROPERTY(bool finalized READ finalized NOTIFY changed FINAL)
    Q_PROPERTY(QString roundId READ roundId NOTIFY changed FINAL)
    Q_PROPERTY(qint64 resultRevision READ resultRevision NOTIFY changed FINAL)
    Q_PROPERTY(int participantCount READ participantCount NOTIFY changed FINAL)
    Q_PROPERTY(
      QStringList winnerMemberIds READ winnerMemberIds NOTIFY changed FINAL)
    Q_PROPERTY(QStringList winnerNames READ winnerNames NOTIFY changed FINAL)
    Q_PROPERTY(int localRank READ localRank NOTIFY changed FINAL)
    Q_PROPERTY(bool localDnf READ localDnf NOTIFY changed FINAL)
    Q_PROPERTY(bool localWinner READ localWinner NOTIFY changed FINAL)
    Q_PROPERTY(QString selectionTitle READ selectionTitle NOTIFY changed FINAL)
    Q_PROPERTY(QString selectionOptionsSummary READ selectionOptionsSummary
                 NOTIFY changed FINAL)
    Q_PROPERTY(
      arena::ArenaStandingsModel* standings READ standings CONSTANT FINAL)

  public:
    explicit ArenaResultModel(QObject* parent = nullptr);

    auto valid() const -> bool;
    auto finalized() const -> bool;
    auto roundId() const -> const QString&;
    auto resultRevision() const -> qint64;
    auto participantCount() const -> int;
    auto winnerMemberIds() const -> const QStringList&;
    auto winnerNames() const -> const QStringList&;
    auto localRank() const -> int;
    auto localDnf() const -> bool;
    auto localWinner() const -> bool;
    auto selectionTitle() const -> const QString&;
    auto selectionOptionsSummary() const -> const QString&;
    auto standings() -> ArenaStandingsModel*;

    [[nodiscard]] auto setPending(QString roundId,
                                  int participantCount,
                                  QString selectionTitle,
                                  QString selectionOptionsSummary,
                                  bool localDnf) -> bool;
    [[nodiscard]] auto replaceFinal(const RoundResultSnapshot& snapshot,
                                    QStringView localMemberId,
                                    QString selectionOptionsSummary) -> bool;
    void clear();

  signals:
    void changed();

  private:
    bool m_valid{};
    bool m_finalized{};
    QString m_roundId{};
    qint64 m_resultRevision{};
    int m_participantCount{};
    QStringList m_winnerMemberIds{};
    QStringList m_winnerNames{};
    int m_localRank{};
    bool m_localDnf{};
    bool m_localWinner{};
    QString m_selectionTitle{};
    QString m_selectionOptionsSummary{};
    ArenaStandingsModel m_standings;
};

} // namespace arena
