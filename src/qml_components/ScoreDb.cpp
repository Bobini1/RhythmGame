//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

namespace qml_components {

ScoreDb::ScoreDb(db::SqliteCppDb* scoreDb)
  : scoreDb(scoreDb)
{
}
auto
ScoreDb::getScoresForMd5(const QString& md5) const
  -> QList<gameplay_logic::BmsResult*>
{
    auto statement = scoreDb->createStatement("SELECT * "
                                              "FROM score "
                                              "WHERE md5 = ?");
    statement.bind(1, md5.toStdString());
    const auto result =
      statement.executeAndGetAll<gameplay_logic::BmsResult::BmsResultDto>();
    auto scores = QList<gameplay_logic::BmsResult*>{};
    for (const auto& row : result) {
        scores.append(gameplay_logic::BmsResult::load(row).release());
    }
    return scores;
}
auto
ScoreDb::getGaugeHistory(int64_t scoreId) -> gameplay_logic::BmsGaugeHistory*
{
    return gameplay_logic::BmsGaugeHistory::load(*scoreDb, scoreId).release();
}
auto
ScoreDb::getReplayData(int64_t scoreId) -> gameplay_logic::BmsReplayData*
{
    return gameplay_logic::BmsReplayData::load(*scoreDb, scoreId).release();
}
int
ScoreDb::getTotalScoreCount()
{
    auto statement = scoreDb->createStatement("SELECT COUNT(*) FROM score");
    return statement.executeAndGet<int>().value_or(0);
}
} // namespace qml_components