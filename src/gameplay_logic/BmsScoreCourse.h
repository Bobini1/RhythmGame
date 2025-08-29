//
// Created by PC on 28/07/2025.
//

#ifndef BMSSCORECOURSE_H
#define BMSSCORECOURSE_H

#include "BmsResultCourse.h"
#include "BmsGaugeHistory.h"
#include "BmsReplayData.h"
#include "BmsScore.h"
namespace gameplay_logic {

/**
 * @brief The result of playing a course.
 */
class BmsScoreCourse : public QObject
{
    Q_OBJECT

    /**
     * @brief The individual scores for each chart in the course.
     */
    Q_PROPERTY(QList<BmsScore*> scores READ getScores CONSTANT)
    /**
     * @brief The overall result of the course.
     */
    Q_PROPERTY(BmsResultCourse* result READ getResult CONSTANT)
    /**
     * @brief The combined gauge history of the course.
     */
    Q_PROPERTY(BmsGaugeHistory* gaugeHistory READ getGaugeHistory CONSTANT)
    /**
     * @brief The combined replay data of the course.
     */
    Q_PROPERTY(BmsReplayData* replayData READ getReplayData CONSTANT)
    BmsResultCourse* result;
    BmsGaugeHistory* gaugeHistory;
    BmsReplayData* replayData;
    QList<BmsScore*> scores;
  public:
    explicit BmsScoreCourse(std::unique_ptr<BmsResultCourse> resultCourse,
                            std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                            std::unique_ptr<BmsReplayData> replayData,
                            QList<BmsScore*> scores,
                            QObject* parent = nullptr);
    auto getScores() const -> const QList<BmsScore*>&;
    auto getResult() const -> BmsResultCourse*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
    auto getReplayData() const -> BmsReplayData*;
    void save(db::SqliteCppDb& db) const;
    static auto fromScores(std::unique_ptr<BmsResultCourse> resultCourse,
                           QList<BmsScore*> scores,
                           QObject* parent = nullptr) -> std::unique_ptr<BmsScoreCourse>;
};

}

#endif // BMSSCORECOURSE_H
