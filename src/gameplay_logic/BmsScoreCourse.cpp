//
// Created by PC on 28/07/2025.
//

#include "BmsScoreCourse.h"
namespace gameplay_logic {
auto
BmsScoreCourse::clone(QObject* parent) const -> std::unique_ptr<BmsScoreCourse>
{
    auto owner = QObject{};
    QList<BmsScore*> stages;
    for (auto* stage : scores)
        stages.append(stage->clone(&owner).release());
    return std::make_unique<BmsScoreCourse>(
      result->clone(),
      gaugeHistory ? std::make_unique<BmsGaugeHistory>(
                       gaugeHistory->getGaugeInfo(), gaugeHistory->getGuid())
                   : nullptr,
      replayData ? replayData->clone() : nullptr,
      stages,
      parent);
}

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
    if (this->gaugeHistory)
        this->gaugeHistory->setParent(this);
    if (this->replayData)
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
    QList<BmsScore::PreparedData> prepared;
    prepared.reserve(scores.size());
    for (const auto* score : scores) {
        prepared.push_back(score->prepareSave());
    }
    auto transaction = db.transaction();
    // they should be saved already but just in case
    for (qsizetype i = 0; i < scores.size(); ++i) {
        scores[i]->save(db, prepared[i]);
    }
    result->save(db);
    transaction.commit();
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
    bool hasReplay = !scores.isEmpty();
    bool hasGauges = !scores.isEmpty();
    for (const auto* score : scores) {
        const auto* replayData = score->getReplayData();
        hasReplay = hasReplay && replayData;
        hasGauges = hasGauges && score->getGaugeHistory();
        const auto& scoreHitEvents =
          replayData ? replayData->getHitEvents() : QList<HitEvent>{};
        for (const auto& event : scoreHitEvents) {
            hitEvents.append(HitEvent(event.getColumn(),
                                      event.getKeyOptional(),
                                      event.getNoteIndex(),
                                      event.getOffsetFromStart() + offset,
                                      event.getPointsOptional(),
                                      event.getAction(),
                                      event.getNoteRemoved()));
        }
        for (const auto& info : score->getGaugeHistory()
                                  ? score->getGaugeHistory()->getGaugeInfo()
                                  : QList<BmsGaugeInfo>{}) {
            auto& gaugeHistory = gaugeHistories[info.name];
            for (const auto& entry : info.gaugeHistory) {
                gaugeHistory.append(rules::GaugeHistoryEntry(
                  entry.getOffsetFromStart() + offset, entry.getGauge()));
            }
        }
        offset += score->getResult()->getLength();
    }
    auto infos = hasGauges ? scores[0]->getGaugeHistory()->getGaugeInfo()
                           : QList<BmsGaugeInfo>{};
    for (auto& info : infos) {
        info.gaugeHistory = gaugeHistories[info.name];
    }
    auto gaugeHistory =
      hasGauges
        ? std::make_unique<BmsGaugeHistory>(infos, resultCourse->getGuid())
        : nullptr;
    auto replayData =
      hasReplay ? std::make_unique<BmsReplayData>(std::move(hitEvents),
                                                  resultCourse->getGuid())
                : nullptr;
    return std::make_unique<BmsScoreCourse>(std::move(resultCourse),
                                            std::move(gaugeHistory),
                                            std::move(replayData),
                                            std::move(scores),
                                            parent);
}
} // namespace gameplay_logic
