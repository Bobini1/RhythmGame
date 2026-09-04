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
    enum class Source
    {
        RhythmGame = 0,
        Lr2 = 1,
        Beatoraja = 2,
        Bokutachi = 3
    };
    Q_ENUM(Source)

    enum class LongNoteMode
    {
        Ln = 0,
        Cn = 1,
        Hcn = 2
    };

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
     * @brief Whether this is an incomplete score imported without the
     * data
     * produced by a local play.
     */
    Q_PROPERTY(Source source READ getSource CONSTANT)
    Q_PROPERTY(QString sourceName READ getSourceName CONSTANT)
    Q_PROPERTY(bool imported READ isImported CONSTANT)
    /**
     * @brief The state of score submission to the online server.
     */
    Q_PROPERTY(SubmissionState submissionState MEMBER submissionState NOTIFY
                 submissionStateChanged)

    BmsResult* result;
    BmsReplayData* replayData;
    BmsGaugeHistory* gaugeHistory;
    Source source{ Source::RhythmGame };
    LongNoteMode longNoteMode{ LongNoteMode::Ln };
    SubmissionState submissionState{ SubmissionState::NotSubmitted };

  public:
    explicit BmsScore(std::unique_ptr<BmsResult> result,
                      std::unique_ptr<BmsReplayData> replayData,
                      std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                      QObject* parent = nullptr);
    BmsScore(std::unique_ptr<BmsResult> result,
             std::unique_ptr<BmsReplayData> replayData,
             std::unique_ptr<BmsGaugeHistory> gaugeHistory,
             QObject* parent,
             Source source,
             LongNoteMode longNoteMode);

    /**
     * @brief Construct an incomplete score from imported aggregate
     * result
     * data.
     * @details Imported scores intentionally have no
     * replay or gauge history.
     */
    static auto fromImportedResult(std::unique_ptr<BmsResult> result,
                                   Source source,
                                   LongNoteMode longNoteMode = LongNoteMode::Ln,
                                   QObject* parent = nullptr)
      -> std::unique_ptr<BmsScore>;

    auto getResult() const -> BmsResult*;
    auto getReplayData() const -> BmsReplayData*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
    auto getSource() const -> Source;
    auto getLongNoteMode() const -> LongNoteMode;
    auto getSourceName() const -> QString;
    auto isImported() const -> bool;
    void setSubmissionState(SubmissionState newState);
    auto getSubmissionState() const -> SubmissionState;
    void save(db::SqliteCppDb& db) const;
  signals:
    void submissionStateChanged();
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCOREAFTERMATH_H
