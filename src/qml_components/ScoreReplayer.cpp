//
// Created by PC on 02/12/2025.
//

#include "ScoreReplayer.h"

namespace qml_components {
ScoreReplayer::ScoreReplayer(QObject* parent)
  : QObject(parent)
{
}
auto
ScoreReplayer::getHitEvents() const -> const QList<gameplay_logic::HitEvent>&
{
    return hitEvents;
}
void
ScoreReplayer::setHitEvents(const QList<gameplay_logic::HitEvent>& hitEvents)
{
    if (this->hitEvents.data() == hitEvents.data()) {
        return;
    }
    auto currentPoints = getPoints();
    this->hitEvents = hitEvents;
    pointElapseds.clear();
    double accumulatedPoints = 0.0;
    for (const auto& hitEvent : hitEvents) {
        if (auto pointsOpt = hitEvent.getPointsOptional();
            pointsOpt.has_value()) {
            accumulatedPoints += pointsOpt->getValue();
            pointElapseds.emplace_back(hitEvent.getOffsetFromStart(),
                                       accumulatedPoints);
        }
    }
    auto newPoints = getPoints();
    if (currentPoints != newPoints) {
        emit pointsChanged();
    }
    emit hitEventsChanged();
}
void
ScoreReplayer::resetHitEvents()
{
    setHitEvents({});
}
void
ScoreReplayer::setElapsed(qint64 elapsed)
{
    if (this->elapsed == elapsed) {
        return;
    }
    auto points = getPoints();
    this->elapsed = elapsed;
    auto newPoints = getPoints();
    if (points != newPoints) {
        emit pointsChanged();
    }
    emit elapsedChanged();
}
void
ScoreReplayer::resetElapsed()
{
    setElapsed(0);
}
auto
ScoreReplayer::getPoints() const -> double
{
    auto res = std::lower_bound(
      pointElapseds.begin(),
      pointElapseds.end(),
      elapsed,
      [](const auto& pair, qint64 ts) { return pair.first < ts; });

    if (res == pointElapseds.end()) {
        return pointElapseds.empty() ? 0.0 : pointElapseds.back().second;
    }
    if (res == pointElapseds.begin()) {
        return 0.0;
    }
    return std::prev(res)->second;
}
} // namespace qml_components