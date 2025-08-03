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
    Q_PROPERTY(double maxGauge MEMBER maxGauge CONSTANT)
    Q_PROPERTY(double threshold MEMBER threshold CONSTANT)
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(bool courseGauge MEMBER courseGauge CONSTANT)
    Q_PROPERTY(
      QList<rules::GaugeHistoryEntry> gaugeHistory MEMBER gaugeHistory CONSTANT)

  public:
    double maxGauge;
    double threshold;
    QString name;
    bool courseGauge;
    QList<rules::GaugeHistoryEntry> gaugeHistory;

    friend auto operator<<(QDataStream& stream,
                           const BmsGaugeInfo& gaugeHistory) -> QDataStream&;
    friend auto operator>>(QDataStream& stream, BmsGaugeInfo& gaugeHistory)
      -> QDataStream&;
};

class BmsGaugeHistory final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<BmsGaugeInfo> gaugeInfo READ getGaugeInfo CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    QList<BmsGaugeInfo> gaugeInfo;
    QString guid;

  public:
    /**
     * @brief Construct a new Bms Gauge History object
     * @param gaugeInfo A list of gauge infos with entries and metadata
     * @param guid The unique identifier for the score
     * @param parent QObject parent
     */
    explicit BmsGaugeHistory(QList<BmsGaugeInfo> gaugeInfo,
                             QString guid,
                             QObject* parent = nullptr);

    auto getGaugeInfo() const -> QList<BmsGaugeInfo>;

    auto getGuid() const -> QString;

    void save(db::SqliteCppDb& db) const;

    struct DTO
    {
        int64_t id;
        std::string scoreGuid;
        std::string gaugeInfo;
    };
    static auto load(const DTO& dto) -> std::unique_ptr<BmsGaugeHistory>;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSGAUGEHISTORY_H
