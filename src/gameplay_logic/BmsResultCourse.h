//
// Created by PC on 17/07/2025.
//

#ifndef BMSRESULTCOURSE_H
#define BMSRESULTCOURSE_H

#include "BmsScore.h"
#include "resource_managers/Tables.h"
#include "support/Version.h"
#include "resource_managers/Vars.h"

#include <QObject>
#include <magic_enum/magic_enum.hpp>

namespace gameplay_logic {
class BmsResultCourse final : public QObject
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
     * @brief The number of normal notes in the course.
     * @details Normal means not long notes, not mines, not invisible notes.
     */
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    /**
     * @brief The number of long notes in the course excluding BSS (scratch
     * lns).
     * @details A long note consists of an LN start and LN end. Such a pair
     * counts as one long note.
     */
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    /**
     * @brief The number of BSS (scratch long notes) in the course.
     * @details A BSS consists of an ln start and ln end. Such a pair
     * counts as one BSS.
     */
    Q_PROPERTY(int bssCount READ getBssCount CONSTANT)
    /**
     * @brief The number of mines (landmines) in the course.
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
     * score, such as "DAN", "EXDAN", "EXHARDDAN", "FAILED", etc.
     */
    Q_PROPERTY(QString clearType READ getClearType CONSTANT)
    /**
     * @brief The random sequence used for parsing the constituent charts.
     * @details If the charts use
     * [#RANDOM](https://hitkey.nekokan.dyndns.info/cmds.htm#RANDOM),
     * this property provides the sequence of random values used to determine
     * the note order. If charts do not use randomization,
     * this list will be empty.
     */
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    /**
     * @brief The timestamp when the score was achieved, in Unix time.
     * @details This property records the time when the score on the
     * last chart of the course was created,
     * represented as the number of seconds since January 1, 1970 (the Unix
     * epoch).
     */
    Q_PROPERTY(int64_t unixTimestamp READ getUnixTimestamp CONSTANT)
    /**
     * @brief The length of the course in milliseconds.
     * @details This property indicates the total duration of the course from
     * start to finish, measured in nanoseconds.
     * @note This is based on the timestamp of the last visible note in each
     * chart.
     */
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    /**
     * @brief The unique identifier of this course score.
     */
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    /**
     * @brief The SHA-256 hashes of constituent charts, joined with spaces.
     * @see BmsResult::sha256
     */
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    /**
     * @brief The MD5 hashes of constituent charts, joined with spaces.
     * @see BmsResult::md5
     */
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
    /**
     * @brief The note order algorithm used for the course.
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
     * @brief The unique identifier of the course, based on md5 hashes of charts
     * and contraints.
     */
    Q_PROPERTY(QString identifier READ getIdentifier CONSTANT)
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
    /**
     * @brief The constraints of the course, like "gauge_lr2" or "grade_mirror"
     */
    Q_PROPERTY(QStringList constraints READ getConstraints CONSTANT)

    QString identifier;
    QList<BmsScore*> scores;
    QString clearType;
    QList<qint64> randomSequence;
    QString guid;
    int maxCombo;
    QStringList constraints;
    QList<resource_managers::Trophy> trophies;
    uint64_t gameVersion;

  public:
    struct DTO
    {
        int64_t id;
        std::string guid;
        std::string identifier;
        std::string scoreGuids;
        std::string clearType;
        int maxCombo;
        std::string constraints;
        int64_t unixTimestamp;
        int64_t gameVersion;
    };
    static auto load(const DTO& dto, QList<BmsScore*>& scores)
      -> std::unique_ptr<BmsResultCourse>;
    void save(db::SqliteCppDb& db) const;
    BmsResultCourse(QString guid,
                    QString identifier,
                    QList<BmsScore*> scores,
                    QString clearType,
                    int maxCombo,
                    QStringList constraints,
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
    auto getRandomSequence() -> QList<qint64>;
    auto getUnixTimestamp() const -> int64_t;
    auto getGuid() const -> QString;
    auto getSha256() const -> QString;
    auto getMd5() const -> QString;
    auto getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm;
    auto getNoteOrderAlgorithmP2() const
      -> resource_managers::NoteOrderAlgorithm;
    auto getGameVersion() const -> uint64_t;
    auto getScores() const -> QList<BmsScore*>;
    auto getConstraints() const -> QStringList;
    auto getTrophies() const -> QVariantList;
    auto getIdentifier() const -> QString;
    auto getDpOptions() const -> resource_managers::DpOptions;
    auto getLength() const -> int64_t;
};
} // namespace gameplay_logic

#endif // BMSRESULTCOURSE_H
