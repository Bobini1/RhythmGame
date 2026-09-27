//
// Created by PC on 09/03/2026.
//

#ifndef RHYTHMGAME_SCORESYNCOPERATION_H
#define RHYTHMGAME_SCORESYNCOPERATION_H

#include <QObject>
#include <QNetworkReply>
#include <QSet>
#include <QStringList>
#include <atomic>
#include <functional>
#include <qqmlintegration.h>

namespace qml_components {

/// Tracks progress and errors for a score upload, download, or import.
/// Uploads and downloads queue scores once the server diff is known and
/// prepare or transfer at most four at once. Errors do not stop the queue.
/// Cancellation aborts owned replies and discards queued work. Finished
/// operations cannot restart.
class ScoreSyncOperation : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by Profile")

    Q_PROPERTY(int done READ getDone NOTIFY progressChanged)
    Q_PROPERTY(int total READ getTotal NOTIFY progressChanged)
    Q_PROPERTY(bool finished READ isFinished NOTIFY finishedChanged)
    Q_PROPERTY(int errorCount READ getErrorCount NOTIFY errorsChanged)
    Q_PROPERTY(QString lastError READ getLastError NOTIFY errorsChanged)
    Q_PROPERTY(bool cancelled READ isCancelled NOTIFY finishedChanged)

    int currentDone{ 0 };
    int total{ 0 };
    bool finishedFlag{ false };
    int errors{};
    QString lastError;
    std::atomic_bool cancelled{};
    QStringList pending;
    qsizetype next{};
    int active{};
    bool dispatching{};
    std::function<void(const QString&)> dispatch;
    QSet<QNetworkReply*> replies;
    void startNext();

  public:
    explicit ScoreSyncOperation(QObject* parent = nullptr);
    ~ScoreSyncOperation() override;
    static constexpr int concurrency = 4;
    void start(QStringList guids, std::function<void(const QString&)> dispatch);
    void ownReply(QNetworkReply* reply);
    Q_INVOKABLE void cancel();

    [[nodiscard]] auto getDone() const -> int { return currentDone; }
    [[nodiscard]] auto getTotal() const -> int { return total; }
    [[nodiscard]] auto isFinished() const -> bool;
    [[nodiscard]] auto isCancelled() const -> bool { return cancelled; }
    [[nodiscard]] auto getErrorCount() const -> int { return errors; }
    [[nodiscard]] auto getLastError() const -> QString { return lastError; }

    /// Sets the total number of items and emits progressChanged().
    /// A zero total finishes the operation.
    void setTotal(int total);

    /// Marks the operation finished when value is true.
    /// Emits finishedChanged() once.
    void setFinished(bool value);

    /// Marks one item done and emits progressChanged().
    /// Finishes the operation when done reaches total.
    void increment();

    /// Records an error without stopping the operation.
    void reportError(const QString& message);

  signals:
    void progressChanged();
    void finishedChanged();
    void error(const QString& message);
    void errorsChanged();
};

} // namespace qml_components

#endif // RHYTHMGAME_SCORESYNCOPERATION_H
