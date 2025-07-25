//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSGAUGE_H
#define RHYTHMGAME_BMSGAUGE_H

#include "gameplay_logic/Judgement.h"

#include <QObject>
#include <QVariant>

namespace gameplay_logic::rules {
class GaugeHistoryEntry
{
    Q_GADGET
    Q_PROPERTY(int64_t offsetFromStart READ getOffsetFromStart CONSTANT)
    Q_PROPERTY(double gauge READ getGauge CONSTANT)
    int64_t offsetFromStart;
    double gauge;

  public:
    GaugeHistoryEntry() = default;
    GaugeHistoryEntry(int64_t offsetFromStart, double gauge);
    [[nodiscard]] auto getOffsetFromStart() const -> int64_t;
    [[nodiscard]] auto getGauge() const -> double;

    friend auto operator<<(QDataStream& stream, const GaugeHistoryEntry& entry)
      -> QDataStream&;
    friend auto operator>>(QDataStream& stream, GaugeHistoryEntry& entry)
      -> QDataStream&;
};

class BmsGauge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(QString awardedClearType READ getAwardedClearType CONSTANT)
    Q_PROPERTY(double gauge READ getGauge NOTIFY gaugeChanged)
    Q_PROPERTY(double gaugeMax READ getGaugeMax CONSTANT)
    Q_PROPERTY(double threshold READ getThreshold CONSTANT)
    Q_PROPERTY(QList<gameplay_logic::rules::GaugeHistoryEntry> gaugeHistory READ
                 getGaugeHistory NOTIFY gaugeChanged)
    QString name;
    QString awardedClearType;

    double gaugeMax;
    double threshold = 0;
    QList<GaugeHistoryEntry> gaugeHistory;

    BmsGauge() = default;

  public:
    explicit BmsGauge(QString name,
                      QString awardedClearType,
                      double gaugeMax,
                      double initialValue,
                      double threshold,
                      QObject* parent = nullptr);
    auto getGauge() const -> double;
    auto getGaugeMax() const -> double;
    // 0 for hard clear, 80 for normal clear, etc.
    auto getThreshold() const -> double;
    auto getGaugeHistory() const -> const QList<GaugeHistoryEntry>&;
    virtual void addHit(std::chrono::nanoseconds offsetFromStart,
                        std::chrono::nanoseconds hitOffset,
                        Judgement judgement) = 0;
    virtual void addMineHit(std::chrono::nanoseconds offsetFromStart,
                            double penalty) = 0;
    auto getAwardedClearType() const -> QString;
    auto getName() const -> QString;

  protected:
    void addGaugeHistoryEntry(GaugeHistoryEntry entry);

  signals:
    void gaugeChanged();
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSGAUGE_H
