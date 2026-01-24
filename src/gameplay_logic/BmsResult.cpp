//
// Created by bobini on 27.09.23.
//

#include "BmsResult.h"

#include "support/Version.h"

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
gameplay_logic::BmsResult::getClearType() const -> const QString&
{
    return clearType;
}
auto
gameplay_logic::BmsResult::getRandomSequence() -> const QList<qint64>&
{
    return randomSequence;
}
gameplay_logic::BmsResult::BmsResult(
  double maxPoints,
  int maxHits,
  int normalNoteCount,
  int lnCount,
  int bssCount,
  int mineCount,
  QString clearType,
  QList<int> judgementCounts,
  int mineHits,
  double points,
  int maxCombo,
  int64_t unixTimestamp,
  int64_t length,
  QList<qint64> randomSequence,
  uint64_t randomSeed,
  resource_managers::NoteOrderAlgorithm noteOrderAlgorithm,
  resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2,
  resource_managers::DpOptions dpOptions,
  QString guid,
  QString sha256,
  QString md5,
  uint64_t gameVersion,
  QObject* parent)
  : QObject(parent)
  , maxPoints(maxPoints)
  , maxHits(maxHits)
  , normalNoteCount(normalNoteCount)
  , lnCount(lnCount)
  , bssCount(bssCount)
  , mineCount(mineCount)
  , clearType(std::move(clearType))
  , judgementCounts(std::move(judgementCounts))
  , randomSequence(std::move(randomSequence))
  , guid(std::move(guid))
  , sha256(std::move(sha256))
  , md5(std::move(md5))
  , mineHits(mineHits)
  , points(points)
  , maxCombo(maxCombo)
  , unixTimestamp(unixTimestamp)
  , randomSeed(randomSeed)
  , length(length)
  , noteOrderAlgorithm(noteOrderAlgorithm)
  , noteOrderAlgorithmP2(noteOrderAlgorithmP2)
  , dpOptions(dpOptions)
  , gameVersion(gameVersion)
{
}
void
gameplay_logic::BmsResult::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement =
      db.createStatement("INSERT OR IGNORE INTO score ("
                         "max_points, "
                         "max_hits, "
                         "normal_note_count, "
                         "ln_count, "
                         "bss_count, "
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
                         "guid,"
                         "sha256,"
                         "md5,"
                         "unix_timestamp,"
                         "length,"
                         "random_sequence,"
                         "random_seed,"
                         "note_order_algorithm,"
                         "note_order_algorithm_p2,"
                         "dp_options, "
                         "game_version"
                         ")"
                         "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
                         "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    statement.bind(1, maxPoints);
    statement.bind(2, maxHits);
    statement.bind(3, normalNoteCount);
    statement.bind(4, lnCount);
    statement.bind(5, bssCount);
    statement.bind(6, mineCount);
    statement.bind(7, clearType.toStdString());
    statement.bind(8, points);
    statement.bind(9, maxCombo);
    statement.bind(10, judgementCounts[static_cast<int>(Judgement::Poor)]);
    statement.bind(11, judgementCounts[static_cast<int>(Judgement::EmptyPoor)]);
    statement.bind(12, judgementCounts[static_cast<int>(Judgement::Bad)]);
    statement.bind(13, judgementCounts[static_cast<int>(Judgement::Good)]);
    statement.bind(14, judgementCounts[static_cast<int>(Judgement::Great)]);
    statement.bind(15, judgementCounts[static_cast<int>(Judgement::Perfect)]);
    statement.bind(16, mineHits);
    statement.bind(17, guid.toStdString());
    statement.bind(18, sha256.toStdString());
    statement.bind(19, md5.toStdString());
    statement.bind(20, unixTimestamp);
    statement.bind(21, length);
    auto randomSequenceCompressed = support::compress(randomSequence);
    statement.bind(
      22, randomSequenceCompressed.data(), randomSequenceCompressed.size());
    statement.bind(23, static_cast<int64_t>(randomSeed));
    statement.bind(24, static_cast<int>(noteOrderAlgorithm));
    statement.bind(25, static_cast<int>(noteOrderAlgorithmP2));
    statement.bind(26, static_cast<int>(dpOptions));
    statement.bind(27, static_cast<int64_t>(gameVersion));
    statement.execute();
}
auto
gameplay_logic::BmsResult::load(const DTO& dto) -> std::unique_ptr<BmsResult>
{
    auto judgementCounts = QList<int>(magic_enum::enum_count<Judgement>());
    judgementCounts[static_cast<int>(Judgement::Poor)] = dto.poorCount;
    judgementCounts[static_cast<int>(Judgement::EmptyPoor)] =
      dto.emptyPoorCount;
    judgementCounts[static_cast<int>(Judgement::Bad)] = dto.badCount;
    judgementCounts[static_cast<int>(Judgement::Good)] = dto.goodCount;
    judgementCounts[static_cast<int>(Judgement::Great)] = dto.greatCount;
    judgementCounts[static_cast<int>(Judgement::Perfect)] = dto.perfectCount;
    auto randomSequence = support::decompress<QList<qint64>>(
      QByteArray::fromStdString(dto.randomSequence));
    auto result = std::make_unique<BmsResult>(
      dto.maxPoints,
      dto.maxHits,
      dto.normalNoteCount,
      dto.lnCount,
      dto.bssCount,
      dto.mineCount,
      QString::fromStdString(dto.clearType),
      judgementCounts,
      dto.mineHits,
      dto.points,
      dto.maxCombo,
      dto.unixTimestamp,
      dto.length,
      randomSequence,
      dto.randomSeed,
      static_cast<resource_managers::NoteOrderAlgorithm>(
        dto.noteOrderAlgorithm),
      static_cast<resource_managers::NoteOrderAlgorithm>(
        dto.noteOrderAlgorithmP2),
      static_cast<resource_managers::DpOptions>(dto.dpOptions),
      QString::fromStdString(dto.guid),
      QString::fromStdString(dto.sha256),
      QString::fromStdString(dto.md5),
      dto.gameVersion);
    result->unixTimestamp = dto.unixTimestamp;
    return result;
}
auto
gameplay_logic::BmsResult::getUnixTimestamp() const -> int64_t
{
    return unixTimestamp;
}
auto
gameplay_logic::BmsResult::getLength() const -> int64_t
{
    return length;
}
auto
gameplay_logic::BmsResult::getGuid() const -> QString
{
    return guid;
}
auto
gameplay_logic::BmsResult::getSha256() const -> QString
{
    return sha256;
}
auto
gameplay_logic::BmsResult::getMd5() const -> QString
{
    return md5;
}
auto
gameplay_logic::BmsResult::getRandomSeed() const -> uint64_t
{
    return randomSeed;
}
auto
gameplay_logic::BmsResult::getNoteOrderAlgorithm() const
  -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithm;
}
auto
gameplay_logic::BmsResult::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return noteOrderAlgorithmP2;
}
auto
gameplay_logic::BmsResult::getDpOptions() const -> resource_managers::DpOptions
{
    return dpOptions;
}
auto
gameplay_logic::BmsResult::getGameVersion() const -> uint64_t
{
    return gameVersion;
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
gameplay_logic::BmsResult::getBssCount() const -> int
{
    return bssCount;
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
