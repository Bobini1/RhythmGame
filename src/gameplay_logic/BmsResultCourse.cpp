//
// Created by PC on 17/07/2025.
//

#include "BmsResultCourse.h"
#include "BmsScore.h"

#include "resource_managers/Tables.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <stdexcept>
#include <tuple>
auto
gameplay_logic::BmsResultCourse::load(const DTO& dto,
                                      const QList<BmsResult*>& results)
  -> std::unique_ptr<BmsResultCourse>
{
    auto constraints =
      QString::fromStdString(dto.constraints).split(' ', Qt::SkipEmptyParts);
    auto result =
      std::make_unique<BmsResultCourse>(QString::fromStdString(dto.guid),
                                        QString::fromStdString(dto.identifier),
                                        results,
                                        QString::fromStdString(dto.clearType),
                                        dto.maxCombo,
                                        std::move(constraints),
                                        dto.gameVersion);
    result->unixTimestamp = dto.unixTimestamp;
    return result;
}
void
gameplay_logic::BmsResultCourse::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    if (!isComplete()) {
        throw std::runtime_error(
          "Cannot save a course result with missing stages");
    }
    auto statement = db.createStatement(
      "INSERT OR IGNORE INTO score_course "
      "(guid, identifier, score_guids, clear_type, max_combo, constraints, "
      "unix_timestamp, game_version) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    statement.bind(1, guid.toStdString());
    statement.bind(2, identifier.toStdString());
    QStringList scoreGuids;
    for (const auto* result : results) {
        scoreGuids.append(result ? result->getGuid() : QString{});
    }
    statement.bind(3, scoreGuids.join(' ').toStdString());
    statement.bind(4, clearType.toStdString());
    statement.bind(5, maxCombo);
    statement.bind(6, constraints.join(' ').toStdString());
    statement.bind(7, getUnixTimestamp());
    statement.bind(8, static_cast<int64_t>(gameVersion));
    statement.execute();
}
gameplay_logic::BmsResultCourse::BmsResultCourse(
  QString guid,
  QString identifier,
  const QList<BmsResult*>& results,
  QString clearType,
  int maxCombo,
  QStringList constraints,
  uint64_t gameVersion,
  QObject* parent)
  : QObject(parent)
  , identifier(std::move(identifier))
  , clearType(std::move(clearType))
  , guid(std::move(guid))
  , maxCombo(maxCombo)
  , constraints(std::move(constraints))
  , gameVersion(gameVersion)
{
    for (auto* result : results) {
        auto copy = result ? result->clone() : nullptr;
        if (copy)
            copy->setParent(this);
        this->results.append(copy.release());
    }
    if (!results.isEmpty() && results.last())
        unixTimestamp = results.last()->getUnixTimestamp();
}

namespace {
auto
stageResults(const QList<gameplay_logic::BmsScore*>& scores)
  -> QList<gameplay_logic::BmsResult*>
{
    QList<gameplay_logic::BmsResult*> results;
    for (auto* score : scores)
        results.append(score ? score->getResult() : nullptr);
    return results;
}
}

gameplay_logic::BmsResultCourse::BmsResultCourse(QString guid,
                                                 QString identifier,
                                                 QList<BmsScore*> scores,
                                                 QString clearType,
                                                 int maxCombo,
                                                 QStringList constraints,
                                                 uint64_t gameVersion,
                                                 QObject* parent)
  : BmsResultCourse(std::move(guid),
                    std::move(identifier),
                    stageResults(scores),
                    std::move(clearType),
                    maxCombo,
                    std::move(constraints),
                    gameVersion,
                    parent)
{
}

bool
gameplay_logic::BmsResultCourse::updateFrom(const BmsResultCourse& other)
{
    Q_ASSERT(guid == other.guid);
    const auto fields = [](auto& value) {
        return std::tie(value.identifier,
                        value.clearType,
                        value.randomSequence,
                        value.maxCombo,
                        value.constraints,
                        value.gameVersion,
                        value.unixTimestamp);
    };
    bool updated = fields(*this) != fields(other);
    fields(*this) = fields(other);
    QList<BmsResult*> stages;
    for (auto* stage : other.results) {
        if (!stage) {
            stages.append(nullptr);
            continue;
        }
        auto* existing = results.value(stages.size());
        if (existing && existing->getGuid() == stage->getGuid()) {
            updated = existing->updateFrom(*stage) || updated;
        } else {
            auto copy = stage->clone();
            copy->setParent(this);
            existing = copy.release();
        }
        stages.append(existing);
    }
    updated = stages != results || updated;
    results = std::move(stages);
    if (updated)
        emit changed();
    return updated;
}

auto
gameplay_logic::BmsResultCourse::clone() const
  -> std::unique_ptr<BmsResultCourse>
{
    auto copy = std::make_unique<BmsResultCourse>(
      guid, identifier, results, clearType, maxCombo, constraints, gameVersion);
    copy->unixTimestamp = unixTimestamp;
    return copy;
}

