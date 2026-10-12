#include "ScreenContexts.h"

#include <algorithm>
#include <utility>
#include <QScopedValueRollback>

namespace rhythm_game_qml {
namespace {
template<typename T>
auto
readOnlyList(QObject* owner, QList<T*>* values) -> QQmlListProperty<T>
{
    return { owner,
             values,
             [](QQmlListProperty<T>* list) {
                 return static_cast<QList<T*>*>(list->data)->size();
             },
             [](QQmlListProperty<T>* list, qsizetype index) {
                 return static_cast<QList<T*>*>(list->data)->value(index);
             } };
}
}

GameplayContext::GameplayContext(QObject* runner,
                                 arena::ArenaSession* arenaSession)
  : QObject(nullptr)
  , chartRunner(qobject_cast<gameplay_logic::ChartRunner*>(runner))
  , courseRunner(qobject_cast<gameplay_logic::CourseRunner*>(runner))
  , arenaManaged(arenaSession != nullptr)
  , session(arenaSession)
{
    if (!arenaManaged)
        runner->setParent(this);
    if (session) {
        connect(session,
                &arena::ArenaSession::competitionChanged,
                this,
                &GameplayContext::arenaActiveChanged);
        connect(session,
                &QObject::destroyed,
                this,
                &GameplayContext::arenaActiveChanged);
    }
    if (chartRunner) {
        chartMetadata = { chartRunner->getChartData()->clone(this).release() };
        connect(chartRunner,
                &gameplay_logic::ChartRunner::bgaLoaded,
                this,
                &GameplayContext::bgaChanged);
        connect(chartRunner,
                &gameplay_logic::ChartRunner::statusChanged,
                this,
                &GameplayContext::statusChanged);
        connect(chartRunner,
                &gameplay_logic::ChartRunner::inputMappingChanged,
                this,
                &GameplayContext::inputMappingChanged);
    } else if (courseRunner) {
        for (const auto* chart : courseRunner->getChartDatas())
            chartMetadata.append(chart->clone(this).release());
        courseDefinition = QVariant::fromValue(courseRunner->getCourse());
        courseParticipants = { courseRunner->getCoursePlayer1() };
        if (auto* second = courseRunner->getCoursePlayer2())
            courseParticipants.append(second);
        // currentChartIndexChanged precedes the runner/player replacement.
        // Publish a coherent stage only after CourseRunner has installed it.
        connect(courseRunner,
                &gameplay_logic::CourseRunner::player1Changed,
                this,
                &GameplayContext::refreshStage);
        connect(courseRunner,
                &gameplay_logic::CourseRunner::bgaChanged,
                this,
                [this] {
                    refreshStage();
                    emit bgaChanged();
                });
        connect(courseRunner,
                &gameplay_logic::CourseRunner::statusChanged,
                this,
                &GameplayContext::statusChanged);
        connect(courseRunner,
                &gameplay_logic::CourseRunner::inputMappingChanged,
                this,
                &GameplayContext::inputMappingChanged);
    }
    connect(runner, &QObject::destroyed, this, [this] {
        currentPlayers.clear();
        courseParticipants.clear();
        emit stageChanged();
        emit statusChanged();
        emit bgaChanged();
        emit inputMappingChanged();
        emit arenaActiveChanged();
    });
    refreshStage();
}

void
GameplayContext::refreshStage()
{
    effectiveKeymode = chartRunner    ? chartRunner->getKeymode()
                       : courseRunner ? courseRunner->getKeymode()
                                      : effectiveKeymode;
    auto nextPlayers = QList<gameplay_logic::Player*>{};
    auto* first = chartRunner    ? chartRunner->getPlayer1()
                  : courseRunner ? courseRunner->getPlayer1()
                                 : nullptr;
    auto* second = chartRunner    ? chartRunner->getPlayer2()
                   : courseRunner ? courseRunner->getPlayer2()
                                  : nullptr;
    if (first)
        nextPlayers.append(first);
    if (second)
        nextPlayers.append(second);
    const auto index = courseRunner ? courseRunner->getCurrentChartIndex() : 0;
    const auto allCharts = charts();
    auto* data =
      index >= 0 && index < allCharts.size() ? allCharts[index] : nullptr;
    if (currentPlayers == nextPlayers && currentStageIndex == index &&
        currentChartData == data)
        return;
    currentPlayers = std::move(nextPlayers);
    currentStageIndex = index;
    currentChartData = data;
    emit stageChanged();
}
auto
GameplayContext::chartData() const -> gameplay_logic::ChartData*
{
    return currentChartData;
}
auto
GameplayContext::players() const -> QList<gameplay_logic::Player*>
{
    return currentPlayers;
}
auto
GameplayContext::bga() const -> qml_components::BgaContainer*
{
    return chartRunner    ? chartRunner->getBga()
           : courseRunner ? courseRunner->getBga()
                          : nullptr;
}
auto
GameplayContext::status() const -> gameplay_logic::ChartRunner::Status
{
    return chartRunner    ? chartRunner->getStatus()
           : courseRunner ? courseRunner->getStatus()
                          : gameplay_logic::ChartRunner::Finished;
}
auto
GameplayContext::keymode() const -> gameplay_logic::ChartData::Keymode
{
    return effectiveKeymode;
}
auto
GameplayContext::isArena() const -> bool
{
    return arenaManaged;
}
auto
GameplayContext::arenaActive() const -> bool
{
    return session && session->arenaGameplayActive() && chartRunner &&
           chartRunner == session->arenaRunner();
}
auto
GameplayContext::course() const -> QVariant
{
    return courseDefinition;
}
auto
GameplayContext::coursePlayers() const -> QList<gameplay_logic::CoursePlayer*>
{
    return courseParticipants;
}
auto
GameplayContext::stageIndex() const -> int
{
    return currentStageIndex;
}
auto
GameplayContext::stageCount() const -> int
{
    return static_cast<int>(charts().size());
}
auto
GameplayContext::charts() const -> QList<gameplay_logic::ChartData*>
{
    return chartMetadata;
}
auto
GameplayContext::qmlPlayers() -> QQmlListProperty<gameplay_logic::Player>
{
    return readOnlyList(this, &currentPlayers);
}
auto
GameplayContext::qmlCoursePlayers()
  -> QQmlListProperty<gameplay_logic::CoursePlayer>
{
    return readOnlyList(this, &courseParticipants);
}
auto
GameplayContext::qmlCharts() -> QQmlListProperty<gameplay_logic::ChartData>
{
    return readOnlyList(this, &chartMetadata);
}
auto
GameplayContext::inputMapping() const -> QList<int>
{
    return chartRunner    ? chartRunner->getInputMapping()
           : courseRunner ? courseRunner->getInputMapping()
                          : QList<int>{};
}
void
GameplayContext::setInputMapping(const QList<int>& mapping)
{
    if (chartRunner)
        chartRunner->setInputMapping(mapping);
    else if (courseRunner)
        courseRunner->setInputMapping(mapping);
}

auto
GameplayContext::runner() const -> QObject*
{
    return chartRunner ? static_cast<QObject*>(chartRunner)
                       : static_cast<QObject*>(courseRunner);
}

bool
GameplayContext::start()
{
    if (lifecycleBusy || stageResult ||
        status() != gameplay_logic::ChartRunner::Ready)
        return false;
    QScopedValueRollback busy(lifecycleBusy, true);
    if (courseRunner)
        courseRunner->start();
    else if (chartRunner)
        chartRunner->start();
    return true;
}

auto
GameplayContext::finishStage() -> ResultContext*
{
    if (lifecycleBusy)
        return nullptr;
    if (stageResult)
        return stageResult;
    if (!runner() || !chartData() ||
        status() == gameplay_logic::ChartRunner::Loading)
        return nullptr;
    QScopedValueRollback busy(lifecycleBusy, true);
    QList<resource_managers::Profile*> profiles;
    for (auto* player : players())
        profiles.append(player->getProfile());
    const auto scores =
      courseRunner ? courseRunner->finishStage() : chartRunner->finish();
    // Single-chart completion returns newly allocated scores. Courses retain
    // their stage scores themselves for the later aggregate result.
    if (!courseRunner)
        for (auto* score : scores)
            if (score)
                score->setParent(this);
    if (scores.isEmpty() || scores.contains(nullptr))
        return nullptr;
    QString roundId;
    if (session && session->submitLocalResult(scores.first()))
        roundId = session->presentedResult()->roundId();
    stageResult =
      new ResultContext(scores, profiles, chartData(), this, roundId);
    stageResult->setParent(this);
    return stageResult;
}

bool
GameplayContext::nextStage()
{
    if (lifecycleBusy || !courseRunner || !stageResult)
        return false;
    QScopedValueRollback busy(lifecycleBusy, true);
    if (!courseRunner->nextStage())
        return false;
    stageResult = nullptr;
    return true;
}

auto
GameplayContext::finishCourse() -> CourseResultContext*
{
    if (lifecycleBusy)
        return nullptr;
    if (courseResult)
        return courseResult;
    if (!courseRunner || !stageResult || stageIndex() + 1 != stageCount())
        return nullptr;
    QScopedValueRollback busy(lifecycleBusy, true);
    QList<resource_managers::Profile*> profiles;
    for (auto* player : players())
        profiles.append(player->getProfile());
    const auto scores = courseRunner->finish();
    if (scores.isEmpty() || scores.contains(nullptr))
        return nullptr;
    courseResult = new CourseResultContext(
      scores, profiles, charts(), courseRunner->getCourse());
    courseResult->setParent(this);
    return courseResult;
}

ResultPlayer::ResultPlayer(resource_managers::Profile* profile,
                           gameplay_logic::BmsScore* score,
                           QObject* parent)
  : QObject(parent)
  , playerProfile(profile)
  , playerScore(score)
{
}
CourseResultPlayer::CourseResultPlayer(resource_managers::Profile* profile,
                                       gameplay_logic::BmsScoreCourse* score,
                                       QObject* parent)
  : QObject(parent)
  , playerProfile(profile)
  , playerScore(score)
{
}
ResultContext::ResultContext(const QList<gameplay_logic::BmsScore*>& scores,
                             const QList<resource_managers::Profile*>& profiles,
                             gameplay_logic::ChartData* chartData,
                             GameplayContext* gameplay,
                             QString arenaRoundId)
  : completedChart(chartData ? chartData->clone(this).release() : nullptr)
  , originatingPlay(gameplay)
  , session(gameplay ? gameplay->arenaSession() : nullptr)
  , completedArenaRoundId(std::move(arenaRoundId))
{
    if (session) {
        connect(session,
                &arena::ArenaSession::competitionChanged,
                this,
                &ResultContext::arenaActiveChanged);
        connect(session->presentedResult(),
                &arena::ArenaResultModel::changed,
                this,
                &ResultContext::arenaActiveChanged);
        connect(session,
                &QObject::destroyed,
                this,
                &ResultContext::arenaActiveChanged);
    }
    if (gameplay && gameplay->course().isValid()) {
        completedCourse = gameplay->course();
        completedStageIndex =
          static_cast<int>(gameplay->charts().indexOf(chartData));
        completedStageCount = gameplay->stageCount();
    }
    for (qsizetype i = 0; i < scores.size(); ++i)
        participants.append(new ResultPlayer(
          profiles.value(i),
          gameplay ? scores[i] : scores[i]->clone(this).release(),
          this));
}
auto
ResultContext::arenaActive() const -> bool
{
    return session && !completedArenaRoundId.isEmpty() &&
           session->resultPresentationActive() &&
           session->presentedResult()->roundId() == completedArenaRoundId;
}
auto
ResultContext::qmlPlayers() -> QQmlListProperty<ResultPlayer>
{
    return readOnlyList(this, &participants);
}
CourseResultContext::CourseResultContext(
  const QList<gameplay_logic::BmsScoreCourse*>& scores,
  const QList<resource_managers::Profile*>& profiles,
  QList<gameplay_logic::ChartData*> charts,
  resource_managers::Course course)
  : completedCourse(std::move(course))
{
    for (const auto* chart : charts)
        completedCharts.append(chart ? chart->clone(this).release() : nullptr);
    for (qsizetype i = 0; i < scores.size(); ++i)
        participants.append(new CourseResultPlayer(
          profiles.value(i), scores[i]->clone(this).release(), this));
}

auto
CourseResultContext::qmlPlayers() -> QQmlListProperty<CourseResultPlayer>
{
    return readOnlyList(this, &participants);
}
auto
CourseResultContext::qmlCharts() -> QQmlListProperty<gameplay_logic::ChartData>
{
    return readOnlyList(this, &completedCharts);
}

auto
ScreenContexts::createGameplay(QObject* runner,
                               arena::ArenaSession* arenaSession) const
  -> GameplayContext*
{
    if (!qobject_cast<gameplay_logic::ChartRunner*>(runner) &&
        !qobject_cast<gameplay_logic::CourseRunner*>(runner))
        return nullptr;
    return new GameplayContext(runner, arenaSession);
}

bool
ScreenContexts::canRetry(GameplayContext* gameplay,
                         qml_components::ChartLoader* loader) const
{
    return gameplay && loader && !gameplay->isArena() &&
           !gameplay->course().isValid() &&
           loader->canRetry(
             qobject_cast<gameplay_logic::ChartRunner*>(gameplay->runner()));
}

auto
ScreenContexts::retryGameplay(GameplayContext* gameplay,
                              qml_components::ChartLoader* loader,
                              bool samePattern) const -> GameplayContext*
{
    if (!canRetry(gameplay, loader))
        return nullptr;
    return createGameplay(loader->retryChart(
      qobject_cast<gameplay_logic::ChartRunner*>(gameplay->runner()),
      samePattern));
}
namespace {
template<typename Score>
bool
validParticipants(const QList<Score*>& scores,
                  const QList<resource_managers::Profile*>& profiles)
{
    if (scores.isEmpty() || scores.size() > 2 ||
        profiles.size() < scores.size())
        return false;
    for (qsizetype i = 0; i < scores.size(); ++i)
        if (!scores[i] || !profiles[i])
            return false;
    return true;
}
}
auto
ScreenContexts::createResult(const QList<gameplay_logic::BmsScore*>& scores,
                             const QList<resource_managers::Profile*>& profiles,
                             gameplay_logic::ChartData* chartData,
                             GameplayContext* gameplay,
                             const QString& arenaRoundId) const
  -> ResultContext*
{
    if (!chartData || !validParticipants(scores, profiles))
        return nullptr;
    return new ResultContext(
      scores, profiles, chartData, gameplay, arenaRoundId);
}
auto
ScreenContexts::createCourseResult(
  const QList<gameplay_logic::BmsScoreCourse*>& scores,
  const QList<resource_managers::Profile*>& profiles,
  const QList<gameplay_logic::ChartData*>& charts,
  const resource_managers::Course& course) const -> CourseResultContext*
{
    if (charts.isEmpty() || !validParticipants(scores, profiles))
        return nullptr;
    return new CourseResultContext(scores, profiles, charts, course);
}
}
