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

class BmsGaugeHistory final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QVariantMap gaugeHistory READ getGaugeHistoryVariant CONSTANT)
    Q_PROPERTY(QVariantMap gaugeInfo READ getGaugeInfoVariant CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    QHash<QString, QList<rules::GaugeHistoryEntry>> gaugeHistory;
    QHash<QString, BmsGaugeInfo> gaugeInfo;
    QString guid;

  public:
    /**
     * @brief Construct a new Bms Gauge History object
     * @param gaugeHistory A map of gauge name to QList of gauge history entries
     * @param gaugeInfo A map of gauge name to BmsGaugeInfo
     * @param guid The unique identifier for the score
     * @param parent QObject parent
     */
    explicit BmsGaugeHistory(
      QHash<QString, QList<rules::GaugeHistoryEntry>> gaugeHistory,
      QHash<QString, BmsGaugeInfo> gaugeInfo,
      QString guid,
      QObject* parent = nullptr);

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

    auto getGuid() const -> QString;

    void save(db::SqliteCppDb& db);
    static auto load(db::SqliteCppDb& db, const QString& guid)
      -> std::unique_ptr<BmsGaugeHistory>;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAUGEHISTORY_H
