//
// Created by bobini on 24.08.23.
//

#include "BmsNotes.h"

namespace gameplay_logic {
auto
BmsNotes::getVisibleNotes() const -> const QVector<QVector<Note>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() const -> const QVector<QVector<Note>>&
{
    return invisibleNotes;
}
BmsNotes::BmsNotes(QVector<QVector<Note>> visibleNotes,
                   QVector<QVector<Note>> invisibleNotes,
                   QVector<BpmChange> bpmChanges,
                   QVector<Time> barLines,
                   QObject* parent)
  : QObject(parent)
  , visibleNotes(std::move(visibleNotes))
  , invisibleNotes(std::move(invisibleNotes))
  , bpmChanges(std::move(bpmChanges))
  , barLines(std::move(barLines))
{
}
auto
BmsNotes::getBarLines() const -> const QVector<Time>&
{
    return barLines;
}
auto
BmsNotes::getBpmChanges() const -> const QVector<BpmChange>&
{
    return bpmChanges;
}
} // namespace gameplay_logic
