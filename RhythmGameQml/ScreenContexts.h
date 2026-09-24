#ifndef RHYTHMGAME_SCREENCONTEXTS_H
#define RHYTHMGAME_SCREENCONTEXTS_H

#include "gameplay_logic/ChartRunner.h"
#include "gameplay_logic/CourseRunner.h"
#include "arena/ArenaSession.h"

#include <QPointer>
#include <QQmlListProperty>
#include <QtQml/qqmlregistration.h>

namespace rhythm_game_qml {

/** Screen input shared by single-chart and course gameplay, including decide.
 */
class GameplayContext final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("GameplayContext is supplied by the screen host")
    Q_PROPERTY(
      gameplay_logic::ChartData* chartData READ chartData NOTIFY stageChanged)
    Q_PROPERTY(QQmlListProperty<gameplay_logic::Player> players READ qmlPlayers
                 NOTIFY stageChanged)
    Q_PROPERTY(qml_components::BgaContainer* bga READ bga NOTIFY bgaChanged)
    Q_PROPERTY(gameplay_logic::ChartRunner::Status status READ status NOTIFY
                 statusChanged)
    Q_PROPERTY(gameplay_logic::ChartData::Keymode keymode READ keymode NOTIFY
                 stageChanged)
    Q_PROPERTY(bool isCourse READ isCourse CONSTANT)
    Q_PROPERTY(bool isArena READ isArena CONSTANT)
    Q_PROPERTY(bool arenaActive READ arenaActive NOTIFY arenaActiveChanged)
    Q_PROPERTY(QVariant course READ course CONSTANT)
    Q_PROPERTY(QQmlListProperty<gameplay_logic::CoursePlayer> coursePlayers READ
                 qmlCoursePlayers CONSTANT)
    Q_PROPERTY(int stageIndex READ stageIndex NOTIFY stageChanged)
    Q_PROPERTY(int stageCount READ stageCount CONSTANT)
    Q_PROPERTY(QQmlListProperty<gameplay_logic::ChartData> charts READ qmlCharts
                 CONSTANT)
    Q_PROPERTY(QList<int> inputMapping READ inputMapping WRITE setInputMapping
                 NOTIFY inputMappingChanged)
    // Used only by the host and the standard gameplay lifecycle.
    Q_PROPERTY(QObject* _runner READ runner CONSTANT)
    Q_PROPERTY(
      QObject* _screen READ hostScreen WRITE setHostScreen NOTIFY screenChanged)

  public:
    GameplayContext(QObject* runner, arena::ArenaSession* arenaSession);
    auto chartData() const -> gameplay_logic::ChartData*;
    auto players() const -> QList<gameplay_logic::Player*>;
    auto bga() const -> qml_components::BgaContainer*;
    auto status() const -> gameplay_logic::ChartRunner::Status;
    auto keymode() const -> gameplay_logic::ChartData::Keymode;
    auto isCourse() const -> bool;
    auto isArena() const -> bool;
    auto arenaActive() const -> bool;
    auto arenaSession() const -> arena::ArenaSession* { return session; }
    auto course() const -> QVariant;
    auto coursePlayers() const -> QList<gameplay_logic::CoursePlayer*>;
    auto stageIndex() const -> int;
    auto stageCount() const -> int;
    auto charts() const -> QList<gameplay_logic::ChartData*>;
    auto qmlPlayers() -> QQmlListProperty<gameplay_logic::Player>;
    auto qmlCoursePlayers() -> QQmlListProperty<gameplay_logic::CoursePlayer>;
    auto qmlCharts() -> QQmlListProperty<gameplay_logic::ChartData>;
    auto inputMapping() const -> QList<int>;
    void setInputMapping(const QList<int>& mapping);
    auto runner() const -> QObject*;
    auto hostScreen() const -> QObject* { return screen; }
    void setHostScreen(QObject* value);

  signals:
    void stageChanged();
    void bgaChanged();
    void statusChanged();
    void inputMappingChanged();
    void screenChanged();
    void arenaActiveChanged();

  private:
    void refreshStage();
    QPointer<gameplay_logic::ChartRunner> chartRunner;
    QPointer<gameplay_logic::CourseRunner> courseRunner;
    QPointer<QObject> screen;
    QPointer<gameplay_logic::ChartData> currentChartData;
    QList<gameplay_logic::Player*> currentPlayers;
    QList<gameplay_logic::CoursePlayer*> courseParticipants;
    QList<gameplay_logic::ChartData*> chartMetadata;
    int currentStageIndex{};
    bool arenaManaged{};
    QPointer<arena::ArenaSession> session;
};

/** One participant in a normal chart result, including a course-stage result.
 */
class ResultPlayer final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ResultPlayer is supplied by ResultContext")
    Q_PROPERTY(resource_managers::Profile* profile READ profile CONSTANT)
    Q_PROPERTY(gameplay_logic::BmsScore* score READ score CONSTANT)
  public:
    ResultPlayer(resource_managers::Profile* profile,
                 gameplay_logic::BmsScore* score,
                 QObject* parent);
    auto profile() const -> resource_managers::Profile*
    {
        return playerProfile;
    }
    auto score() const -> gameplay_logic::BmsScore* { return playerScore; }

  private:
    QPointer<resource_managers::Profile> playerProfile;
    QPointer<gameplay_logic::BmsScore> playerScore;
};

