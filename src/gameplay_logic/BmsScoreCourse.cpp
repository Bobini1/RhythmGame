//
// Created by PC on 17/07/2025.
//

#include "BmsScoreCourse.h"

#include "resource_managers/Tables.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
auto
gameplay_logic::BmsScoreCourse::load(const DTO& dto, QList<BmsScore*>& scores)
  -> std::unique_ptr<BmsScoreCourse>
{
    auto constraints =
      QString::fromStdString(dto.constraints).split(' ', Qt::SkipEmptyParts);
    auto trophies = QList<resource_managers::Trophy>{};
    for (const auto& trophy :
         QJsonDocument::fromJson(QByteArray::fromStdString(dto.trophies))
           .array()) {
        trophies.append(resource_managers::Trophy::fromJson(trophy.toObject()));
    }
    return std::make_unique<BmsScoreCourse>(
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
gameplay_logic::BmsScoreCourse::save(db::SqliteCppDb& db) const
{
    auto statement = db.createStatement(
      "INSERT OR REPLACE INTO score_course "
      "(guid, identifier, score_guids, clear_type, max_combo, constraints, "
      "trophies, game_version) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
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
    statement.bind(8, static_cast<int64_t>(gameVersion));
}
gameplay_logic::BmsScoreCourse::BmsScoreCourse(
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
    for (auto* score : this->scores) {
        score->setParent(this);
    }
}
auto
gameplay_logic::BmsScoreCourse::getMaxPoints() const -> double
{
    return std::accumulate(
      scores.begin(), scores.end(), 0.0, [](double sum, const BmsScore* score) {
          return sum + score->getResult()->getPoints();
      });
}
auto
gameplay_logic::BmsScoreCourse::getMaxHits() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMaxHits();
      });
}
auto
gameplay_logic::BmsScoreCourse::getNormalNoteCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getNormalNoteCount();
      });
}

auto
gameplay_logic::BmsScoreCourse::getLnCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getLnCount();
      });
}
auto
gameplay_logic::BmsScoreCourse::getMineCount() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMineCount();
      });
}
auto
gameplay_logic::BmsScoreCourse::getPoints() const -> double
{
    return std::accumulate(
      scores.begin(), scores.end(), 0.0, [](double sum, const BmsScore* score) {
          return sum + score->getResult()->getPoints();
      });
}
auto
gameplay_logic::BmsScoreCourse::getMaxCombo() const -> int
{
    return maxCombo;
}
auto
gameplay_logic::BmsScoreCourse::getJudgementCounts() const -> QList<int>
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
gameplay_logic::BmsScoreCourse::getMineHits() const -> int
{
    return std::accumulate(
      scores.begin(), scores.end(), 0, [](int sum, const BmsScore* score) {
          return sum + score->getResult()->getMineHits();
      });
}
auto
gameplay_logic::BmsScoreCourse::getClearType() const -> const QString&
{
    return clearType;
}
auto
gameplay_logic::BmsScoreCourse::getRandomSequence() -> QList<qint64>
{
    auto seq = QList<qint64>{};
    for (const auto* score : scores) {
        const auto& scoreSeq = score->getResult()->getRandomSequence();
        seq.append(scoreSeq);
    }
    return seq;
}
auto
gameplay_logic::BmsScoreCourse::getUnixTimestamp() const -> int64_t
{
    if (scores.isEmpty()) {
        return 0;
    }
    return scores.last()->getResult()->getUnixTimestamp();
}
auto
gameplay_logic::BmsScoreCourse::getGuid() const -> QString
{
    return guid;
}
auto
gameplay_logic::BmsScoreCourse::getSha256() const -> QString
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
gameplay_logic::BmsScoreCourse::getMd5() const -> QString
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
gameplay_logic::BmsScoreCourse::getNoteOrderAlgorithm() const
  -> resource_managers::NoteOrderAlgorithm
{
    return scores.isEmpty()
             ? resource_managers::NoteOrderAlgorithm::Normal
             : scores.first()->getResult()->getNoteOrderAlgorithm();
}

auto
gameplay_logic::BmsScoreCourse::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return scores.isEmpty()
             ? resource_managers::NoteOrderAlgorithm::Normal
             : scores.first()->getResult()->getNoteOrderAlgorithmP2();
}
auto
gameplay_logic::BmsScoreCourse::getGameVersion() const -> uint64_t
{
    return gameVersion;
}
auto
gameplay_logic::BmsScoreCourse::getScores() const -> QList<BmsScore*>
{
    return scores;
}
auto
gameplay_logic::BmsScoreCourse::getConstraints() const -> QStringList
{
    return constraints;
}
auto
gameplay_logic::BmsScoreCourse::getTrophies() const -> QVariantList
{
    QVariantList trophyList;
    for (const auto& trophy : trophies) {
        trophyList.append(QVariant::fromValue(trophy));
    }
    return trophyList;
}