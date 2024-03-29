//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSGAUGEHISTORY_H
#define RHYTHMGAME_BMSGAUGEHISTORY_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include "db/SqliteCppDb.h"
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

    Q_PROPERTY(QVariantMap gaugeHistory READ getGaugeHistory CONSTANT)
    Q_PROPERTY(QVariantMap gaugeInfo READ getGaugeInfo CONSTANT)
    QVariantMap gaugeHistory;
    QVariantMap gaugeInfo;

  public:
    /**
     * @brief Construct a new Bms Gauge History object
     * @param gaugeHistory A map of gauge name to QList of gauge history entries
     * @param parent QObject parent
     */
    explicit BmsGaugeHistory(QVariantMap gaugeHistory,
                             QVariantMap gaugeInfo,
                             QObject* parent = nullptr);
    BmsGaugeHistory() = default;

    /**
     * @brief Get the gauge history
     * @return A map of gauge name to QList of gauge history entries
     */
    auto getGaugeHistory() const -> QVariantMap;

    /**
     *
     * @brief Get info about gauges - maxGauge and threshold
     * @return A map of gauge name to BmsGaugeInfo
     */
    auto getGaugeInfo() const -> QVariantMap;

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
