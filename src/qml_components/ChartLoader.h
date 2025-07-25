//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include "ProfileList.h"
#include "gameplay_logic/BmsScoreCourse.h"

#include <memory>
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/CourseRunner.h"
#include "resource_managers/ChartFactory.h"
#include "gameplay_logic/rules/TimingWindows.h"
#include "gameplay_logic/rules/BmsRanks.h"
#include "gameplay_logic/rules/BmsHitRules.h"
#include "resource_managers/Tables.h"

namespace qml_components {

class ChartLoader : public QObject
{
    Q_OBJECT

  public:
    using GaugeFactory = std::function<QList<gameplay_logic::rules::BmsGauge*>(
      resource_managers::Profile* profile,
      gameplay_logic::rules::TimingWindows timingWindows,
      double total,
      int noteCount)>;
    using TimingWindowsFactory =
      std::function<gameplay_logic::rules::TimingWindows(
        gameplay_logic::rules::BmsRank)>;
    using HitValueFactory = std::function<double(std::chrono::nanoseconds,
                                                 gameplay_logic::Judgement)>;
    using HitRulesFactory =
      std::function<std::unique_ptr<gameplay_logic::rules::BmsHitRules>(
        gameplay_logic::rules::TimingWindows,
        HitValueFactory)>;
    using GetChartPathFromMd5 =
      std::function<std::optional<std::filesystem::path>(const QString& hash, const std::filesystem::path& hint)>;

  private:
    resource_managers::ChartDataFactory* chartDataFactory;
    TimingWindowsFactory timingWindowsFactory;
    HitRulesFactory hitRulesFactory;
    HitValueFactory hitValueFactory;
    GaugeFactory gaugeFactory;
    GaugeFactory gaugeFactoryCourse;
    GetChartPathFromMd5 getChartPathFromMd5;
    resource_managers::ChartFactory* chartFactory;
    ProfileList* profileList;
    input::InputTranslator* inputTranslator;

    gameplay_logic::ChartRunner* createChart(
      resource_managers::Profile* player1,
      bool player1AutoPlay,
      gameplay_logic::BmsScore* replayedScore1,
      resource_managers::Profile* player2,
      bool player2AutoPlay,
      gameplay_logic::BmsScore* replayedScore2,
      const resource_managers::ChartDataFactory::ChartComponents&
        chartComponents) const;

  public:
    ChartLoader(ProfileList* profileList,
                input::InputTranslator* inputTranslator,
                resource_managers::ChartDataFactory* chartDataFactory,
                TimingWindowsFactory timingWindowsFactory,
                HitRulesFactory hitRulesFactory,
                HitValueFactory hitValueFactory,
                GaugeFactory gaugeFactory,
                GetChartPathFromMd5 getChartPathFromSha256,
                resource_managers::ChartFactory* chartFactory,
                QObject* parent = nullptr);

    /**
     * @brief Loads a chart with the given parameters.
     * @param filename The path to the chart file.
     * @param player1 The first player profile.
     * @param player1AutoPlay Whether the first player is in auto-play mode.
     * @param score1 The score to replay for the first player, optional. Incompatible with auto-play.
     * @param player2 The second player profile, can be nullptr for single-player.
     * @param player2AutoPlay Whether the second player is in auto-play mode.
     * @param score2 The score to replay for the second player. Incompatible with auto-play.
     * @return A pointer to the loaded chart, or nullptr on failure.
     */
    Q_INVOKABLE gameplay_logic::ChartRunner* loadChart(
      const QString& filename,
      resource_managers::Profile* player1,
      bool player1AutoPlay,
      gameplay_logic::BmsScore* score1,
      resource_managers::Profile* player2,
      bool player2AutoPlay,
      gameplay_logic::BmsScore* score2) const;

    /**
     * @brief Loads a course with the given parameters.
     * @param course The course to load.
     * @param player1 The first player profile.
     * @param player1AutoPlay Whether the first player is in auto-play mode.
     * @param score1 The score to replay for the first player, optional. Incompatible with auto-play.
     * @param player2 The second player profile, can be nullptr for single-player.
     * @param player2AutoPlay Whether the second player is in auto-play mode.
     * @param score2 The score to replay for the second player. Incompatible with auto-play.
     * @return A pointer to the loaded course, or nullptr on failure.
     */
    Q_INVOKABLE gameplay_logic::CourseRunner* loadCourse(
      const resource_managers::Course& course,
      resource_managers::Profile* player1,
      bool player1AutoPlay,
      gameplay_logic::BmsScoreCourse* score1,
        resource_managers::Profile* player2,
        bool player2AutoPlay,
        gameplay_logic::BmsScoreCourse* score2) const;


    gameplay_logic::ChartRunner* loadCourseChart(
      resource_managers::ChartDataFactory::ChartComponents chartComponents,
      resource_managers::Profile* player1,
      bool player1AutoPlay,
      gameplay_logic::BmsScore* score1,
      resource_managers::Profile* player2,
      bool player2AutoPlay,
      gameplay_logic::BmsScore* score2) const;
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
