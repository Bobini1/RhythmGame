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
  public:
    enum class SubmissionState
    {
        NotSubmitted,
        NotSubmitting,
        Submitting,
        Submitted,
        Failed,
        Duplicate
    };
    Q_ENUM(SubmissionState)
  private:
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
    /**
     * @brief The state of score submission to the online server.
     */
    Q_PROPERTY(SubmissionState submissionState MEMBER submissionState NOTIFY
                 submissionStateChanged)

    BmsResult* result;
    BmsReplayData* replayData;
    BmsGaugeHistory* gaugeHistory;
    SubmissionState submissionState{ SubmissionState::NotSubmitted };

  public:
    explicit BmsScore(std::unique_ptr<BmsResult> result,
                      std::unique_ptr<BmsReplayData> replayData,
                      std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                      QObject* parent = nullptr);

    auto getResult() const -> BmsResult*;
    auto getReplayData() const -> BmsReplayData*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
    void setSubmissionState(SubmissionState newState);
    auto getSubmissionState() const -> SubmissionState;
    void save(db::SqliteCppDb& db) const;
  signals:
    void submissionStateChanged();
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCOREAFTERMATH_H
