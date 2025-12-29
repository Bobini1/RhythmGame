//
// Created by bobini on 25.06.23.
//

#ifndef RHYTHMGAME_BMSSCORE_H
#define RHYTHMGAME_BMSSCORE_H
#include <magic_enum/magic_enum.hpp>
#include "gameplay_logic/rules/BmsGauge.h"
#include "HitEvent.h"
#include "BmsResult.h"
#include "BmsGaugeHistory.h"
#include "BmsReplayData.h"
#include "resource_managers/Vars.h"

#include <QAbstractListModel>
#include <QUuid>

namespace gameplay_logic {

class JudgementCounts final : public QAbstractListModel
{
    Q_OBJECT

    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());

  public:
    explicit JudgementCounts(QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;

    enum Roles
    {
        JudgementRole = Qt::UserRole + 1,
        CountRole
    };

    auto roleNames() const -> QHash<int, QByteArray> override;

    auto data(const QModelIndex& index, int role) const -> QVariant override;

    void addJudgement(Judgement judgement);

    auto getJudgementCounts() const -> const QList<int>&;
};

/**
 * @brief The score that gets updated during gameplay.
 * @details The BmsLiveScore class keeps track of the current score,
 * combo, gauge and judgement counts during gameplay.
 * It provides methods to add hits and retrieve the current score state.
 */
class BmsLiveScore final : public QObject
{
    Q_OBJECT

