//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSGAUGEHISTORY_H
#define RHYTHMGAME_BMSGAUGEHISTORY_H

#include "db/SqliteCppDb.h"
#include "rules/BmsGauge.h"
namespace gameplay_logic {

/**
 * @brief Metadata and history of a single gauge used in BMS scoring.
 */
class BmsGaugeInfo
{
    Q_GADGET
    /**
     * @brief The maximum possible value of the gauge.
     */
    Q_PROPERTY(double maxGauge MEMBER maxGauge CONSTANT)
    /**
     * @brief The threshold value of the gauge.
     * @details If the gauge is below this value at the end of the song,
     * the player fails.
     */
    Q_PROPERTY(double threshold MEMBER threshold CONSTANT)
    /**
     * @brief The name of the gauge.
     */
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    /**
     * @brief Whether the gauge is a course gauge.
     * @details Course gauges are only used in courses and have different
     * behavior. They give NOPLAY clears on the charts that constitute the
     * course.
     */
    Q_PROPERTY(bool courseGauge MEMBER courseGauge CONSTANT)
    /**
     * @brief The history of the gauge during gameplay.
     * @details A list of entries that record the value of the gauge at the
     * timestamps when it changed.
     */
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

/**
 * @brief The history of all gauges used in a BMS score.
 * @details Besides a list of gauge infos, it also contains the unique
 * identifier of the score it belongs to.
 */
class BmsGaugeHistory final : public QObject
{
    Q_OBJECT

    /**
     * @brief A list of gauge infos with entries and metadata.
     */
    Q_PROPERTY(QList<BmsGaugeInfo> gaugeInfo READ getGaugeInfo CONSTANT)
    /**
     * @brief The unique identifier for the score.
     */
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
