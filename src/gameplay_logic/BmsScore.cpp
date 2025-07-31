//
// Created by bobini on 30.09.23.
//

#include "BmsScore.h"
gameplay_logic::BmsScore::BmsScore(
  std::unique_ptr<BmsResult> result,
  std::unique_ptr<BmsReplayData> replayData,
  std::unique_ptr<BmsGaugeHistory> gaugeHistory,
  QObject* parent)
  : QObject(parent)
  , result(result.release())
  , replayData(replayData.release())
  , gaugeHistory(gaugeHistory.release())
{
    this->result->setParent(this);
    this->replayData->setParent(this);
    this->gaugeHistory->setParent(this);
}
auto
gameplay_logic::BmsScore::getResult() const -> BmsResult*
{
    return result;
}
auto
gameplay_logic::BmsScore::getReplayData() const -> BmsReplayData*
{
    return replayData;
}
auto
gameplay_logic::BmsScore::getGaugeHistory() const -> BmsGaugeHistory*
{
    return gaugeHistory;
}
void
gameplay_logic::BmsScore::save(db::SqliteCppDb& db) const
{
    result->save(db);
    replayData->save(db);
    gaugeHistory->save(db);
}
