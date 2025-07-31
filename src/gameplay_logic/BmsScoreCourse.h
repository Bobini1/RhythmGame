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
class BmsScoreCourse : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<BmsScore*> scores READ getScores CONSTANT)
    Q_PROPERTY(BmsResultCourse* result READ getResult CONSTANT)
    Q_PROPERTY(BmsGaugeHistory* gaugeHistory READ getGaugeHistory CONSTANT)
    Q_PROPERTY(BmsReplayData* replayData READ getReplayData CONSTANT)
    BmsResultCourse* result;
    BmsGaugeHistory* gaugeHistory;
    BmsReplayData* replayData;
    QList<BmsScore*> scores;
  public:
    explicit BmsScoreCourse(std::unique_ptr<BmsResultCourse> result,
                            std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                            std::unique_ptr<BmsReplayData> replayData,
                            QList<BmsScore*> scores,
                            QObject* parent = nullptr);
    auto getScores() const -> const QList<BmsScore*>&;
    auto getResult() const -> BmsResultCourse*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
    auto getReplayData() const -> BmsReplayData*;
    void save(db::SqliteCppDb& db) const;
    static auto fromScores(std::unique_ptr<BmsResultCourse> result,
                           QList<BmsScore*> scores,
                           QObject* parent = nullptr) -> std::unique_ptr<BmsScoreCourse>;
};

}

#endif // BMSSCORECOURSE_H
