#ifndef RHYTHMGAME_BACKBEATCATALOG_H
#define RHYTHMGAME_BACKBEATCATALOG_H

#include "BackbeatSource.h"
#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <optional>

namespace resource_managers {

class BackbeatCatalog : public QObject
{
    Q_OBJECT

  public:
    struct Update
    {
        QList<Table> collections;
        QString error;
        std::optional<qint64> revision;
        int added = 0;
        int removed = 0;
    };

    BackbeatCatalog(std::shared_ptr<BackbeatSource> source,
                    const std::filesystem::path& databasePath,
                    db::SqliteCppDb* modelDatabase,
                    QObject* parent = nullptr);
    ~BackbeatCatalog() override;
    void refresh(bool force = false);

    // Runs on the catalog worker. Uses its own SQLite connection so its
    // transaction cannot accidentally include a UI query or filesystem scan.
    auto synchronize(std::optional<qint64> previousRevision = {}) -> Update;

  signals:
    void busyChanged(bool busy);
    void updated(const QList<resource_managers::Table>& collections);
    void chartSetChanged();
    void errorChanged(const QString& error);

  private:
    std::shared_ptr<BackbeatSource> source;
    std::filesystem::path databasePath;
    db::SqliteCppDb* modelDatabase;
    std::atomic_bool cancelled = false;
    QFutureWatcher<Update> watcher;
    QTimer refreshTimer;
    std::optional<qint64> revision;
    bool pendingRefresh = false;
    bool active = false;
};

} // namespace resource_managers
#endif
