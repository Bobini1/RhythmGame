//
// Created by bobini on 24.08.23.
//

#include "BmsNotes.h"

namespace gameplay_logic {
auto
BmsNotes::getVisibleNotes() const -> const QList<QList<int64_t>>&
{
    return visibleNotes;
}
auto
BmsNotes::getInvisibleNotes() const -> const QList<QList<int64_t>>&
{
    return invisibleNotes;
}
BmsNotes::BmsNotes(QList<QList<int64_t>> visibleNotes,
                   QList<QList<int64_t>> invisibleNotes,
                   QObject* parent)
  : QObject(parent)
  , visibleNotes(std::move(visibleNotes))
  , invisibleNotes(std::move(invisibleNotes))
{
}
} // namespace gameplay_logic
