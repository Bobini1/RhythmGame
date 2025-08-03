//
// Created by PC on 28/07/2025.
//

#include "BmsScoreCourse.h"
namespace gameplay_logic {
BmsScoreCourse::BmsScoreCourse(std::unique_ptr<BmsResultCourse> resultCourse,
                               std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                               std::unique_ptr<BmsReplayData> replayData,
                               QList<BmsScore*> scores,
                               QObject* parent)
  : QObject(parent)
  , result(resultCourse.release())
  , gaugeHistory(gaugeHistory.release())
  , replayData(replayData.release())
  , scores(std::move(scores))
{
    this->result->setParent(this);
    this->gaugeHistory->setParent(this);
    this->replayData->setParent(this);
    for (auto* score : this->scores) {
        score->setParent(this);
    }
}
auto
BmsScoreCourse::getScores() const -> const QList<BmsScore*>&
{
    return scores;
}
auto
BmsScoreCourse::getResult() const -> BmsResultCourse*
{
    return result;
}
auto
BmsScoreCourse::getGaugeHistory() const -> BmsGaugeHistory*
{
    return gaugeHistory;
}
auto
BmsScoreCourse::getReplayData() const -> BmsReplayData*
{
    return replayData;
}
void
BmsScoreCourse::save(db::SqliteCppDb& db) const
{
    // they should be saved already but just in case
    for (const auto* score : scores) {
        score->save(db);
    }
    result->save(db);
    // these are not saved! We generate them from scores
    // gaugeHistory->save(db);
    // replayData->save(db);
}
auto
BmsScoreCourse::fromScores(std::unique_ptr<BmsResultCourse> resultCourse,
                           QList<BmsScore*> scores,
                           QObject* parent) -> std::unique_ptr<BmsScoreCourse>
{
    auto hitEvents = QList<HitEvent>{};
    auto gaugeHistories = QHash<QString, QList<rules::GaugeHistoryEntry>>{};
    auto offset = 0LL;
    for (const auto* score : scores) {
        const auto* replayData = score->getReplayData();
        const auto& scoreHitEvents = replayData->getHitEvents();
        for (const auto& event : scoreHitEvents) {
            hitEvents.append(HitEvent(event.getColumn(),
                                      event.getNoteIndex(),
                                      event.getOffsetFromStart() + offset,
                                      event.getPointsOptional(),
                                      event.getAction(),
                                      event.getNoteRemoved()));
        }
        for (const auto& info : score->getGaugeHistory()->getGaugeInfo()) {
            auto& gaugeHistory = gaugeHistories[info.name];
            for (const auto& entry : info.gaugeHistory) {
                gaugeHistory.append(rules::GaugeHistoryEntry(
                  entry.getOffsetFromStart() + offset, entry.getGauge()));
            }
        }
        offset += score->getResult()->getLength();
    }
    auto infos = scores[0]->getGaugeHistory()->getGaugeInfo();
    for (auto& info : infos) {
        info.gaugeHistory = gaugeHistories[info.name];
    }
    auto gaugeHistory =
      std::make_unique<BmsGaugeHistory>(infos, resultCourse->getGuid());
    auto replayData = std::make_unique<BmsReplayData>(std::move(hitEvents),
                                                      resultCourse->getGuid());
    return std::make_unique<BmsScoreCourse>(std::move(resultCourse),
                                            std::move(gaugeHistory),
                                            std::move(replayData),
                                            std::move(scores),
                                            parent);
}
} // namespace gameplay_logic