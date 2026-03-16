//
// Created by PC on 09/03/2026.
//

#ifndef RHYTHMGAME_SCORESYNCOPERATION_H
#define RHYTHMGAME_SCORESYNCOPERATION_H

#include <QObject>
#include <qqmlintegration.h>

namespace qml_components {

/**
 * @brief Tracks the progress of an asynchronous score sync (upload or
 * download).
 * @details Instances are created by Profile::uploadScores() and
 * Profile::downloadScores(). The total is set once the server diff is known.
 * Each individual score completion calls increment(). Errors per score fire
 * the error() signal but do not halt the operation — finished() is always
 * emitted exactly once when done == total.
 */
class ScoreSyncOperation final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by Profile")

    Q_PROPERTY(int done READ getDone NOTIFY progressChanged)
    Q_PROPERTY(int total READ getTotal NOTIFY progressChanged)

    int currentDone{ 0 };
    int total{ 0 };

  public:
    explicit ScoreSyncOperation(QObject* parent = nullptr);

    [[nodiscard]] auto getDone() const -> int { return currentDone; }
    [[nodiscard]] auto getTotal() const -> int { return total; }

    /**
     * @brief Set the total number of items once it is known.
     * @details Emits progressChanged(). If total is 0, also emits finished().
     */
    void setTotal(int total);

    /**
     * @brief Advance the done counter by one and emit progressChanged().
     * @details Emits finished() when done reaches total.
     */
    void increment();

    /**
     * @brief Report a per-score error without halting the operation.
     */
    void reportError(const QString& message);

  signals:
    void progressChanged();
    void finished();
    void error(const QString& message);
};

} // namespace qml_components

#endif // RHYTHMGAME_SCORESYNCOPERATION_H
