//
// Created by bobini on 07.10.23.
//

#include "ScoreDb.h"

namespace qml_components {

ScoreDb::ScoreDb(std::function<db::SqliteCppDb&()> scoreDb)
  : scoreDb(std::move(scoreDb))
{
}
auto
ScoreDb::getScoresForChart(QString sha256) -> QList<gameplay_logic::BmsResult*>
{
    auto& db = scoreDb();
    auto statement = db.createStatement("SELECT * "
                                        "FROM score "
                                        "WHERE sha256 = ?");
    statement.bind(1, sha256.toStdString());
    auto result =
      statement.executeAndGetAll<gameplay_logic::BmsResult::BmsResultDto>();
    auto scores = QList<gameplay_logic::BmsResult*>{};
    for (auto& row : result) {
        scores.append(gameplay_logic::BmsResult::load(row).release());
    }
    return scores;
}
auto
ScoreDb::getGaugeHistory(int64_t scoreId) -> gameplay_logic::BmsGaugeHistory*
{
    return gameplay_logic::BmsGaugeHistory::load(scoreDb(), scoreId).release();
}
auto
ScoreDb::getReplayData(int64_t scoreId) -> gameplay_logic::BmsReplayData*
{
    return gameplay_logic::BmsReplayData::load(scoreDb(), scoreId).release();
}
} // namespace qml_components