bool
gameplay_logic::BmsResultCourse::isComplete() const
{
    return !results.isEmpty() && std::ranges::all_of(results, [](auto* result) {
        return result != nullptr;
    });
}

bool
gameplay_logic::BmsResultCourse::isReplayAvailable() const
{
    return isComplete() && std::ranges::all_of(results, [](auto* result) {
               return result->isReplayAvailable();
           });
}

bool
gameplay_logic::BmsResultCourse::isImported() const
{
    return std::ranges::any_of(
      results, [](auto* result) { return result && result->isImported(); });
}

auto
gameplay_logic::BmsResultCourse::getMaxPoints() const -> double
{
    return std::accumulate(results.begin(),
                           results.end(),
                           0.0,
                           [](double sum, const BmsResult* result) {
                               return sum +
                                      (result ? result->getMaxPoints() : 0);
                           });
}
auto
gameplay_logic::BmsResultCourse::getMaxHits() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getMaxHits() : 0);
      });
}
auto
gameplay_logic::BmsResultCourse::getNormalNoteCount() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getNormalNoteCount() : 0);
      });
}

auto
gameplay_logic::BmsResultCourse::getScratchCount() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getScratchCount() : 0);
      });
}

auto
gameplay_logic::BmsResultCourse::getLnCount() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getLnCount() : 0);
      });
}
auto
gameplay_logic::BmsResultCourse::getBssCount() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getBssCount() : 0);
      });
}
auto
gameplay_logic::BmsResultCourse::getMineCount() const -> int
{
    return std::accumulate(
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getMineCount() : 0);
      });
}
auto
gameplay_logic::BmsResultCourse::getPoints() const -> double
{
    return std::accumulate(results.begin(),
                           results.end(),
                           0.0,
                           [](double sum, const BmsResult* result) {
                               return sum + (result ? result->getPoints() : 0);
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
    for (const auto* result : results) {
        if (!result)
            continue;
        const auto& scoreCounts = result->getJudgementCounts();
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
      results.begin(), results.end(), 0, [](int sum, const BmsResult* result) {
          return sum + (result ? result->getMineHits() : 0);
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
    for (const auto* result : results) {
        if (!result)
            continue;
        const auto& scoreSeq = result->getRandomSequence();
        seq.append(scoreSeq);
    }
    return seq;
}
auto
gameplay_logic::BmsResultCourse::getUnixTimestamp() const -> int64_t
{
    return unixTimestamp;
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
    for (const auto* result : results) {
        if (!result)
            continue;
        if (!sha256.isEmpty()) {
            sha256 += ' ';
        }
        sha256 += result->getSha256();
    }
    return sha256;
}
auto
gameplay_logic::BmsResultCourse::getMd5() const -> QString
{
    QString md5;
    for (const auto* result : results) {
        if (!result)
            continue;
        if (!md5.isEmpty()) {
            md5 += ' ';
        }
        md5 += result->getMd5();
    }
    return md5;
}
auto
gameplay_logic::BmsResultCourse::getNoteOrderAlgorithm() const
  -> resource_managers::NoteOrderAlgorithm
{
    return (results.isEmpty() || !results.first())
             ? resource_managers::NoteOrderAlgorithm::Normal
             : results.first()->getNoteOrderAlgorithm();
}

auto
gameplay_logic::BmsResultCourse::getNoteOrderAlgorithmP2() const
  -> resource_managers::NoteOrderAlgorithm
{
    return (results.isEmpty() || !results.first())
             ? resource_managers::NoteOrderAlgorithm::Normal
             : results.first()->getNoteOrderAlgorithmP2();
}
auto
gameplay_logic::BmsResultCourse::getGameVersion() const -> uint64_t
{
    return gameVersion;
}
auto
gameplay_logic::BmsResultCourse::getResults() const -> const QList<BmsResult*>&
{
    return results;
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
gameplay_logic::BmsResultCourse::getDpOptions() const
  -> resource_managers::DpOptions
{
    return (results.isEmpty() || !results.first())
             ? resource_managers::DpOptions::Off
             : results.first()->getDpOptions();
}
auto
gameplay_logic::BmsResultCourse::getLength() const -> int64_t
{
    return std::accumulate(results.begin(),
                           results.end(),
                           0LL,
                           [](int64_t sum, const BmsResult* result) {
                               return sum + (result ? result->getLength() : 0);
                           });
}
auto
gameplay_logic::BmsResultCourse::getKeymode() const
  -> gameplay_logic::ChartData::Keymode
{
    return (results.isEmpty() || !results.first())
             ? ChartData::Keymode::K7
             : static_cast<ChartData::Keymode>(results.first()->getKeymode());
}
