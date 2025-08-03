//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTLOADER_H
#define RHYTHMGAME_CHARTLOADER_H

#include "ProfileList.h"
#include "gameplay_logic/BmsResultCourse.h"
#include "gameplay_logic/BmsScoreCourse.h"

#include <memory>
#include "gameplay_logic/ChartData.h"
#include "gameplay_logic/ChartRunner.h"
#include "resource_managers/ChartFactory.h"
#include "gameplay_logic/rules/TimingWindows.h"
#include "gameplay_logic/rules/BmsRanks.h"
#include "resource_managers/Tables.h"

namespace gameplay_logic {
class CourseRunner;
}
namespace qml_components {

class ChartLoader : public QObject
{
    Q_OBJECT

  public:
    using GaugeFactory = std::function<QList<gameplay_logic::rules::BmsGauge*>(
      resource_managers::Profile* profile,
      double total,
      int noteCount)>;
    using GaugeFactoryCourse =
      std::function<QList<gameplay_logic::rules::BmsGauge*>(
        resource_managers::Profile* profile,
        const QHash<QString, double>& initialValues)>;
    using TimingWindowsFactory =
      std::function<gameplay_logic::rules::TimingWindows(
        gameplay_logic::rules::BmsRank)>;
    using HitValueFactory = std::function<double(std::chrono::nanoseconds,
                                                 gameplay_logic::Judgement)>;
    using GetChartPathFromMd5 =
      std::function<std::optional<std::filesystem::path>(
        const QString& hash,
        const std::filesystem::path& hint)>;

  private:
    resource_managers::ChartDataFactory* chartDataFactory;
    TimingWindowsFactory timingWindowsFactory;
    HitValueFactory hitValueFactory;
    GaugeFactory gaugeFactory;
    GaugeFactoryCourse gaugeFactoryCourse;
    GetChartPathFromMd5 getChartPathFromMd5;
    resource_managers::ChartFactory* chartFactory;
    ProfileList* profileList;
    input::InputTranslator* inputTranslator;
    db::SqliteCppDb* db;

    auto createChart(resource_managers::Profile* player1,
                     bool player1AutoPlay,
                     gameplay_logic::BmsScore* replayedScore1,
                     resource_managers::Profile* player2,
                     bool player2AutoPlay,
                     gameplay_logic::BmsScore* replayedScore2,
                     resource_managers::ChartDataFactory::ChartComponents
                       chartComponents) const
      -> std::unique_ptr<gameplay_logic::ChartRunner>;

    auto loadCourseChart(
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
      -> std::unique_ptr<gameplay_logic::ChartRunner>;

  public:
    ChartLoader(ProfileList* profileList,
                input::InputTranslator* inputTranslator,
                resource_managers::ChartDataFactory* chartDataFactory,
                TimingWindowsFactory timingWindowsFactory,
                HitValueFactory hitValueFactory,
                GaugeFactory gaugeFactory,
                GaugeFactoryCourse gaugeFactoryCourse,
                GetChartPathFromMd5 getChartPathFromSha256,
                resource_managers::ChartFactory* chartFactory,
                db::SqliteCppDb* db,
                QObject* parent = nullptr);

    /**
     * @brief Loads a chart with the given parameters.
     * @param filename The path to the chart file.
     * @param player1 The first player profile.
     * @param player1AutoPlay Whether the first player is in auto-play mode.
     * @param score1 The score to replay for the first player, optional.
     * Incompatible with auto-play.
     * @param player2 The second player profile, can be nullptr for
     * single-player.
     * @param player2AutoPlay Whether the second player is in auto-play mode.
     * @param score2 The score to replay for the second player. Incompatible
     * with auto-play.
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
     * @param score1 The score to replay for the first player, optional.
     * Incompatible with auto-play.
     * @param player2 The second player profile, can be nullptr for
     * single-player.
     * @param player2AutoPlay Whether the second player is in auto-play mode.
     * @param score2 The score to replay for the second player. Incompatible
     * with auto-play.
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

    Q_INVOKABLE gameplay_logic::ChartData* loadChartData(
      const QString& filename,
      const QString& md5 = "",
      QList<int64_t> randomSequence = {}) const;
    Q_INVOKABLE QVariantMap loadChartDataFromDb(QList<QString> md5s) const;
};

} // namespace qml_components

#endif // RHYTHMGAME_CHARTLOADER_H
