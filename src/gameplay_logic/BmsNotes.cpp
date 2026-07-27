//
// Created by bobini on 24.08.23.
//

#include <zstd.h>
#include "BmsNotes.h"
#include "support/Compress.h"
#include <QIODevice>
#include <QVariant>

namespace gameplay_logic {
auto
BmsNotes::getNotes() -> QList<QList<Note>>&
{
    return notes;
}
auto
BmsNotes::getNotes() const -> const QList<QList<Note>>&
{
    return notes;
}
BmsNotes::BmsNotes(QList<QList<Note>> notes,
                   QList<Time> barLines,
                   QObject* parent)
  : QObject(parent)
  , notes(std::move(notes))
  , barLines(std::move(barLines))
{
}
auto
BmsNotes::getBarLines() const -> const QList<Time>&
{
    return barLines;
}
auto
BmsNotes::serialize() const -> QByteArray
{
    return support::compress(*this);
}
} // namespace gameplay_logic
