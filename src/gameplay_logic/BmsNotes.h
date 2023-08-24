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
    Q_PROPERTY(QList<QList<int>> visibleNotes READ getVisibleNotes CONSTANT)
    Q_PROPERTY(QList<QList<int>> invisibleNotes READ getInvisibleNotes CONSTANT)

    QList<QList<int>> visibleNotes;
    QList<QList<int>> invisibleNotes;

  public:
    explicit BmsNotes(QList<QList<int>> visibleNotes,
                      QList<QList<int>> invisibleNotes,
                      QObject* parent = nullptr);

    [[nodiscard]] auto getVisibleNotes() const -> const QList<QList<int>>&;

    [[nodiscard]] auto getInvisibleNotes() const -> const QList<QList<int>>&;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSNOTES_H
