//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_BMSRESULT_H
#define RHYTHMGAME_BMSRESULT_H

#include <magic_enum/magic_enum.hpp>
#include "Judgement.h"
#include "db/SqliteCppDb.h"
#include "resource_managers/Vars.h"
#include "support/Version.h"

#include <QObject>
namespace gameplay_logic {
/**
 * @brief The aggregated info about a score.
 */
class BmsResult final : public QObject
{
    Q_OBJECT

    /**
     * @brief The maximum possible points achievable in the score.
     */
    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    /**
     * @brief The maximum number of possible hits (normal notes + long notes)
     * in the chart.
     */
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
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
     * @brief The points achieved in the score.
     */
    Q_PROPERTY(double points READ getPoints CONSTANT)
    /**
     * @brief The best combo achieved in the score.
     */
    Q_PROPERTY(int maxCombo READ getMaxCombo CONSTANT)
    /**
     * @brief The counts of each judgement type achieved in the score.
     * @details The order is defined in the Judgement enum.
     * @see Judgement
     */
    Q_PROPERTY(QList<int> judgementCounts READ getJudgementCounts CONSTANT)
    /**
     * @brief The number of mines hit in the score.
     */
    Q_PROPERTY(int mineHits READ getMineHits CONSTANT)
    /**
     * @brief The clear type achieved in the score.
     * @details This property indicates the type of clear achieved in the
     * score, such as "HARD", "EXHARD", "FC", "FAILED", etc.
     * For charts played with course gauges, it will always be "NOPLAY".
     */
    Q_PROPERTY(QString clearType READ getClearType CONSTANT)
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
     * @brief The timestamp when the score was achieved, in Unix time.
     * @details This property records the time when the score was created,
     * represented as the number of seconds since January 1, 1970 (the Unix
     * epoch).
     */
    Q_PROPERTY(int64_t unixTimestamp READ getUnixTimestamp CONSTANT)
    /**
     * @brief The length of the chart in nanoseconds.
     * @details This property indicates the total duration of the chart from
     * start to finish, measured in nanoseconds.
     * @note This is based on the timestamp of the last visible note.
     */
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    /**
     * @brief The unique identifier for the score.
     * @details This GUID is generated when the BmsLiveScore instance is
     * created and can be used to uniquely identify the score instance.
     */
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    /**
     * @brief The SHA-256 hash of the chart file, in hexadecimal upper-case
     * format.
     * @details This can be used to uniquely identify a chart file.
     */
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    /**
     * @brief The MD5 hash of the chart file, in hexadecimal upper-case format.
     * @details This can be used to uniquely identify a chart file.
     */
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
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
     * @brief The note order algorithm used for the chart.
     * @details This property indicates the algorithm used to modify the
     * arrangement of notes in the chart.
     * @see resource_managers::note_order_algorithm::NoteOrderAlgorithm
     */
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithm READ
                 getNoteOrderAlgorithm CONSTANT)
    /**
     * @brief The note order algorithm used for player 2 side in DP charts.
     * @see noteOrderAlgorithm
     */
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2 READ
                 getNoteOrderAlgorithmP2 CONSTANT)
    /**
     * @brief The DP options used for the score.
     * @details This property indicates the DP options that were active during
     * gameplay.
     * @see resource_managers::DpOptions
     */
    Q_PROPERTY(
      resource_managers::DpOptions dpOptions READ getDpOptions CONSTANT)
    /**
     * @brief The game version when the score was achieved.
     * @details For migrations.
     */
    Q_PROPERTY(uint64_t gameVersion READ getGameVersion CONSTANT)

    double maxPoints;
    int maxHits;
    int normalNoteCount;
    int lnCount;
    int bssCount;
    int mineCount;
    QString clearType;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
    QList<qint64> randomSequence;
    QString guid;
    QString sha256;
    QString md5;
    int mineHits;
    double points;
    int maxCombo;
    int64_t unixTimestamp;
    int64_t length;
    uint64_t randomSeed;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithm;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2;
    resource_managers::DpOptions dpOptions;
    uint64_t gameVersion;

  public:
    struct DTO
    {
        int64_t id;
        std::string guid;
        std::string sha256;
        std::string md5;
        double points;
        double maxPoints;
        int maxHits;
        int normalNoteCount;
        int lnCount;
        int bssCount;
        int mineCount;
        int maxCombo;
        int poorCount;
        int emptyPoorCount;
        int badCount;
        int goodCount;
        int greatCount;
        int perfectCount;
        int mineHits;
        std::string clearType;
        int64_t unixTimestamp;
        int64_t length;
        std::string randomSequence;
        int64_t randomSeed;
        int noteOrderAlgorithm;
        int noteOrderAlgorithmP2;
        int dpOptions;
        int64_t gameVersion;
    };
    explicit BmsResult(
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
      uint64_t gameVersion = support::currentVersion,
      QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getNormalNoteCount() const -> int;
    auto getLnCount() const -> int;
    auto getBssCount() const -> int;
    auto getMineCount() const -> int;
    auto getPoints() const -> double;
    auto getMaxCombo() const -> int;
    auto getJudgementCounts() const -> QList<int>;
    auto getMineHits() const -> int;
    auto getClearType() const -> const QString&;
    auto getRandomSequence() -> const QList<qint64>&;
    auto getUnixTimestamp() const -> int64_t;
    auto getLength() const -> int64_t;
    auto getGuid() const -> QString;
    auto getSha256() const -> QString;
    auto getMd5() const -> QString;
    auto getRandomSeed() const -> uint64_t;
    auto getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm;
    auto getNoteOrderAlgorithmP2() const
      -> resource_managers::NoteOrderAlgorithm;
    auto getDpOptions() const -> resource_managers::DpOptions;
    auto getGameVersion() const -> uint64_t;

    void save(db::SqliteCppDb& db) const;
    static auto load(const DTO& dto) -> std::unique_ptr<BmsResult>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRESULT_H
