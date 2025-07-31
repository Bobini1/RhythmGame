//
// Created by PC on 17/07/2025.
//

#include "BmsResultCourse.h"

#include "resource_managers/Tables.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
auto
gameplay_logic::BmsResultCourse::load(const DTO& dto, QList<BmsScore*>& scores)
  -> std::unique_ptr<BmsResultCourse>
{
    auto constraints =
      QString::fromStdString(dto.constraints).split(' ', Qt::SkipEmptyParts);
    auto trophies = QList<resource_managers::Trophy>{};
    for (const auto& trophy :
         QJsonDocument::fromJson(QByteArray::fromStdString(dto.trophies))
           .array()) {
        trophies.append(resource_managers::Trophy::fromJson(trophy.toObject()));
    }
    return std::make_unique<BmsResultCourse>(
      QString::fromStdString(dto.guid),
      QString::fromStdString(dto.identifier),
      std::move(scores),
      QString::fromStdString(dto.clearType),
      dto.maxCombo,
      std::move(constraints),
      std::move(trophies),
      dto.gameVersion);
}
void
gameplay_logic::BmsResultCourse::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement = db.createStatement(
      "INSERT OR IGNORE INTO score_course "
      "(guid, identifier, score_guids, clear_type, max_combo, constraints, "
      "trophies, unix_timestamp, game_version) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    statement.bind(1, guid.toStdString());
    statement.bind(2, identifier.toStdString());
    QStringList scoreGuids;
    for (const auto* score : scores) {
        scoreGuids.append(score->getResult()->getGuid());
    }
    statement.bind(3, scoreGuids.join(' ').toStdString());
    statement.bind(4, clearType.toStdString());
    statement.bind(5, maxCombo);
    statement.bind(6, constraints.join(' ').toStdString());
    QJsonArray trophyArray;
    for (const auto& trophy : trophies) {
        trophyArray.append(trophy.toJson());
    }
    statement.bind(
      7,
      QJsonDocument(trophyArray).toJson(QJsonDocument::Compact).toStdString());
    statement.bind(
      8, scores.last() ? scores.last()->getResult()->getUnixTimestamp() : 0);
    statement.bind(9, static_cast<int64_t>(gameVersion));
    statement.execute();
}
gameplay_logic::BmsResultCourse::BmsResultCourse(
  QString guid,
  QString identifier,
  QList<BmsScore*> scores,
  QString clearType,
  int maxCombo,
  QStringList constraints,
  QList<resource_managers::Trophy> trophies,
  uint64_t gameVersion,
  QObject* parent)
  : QObject(parent)
  , identifier(std::move(identifier))
  , scores(std::move(scores))
  , clearType(std::move(clearType))
  , guid(std::move(guid))
  , maxCombo(maxCombo)
  , constraints(std::move(constraints))
  , trophies(std::move(trophies))
  , gameVersion(gameVersion)
{
}
auto
gameplay_logic::BmsResultCourse::getMaxPoints() const -> double
{
    return std::accumulate(
      scores.begin(), scores.end(), 0.0, [](double sum, const BmsScore* score) {
          return sum + score->getResult()->getMaxPoints();
      });
}
auto
gameplay_logic::BmsResultCourse::getMaxHits() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMaxHits();
      });
}
auto
gameplay_logic::BmsResultCourse::getNormalNoteCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getNormalNoteCount();
      });
}

auto
gameplay_logic::BmsResultCourse::getLnCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getLnCount();
      });
}
auto
gameplay_logic::BmsResultCourse::getMineCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMineCount();
      });
}
auto
gameplay_logic::BmsResultCourse::getPoints() const -> double
{
    return std::accumulate(
      scores.begin(), scores.end(), 0.0, [](double sum, const BmsScore* score) {
          return sum + score->getResult()->getPoints();
      });
}
auto
gameplay_logic::BmsResultCourse::getMaxCombo() const -> int
{
    return maxCombo;
}
auto
gameplay_logic::BmsResultCourse::getJudgementCounts() const -> QList<int>
{
    QList<int> counts(magic_enum::enum_count<Judgement>(), 0);
    for (const auto* score : scores) {
        const auto& scoreCounts = score->getResult()->getJudgementCounts();
        for (int i = 0; i < counts.size(); ++i) {
            counts[i] += scoreCounts[i];
        }
    }
    return counts;
}

auto
gameplay_logic::BmsResultCourse::getMineHits() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMineHits();
      });
}
auto
gameplay_logic::BmsResultCourse::getClearType() const -> const QString&
{
    return clearType;
}
auto
gameplay_logic::BmsResultCourse::getRandomSequence() -> QList<qint64>
{
    auto seq = QList<qint64>{};
    for (const auto* score : scores) {
        const auto& scoreSeq = score->getResult()->getRandomSequence();
        seq.append(scoreSeq);
    }
    return seq;
}
auto
gameplay_logic::BmsResultCourse::getUnixTimestamp() const -> int64_t
{
    if (scores.isEmpty()) {
        return 0;
    }
    return scores.last()->getResult()->getUnixTimestamp();
}
auto
gameplay_logic::BmsResultCourse::getGuid() const -> QString
{
    return guid;
}
auto
gameplay_logic::BmsResultCourse::getSha256() const -> QString
{
    QString sha256;
    for (const auto* score : scores) {
        if (!sha256.isEmpty()) {
            sha256 += ' ';
        }
        sha256 += score->getResult()->getSha256();
    }
    return sha256;
}
auto
gameplay_logic::BmsResultCourse::getMd5() const -> QString
{
    QString md5;
    for (const auto* score : scores) {
        if (!md5.isEmpty()) {
            md5 += ' ';
        }
        md5 += score->getResult()->getMd5();
    }
    return md5;
}
auto
gameplay_logic::BmsResultCourse::getNoteOrderAlgorithm() const
  -> resource_managers::NoteOrderAlgorithm
{
    return scores.isEmpty()
             ? resource_managers::NoteOrderAlgorithm::Normal
             : scores.first()->getResult()->getNoteOrderAlgorithm();
}

auto
gameplay_logic::BmsResultCourse::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return scores.isEmpty()
             ? resource_managers::NoteOrderAlgorithm::Normal
             : scores.first()->getResult()->getNoteOrderAlgorithmP2();
}
auto
gameplay_logic::BmsResultCourse::getGameVersion() const -> uint64_t
{
    return gameVersion;
}
auto
gameplay_logic::BmsResultCourse::getScores() const -> QList<BmsScore*>
{
    return scores;
}
auto
gameplay_logic::BmsResultCourse::getConstraints() const -> QStringList
{
    return constraints;
}
auto
gameplay_logic::BmsResultCourse::getTrophies() const -> QVariantList
{
    QVariantList trophyList;
    for (const auto& trophy : trophies) {
        trophyList.append(QVariant::fromValue(trophy));
    }
    return trophyList;
}
auto
gameplay_logic::BmsResultCourse::getIdentifier() const -> QString
{
    return identifier;
}
auto
gameplay_logic::BmsResultCourse::getLength() const -> int64_t
{
    return std::accumulate(
      scores.begin(), scores.end(), 0LL, [](int64_t sum, const BmsScore* score) {
          return sum + score->getResult()->getLength();
      });
}