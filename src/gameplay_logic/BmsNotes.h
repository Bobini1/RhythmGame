//
// Created by bobini on 24.08.23.
//

#ifndef RHYTHMGAME_BMSNOTES_H
#define RHYTHMGAME_BMSNOTES_H
#include <QObject>

namespace gameplay_logic {
class BmsNotes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QList<int64_t>> visibleNotes READ getVisibleNotes CONSTANT)
    Q_PROPERTY(
      QList<QList<int64_t>> invisibleNotes READ getInvisibleNotes CONSTANT)

    QList<QList<int64_t>> visibleNotes;
    QList<QList<int64_t>> invisibleNotes;

  public:
    explicit BmsNotes(QList<QList<int64_t>> visibleNotes,
                      QList<QList<int64_t>> invisibleNotes,
                      QObject* parent = nullptr);

    [[nodiscard]] auto getVisibleNotes() const -> const QList<QList<int64_t>>&;

    [[nodiscard]] auto getInvisibleNotes() const
      -> const QList<QList<int64_t>>&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSNOTES_H
