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
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithm READ
                 getNoteOrderAlgorithm CONSTANT)
    Q_PROPERTY(resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2 READ
                 getNoteOrderAlgorithmP2 CONSTANT)
    Q_PROPERTY(QString identifier READ getIdentifier CONSTANT)
    Q_PROPERTY(uint64_t gameVersion READ getGameVersion CONSTANT)

    Q_PROPERTY(QStringList constraints READ getConstraints CONSTANT)
    Q_PROPERTY(QVariantList trophies READ getTrophies CONSTANT)

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
        std::string identifier;
        std::string guid;
        std::string scoreGuids;
        std::string clearType;
        int maxCombo;
        std::string constraints;
        std::string trophies;
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
                   QList<resource_managers::Trophy> trophies,
                   uint64_t gameVersion = support::currentVersion,
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
    auto getLength() const -> int64_t;
};
} // namespace gameplay_logic

#endif // BMSRESULTCOURSE_H
