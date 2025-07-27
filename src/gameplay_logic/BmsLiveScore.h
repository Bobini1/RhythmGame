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
    explicit JudgementCounts(QObject* parent = nullptr)
      : QAbstractListModel(parent)
    {
    }

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override
    {
        return judgementCounts.size();
    }

    enum Roles
    {
        JudgementRole = Qt::UserRole + 1,
        CountRole
    };

    auto roleNames() const -> QHash<int, QByteArray> override
    {
        return { { JudgementRole, "judgement" }, { CountRole, "count" } };
    }

    auto data(const QModelIndex& index, int role) const -> QVariant override
    {
        if (!index.isValid() || index.row() >= judgementCounts.size()) {
            return {};
        }
        if (role == JudgementRole) {
            return QVariant::fromValue(static_cast<Judgement>(index.row()));
        }
        if (role == CountRole) {
            return judgementCounts[index.row()];
        }

        return {};
    }

    void addJudgement(Judgement judgement)
    {
        auto index = magic_enum::enum_integer(judgement);
        judgementCounts[index]++;
        emit dataChanged(
          createIndex(index, 0), createIndex(index, 0), { CountRole });
    }

    auto getJudgementCounts() const -> const QList<int>&
    {
        return judgementCounts;
    }
};

class BmsLiveScore final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(double maxPoints READ getMaxPoints CONSTANT)
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    Q_PROPERTY(int maxHits READ getMaxHits CONSTANT)
    Q_PROPERTY(double points READ getPoints NOTIFY pointsChanged)
    Q_PROPERTY(int combo READ getCombo NOTIFY comboChanged)
    Q_PROPERTY(int maxCombo READ getMaxCombo NOTIFY maxComboChanged)
    Q_PROPERTY(
      JudgementCounts* judgementCounts READ getJudgementCounts CONSTANT)
    Q_PROPERTY(int mineHits READ getMineHits NOTIFY mineHitsChanged)
    Q_PROPERTY(QList<rules::BmsGauge*> gauges READ getGauges CONSTANT)
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithm READ getNoteOrderAlgorithm CONSTANT)
    Q_PROPERTY(resource_managers::note_order_algorithm::NoteOrderAlgorithm
                 noteOrderAlgorithmP2 READ getNoteOrderAlgorithmP2 CONSTANT)
    Q_PROPERTY(QList<int> permutation READ getPermutation CONSTANT)
    Q_PROPERTY(uint64_t randomSeed READ getRandomSeed CONSTANT)
    Q_PROPERTY(QString guid READ getGuid CONSTANT)

    double maxPoints;
    int mineCount;
    int normalNoteCount;
    int lnCount;
    int maxHits;
    QList<HitEvent> hits;
    QList<rules::BmsGauge*> gauges;
    JudgementCounts judgementCounts;
    QList<qint64> randomSequence;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithm;
    resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2;
    QList<int> permutation;
    QString sha256;
    QString md5;
    QString guid;
    double points = 0;
    int combo = 0;
    int maxCombo = 0;
    int mineHits = 0;
    uint64_t randomSeed;

    void resetCombo();
    void increaseCombo();

  public:
    void addHit(const HitEvent& tap, bool notify = true);
    explicit BmsLiveScore(
      int normalNoteCount,
      int lnCount,
      int mineCount,
      int maxHits,
      double maxHitValue,
      QList<rules::BmsGauge*> gauges,
      QList<qint64> randomSequence,
      resource_managers::NoteOrderAlgorithm noteOrderAlgorithm,
      resource_managers::NoteOrderAlgorithm noteOrderAlgorithmP2,
      QList<int> permutation,
      uint64_t seed,
      QString sha256,
      QString md5,
      QString guid = QUuid::createUuid().toString(),
      QObject* parent = nullptr);

    auto getMaxPoints() const -> double;
    auto getMaxHits() const -> int;
    auto getNormalNoteCount() const -> int;
    auto getLnCount() const -> int;
    auto getMineCount() const -> int;
    auto getPoints() const -> double;
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

    void hit(HitEvent hit);
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCORE_H
