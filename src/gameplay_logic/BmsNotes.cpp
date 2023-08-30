//
// Created by bobini on 24.08.23.
//

#include "BmsNotes.h"

namespace gameplay_logic {
auto
BmsNotes::getVisibleNotes() const -> const QList<QList<Note>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() const -> const QList<QList<Note>>&
{
    return invisibleNotes;
}
BmsNotes::BmsNotes(QList<QList<Note>> visibleNotes,
                   QList<QList<Note>> invisibleNotes,
                   QList<BpmChange> bpmChanges,
                   QList<Time> barLines,
                   QObject* parent)
  : QObject(parent)
  , visibleNotes(std::move(visibleNotes))
  , invisibleNotes(std::move(invisibleNotes))
  , bpmChanges(std::move(bpmChanges))
  , barLines(std::move(barLines))
{
}
auto
BmsNotes::getBarLines() const -> const QList<Time>&
{
    return barLines;
}
auto
BmsNotes::getBpmChanges() const -> const QList<BpmChange>&
{
    return bpmChanges;
}
} // namespace gameplay_logic
