#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVector>
#include <QtTypes>

#include <optional>

namespace arena {

struct FinalResult;
struct LiveStandingsSnapshot;
struct PublicIdentity;
struct RoundResultSnapshot;
struct TelemetrySnapshot;

class ArenaStandingsModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString roundId READ roundId NOTIFY snapshotChanged FINAL)
    Q_PROPERTY(qint64 revision READ revision NOTIFY snapshotChanged FINAL)

  public:
    enum Roles
    {
        MemberIdRole = Qt::UserRole + 1,
        DisplayNameRole,
        AvatarUrlRole,
        ConnectedRole,
        CompetitionStateRole,
        RankRole,
        HasScoreRole,
        ExScoreRole,
        ProgressPermilleRole,
        MaxComboRole,
        BadPoorCountRole,
        PerfectRole,
        GreatRole,
        GoodRole,
        BadRole,
        PoorRole,
        EmptyPoorRole,
        GaugeTypeRole,
        GaugeValueMilliRole,
        ClearTypeRole,
        LobbyWinsAfterRole,
        DnfReasonRole,
    };
    Q_ENUM(Roles)

    explicit ArenaStandingsModel(QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = {}) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;

    auto roundId() const -> const QString&;
    auto revision() const -> qint64;

    [[nodiscard]] auto replace(const LiveStandingsSnapshot& snapshot,
                               const QHash<QString, PublicIdentity>& identities)
      -> bool;
    [[nodiscard]] auto replaceFinal(const RoundResultSnapshot& snapshot)
      -> bool;
    void clear();

  signals:
    void snapshotChanged();

  private:
    enum class SnapshotKind
    {
        None,
        Live,
        Final,
    };

    struct StandingRow
    {
        QString memberId;
        QString displayName;
        QString avatarUrl;
        bool connected{};
        QString competitionState;
        int rank{};
        bool hasScore{};
        qint64 exScore{};
        qint64 progressPermille{};
        qint64 maxCombo{};
        qint64 badPoorCount{};
        qint64 perfect{};
        qint64 great{};
        qint64 good{};
        qint64 bad{};
        qint64 poor{};
        qint64 emptyPoor{};
        QString gaugeType;
        qint64 gaugeValueMilli{};
        QString clearType;
        qint64 lobbyWinsAfter{ -1 };
        QString dnfReason;
    };

    static auto liveRows(const LiveStandingsSnapshot& snapshot,
                         const QHash<QString, PublicIdentity>& identities)
      -> std::optional<QVector<StandingRow>>;
    static auto finalRows(const RoundResultSnapshot& snapshot)
      -> std::optional<QVector<StandingRow>>;
    static void applyTelemetry(StandingRow& row,
                               const TelemetrySnapshot& telemetry);
    static void applyFinalResult(StandingRow& row, const FinalResult& result);
    void install(QVector<StandingRow> rows,
                 QString roundId,
                 qint64 revision,
                 SnapshotKind kind);

    QVector<StandingRow> m_rows{};
    QString m_roundId{};
    qint64 m_revision{};
    SnapshotKind m_kind{ SnapshotKind::None };
};

} // namespace arena
