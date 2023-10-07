//
// Created by bobini on 27.09.23.
//

#include "BmsResult.h"
#include <QIODevice>
#include <QDataStream>
auto
gameplay_logic::BmsResult::getMaxPoints() const -> double
{
    return maxPoints;
}
auto
gameplay_logic::BmsResult::getMaxHits() const -> int
{
    return maxHits;
}
auto
gameplay_logic::BmsResult::getPoints() const -> double
{
    return points;
}
auto
gameplay_logic::BmsResult::getMaxCombo() const -> int
{
    return maxCombo;
}
auto
gameplay_logic::BmsResult::getJudgementCounts() const -> QList<int>
{
    return judgementCounts;
}
auto
gameplay_logic::BmsResult::getClearType() const -> QString
{
    return clearType;
}
gameplay_logic::BmsResult::BmsResult(double maxPoints,
                                     int maxHits,
                                     QString clearType,
                                     QList<int> judgementCounts,
                                     double points,
                                     int maxCombo,
                                     QObject* parent)
  : QObject(parent)
  , maxPoints(maxPoints)
  , maxHits(maxHits)
  , clearType(std::move(clearType))
  , judgementCounts(std::move(judgementCounts))
  , points(points)
  , maxCombo(maxCombo)
{
}
auto
gameplay_logic::BmsResult::save(db::SqliteCppDb& db,
                                support::Sha256 sha256) const -> int64_t
{
    auto statement =
      db.createStatement("INSERT INTO score ("
                         "max_points, "
                         "max_hits, "
                         "clear_type, "
                         "points, "
                         "max_combo, "
                         "poor, "
                         "empty_poor, "
                         "bad, "
                         "good, "
                         "great, "
                         "perfect,"
                         "sha256"
                         ")"
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    statement.bind(1, maxPoints);
    statement.bind(2, maxHits);
    statement.bind(3, clearType.toStdString());
    statement.bind(4, points);
    statement.bind(5, maxCombo);
    statement.bind(6, judgementCounts[static_cast<int>(Judgement::Poor)]);
    statement.bind(7, judgementCounts[static_cast<int>(Judgement::EmptyPoor)]);
    statement.bind(8, judgementCounts[static_cast<int>(Judgement::Bad)]);
    statement.bind(9, judgementCounts[static_cast<int>(Judgement::Good)]);
    statement.bind(10, judgementCounts[static_cast<int>(Judgement::Great)]);
    statement.bind(11, judgementCounts[static_cast<int>(Judgement::Perfect)]);
    statement.bind(12, sha256);
    return statement.execute();
}
auto
gameplay_logic::BmsResult::load(const BmsResultDto& dto)
  -> std::unique_ptr<BmsResult>
{
    auto judgementCounts = QList<int>(magic_enum::enum_count<Judgement>());
    judgementCounts[static_cast<int>(Judgement::Poor)] = dto.poorCount;
    judgementCounts[static_cast<int>(Judgement::EmptyPoor)] =
      dto.emptyPoorCount;
    judgementCounts[static_cast<int>(Judgement::Bad)] = dto.badCount;
    judgementCounts[static_cast<int>(Judgement::Good)] = dto.goodCount;
    judgementCounts[static_cast<int>(Judgement::Great)] = dto.greatCount;
    judgementCounts[static_cast<int>(Judgement::Perfect)] = dto.perfectCount;
    return std::make_unique<BmsResult>(dto.maxPoints,
                                       dto.maxHits,
                                       QString::fromStdString(dto.clearType),
                                       judgementCounts,
                                       dto.points,
                                       dto.maxCombo);
}
auto
gameplay_logic::BmsResult::setId(int64_t newId) -> void
{
    id = newId;
}
auto
gameplay_logic::BmsResult::getId() const -> int64_t
{
    return id;
}
