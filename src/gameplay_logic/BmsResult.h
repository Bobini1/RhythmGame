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
    Q_PROPERTY(int64_t id READ getId CONSTANT)
    Q_PROPERTY(int64_t unixTimestamp READ getUnixTimestamp CONSTANT)

    int64_t id = -1;
    double maxPoints;
    int maxHits;
    QString clearType;
    QList<int> judgementCounts =
      QList<int>(magic_enum::enum_count<Judgement>());
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
        int maxCombo;
        int poorCount;
        int emptyPoorCount;
        int badCount;
        int goodCount;
        int greatCount;
        int perfectCount;
        std::string clearType;
        int64_t unixTimestamp;
    };
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
    auto setId(int64_t id) -> void;
    auto getId() const -> int64_t;
    auto getUnixTimestamp() const -> int64_t;

    auto save(db::SqliteCppDb& db, support::Sha256 sha256) const -> int64_t;
    static auto load(const BmsResultDto& dto) -> std::unique_ptr<BmsResult>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSRESULT_H
