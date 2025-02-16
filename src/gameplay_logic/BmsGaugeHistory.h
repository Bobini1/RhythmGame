//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSGAUGEHISTORY_H
#define RHYTHMGAME_BMSGAUGEHISTORY_H

#include "db/SqliteCppDb.h"
#include "rules/BmsGauge.h"
namespace gameplay_logic {

class BmsGaugeInfo
{
    Q_GADGET
    Q_PROPERTY(double maxGauge MEMBER maxGauge)
    Q_PROPERTY(double threshold MEMBER threshold)

  public:
    double maxGauge;
    double threshold;

    friend auto operator<<(QDataStream& stream,
                           const BmsGaugeInfo& gaugeHistory) -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsGaugeInfo& gaugeHistory)
      -> QDataStream&;
};

class BmsGaugeHistory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap gaugeHistory READ getGaugeHistoryVariant CONSTANT)
    Q_PROPERTY(QVariantMap gaugeInfo READ getGaugeInfoVariant CONSTANT)
    QHash<QString, QList<rules::GaugeHistoryEntry>> gaugeHistory;
    QHash<QString, BmsGaugeInfo> gaugeInfo;

  public:
    /**
     * @brief Construct a new Bms Gauge History object
     * @param gaugeHistory A map of gauge name to QList of gauge history entries
     * @param parent QObject parent
     */
    explicit BmsGaugeHistory(
      QHash<QString, QList<rules::GaugeHistoryEntry>> gaugeHistory,
      QHash<QString, BmsGaugeInfo> gaugeInfo,
      QObject* parent = nullptr);
    BmsGaugeHistory() = default;

    /**
     * @brief Get the gauge history
     * @return A map of gauge name to QList of gauge history entries
     */
    auto getGaugeHistory() const
      -> QHash<QString, QList<rules::GaugeHistoryEntry>>;

    /**
     *
     * @brief Get info about gauges - maxGauge and threshold
     * @return A map of gauge name to BmsGaugeInfo
     */
    auto getGaugeInfo() const -> QHash<QString, BmsGaugeInfo>;

    auto getGaugeHistoryVariant() const -> QVariantMap;

    auto getGaugeInfoVariant() const -> QVariantMap;

    friend auto operator<<(QDataStream& stream,
                           const BmsGaugeHistory& gaugeHistory) -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsGaugeHistory& gaugeHistory)
      -> QDataStream&;

    void save(db::SqliteCppDb& db, int64_t scoreId);
    static auto load(db::SqliteCppDb& db, int64_t scoreId)
      -> std::unique_ptr<BmsGaugeHistory>;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAUGEHISTORY_H
