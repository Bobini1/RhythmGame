//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_BMSRESULT_H
#define RHYTHMGAME_BMSRESULT_H

#include <magic_enum/magic_enum.hpp>
#include "Judgement.h"
#include "db/SqliteCppDb.h"
#include "resource_managers/Vars.h"
namespace gameplay_logic {
class BmsResult final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    Q_PROPERTY(double points READ getPoints CONSTANT)
    Q_PROPERTY(int maxCombo READ getMaxCombo CONSTANT)
    Q_PROPERTY(QList<int> judgementCounts READ getJudgementCounts CONSTANT)
    Q_PROPERTY(int mineHits READ getMineHits CONSTANT)
    Q_PROPERTY(QString clearType READ getClearType CONSTANT)
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    Q_PROPERTY(int64_t unixTimestamp READ getUnixTimestamp CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
    Q_PROPERTY(uint64_t randomSeed READ getRandomSeed CONSTANT)
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithm
               READ getNoteOrderAlgorithm CONSTANT)
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2
               READ getNoteOrderAlgorithmP2 CONSTANT)

    double maxPoints;
    int maxHits;
    int normalNoteCount;
    int lnCount;
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
    int64_t randomSeed;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithm;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2;

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
        std::string randomSequence;
        int64_t randomSeed;
        int noteOrderAlgorithm;
        int noteOrderAlgorithmP2;
    };
    explicit BmsResult(double maxPoints,
                       int maxHits,
                       int normalNoteCount,
                       int lnCount,
                       int mineCount,
                       QString clearType,
                       QList<int> judgementCounts,
                       int mineHits,
                       double points,
                       int maxCombo,
                       QList<qint64> randomSequence,
                       uint64_t randomSeed,
                       resource_managers::NoteOrderAlgorithm noteOrderAlgorithm,
                          resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2,
                       QString guid,
                       QString sha256,
                       QString md5,
                       QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getNormalNoteCount() const -> int;
    auto getLnCount() const -> int;
    auto getMineCount() const -> int;
    auto getPoints() const -> double;
    auto getMaxCombo() const -> int;
    auto getJudgementCounts() const -> QList<int>;
    auto getMineHits() const -> int;
    auto getClearType() const -> const QString&;
    auto getRandomSequence() -> const QList<qint64>&;
    auto getUnixTimestamp() const -> int64_t;
    auto getGuid() const -> QString;
    auto getSha256() const -> QString;
    auto getMd5() const -> QString;
    auto getRandomSeed() const -> uint64_t;
    auto getNoteOrderAlgorithm() const -> resource_managers::NoteOrderAlgorithm;
    auto getNoteOrderAlgorithmP2() const -> resource_managers::NoteOrderAlgorithm;

    void save(db::SqliteCppDb& db) const;
    static auto load(const DTO& dto) -> std::unique_ptr<BmsResult>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRESULT_H
