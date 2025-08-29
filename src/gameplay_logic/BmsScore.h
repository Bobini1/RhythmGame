//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSSCOREAFTERMATH_H
#define RHYTHMGAME_BMSSCOREAFTERMATH_H

#include "BmsGaugeHistory.h"
#include "BmsResult.h"
#include "BmsReplayData.h"
namespace gameplay_logic {
/**
 * @brief The result of playing a chart.
 */
class BmsScore final : public QObject
{
    Q_OBJECT

    /**
     * @brief The aggregated info about the score.
     */
    Q_PROPERTY(BmsResult* result READ getResult CONSTANT)
    /**
     * @brief The replay data of the score.
     */
    Q_PROPERTY(BmsReplayData* replayData READ getReplayData CONSTANT)
    /**
     * @brief The gauge history of the score.
     */
    Q_PROPERTY(BmsGaugeHistory* gaugeHistory READ getGaugeHistory CONSTANT)

    BmsResult* result;
    BmsReplayData* replayData;
    BmsGaugeHistory* gaugeHistory;

  public:
    explicit BmsScore(std::unique_ptr<BmsResult> result,
                      std::unique_ptr<BmsReplayData> replayData,
                      std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                      QObject* parent = nullptr);

    auto getResult() const -> BmsResult*;
    auto getReplayData() const -> BmsReplayData*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
    void save(db::SqliteCppDb& db) const;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCOREAFTERMATH_H
