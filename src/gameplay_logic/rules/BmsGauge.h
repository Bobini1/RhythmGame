//
// Created by bobini on 10.09.23.
//

#ifndef RHYTHMGAME_BMSGAUGE_H
#define RHYTHMGAME_BMSGAUGE_H

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

    Q_PROPERTY(double gauge READ getGauge NOTIFY gaugeChanged)
    Q_PROPERTY(double gaugeMax READ getGaugeMax CONSTANT)
    Q_PROPERTY(double threshold READ getThreshold CONSTANT)
    Q_PROPERTY(
      QVariantList gaugeHistory READ getGaugeHistory NOTIFY gaugeChanged)

    double gaugeMax;
    double threshold = 0;
    QVariantList gaugeHistory;
    std::vector<GaugeHistoryEntry> gaugeHistoryList;

  public:
    explicit BmsGauge(double gaugeMax,
                      double initialValue,
                      double threshold,
                      QObject* parent = nullptr);
    auto getGauge() const -> double;
    auto getGaugeMax() const -> double;
    // 0 for hard clear, 80 for normal clear, etc.
    auto getThreshold() const -> double;
    auto getGaugeHistory() const -> QVariantList;
    auto getGaugeHistoryVector() const -> const std::vector<GaugeHistoryEntry>&;
    virtual void addHit(std::chrono::nanoseconds offsetFromStart,
                        std::chrono::nanoseconds hitOffset) = 0;

  protected:
    void addGaugeHistoryEntry(std::chrono::nanoseconds offsetFromStart,
                              double gauge);

  signals:
    void gaugeChanged();
};
} // namespace gameplay_logic::rules

#endif // RHYTHMGAME_BMSGAUGE_H