/** One participant in the overall course result. */
class CourseResultPlayer final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("CourseResultPlayer is supplied by CourseResultContext")
    Q_PROPERTY(resource_managers::Profile* profile READ profile CONSTANT)
    Q_PROPERTY(gameplay_logic::BmsScoreCourse* score READ score CONSTANT)
  public:
    CourseResultPlayer(resource_managers::Profile* profile,
                       gameplay_logic::BmsScoreCourse* score,
                       QObject* parent);
    auto profile() const -> resource_managers::Profile*
    {
        return playerProfile;
    }
    auto score() const -> gameplay_logic::BmsScoreCourse*
    {
        return playerScore;
    }

  private:
    QPointer<resource_managers::Profile> playerProfile;
    QPointer<gameplay_logic::BmsScoreCourse> playerScore;
};

/** Fixed metadata and scores for one completed chart. */
class ResultContext final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ResultContext is supplied by the screen host")
    Q_PROPERTY(gameplay_logic::ChartData* chartData READ chartData CONSTANT)
    Q_PROPERTY(QQmlListProperty<rhythm_game_qml::ResultPlayer> players READ
                 qmlPlayers CONSTANT)
    Q_PROPERTY(
      rhythm_game_qml::GameplayContext* gameplay READ gameplay CONSTANT)
    Q_PROPERTY(QString arenaRoundId READ arenaRoundId CONSTANT)
    Q_PROPERTY(bool arenaActive READ arenaActive NOTIFY arenaActiveChanged)
    Q_PROPERTY(QVariant course READ course CONSTANT)
    Q_PROPERTY(int stageIndex READ stageIndex CONSTANT)
    Q_PROPERTY(int stageCount READ stageCount CONSTANT)
  public:
    ResultContext(const QList<gameplay_logic::BmsScore*>& scores,
                  const QList<resource_managers::Profile*>& profiles,
                  gameplay_logic::ChartData* chartData,
                  GameplayContext* gameplay,
                  QString arenaRoundId = {});
    auto chartData() const -> gameplay_logic::ChartData*
    {
        return completedChart;
    }
    auto players() const -> QList<ResultPlayer*> { return participants; }
    auto qmlPlayers() -> QQmlListProperty<ResultPlayer>;
    auto gameplay() const -> GameplayContext* { return originatingPlay; }
    auto arenaRoundId() const -> QString { return completedArenaRoundId; }
    auto arenaActive() const -> bool;
    auto course() const -> QVariant { return completedCourse; }
    auto stageIndex() const -> int { return completedStageIndex; }
    auto stageCount() const -> int { return completedStageCount; }

  signals:
    void arenaActiveChanged();

  private:
    QPointer<gameplay_logic::ChartData> completedChart;
    QList<ResultPlayer*> participants;
    QPointer<GameplayContext> originatingPlay;
    QPointer<arena::ArenaSession> session;
    QString completedArenaRoundId;
    QVariant completedCourse;
    int completedStageIndex{};
    int completedStageCount{ 1 };
};

/** Fixed metadata and aggregate scores for a completed course. */
class CourseResultContext final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("CourseResultContext is supplied by the screen host")
    Q_PROPERTY(resource_managers::Course course READ course CONSTANT)
    Q_PROPERTY(QQmlListProperty<gameplay_logic::ChartData> charts READ qmlCharts
                 CONSTANT)
    Q_PROPERTY(QQmlListProperty<rhythm_game_qml::CourseResultPlayer> players
                 READ qmlPlayers CONSTANT)
  public:
    CourseResultContext(const QList<gameplay_logic::BmsScoreCourse*>& scores,
                        const QList<resource_managers::Profile*>& profiles,
                        QList<gameplay_logic::ChartData*> charts,
                        resource_managers::Course course);
    auto course() const -> resource_managers::Course { return completedCourse; }
    auto charts() const -> QList<gameplay_logic::ChartData*>
    {
        return completedCharts;
    }
    auto players() const -> QList<CourseResultPlayer*> { return participants; }
    auto qmlPlayers() -> QQmlListProperty<CourseResultPlayer>;
    auto qmlCharts() -> QQmlListProperty<gameplay_logic::ChartData>;

  private:
    resource_managers::Course completedCourse;
    QList<gameplay_logic::ChartData*> completedCharts;
    QList<CourseResultPlayer*> participants;
};

/** \internal Creates the typed inputs supplied to screens. */
class ScreenContexts : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
  public:
    explicit ScreenContexts(QObject* parent = nullptr)
      : QObject(parent)
    {
    }
    Q_INVOKABLE rhythm_game_qml::GameplayContext* createGameplay(
      QObject* runner,
      arena::ArenaSession* arenaSession = nullptr) const;
    Q_INVOKABLE rhythm_game_qml::ResultContext* createResult(
      const QList<gameplay_logic::BmsScore*>& scores,
      const QList<resource_managers::Profile*>& profiles,
      gameplay_logic::ChartData* chartData,
      rhythm_game_qml::GameplayContext* gameplay,
      const QString& arenaRoundId = {}) const;
    Q_INVOKABLE rhythm_game_qml::CourseResultContext* createCourseResult(
      const QList<gameplay_logic::BmsScoreCourse*>& scores,
      const QList<resource_managers::Profile*>& profiles,
      const QList<gameplay_logic::ChartData*>& charts,
      const resource_managers::Course& course) const;
};
}
#endif
