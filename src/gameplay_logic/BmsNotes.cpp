//
// Created by bobini on 24.08.23.
//

#include "BmsNotes.h"

namespace gameplay_logic {
auto
BmsNotes::getVisibleNotes() const -> const QList<QList<int>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() const -> const QList<QList<int>>&
{
    return invisibleNotes;
}
BmsNotes::BmsNotes(QList<QList<int>> visibleNotes,
                   QList<QList<int>> invisibleNotes,
                   QObject* parent)
  : QObject(parent)
  , visibleNotes(std::move(visibleNotes))
  , invisibleNotes(std::move(invisibleNotes))
{
}
} // namespace gameplay_logic
