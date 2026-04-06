//
// Created by PC on 02/12/2025.
//

#include "ScoreReplayer.h"

namespace qml_components {

namespace {
// Packs (column, noteIndex) into a single 64-bit key.
// column is at most 15 (4 bits), noteIndex fits in 32 bits.
constexpr qint64
noteKey(int column, int noteIndex)
{
    return (static_cast<qint64>(column) << 32) |
           static_cast<quint32>(noteIndex);
}
} // namespace

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
ScoreReplayer::setHitEvents(const QList<gameplay_logic::HitEvent>& newHitEvents)
{
    if (hitEvents == newHitEvents) {
        return;
    }
    hitEvents = newHitEvents;
    m_savedPointsByNote.clear();

    // Index the saved replay by note.  We only care about noteRemoved=true
    // events: those fire exactly once per note (on hit or on timeout) and give
    // the definitive points the saved player earned for that note.
    // LN begin and LN end carry different noteIndex values, so each counts
    // as its own independent entry.
    for (const auto& ev : hitEvents) {
        if (ev.getNoteIndex() == -1) {
            continue;
        }
        const auto key = noteKey(ev.getColumn(), ev.getNoteIndex());
        double pts = 0.0;
        if (const auto p = ev.getPointsOptional(); p.has_value()) {
            pts = p->getValue();
        }
        m_savedPointsByNote[key] += pts;
    }

    // Resetting accumulated points when the target changes makes sense:
    // the new target is a different reference curve.
    const double prev = m_accumulatedPoints;
    m_accumulatedPoints = 0.0;
    if (prev != 0.0) {
        emit pointsChanged();
    }
    emit hitEventsChanged();
}

void
ScoreReplayer::resetHitEvents()
{
    setHitEvents({});
}

auto
ScoreReplayer::getPoints() const -> double
{
    return m_accumulatedPoints;
}

void
ScoreReplayer::notifyHit(const gameplay_logic::HitEvent& currentHit)
{
    // Empty poors have noteRemoved=false and can fire many times on the same
    // note — ignore them.  Only advance once the note is definitively consumed.
    if (!currentHit.getNoteRemoved() || currentHit.getNoteIndex() == -1) {
        return;
    }

    const auto key = noteKey(currentHit.getColumn(), currentHit.getNoteIndex());
    const auto it = m_savedPointsByNote.constFind(key);
    if (it != m_savedPointsByNote.constEnd()) {
        m_accumulatedPoints += it.value();
        emit pointsChanged();
    }
    // Note absent from saved replay → saved player never reached it; add 0.
}

void
ScoreReplayer::resetPoints()
{
    if (m_accumulatedPoints == 0.0) {
        return;
    }
    m_accumulatedPoints = 0.0;
    emit pointsChanged();
}

} // namespace qml_components
