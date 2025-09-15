//
// Created by bobini on 14.08.23.
//

#include "ChartLoader.h"

#include <utility>
#include "gameplay_logic/ChartRunner.h"
#include "support/QStringToPath.h"
#include "magic_enum/magic_enum.hpp"
#include <QDir>
#include "gameplay_logic/CourseRunner.h"
#include <ranges>

#include <spdlog/spdlog.h>

namespace qml_components {

auto
ChartLoader::createChart(
  resource_managers::Profile* player1,
  bool player1AutoPlay,
  gameplay_logic::BmsScore* replayedScore1,
  resource_managers::Profile* player2,
  bool player2AutoPlay,
  gameplay_logic::BmsScore* replayedScore2,
  resource_managers::ChartDataFactory::ChartComponents chartComponents) const
  -> std::unique_ptr<gameplay_logic::ChartRunner>
{
    if (isDp(chartComponents.chartData->getKeymode()) && player1 && player2) {
        spdlog::error("Can't launch DP for two players");
        return nullptr;
    }
    const auto rankInt = chartComponents.chartData->getRank();
    const auto rank =
      magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(rankInt).value_or(
        gameplay_logic::rules::defaultBmsRank);
    auto hitRules =
      std::vector<std::unique_ptr<gameplay_logic::rules::HitRules>>{};
    auto timingWindows = timingWindowsFactory(rank);
    auto maxHitValue = hitValueFactory(std::chrono::nanoseconds{ 0 },
                                       gameplay_logic::Judgement::Perfect);
    auto p1DpOptions = player2
                         ? resource_managers::DpOptions::Off
                         : player1->getVars()->getGeneralVars()->getDpOptions();
    auto p1NoteOrderAlgorithm =
      player1->getVars()->getGeneralVars()->getNoteOrderAlgorithm();
    auto p1NoteOrderAlgorithmP2 =
      player2 ? resource_managers::NoteOrderAlgorithm::Normal
              : player1->getVars()->getGeneralVars()->getNoteOrderAlgorithmP2();
    auto player1data = resource_managers::ChartFactory::PlayerSpecificData{
        player1,
        gaugeFactory(player1,
                     chartComponents.chartData->getTotal(),
                     chartComponents.chartData->getNormalNoteCount()),
        gameplay_logic::rules::HitRules(timingWindows, hitValueFactory),
        replayedScore1,
        p1NoteOrderAlgorithm,
        p1NoteOrderAlgorithmP2,
        p1DpOptions,
        player1AutoPlay
    };
    auto player2data =
      player2
        ? std::make_optional<
            resource_managers::ChartFactory::PlayerSpecificData>(
            player2,
            gaugeFactory(player2,
                         chartComponents.chartData->getTotal(),
                         chartComponents.chartData->getNormalNoteCount()),
            gameplay_logic::rules::HitRules(timingWindows, hitValueFactory),
            replayedScore2,
            player2->getVars()->getGeneralVars()->getNoteOrderAlgorithm(),
            resource_managers::NoteOrderAlgorithm::Normal,
            resource_managers::DpOptions::Off,
            player2AutoPlay)
        : std::nullopt;
    return chartFactory->createChart(std::move(chartComponents),
                                     std::move(player1data),
                                     std::move(player2data),
                                     maxHitValue);
}

template<typename Score>
bool
validateParams(resource_managers::Profile* player1,
               bool player1AutoPlay,
               Score* score1,
               resource_managers::Profile* player2,
               bool player2AutoPlay,
               Score* score2)
{
    if (!player1) {
        spdlog::error("Player 1 is null");
        return false;
    }
    if (!player2 && player2AutoPlay) {
        spdlog::error("Player 2 autoplay requested but player 2 is null");
        return false;
    }
    if (!player2 && score2) {
        spdlog::error("Player 2 replay requested but player 2 is null");
        return false;
    }
    if constexpr (std::is_same_v<Score, gameplay_logic::BmsResultCourse>) {
        if (score1 && score2) {
            if (score1->getIdentifier() != score2->getIdentifier()) {
                spdlog::error("Score 1 and score 2 aren't for the same course");
                return false;
            }
            for (auto i = 0; i < score1->getScores().size(); ++i) {
                if (score1->getScores()[i]->getResult()->getRandomSequence() !=
                    score2->getScores()[i]->getResult()->getRandomSequence()) {
                    spdlog::error("Score 1 and score 2 have different #RANDOM "
                                  "sequences for course");
                    return false;
                }
            }
        }
    } else {
        if (score1 && score2) {
            if (score1->getResult()->getSha256() !=
                score2->getResult()->getSha256()) {
                spdlog::error("Score 1 and score 2 have different sha256");
                return false;
            }
            if (score1->getResult()->getRandomSequence() !=
                score2->getResult()->getRandomSequence()) {
                spdlog::error(
                  "Score 1 and score 2 have different #RANDOM sequences");
                return false;
            }
        }
    }

    if (player1AutoPlay && score1) {
        spdlog::error("Player 1 autoplay requested but replay also provided");
        return false;
    }
    if (player2AutoPlay && score2) {
        spdlog::error("Player 2 autoplay requested but replay also provided");
        return false;
    }
    return true;
}
auto
ChartLoader::loadChart(const QString& filename,
                       resource_managers::Profile* player1,
                       bool player1AutoPlay,
                       gameplay_logic::BmsScore* score1,
                       resource_managers::Profile* player2,
                       bool player2AutoPlay,
                       gameplay_logic::BmsScore* score2) const
  -> gameplay_logic::ChartRunner*
{
    if (!validateParams(
          player1, player1AutoPlay, score1, player2, player2AutoPlay, score2)) {
        return nullptr;
    }
    auto randomSequence = score1   ? score1->getResult()->getRandomSequence()
                          : score2 ? score2->getResult()->getRandomSequence()
                                   : QList<qint64>{};
    auto randomGenerator =
      [randomSequence = std::move(randomSequence), counter = 0](
        const charts::ParsedBmsChart::RandomRange randomRange) mutable {
          thread_local auto randomEngine =
            std::default_random_engine{ std::random_device{}() };
          if (counter < randomSequence.size()) {
              return static_cast<charts::ParsedBmsChart::RandomRange>(
                randomSequence[counter++]);
          }
          if (randomRange <= 1) {
              return charts::ParsedBmsChart::RandomRange{ 1 };
          }
          return std::uniform_int_distribution{
              charts::ParsedBmsChart::RandomRange{ 1 }, randomRange
          }(randomEngine);
      };
    try {
        const auto fileAbsolute =
          support::qStringToPath(QFileInfo(filename).absoluteFilePath());
        const auto file = [&] {
            if (score1 || score2) {
                return getChartPathFromMd5(
                  (score1 ? score1 : score2)->getResult()->getMd5(),
                  fileAbsolute);
            }
            return !filename.isEmpty()
                     ? std::optional{ support::qStringToPath(filename) }
                     : std::nullopt;
        }();
        if (!file) {
            spdlog::error("Failed to find chart path to load replay");
            return nullptr;
        }
        auto chartComponents = chartDataFactory->loadChartData(
          fileAbsolute, std::move(randomGenerator));
        return createChart(player1,
                           player1AutoPlay,
                           score1,
                           player2,
                           player2AutoPlay,
                           score2,
                           std::move(chartComponents))
          .release();
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return nullptr;
    }
}

auto
constrainNoteOrderAlgorithm(resource_managers::NoteOrderAlgorithm algo,
                            bool mirror)
  -> resource_managers::NoteOrderAlgorithm
{
    if (mirror && algo != resource_managers::NoteOrderAlgorithm::Mirror) {
        return resource_managers::NoteOrderAlgorithm::Normal;
    }
    return resource_managers::NoteOrderAlgorithm::Normal;
}

gameplay_logic::CourseRunner*
ChartLoader::loadCourse(const resource_managers::Course& course,
                        resource_managers::Profile* player1,
                        bool player1AutoPlay,
                        gameplay_logic::BmsScoreCourse* score1,
                        resource_managers::Profile* player2,
                        bool player2AutoPlay,
                        gameplay_logic::BmsScoreCourse* score2) const
{
    if (!validateParams(
          player1, player1AutoPlay, score1, player2, player2AutoPlay, score2)) {
        return nullptr;
    }
    auto chartComponents =
      QList<resource_managers::ChartDataFactory::ChartComponents>{};
    for (const auto& [i, md5] : std::ranges::views::enumerate(course.md5s)) {
        try {
            auto path = getChartPathFromMd5(md5.toUpper(), {});
            if (!path) {
                spdlog::error("Failed to find chart path for course: {}",
                              md5.toStdString());
                return nullptr;
            }
            auto randomSequence =
              score1 ? score1->getScores()[i]->getResult()->getRandomSequence()
              : score2
                ? score2->getScores()[i]->getResult()->getRandomSequence()
                : QList<qint64>{};

            auto randomGenerator =
              [randomSequence = std::move(randomSequence), counter = 0](
                const charts::ParsedBmsChart::RandomRange randomRange) mutable {
                  thread_local auto randomEngine =
                    std::default_random_engine{ std::random_device{}() };
                  if (counter < randomSequence.size()) {
                      return static_cast<charts::ParsedBmsChart::RandomRange>(
                        randomSequence[counter++]);
                  }
                  if (randomRange <= 1) {
                      return charts::ParsedBmsChart::RandomRange{ 1 };
                  }
                  return std::uniform_int_distribution{
                      charts::ParsedBmsChart::RandomRange{ 1 }, randomRange
                  }(randomEngine);
              };
            auto components =
              chartDataFactory->loadChartData(*path, randomGenerator);
            chartComponents.append(std::move(components));
        } catch (const std::exception& e) {
            spdlog::error("Failed to load chart: {}", e.what());
            return nullptr;
        }
    }
    if (isDp(chartComponents[0].chartData->getKeymode()) && player1 &&
        player2) {
        spdlog::error("Can't launch DP for two players");
        return nullptr;
    }
    // check if keymode is the same for all charts
    const auto keymode = chartComponents[0].chartData->getKeymode();
    for (const auto& chartComponent : chartComponents) {
        if (chartComponent.chartData->getKeymode() != keymode) {
            spdlog::error("Keymode mismatch in course charts");
            return nullptr;
        }
    }
    auto p1DpOptions = player2
                         ? resource_managers::DpOptions::Off
                         : player1->getVars()->getGeneralVars()->getDpOptions();
    auto p1NoteOrderAlgorithm =
      player1->getVars()->getGeneralVars()->getNoteOrderAlgorithm();
    auto p1NoteOrderAlgorithmP2 =
      player2 ? resource_managers::NoteOrderAlgorithm::Normal
              : player1->getVars()->getGeneralVars()->getNoteOrderAlgorithmP2();
    auto p2NoteOrderAlgorithm =
      player2 ? player2->getVars()->getGeneralVars()->getNoteOrderAlgorithm()
              : resource_managers::NoteOrderAlgorithm::Normal;
    const auto mirror = course.constraints.contains("grade_mirror");
    p1NoteOrderAlgorithm =
      constrainNoteOrderAlgorithm(p1NoteOrderAlgorithm, mirror);
    p1NoteOrderAlgorithmP2 =
      constrainNoteOrderAlgorithm(p1NoteOrderAlgorithmP2, mirror);
    p2NoteOrderAlgorithm =
      constrainNoteOrderAlgorithm(p2NoteOrderAlgorithm, mirror);
    auto loadCourseChartPartial =
      [=,
       this,
       player1 = QPointer(player1),
       player2 = QPointer(player2),
       previous1 = QList<gameplay_logic::rules::BmsGauge*>{},
       previous2 = QList<gameplay_logic::rules::BmsGauge*>{},
       index = 0]() mutable -> std::unique_ptr<gameplay_logic::ChartRunner> {
        if (!player1) {
            return nullptr;
        }
        if (index >= chartComponents.size()) {
            return nullptr;
        }
        auto previous1Vals = QHash<QString, double>{};
        for (const auto& gauge : previous1) {
            previous1Vals[gauge->getName()] = gauge->getGauge();
        }
        auto gauges1 = gaugeFactoryCourse(player1, previous1Vals);
        auto gauges2 = QList<gameplay_logic::rules::BmsGauge*>{};
        if (player2) {
            auto previous2Vals = QHash<QString, double>{};
            for (const auto& gauge : previous2) {
                previous2Vals[gauge->getName()] = gauge->getGauge();
            }
            gauges2 = gaugeFactoryCourse(player2, previous2Vals);
        }
        auto course = loadCourseChart(
          chartComponents[index],
          player1,
          player1AutoPlay,
          score1 ? score1->getScores().value(index, nullptr) : nullptr,
          player2,
          player2AutoPlay,
          score2 ? score2->getScores().value(index, nullptr) : nullptr,
          gauges1,
          gauges2,
          p1NoteOrderAlgorithm,
          p1NoteOrderAlgorithmP2,
          p1DpOptions,
          p2NoteOrderAlgorithm);
        previous1 = std::move(gauges1);
        previous2 = std::move(gauges2);
        index++;
        return course;
    };
    auto chartDatas = QList<gameplay_logic::ChartData*>{};
    chartDatas.reserve(chartComponents.size());
    for (const auto& components : chartComponents) {
        chartDatas.append(components.chartData->clone().release());
    }
    auto guid1 = [&]() -> QString {
        if (score1) {
            return score1->getResult()->getGuid();
        }
        if (player1AutoPlay) {
            return QStringLiteral("");
        }
        return QUuid::createUuid().toString();
    }();
    auto guid2 = [&]() -> QString {
        if (score2) {
            return score2->getResult()->getGuid();
        }
        if (player2AutoPlay) {
            return QStringLiteral("");
        }
        return QUuid::createUuid().toString();
    }();

    auto* coursePlayer1 = new gameplay_logic::CoursePlayer(guid1);
    auto* coursePlayer2 =
      player2 ? new gameplay_logic::CoursePlayer(guid2) : nullptr;
    return new gameplay_logic::CourseRunner{ coursePlayer1,
                                             coursePlayer2,
                                             course,
                                             std::move(chartDatas),
                                             loadCourseChartPartial };
}
gameplay_logic::ChartData*
ChartLoader::loadChartData(const QString& filename,
                           const QString& md5,
                           QList<int64_t> randomSequence) const
{
    try {
        auto path =
          getChartPathFromMd5(md5.toUpper(), support::qStringToPath(filename));
        if (!path) {
            spdlog::error("Failed to find chart path for course: {}",
                          md5.toStdString());
            return nullptr;
        }

        auto randomGenerator =
          [randomSequence = std::move(randomSequence), counter = 0](
            const charts::ParsedBmsChart::RandomRange randomRange) mutable {
              thread_local auto randomEngine =
                std::default_random_engine{ std::random_device{}() };
              if (counter < randomSequence.size()) {
                  return static_cast<charts::ParsedBmsChart::RandomRange>(
                    randomSequence[counter++]);
              }
              if (randomRange <= 1) {
                  return charts::ParsedBmsChart::RandomRange{ 1 };
              }
              return std::uniform_int_distribution{
                  charts::ParsedBmsChart::RandomRange{ 1 }, randomRange
              }(randomEngine);
          };
        auto components =
          chartDataFactory->loadChartData(*path, randomGenerator);
        return components.chartData.release();
    } catch (const std::exception& e) {
        spdlog::error("Failed to load chart: {}", e.what());
        return nullptr;
    }
}
constexpr int maxVariables = 999;
auto
ChartLoader::loadChartDataFromDb(QList<QString> md5s) const -> QVariantMap
{
    auto uniqueMd5s = QSet<QString>{};
    for (const auto& md5 : md5s) {
        uniqueMd5s.insert(md5.toUpper());
    }
    const auto md5sToFetch = uniqueMd5s.values();
    std::vector<gameplay_logic::ChartData::DTO> allResults;

    for (int i = 0; i < md5sToFetch.size(); i += maxVariables) {
        auto chunk = md5sToFetch.mid(i, maxVariables);
        auto statement = db->createStatement(
          "SELECT id, title, artist, subtitle, subartist, "
          "genre, stage_file, banner, back_bmp, rank, total, "
          "play_level, difficulty, is_random, random_sequence, "
          "normal_note_count, ln_count, mine_count, length, initial_bpm, "
          "max_bpm, min_bpm, main_bpm, avg_bpm, path, directory, sha256, md5, "
          "keymode FROM charts WHERE md5 IN (" +
          QString("?, ").repeated(chunk.size()).chopped(2).toStdString() +
          ")  ORDER BY title, subtitle ASC");

        for (int j = 0; j < chunk.size(); ++j) {
            statement.bind(j + 1, chunk[j].toStdString());
        }

        const auto result =
          statement.executeAndGetAll<gameplay_logic::ChartData::DTO>();
        allResults.append_range(result);
    }
    auto ret = QVariantMap{};
    for (const auto& result : allResults) {
        ret[QString::fromStdString(result.md5)] = QVariant::fromValue(
          gameplay_logic::ChartData::load(result).release());
    }
    return ret;
}
auto
ChartLoader::loadCourseChart(
  resource_managers::ChartDataFactory::ChartComponents chartComponents,
  resource_managers::Profile* player1,
  bool player1AutoPlay,
  gameplay_logic::BmsScore* score1,
  resource_managers::Profile* player2,
  bool player2AutoPlay,
  gameplay_logic::BmsScore* score2,
  QList<gameplay_logic::rules::BmsGauge*> gauges1,
  QList<gameplay_logic::rules::BmsGauge*> gauges2,
  resource_managers::NoteOrderAlgorithm p1NoteOrderAlgorithm,
  resource_managers::NoteOrderAlgorithm p1NoteOrderAlgorithmP2,
  resource_managers::DpOptions p1DpOptions,
  resource_managers::NoteOrderAlgorithm p2NoteOrderAlgorithm) const
  -> std::unique_ptr<gameplay_logic::ChartRunner>
{
    const auto rankInt = chartComponents.chartData->getRank();
    const auto rank =
      magic_enum::enum_cast<gameplay_logic::rules::BmsRank>(rankInt).value_or(
        gameplay_logic::rules::defaultBmsRank);
    auto hitRules =
      std::vector<std::unique_ptr<gameplay_logic::rules::HitRules>>{};
    auto timingWindows = timingWindowsFactory(rank);
    auto maxHitValue = hitValueFactory(std::chrono::nanoseconds{ 0 },
                                       gameplay_logic::Judgement::Perfect);
    auto player1data = resource_managers::ChartFactory::PlayerSpecificData{
        player1,
        std::move(gauges1),
        gameplay_logic::rules::HitRules(timingWindows, hitValueFactory),
        score1,
        p1NoteOrderAlgorithm,
        p1NoteOrderAlgorithmP2,
        p1DpOptions,
        player1AutoPlay
    };
    auto player2data =
      player2
        ? std::make_optional<
            resource_managers::ChartFactory::PlayerSpecificData>(
            player2,
            std::move(gauges2),
            gameplay_logic::rules::HitRules(timingWindows, hitValueFactory),
            score2,
            p2NoteOrderAlgorithm,
            resource_managers::NoteOrderAlgorithm::Normal,
            resource_managers::DpOptions::Off,
            player2AutoPlay)
        : std::nullopt;
    return chartFactory->createChart(std::move(chartComponents),
                                     std::move(player1data),
                                     std::move(player2data),
                                     maxHitValue);
}
ChartLoader::ChartLoader(ProfileList* profileList,
                         input::InputTranslator* inputTranslator,
                         resource_managers::ChartDataFactory* chartDataFactory,
                         TimingWindowsFactory timingWindowsFactory,
                         HitValueFactory hitValueFactory,
                         GaugeFactory gaugeFactory,
                         GaugeFactoryCourse gaugeFactoryCourse,
                         GetChartPathFromMd5 getChartPathFromSha256,
                         resource_managers::ChartFactory* chartFactory,
                         db::SqliteCppDb* db,
                         QObject* parent)
  : QObject(parent)
  , chartDataFactory(chartDataFactory)
  , timingWindowsFactory(std::move(timingWindowsFactory))
  , hitValueFactory(std::move(hitValueFactory))
  , gaugeFactory(std::move(gaugeFactory))
  , gaugeFactoryCourse(std::move(gaugeFactoryCourse))
  , getChartPathFromMd5(std::move(getChartPathFromSha256))
  , chartFactory(chartFactory)
  , profileList(profileList)
  , inputTranslator(inputTranslator)
  , db(db)
{
}
} // namespace qml_components