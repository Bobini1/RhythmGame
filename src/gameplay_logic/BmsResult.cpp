//
// Created by bobini on 27.09.23.
//

#include "BmsResult.h"
#include <QIODevice>
#include <QDataStream>
#include <QDateTime>
#include <support/Compress.h>
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
auto
gameplay_logic::BmsResult::getRandomSequence() -> QList<int64_t>
{
    return randomSequence;
}
gameplay_logic::BmsResult::BmsResult(double maxPoints,
                                     int maxHits,
                                     int normalNoteCount,
                                     int lnCount,
                                     int mineCount,
                                     QString clearType,
                                     QList<int> judgementCounts,
                                     int mineHits,
                                     double points,
                                     int maxCombo,
                                     QList<int64_t> randomSequence,
                                     support::Sha256 sha256,
                                     QObject* parent)
  : QObject(parent)
  , maxPoints(maxPoints)
  , maxHits(maxHits)
  , normalNoteCount(normalNoteCount)
  , lnCount(lnCount)
  , mineCount(mineCount)
  , clearType(std::move(clearType))
  , judgementCounts(std::move(judgementCounts))
  , mineHits(mineHits)
  , points(points)
  , maxCombo(maxCombo)
  , unixTimestamp(QDateTime::currentSecsSinceEpoch())
  , randomSequence(std::move(randomSequence))
  , sha256(std::move(sha256))
{
}
auto
gameplay_logic::BmsResult::save(db::SqliteCppDb& db) const -> int64_t
{
    auto statement = db.createStatement(
      "INSERT INTO score ("
      "max_points, "
      "max_hits, "
      "normal_note_count, "
      "ln_count, "
      "mine_count, "
      "clear_type, "
      "points, "
      "max_combo, "
      "poor, "
      "empty_poor, "
      "bad, "
      "good, "
      "great, "
      "perfect,"
      "mine_hits,"
      "sha256,"
      "unix_timestamp,"
      "random_sequence"
      ")"
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    auto randomSequenceCompressed = support::compress(randomSequence);
    statement.bind(1, maxPoints);
    statement.bind(2, maxHits);
    statement.bind(3, normalNoteCount);
    statement.bind(4, lnCount);
    statement.bind(5, mineCount);
    statement.bind(6, clearType.toStdString());
    statement.bind(7, points);
    statement.bind(8, maxCombo);
    statement.bind(9, judgementCounts[static_cast<int>(Judgement::Poor)]);
    statement.bind(10, judgementCounts[static_cast<int>(Judgement::EmptyPoor)]);
    statement.bind(11, judgementCounts[static_cast<int>(Judgement::Bad)]);
    statement.bind(12, judgementCounts[static_cast<int>(Judgement::Good)]);
    statement.bind(13, judgementCounts[static_cast<int>(Judgement::Great)]);
    statement.bind(14, judgementCounts[static_cast<int>(Judgement::Perfect)]);
    statement.bind(15, mineHits);
    statement.bind(16, sha256);
    statement.bind(17, unixTimestamp);
    statement.bind(18, randomSequenceCompressed.data(), randomSequenceCompressed.size());
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
    auto result =
      std::make_unique<BmsResult>(dto.maxPoints,
                                  dto.maxHits,
                                  dto.normalNoteCount,
                                  dto.lnCount,
                                  dto.mineCount,
                                  QString::fromStdString(dto.clearType),
                                  judgementCounts,
                                  dto.mineHits,
                                  dto.points,
                                  dto.maxCombo,
                                  support::decompress<QList<int64_t>>(QByteArray::fromStdString(dto.randomSequence)),
                                  dto.sha256);
    result->setId(dto.id);
    result->unixTimestamp = dto.unixTimestamp;
    return result;
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
auto
gameplay_logic::BmsResult::getUnixTimestamp() const -> int64_t
{
    return unixTimestamp;
}
auto
gameplay_logic::BmsResult::getNormalNoteCount() const -> int
{
    return normalNoteCount;
}
auto
gameplay_logic::BmsResult::getLnCount() const -> int
{
    return lnCount;
}
auto
gameplay_logic::BmsResult::getMineCount() const -> int
{
    return mineCount;
}
auto
gameplay_logic::BmsResult::getMineHits() const -> int
{
    return mineHits;
}
