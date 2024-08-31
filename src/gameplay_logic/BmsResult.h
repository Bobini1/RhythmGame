//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_BMSRESULT_H
#define RHYTHMGAME_BMSRESULT_H

#include <QObject>
#include <magic_enum.hpp>
#include "Judgement.h"
#include "db/SqliteCppDb.h"
#include "support/Sha256.h"
namespace gameplay_logic {
class BmsResult : public QObject
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
    Q_PROPERTY(QList<int64_t> randomSequence READ getRandomSequence CONSTANT)
    Q_PROPERTY(int64_t id READ getId CONSTANT)
    Q_PROPERTY(int64_t unixTimestamp READ getUnixTimestamp CONSTANT)

    int64_t id = -1;
    double maxPoints;
    int maxHits;
    int normalNoteCount;
    int lnCount;
    int mineCount;
    QString clearType;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
    QList<int64_t> randomSequence;
    support::Sha256 sha256;
    int mineHits;
    double points;
    int maxCombo;
    int64_t unixTimestamp;

  public:
    struct BmsResultDto
    {
        int64_t id;
        std::string sha256;
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
                       QList<int64_t> randomSequence,
                       support::Sha256 sha256,
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
    auto getClearType() const -> QString;
    auto getRandomSequence() -> QList<int64_t>;
    auto setId(int64_t id) -> void;
    auto getId() const -> int64_t;
    auto getUnixTimestamp() const -> int64_t;

    auto save(db::SqliteCppDb& db) const -> int64_t;
    static auto load(const BmsResultDto& dto) -> std::unique_ptr<BmsResult>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRESULT_H
