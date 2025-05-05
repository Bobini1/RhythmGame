//
// Created by bobini on 30.09.23.
//

#include "BmsScoreAftermath.h"
gameplay_logic::BmsScoreAftermath::BmsScoreAftermath(
  resource_managers::Profile* profile,
  std::unique_ptr<BmsResult> result,
  std::unique_ptr<BmsReplayData> replayData,
  std::unique_ptr<BmsGaugeHistory> gaugeHistory,
  QObject* parent)
  : QObject(parent)
  , profile(profile)
  , result(result.release())
  , replayData(replayData.release())
  , gaugeHistory(gaugeHistory.release())
{
    this->result->setParent(this);
    this->replayData->setParent(this);
    this->gaugeHistory->setParent(this);
}
auto
gameplay_logic::BmsScoreAftermath::getProfile() const
  -> resource_managers::Profile*
{
    return profile;
}
auto
gameplay_logic::BmsScoreAftermath::getResult() const -> BmsResult*
{
    return result;
}
auto
gameplay_logic::BmsScoreAftermath::getReplayData() const -> BmsReplayData*
{
    return replayData;
}
auto
gameplay_logic::BmsScoreAftermath::getGaugeHistory() const -> BmsGaugeHistory*
{
    return gaugeHistory;
}