    /**
     * @brief The maximum possible points achievable in the score.
     */
    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    /**
     * @brief The number of points the player would have if they played
     * perfectly till now.
     */
    Q_PROPERTY(
      double maxPointsNow READ getMaxPointsNow NOTIFY maxPointsNowChanged)
    /**
     * @brief The number of normal notes in the chart.
     * @details Normal means not long notes, not mines, not invisible notes.
     */
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    /**
     * @brief The number of long notes in the chart excluding BSS (scratch lns).
     * @details A long note consists of an LN start and LN end. Such a pair
     * counts as one long note.
     */
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    /**
     * @brief The number of BSS (scratch long notes) in the chart.
     * @details A BSS consists of an ln start and ln end. Such a pair
     * counts as one BSS.
     */
    Q_PROPERTY(int bssCount READ getBssCount CONSTANT)
    /**
     * @brief The number of mines (landmines) in the chart.
     */
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    /**
     * @brief The maximum number of possible hits (normal notes + long notes)
     * in the chart.
     */
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    /**
     * @brief The current points achieved in the score.
     */
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    /**
     * @brief The current combo count.
     */
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    /**
     * @brief The best combo achieved so far in the score.
     */
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    /**
     * @brief The counts of each judgement type achieved in the score.
     * @details This property provides access to a model that contains the
     * counts of each judgement type (e.g., Perfect, Great, Good, etc.).
     * It can be used in QML to display the judgement counts.
     * @see JudgementCounts
     * @see Judgement
     */
    Q_PROPERTY(
      JudgementCounts* judgementCounts READ getJudgementCounts CONSTANT)
    /**
     * @brief The number of mines hit during the score.
     */
    Q_PROPERTY(int mineHits READ getMineHits NOTIFY mineHitsChanged)
    /**
     * @brief The gauges used in the score.
     * @details This property provides access to the list of gauges
     * (e.g., NORMAL, HARD, EXHARD, etc.) used in the score.
     * Each gauge can be queried for its current value and history.
     * @note The gameplay theme should display the first gauge that's above
     * its threshold, or the last gauge if none are above the threshold.
     * @see rules::BmsGauge
     * @see rules::Lr2Gauge
     */
    Q_PROPERTY(QList<rules::BmsGauge*> gauges READ getGauges CONSTANT)
    /**
     * @brief The random sequence used for parsing the chart.
     * @details If the chart uses
     * [#RANDOM](https://hitkey.nekokan.dyndns.info/cmds.htm#RANDOM),
     * this property provides the sequence of random values used to determine
     * the note order. If the chart does not use randomization,
     * this list will be empty.
     */
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    /**
     * @brief The note order algorithm used for the chart.
     * @details This property indicates the algorithm used to modify the
     * arrangement of notes in the chart.
     * @see resource_managers::note_order_algorithm::NoteOrderAlgorithm
     */
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm CONSTANT)
    /**
     * @brief The note order algorithm used for player 2 side in DP charts.
     * @see noteOrderAlgorithm
     */
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 CONSTANT)
    /**
     * @brief The permutation of columns that resulted from NoteOrderAlgorithm.
     * @details It will contain the default iota for
     * resource_managers::NoteOrderAlgorithm::SRandom and
     * resource_managers::NoteOrderAlgorithm::SRandomPlus.
     */
    Q_PROPERTY(QList<int> permutation READ getPermutation CONSTANT)
    /**
     * @brief The random seed used for note order shuffling.
     * @details This seed is used to initialize the mt19937 random number
     * generator for shuffling notes according to the selected note order
     * algorithm.
     * @note This seed is *not* used for #RANDOM parsing. Use randomSequence
     * for that.
     * @see resource_managers::NoteOrderAlgorithm
     * @see noteOrderAlgorithm
     */
    Q_PROPERTY(uint64_t randomSeed READ getRandomSeed CONSTANT)
    /**
     * @brief The unique identifier for the score.
     * @details This GUID is generated when the BmsLiveScore instance is
     * created and can be used to uniquely identify the score instance.
     */
    Q_PROPERTY(QString guid READ getGuid CONSTANT)

    double maxHitValue;
    double maxPoints;
    int mineCount;
    int normalNoteCount;
    int lnCount;
    int bssCount;
    int maxHits;
    QList<HitEvent> hits;
    QList<rules::BmsGauge*> gauges;
    JudgementCounts judgementCounts;
    QList<qint64> randomSequence;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithm;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2;
    resource_managers::DpOptions dpOptions;
    QList<int> permutation;
    QString sha256;
    QString md5;
    QString guid;
    double points = 0;
    double maxPointsNow = 0;
    int combo = 0;
    int maxCombo = 0;
    int mineHits = 0;
    uint64_t randomSeed;
    int64_t length;

    void resetCombo();
    void increaseCombo();

  public:
    void addHit(const HitEvent& tap);
    explicit BmsLiveScore(
      int normalNoteCount,
      int lnCount,
      int bssCount,
      int mineCount,
      int maxHits,
      double maxHitValue,
      QList<rules::BmsGauge*> gauges,
      QList<qint64> randomSequence,
      resource_managers::NoteOrderAlgorithm noteOrderAlgorithm,
      resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2,
      resource_managers::DpOptions dpOptions,
      QList<int> permutation,
      uint64_t seed,
      int64_t length,
      QString sha256,
      QString md5,
      QString guid = QUuid::createUuid().toString(),
      QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getNormalNoteCount() const -> int;
    auto getLnCount() const -> int;
    auto getBssCount() const -> int;
    auto getMineCount() const -> int;
    auto getPoints() const -> double;
    auto getMaxPointsNow() const -> double;
    auto getJudgementCounts() -> JudgementCounts*;
    auto getJudgementCounts() const -> const JudgementCounts*;
    void sendVisualOnlyTap(HitEvent tap);
    void sendVisualOnlyRelease(const HitEvent& release);
    auto getCombo() const -> int;
    auto getMaxCombo() const -> int;
    auto getMineHits() const -> int;
    auto getGauges() const -> QList<rules::BmsGauge*>;
    auto getRandomSequence() const -> const QList<qint64>&;
    auto getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm;
    auto getNoteOrderAlgorithmP2() const
      -> resource_managers::NoteOrderAlgorithm;
    auto getPermutation() const -> const QList<int>&;
    auto getRandomSeed() const -> uint64_t;
    auto getGuid() const -> QString;

    auto getResult() const -> std::unique_ptr<BmsResult>;
    auto getReplayData() const -> std::unique_ptr<BmsReplayData>;
    auto getGaugeHistory() const -> std::unique_ptr<BmsGaugeHistory>;

  signals:
    void pointsChanged();
    void comboChanged();
    void comboDropped();
    void comboIncreased();
    void maxComboChanged();
    void mineHitsChanged();
    void maxPointsNowChanged();

    void hit(HitEvent hit);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
