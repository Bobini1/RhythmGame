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
    Q_PROPERTY(double points READ getPoints CONSTANT)
    Q_PROPERTY(int maxCombo READ getMaxCombo CONSTANT)
    Q_PROPERTY(QList<int> judgementCounts READ getJudgementCounts CONSTANT)
    Q_PROPERTY(QString clearType READ getClearType CONSTANT)

    double maxPoints;
    int maxHits;
    QString clearType;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
    double points;
    int maxCombo;

    struct BmsResultDto
    {
        double maxPoints;
        int maxHits;
        std::string clearType;
        int poorCount;
        int emptyPoorCount;
        int badCount;
        int goodCount;
        int greatCount;
        int perfectCount;
        double points;
        int maxCombo;
    };

  public:
    explicit BmsResult(double maxPoints,
                       int maxHits,
                       QString clearType,
                       QList<int> judgementCounts,
                       double points,
                       int maxCombo,
                       QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getPoints() const -> double;
    auto getMaxCombo() const -> int;
    auto getJudgementCounts() const -> QList<int>;
    auto getClearType() const -> QString;

    auto save(db::SqliteCppDb& db, support::Sha256 sha256) const -> int64_t;
    static auto load(db::SqliteCppDb& db, int64_t scoreId)
      -> std::unique_ptr<BmsResult>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRESULT_H
